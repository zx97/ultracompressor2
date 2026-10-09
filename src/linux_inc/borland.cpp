/*
 * UltraCompressor II - borland.cpp
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
#include <ctype.h>
 * borland.cpp - implementations of assorted Borland helper shims.
 * Most are stubs that behave benignly on Linux (DOS real-mode services
 * are unavailable); a few map to POSIX where it makes sense.
 */
#include <portdefs.h>
#include <borland.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

char *strupr(char *s)
{
    if (s) for (char *p = s; *p; p++)
        *p = (char)((*p >= 'a' && *p <= 'z') ? (*p - 'a' + 'A') : *p);
    return s;
}

int stricmp(const char *a, const char *b)
{
    return strcasecmp(a, b);
}

void setmem(void *p, unsigned len, int val)
{
    memset(p, val, (size_t)len);
}

int getcurdir(int drive, char *buf)
{
    (void)drive;
    if (!getcwd(buf, 260)) return -1;
    return 0;
}

void intr(int intno, void *regs)
{
    (void)intno;
    if (regs) int86(intno, (union REGS *)regs, (union REGS *)regs);
}
int intdos(union REGS *regs)
{
    if (regs) return int86(0x21, regs, regs);
    return 0;
}
int intdosx(void *r)
{
    (void)r;
    return 0;
}

int dosexterr(void *e)
{
    (void)e;
    return 0;
}

char *getdcwd(int drive, char *buf, int buflen)
{
    (void)drive;
    if (getcwd(buf, buflen)) return buf;
    return NULL;
}

unsigned allocmem(unsigned size, unsigned *seg)
{
    /* Emulate a real-mode allocation with a normal malloc; the returned
     * "segment" is meaningless on Linux but kept for API compatibility. */
    void *p = malloc((size_t)size);
    if (!p) return 0xFFFF;       /* failure code (Borland) */
    if (seg) *seg = (unsigned)((unsigned long)p >> 4);
    return 0;
}
void freemem(unsigned seg)
{
    (void)seg;
    /* We cannot recover the original pointer from a segment; callers that
     * mix allocmem/freemem are DOS-only paths and are not exercised. */
}

void textmode(int mode) { (void)mode; }

void *farqalloc(unsigned long size)
{
    return malloc((size_t)size);
}

char *ultoa(unsigned long value, char *buf, int radix)
{
    char tmp[40];
    int i = 0;
    unsigned long v = value;
    if (v == 0) tmp[i++] = '0';
    while (v) { tmp[i++] = "0123456789abcdef"[v % (unsigned)radix]; v /= (unsigned)radix; }
    while (i) *buf++ = tmp[--i];
    *buf = 0;
    return buf;
}

char *gets(char *s)
{
    /* Deprecated C function; emulate with fgets + strip newline. */
    if (fgets(s, 1024, stdin)) {
        size_t n = strlen(s);
        if (n && s[n-1] == '\n') s[n-1] = 0;
        return s;
    }
    return NULL;
}

void setdta(void *dta) { (void)dta; }
void *getdta(void) { return NULL; }

int _argc = 0;
char **_argv = NULL;

char *_swappath = 0;
int _useems = 0;
