/*
 * UltraCompressor II - borland.h
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
/* borland.h - assorted Borland helper shims for the Linux UC2 port. */
#ifndef __UC2_BORLAND_H
#define __UC2_BORLAND_H
#include <portdefs.h>
#include <dos.h>

char *strupr(char *s);
int   stricmp(const char *a, const char *b);
void  setmem(void *p, unsigned len, int val);
int   getcurdir(int drive, char *buf);
void  intr(int intno, void *regs);
int   intdos(union REGS *regs);
/* Borland REGPACK (registers as flat union members, x86 style) */
union REGPACK {
    unsigned r_ax, r_bx, r_cx, r_dx, r_bp, r_si, r_di, r_ds, r_es, r_flags;
    unsigned char r_al, r_ah, r_bl, r_bh, r_cl, r_ch, r_dl, r_dh;
};
int   intdosx(void *r);
int   dosexterr(void *e);

/* Borland DOSERROR structure and critical-error handling */
struct DOSERROR {
    int de_exterror;
    int de_class;
    int de_action;
    int de_locus;
};
char *getdcwd(int drive, char *buf, int buflen);
void  freemem(unsigned seg);
void  textmode(int mode);
void *farqalloc(unsigned long size);
char *ultoa(unsigned long value, char *buf, int radix);
char *gets(char *s);
#define _fsopen(path, mode, share)  fopen((path), (mode))
void  setdta(void *dta);
void *getdta(void);
unsigned allocmem(unsigned size, unsigned *seg);
char *_getdcwd(int drive, char *buf, int buflen);
#define _getdcwd(d,b,l) getdcwd((d),(b),(l))

/* Borland command-line globals */
extern int   _argc;
extern char **_argv;

#endif
