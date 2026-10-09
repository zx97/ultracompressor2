/*
   unuc2sea.c — extract a SEA (.EXE): it is an ordinary UC2 archive sitting
   behind a DOS stub, so skip the stub and decode the embedded stream like any
   other archive (via libunuc2.c).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libunuc2.c"   /* brings in the static Ultra engine + supermaster[] */

/* ------------------------------------------------------------------ */
/* input file -> memory reader                                        */
/* ------------------------------------------------------------------ */

static const u8 *g_in;
static size_t     g_in_len;

static int mem_read(void *ctx, unsigned pos, void *buf, unsigned size)
{
	(void)ctx;
	if (pos >= g_in_len)
		return 0;
	size_t n = size;
	if (n > g_in_len - pos)
		n = g_in_len - pos;
	memcpy(buf, g_in + pos, n);
	return (int)n;
}

/* ------------------------------------------------------------------ */
/* growable writer                                                    */
/* ------------------------------------------------------------------ */

struct grow {
	u8     *ptr;
	size_t  size;
	size_t  cap;
};

static int grow_write(void *ctx, const void *p, unsigned n)
{
	struct grow *g = ctx;
	if (g->size + n > g->cap) {
		size_t nc = g->cap ? g->cap : (1u << 20);
		while (nc < g->size + n)
			nc <<= 1;
		u8 *np = realloc(g->ptr, nc);
		if (!np)
			return -1;
		g->ptr = np;
		g->cap = nc;
	}
	memcpy(g->ptr + g->size, p, n);
	g->size += n;
	return 0;
}

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */

static void *xalloc(void *ctx, unsigned size) { (void)ctx; return malloc(size ? size : 1); }
static void  xfree_(void *ctx, void *p)       { (void)ctx; free(p); }

static unsigned u24le(const u8 *p)
{
	return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16);
}

/* Make a host-safe name: drop any DOS drive/dir, backslashes -> '/'. */
static void sanitize(char *dst, size_t dstsz, const u8 *name, unsigned n)
{
	size_t j = 0;
	unsigned start = 0;
	for (unsigned i = 0; i < n; i++) {
		if (name[i] == '\\' || name[i] == '/' || name[i] == ':')
			start = i + 1;
	}
	for (unsigned i = start; i < n && j + 1 < dstsz; i++) {
		unsigned char c = name[i];
		if (c < 0x20 || c == 0x7f)
			c = '_';
		dst[j++] = (char)c;
	}
	dst[j] = 0;
}

static int make_dirs(const char *path)
{
	char tmp[1024];
	size_t n = strlen(path);
	if (n >= sizeof tmp)
		return -1;
	memcpy(tmp, path, n + 1);
	for (size_t i = 1; i < n; i++) {
		if (tmp[i] == '/') {
			tmp[i] = 0;
			mkdir(tmp, 0777);
			tmp[i] = '/';
		}
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* public API                                                         */
/* ------------------------------------------------------------------ */

/* Extract every member of the archive into `outdir`.
   Returns the number of files extracted (>= 0) or a negative value on error:
     -1 : I/O error, -2 : not a SEA archive, -3 : decompression failure. */
int unuc2sea_extract(const char *inpath, const char *outdir)
{
	FILE *f = fopen(inpath, "rb");
	if (!f) { fprintf(stderr, "unuc2sea: %s: %s\n", inpath, strerror(errno)); return -1; }
	fseek(f, 0, SEEK_END);
	long fsz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (fsz <= 0) { fprintf(stderr, "unuc2sea: %s: empty\n", inpath); fclose(f); return -1; }
	u8 *file = malloc((size_t)fsz);
	if (!file || fread(file, 1, (size_t)fsz, f) != (size_t)fsz) {
		fprintf(stderr, "unuc2sea: %s: read error\n", inpath);
		fclose(f); return -1;
	}
	fclose(f);
	g_in = file;
	g_in_len = (size_t)fsz;

	/* 1. locate "UC2SFX Header" */
	static const char SIG[] = "UC2SFX Header";
	long hdr = -1;
	for (long i = 0; i + (long)sizeof(SIG) <= fsz; i++) {
		if (memcmp(file + i, SIG, sizeof(SIG) - 1) == 0) { hdr = i; break; }
	}
	if (hdr < 0) {
		fprintf(stderr, "unuc2sea: %s: no 'UC2SFX Header' signature (not a SEA archive)\n", inpath);
		free(file); return -2;
	}
	if (hdr + 0x25 > fsz) {
		fprintf(stderr, "unuc2sea: %s: truncated UC2SFX header\n", inpath);
		free(file); return -2;
	}
	unsigned stored_off = file[hdr + 0x0f] | (file[hdr + 0x10] << 8);
	unsigned file_count = file[hdr + 0x15] | (file[hdr + 0x16] << 8);
	unsigned payload = (unsigned)(hdr + 0x25);
	if (stored_off && stored_off < (unsigned)fsz)
		payload = stored_off;

	/* 2. decode the Ultra stream */
	struct uc2_io io = { .read = mem_read, .alloc = xalloc, .free = xfree_ };
	struct uc2_context ctx;
	memset(&ctx, 0, sizeof ctx);
	ctx.io = &io;

	struct archive_ctx ar = { .offset = payload, .uc2 = &ctx };
	struct reader rd = { .read = archive_read, .context = &ar };
	struct grow out = { 0, 0, 0 };
	struct writer wr = { .write = grow_write, .context = &out };

	u16 csum = 0;
	int rc = decompressor(&ctx, 4 /* Ultra, delta 0 */, &rd, &wr, NoMaster, 0x7fffffff, &csum);
	if (rc < 0) {
		fprintf(stderr, "unuc2sea: decompression failed (%d)\n", rc);
		free(file); return -3;
	}
	fprintf(stderr, "unuc2sea: %s: UC2SFX @ 0x%lx, %zu bytes decoded, %u file(s) in header\n",
		inpath, hdr, out.size, file_count);

	/* 3. walk the record list */
	size_t off = 0;
	int found = 0;
	while (off + 15 <= out.size) {
		const u8 *h = out.ptr + off;
		int nlen = (int)h[0] - 0x0e;
		if (nlen <= 0 || nlen > 255)
			break;
		unsigned size = u24le(h + 11);
		size_t need = 15u + (unsigned)nlen + size;
		if (off + need > out.size) {
			fprintf(stderr, "unuc2sea: record at 0x%zx truncated (name_len=%d size=%u)\n",
				off, nlen, size);
			break;
		}
		char name[1024];
		sanitize(name, sizeof name, h + 15, (unsigned)nlen);
		const u8 *data = h + 15 + nlen;

		char path[2048];
		snprintf(path, sizeof path, "%s/%s", outdir, name);
		make_dirs(path);
		FILE *o = fopen(path, "wb");
		if (!o) {
			fprintf(stderr, "unuc2sea: cannot create %s: %s\n", path, strerror(errno));
		} else {
			if (size && fwrite(data, 1, size, o) != size)
				fprintf(stderr, "unuc2sea: short write on %s\n", path);
			fclose(o);
			found++;
		}
		off += need;
	}

	if (file_count && (unsigned)found != file_count)
		fprintf(stderr, "unuc2sea: warning: header says %u files, extracted %d\n", file_count, found);

	free(out.ptr);
	free(file);
	return found;
}

/* ------------------------------------------------------------------ */
/* standalone CLI                                                     */
/* ------------------------------------------------------------------ */

#ifdef UNUC2SEA_MAIN
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <archive.exe> [output-dir]\n", argv[0]);
		return 2;
	}
	int n = unuc2sea_extract(argv[1], argc > 2 ? argv[2] : ".");
	if (n < 0)
		return 1;
	fprintf(stderr, "unuc2sea: extracted %d file(s)\n", n);
	return 0;
}
#endif
