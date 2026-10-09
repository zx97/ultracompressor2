/*
 * UltraCompressor II - mem.cpp
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
 * mem.cpp - UC2 memory manager (Linux port).
 *
 * The original implemented EMS / XMS / UMB / INT15 "extended" memory and a
 * 16k cache via real-mode segment tricks.  On Linux (flat 32/64-bit model)
 * all of that is just malloc().  The public API from mem.h is preserved.
 */
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "mem.h"

static void *mem16cache[16];
int fExitMem = 0;
unsigned gmaxEMS=0, gmaxXMS=0, gmaxI15=0, gmaxUMB=0;

void far* exmalloc (long size){
    return (void far*)malloc((size_t)(size > 0 ? size : 1));
}
void exfree (void far* adr){
    if (adr) free((void*)adr);
}
int extest (void far* adr){
    (void)adr;
    return 1;
}
int coreleft4 (void){
    return 0x7FFF;        /* plenty of 4k blocks */
}
void donate16(void far* m){
    (void)m;
}

void* malloc16 (int grp){
    void *p = malloc(16384U);
    if (grp >= 0 && grp < 16) mem16cache[grp] = p;
    return p;
}
void free16 (int grp, int idx){
    (void)idx;
    if (grp >= 0 && grp < 16){
        if (mem16cache[grp]) free(mem16cache[grp]);
        mem16cache[grp] = NULL;
    }
}
int coreleft16 (int grp){
    (void)grp;
    return 0;  /* Linux: no EMS/XMS cache pages */
}

void to16 (int grp, unsigned dummy, void far* src, int len){
    void *dst = (grp >= 0 && grp < 16) ? mem16cache[grp] : NULL;
    if (dst && len > 0) memcpy(dst, src, (size_t)len);
}
void from16 (void far* dst, int grp, unsigned dummy, int len){
    void *src = (grp >= 0 && grp < 16) ? mem16cache[grp] : NULL;
    if (src && len > 0) memcpy(dst, src, (size_t)len);
}

unsigned long coreleft (void){ return 0x7FFFFFFFUL; }
unsigned long farcoreleft (void){ return 0x7FFFFFFFUL; }
