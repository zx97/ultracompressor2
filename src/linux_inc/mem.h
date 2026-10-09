/*
 * UltraCompressor II - mem.h
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
/* mem.h - Borland far-memory block ops.  Flat model: plain memcpy etc. */
#ifndef __UC2_MEM_H
#define __UC2_MEM_H
#include <portdefs.h>
#include <string.h>

#define fmemcpy(d, s, n)    memcpy((void *)(d), (const void *)(s), (size_t)(n))
#define fmemset(d, c, n)    memset((void *)(d), (int)(c), (size_t)(n))
#define fmemcmp(a, b, n)    memcmp((const void *)(a), (const void *)(b), (size_t)(n))
#define fmemmove(d, s, n)   memmove((void *)(d), (const void *)(s), (size_t)(n))
#define movmem(s, d, n)     memmove((void *)(d), (const void *)(s), (size_t)(n))
/* movedata(seg, off, dseg, doff, len): copy raw bytes between far areas.
 * On Linux there are no segments, so treat the seg:off pairs as opaque
 * linear pointers built via MK_FP. */
#define movedata(sseg, soff, dseg, doff, len) \
    memcpy(MK_FP((dseg), (doff)), MK_FP((sseg), (soff)), (size_t)(len))

#endif
