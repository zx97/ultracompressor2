/*
 * UltraCompressor II - uc2sas.cpp
 *
 * Copyright (C) 1992-1996 Nico de Vries / AIP-NL (Ad Infinitum Programs)
 * Linux port: Manuel FLURY (2026)
 *
 * This file is part of UltraCompressor II, released by its author under the
 * GNU Lesser General Public License (LGPL). It is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.  A copy ships with this program (see LICENSE); if not, see
 * <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Credits and references: see README.md.
 */
/*
   uc2sas.cpp - SAS-compatible UC2 archive splitter / joiner.

   Reproduces the "SAS" tool ("simple archive splitter") shipped with
   UltraCompressor II:

       uc2 split <archive> <sizeKiB>   split archive into <archive>.P01, .P02, ...
       uc2 join  <archive>             rebuild <archive> from its pieces

   Piece layout (little-endian), reverse-engineered and verified against SAS.EXE:

       offset 0 : magic  45 14 02 0f            (fixed, 4 bytes)
       offset 4 : original archive size         (uint32)
       offset 8 : this piece's total size       (uint32)
       offset 12: piece index (1-based)         (uint16)
       offset 14: data (pieceSize - 14 bytes)

   Data bytes per piece = sizeKiB*1024 - 14; concatenating the data of every
   piece reproduces the original archive byte-for-byte.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <dirent.h>

#define SAS_HDR 14

static uint32_t rd32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

static void pieceName(const char *arch, int idx, char *out, size_t outsz)
{
    const char *dot = strrchr(arch, '.');
    size_t base = dot ? (size_t)(dot - arch) : strlen(arch);
    if (base > outsz - 5)
        base = outsz - 5;
    memcpy(out, arch, base);
    snprintf(out + base, outsz - base, ".P%02d", idx);
}

static FILE *fopen_ci(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f)
        return f;
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", path);
    char *slash = strrchr(buf, '/');
    const char *dir = ".";
    const char *base = buf;
    if (slash) {
        *slash = '\0';
        dir = buf;
        base = slash + 1;
    }
    DIR *d = opendir(dir);
    if (!d)
        return NULL;
    FILE *out = NULL;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcasecmp(de->d_name, base) == 0) {
            char full[1024];
            snprintf(full, sizeof full, "%s/%s", dir, de->d_name);
            out = fopen(full, "rb");
            break;
        }
    }
    closedir(d);
    return out;
}

int UCSplit(const char *arch, int kib)
{
    if (kib <= 0) {
        fprintf(stderr, "uc2 split: invalid piece size\n");
        return 1;
    }
    long piece = (long)kib * 1024;
    if (piece <= SAS_HDR) {
        fprintf(stderr, "uc2 split: piece size too small\n");
        return 1;
    }
    FILE *f = fopen(arch, "rb");
    if (!f) {
        fprintf(stderr, "uc2 split: cannot open %s\n", arch);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) {
        fclose(f);
        fprintf(stderr, "uc2 split: %s is empty\n", arch);
        return 1;
    }

    long dataPer = piece - SAS_HDR;
    uint8_t *buf = (uint8_t *)malloc((size_t)dataPer);
    if (!buf) {
        fclose(f);
        return 1;
    }

    int idx = 0;
    long remaining = sz;
    while (remaining > 0) {
        long n = remaining > dataPer ? dataPer : remaining;
        idx++;
        char name[1024];
        pieceName(arch, idx, name, sizeof name);
        FILE *o = fopen(name, "wb");
        if (!o) {
            fprintf(stderr, "uc2 split: cannot create %s\n", name);
            free(buf);
            fclose(f);
            return 1;
        }
        uint8_t hdr[SAS_HDR];
        hdr[0] = 0x45; hdr[1] = 0x14; hdr[2] = 0x02; hdr[3] = 0x0f;
        wr32le(hdr + 4, (uint32_t)sz);
        wr32le(hdr + 8, (uint32_t)(n + SAS_HDR));
        hdr[12] = (uint8_t)(idx & 0xff);
        hdr[13] = (uint8_t)((idx >> 8) & 0xff);
        fwrite(hdr, 1, SAS_HDR, o);
        if (fread(buf, 1, (size_t)n, f) != (size_t)n ||
            fwrite(buf, 1, (size_t)n, o) != (size_t)n) {
            fprintf(stderr, "uc2 split: I/O error on %s\n", name);
            fclose(o);
            free(buf);
            fclose(f);
            return 1;
        }
        fclose(o);
        remaining -= n;
    }
    free(buf);
    fclose(f);
    printf("uc2: split %s (%ld bytes) into %d piece(s) of %d KiB\n",
           arch, sz, idx, kib);
    return 0;
}

int UCJoin(const char *arch)
{
    FILE *o = fopen(arch, "wb");
    if (!o) {
        fprintf(stderr, "uc2 join: cannot create %s\n", arch);
        return 1;
    }
    uint8_t hdr[SAS_HDR];
    uint8_t buf[65536];
    uint32_t total = 0;
    uint32_t written = 0;
    for (int idx = 1;; idx++) {
        char name[1024];
        pieceName(arch, idx, name, sizeof name);
        FILE *p = fopen_ci(name);
        if (!p)
            break;
        if (fread(hdr, 1, SAS_HDR, p) != SAS_HDR) {
            fprintf(stderr, "uc2 join: bad header in %s\n", name);
            fclose(p);
            fclose(o);
            return 1;
        }
        if (idx == 1)
            total = rd32le(hdr + 4);
        long data = (long)rd32le(hdr + 8) - SAS_HDR;
        if (data < 0)
            data = 0;
        long got = 0;
        while (got < data) {
            long n = data - got;
            if (n > (long)sizeof buf)
                n = (long)sizeof buf;
            size_t r = fread(buf, 1, (size_t)n, p);
            if (r == 0)
                break;
            fwrite(buf, 1, r, o);
            written += (uint32_t)r;
            got += (long)r;
        }
        fclose(p);
        if (total && written >= total)
            break;
    }
    fclose(o);
    if (written == 0) {
        fprintf(stderr, "uc2 join: no pieces found for %s\n", arch);
        remove(arch);
        return 1;
    }
    printf("uc2: joined %s (%u bytes) from its pieces\n", arch, written);
    return 0;
}
