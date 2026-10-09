/*
 * UltraCompressor II - portdefs.h
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
 * portdefs.h - Forced-include shim for porting UC2 (Borland C++ / DOS) to Linux.
 *
 * Neutralises Borland memory-model and calling-convention keywords
 * (flat 32/64-bit model has no near/far/huge segments) and provides
 * common type aliases used across the codebase.
 *
 * Included first via g++ -include so every translation unit sees it.
 */
#ifndef __UC2_PORTDEFS_H
#define __UC2_PORTDEFS_H

/* --- memory model / calling convention keywords -> no-ops --- */
#define far
#define near
#define huge
#define _far
#define _near
#define _huge
#define pascal
#define _pascal
#define cdecl
#define _cdecl
#define _Cdecl
#define _fastcall
#define interrupt
#define _interrupt
#define _export
#define _export
#define _loadds
#define _seg
#define _ss
#define _fortran
#define _stdcall
#define _saveregs
#define _cdecl_
#define _pascal_
#define _huge_
#define __huge
#define __ss
#define __seg
#define __export
#define __loadds
#define __far
#define __near
#define __pascal
#define __cdecl
#define __interrupt
#define __fastcall
#define __fortran
#define __stdcall
#define __saveregs
#define _FAR
#define _NEAR
#define _HUGE

/* --- common integer aliases (exact sizes for archive compatibility) --- */
#include <stdint.h>
#ifndef BYTE
typedef uint8_t  BYTE;
#endif
#ifndef WORD
typedef uint16_t WORD;
#endif
#ifndef DWORD
typedef uint32_t DWORD;
#endif

/* small runtime helpers used by the original code */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#define randomize()  srand((unsigned)(time(NULL)))
#define flushall()   fflush(NULL)
#define random(n)    ((int)((long)rand() * (n) / (RAND_MAX + 1L)))

#ifndef NULL
#define NULL 0
#endif

/* Borland-style huge/long pointer math helpers (flat model: informational) */
#ifndef MK_FP
#define MK_FP(seg, off)  ((void *)((((unsigned long)(seg)) << 4) | ((unsigned)(off) & 0x000F)))
#endif
#ifndef FP_SEG
#define FP_SEG(p)       ((unsigned)(((unsigned long)(p)) >> 4))
#endif
#ifndef FP_OFF
#define FP_OFF(p)       ((unsigned)((unsigned long)(p) & 0x000F))
#endif

/* Tiny / huge memory helpers sometimes referenced */
#ifndef peek
#define peek(s, o)      (*((unsigned short *)MK_FP((s), (o))))
#endif
#ifndef poke
#define poke(s, o, v)   (*((unsigned short *)MK_FP((s), (o))) = (v))
#endif
#ifndef peekb
#define peekb(s, o)     (*((unsigned char  *)MK_FP((s), (o))))
#endif
#ifndef pokeb
#define pokeb(s, o, v)  (*((unsigned char  *)MK_FP((s), (o))) = (v))
#endif

/* VC++/DOS virtual-pointer type used pervasively (VPTR, Vmalloc, ...) */
#ifndef UC_MAIN_H
#define UC_MAIN_H
#endif
#include <vmem.h>

#endif /* __UC2_PORTDEFS_H */
