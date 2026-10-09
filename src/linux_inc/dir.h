/*
 * UltraCompressor II - dir.h
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
/* dir.h - Borland <dir.h> shim: directory scanning + path splitting.
 * Implemented on POSIX opendir/readdir + stat. */
#ifndef __UC2_DIR_H
#define __UC2_DIR_H
#include <portdefs.h>
#include <sys/types.h>

/* DOS attribute bits (used in ff_attrib and findfirst attrib mask) */
#define _A_NORMAL  0x00
#define _A_RDONLY  0x01
#define _A_HIDDEN  0x02
#define _A_SYSTEM  0x04
#define _A_VOLID   0x08
#define _A_SUBDIR  0x10
#define _A_ARCH    0x20
#define FA_NORMAL  _A_NORMAL
#define FA_RDONLY  _A_RDONLY
#define FA_HIDDEN  _A_HIDDEN
#define FA_SYSTEM  _A_SYSTEM
#define FA_LABEL   _A_VOLID
#define FA_DIREC   _A_SUBDIR
#define FA_ARCH    _A_ARCH

struct ffblk {
    char   ff_reserved[21];
    char   ff_attrib;
    unsigned ff_ftime;
    unsigned ff_fdate;
    long   ff_fsize;
    char   ff_name[256];
};

#define MAXPATH   1024
#define MAXDRIVE    4
#define MAXDIR   1024
#define MAXFILE   256
#define MAXEXT    256

/* WILD_* return bits for fnsplit */
#define WILDDRIVE 0x0008
#define WILDDIR   0x0004
#define WILDFILE  0x0002
#define WILDEXT   0x0001

int  resolvedir_ci(const char *in, char *out, size_t outsz);
int  findfirst(const char *path, struct ffblk *ffblk, int attrib);
int  findnext(struct ffblk *ffblk);
int  fnsplit(const char *path, char *drive, char *dir, char *name, char *ext);
void fnmerge(char *path, const char *drive, const char *dir,
             const char *name, const char *ext);
char *searchpath(const char *name);

/* service: convert a time_t to DOS 16-bit date/time packs */
unsigned dos_fdate(time_t t);
unsigned dos_ftime(time_t t);

#endif
