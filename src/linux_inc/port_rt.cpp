/*
 * UltraCompressor II - port_rt.cpp
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
 * port_rt.cpp - implementations of the Borland <dos.h> shim routines
 * that cannot be expressed as inline macros.  Real-mode interrupts are
 * stubbed (return "not present"); date/time map to POSIX.
 */
#include <portdefs.h>
#include <dos.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

union REGS _regs;
struct SREGS _sregs;

int int86(int intno, union REGS *in, union REGS *out)
{
    (void)intno;
    if (in) _regs = *in;
    /* No real-mode interrupt available.  Mark carry flag set so callers
     * that test for success see "failed / not supported". */
    _regs.x.flags |= 0x0001;
    _regs.x.cflag  = 1;
    if (out) *out = _regs;
    return 0;
}

int int86x(int intno, union REGS *in, union REGS *out, struct SREGS *sregs)
{
    (void)sregs;
    return int86(intno, in, out);
}

int bdos(int dosfn, unsigned dx, unsigned al)
{
    (void)dosfn; (void)dx; (void)al;
    return 0;
}

int bdosptr(int dosfn, void *dx, unsigned al)
{
    (void)dosfn; (void)dx; (void)al;
    return 0;
}

void (*getvect(int intr))(void)
{
    (void)intr;
    return 0;
}

void setvect(int intr, void (*func)(void))
{
    (void)intr; (void)func;
}

static void posix_to_dosdate(struct dosdate_t *d)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static const int dow[] = {0,1,2,3,4,5,6};
    d->day       = (unsigned char)tm->tm_mday;
    d->month     = (unsigned char)(tm->tm_mon + 1);
    d->year      = (unsigned short)(tm->tm_year + 1900);
    d->dayofweek = (unsigned char)dow[tm->tm_wday];
}

static void posix_to_dostime(struct dostime_t *t)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    t->hour    = (unsigned char)tm->tm_hour;
    t->minute  = (unsigned char)tm->tm_min;
    t->second  = (unsigned char)tm->tm_sec;
    t->hsecond = 0;
}

void _dos_getdate(struct dosdate_t *d) { posix_to_dosdate(d); }
void uc2_getdate(struct date *d)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    d->da_year = tm->tm_year + 1900;
    d->da_mon  = (char)(tm->tm_mon + 1);
    d->da_day  = (char)tm->tm_mday;
}
void _dos_gettime(struct dostime_t *t) { posix_to_dostime(t); }
void uc2_gettime(struct time *t)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    t->ti_hour = (unsigned char)tm->tm_hour;
    t->ti_min  = (unsigned char)tm->tm_min;
    t->ti_sec  = (unsigned char)tm->tm_sec;
    t->ti_hund = 0;
}

unsigned _dos_setftime(int handle, unsigned date, unsigned time)
{
    (void)handle; (void)date; (void)time;
    return 0;
}
unsigned _dos_getftime(int handle, unsigned *date, unsigned *time)
{
    (void)handle;
    if (date) *date = 0;
    if (time) *time = 0;
    return 0;
}
unsigned _dos_setfileattr(const char *fname, WORD attrib)
{
    (void)fname; (void)attrib;
    return 0;
}
unsigned _dos_getfileattr(const char *fname, WORD *attrib)
{
    (void)fname;
    if (attrib) *attrib = 0;
    return 0;
}

void delay(unsigned milliseconds)
{
    usleep((useconds_t)milliseconds * 1000);
}

/* ---------------- alloc.h ---------------- */
void *farmalloc(unsigned long size)
{
    return malloc((size_t)(size ? size : 1));
}
void farfree(void *block)
{
    free(block);
}
void *farcalloc(unsigned long nmemb, unsigned long size)
{
    return calloc((size_t)nmemb, (size_t)size);
}
void *farmrealloc(void *block, unsigned long size)
{
    return realloc(block, (size_t)size);
}

/* ---------------- direct.h ---------------- */
/* getcwd / rmdir / chdir are standard POSIX (unistd.h); do NOT wrap them
 * here, or the wrapper would recurse into itself. */
int   getdisk(void) { return 0; }
int   setdisk(int drive) { (void)drive; return 1; }

/* ---------------- io.h extras ---------------- */
long tell(int handle) { return (long)lseek(handle, 0, SEEK_CUR); }
int  eof(int handle)
{
    off_t cur = lseek(handle, 0, SEEK_CUR);
    off_t end = lseek(handle, 0, SEEK_END);
    lseek(handle, cur, SEEK_SET);
    return (cur >= end) ? 1 : 0;
}
long filelength(int handle)
{
    struct stat st;
    if (fstat(handle, &st) != 0) return -1L;
    return (long)st.st_size;
}
int setmode(int handle, int mode) { (void)handle; (void)mode; return 0; }
int chsize(int handle, long size)
{
    return ftruncate(handle, (off_t)size);
}

/* ---------------- bios.h ---------------- */
int _bios_memsize(void)
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long psize = sysconf(_SC_PAGESIZE);
    if (pages <= 0 || psize <= 0) return 640;
    return (int)((pages * psize) / 1024);
}

/* ---------------- process.h ---------------- */
int xsystem(const char *command)
{
    return system(command);
}

/* ---------------- dos.h: ioctl shim ---------------- */
int uc2_ioctl(int handle, ...)
{
    (void)handle;
    return 0;   /* "not a device" on Linux */
}

/* ---------------- io.h: _chmod ---------------- */
int _chmod(const char *path, int func, int attrib)
{
    if (func == 0) {
        struct stat st;
        if (stat(path, &st) != 0) return -1;
        int a = 0;
        if (!(st.st_mode & 0200)) a |= 0x01;          /* read-only */
        if (st.st_mode & 02000) a |= 0x02;
        return a;
    } else {
        int mode = (attrib & 0x01) ? 0444 : 0644;
        chmod(path, mode);
        return attrib;
    }
}
