/*
 * UltraCompressor II - fletch.cpp
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
#pragma inline
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "main.h"
#include "fletch.h"

void FletchInit (struct FREC *fr){
   fr->sum1 = 0;
   fr->sum2 = 0;
}

void FletchUpdate (struct FREC *fr, BYTE far *dptr, unsigned len){
   if (!len) return;
   unsigned sum2 = fr->sum2;
   /* process pending halfword from previous call */
   if (fr->sum1){
      sum2 ^= (unsigned)(*dptr) << 8;
      dptr++;
      len--;
      fr->sum1 = 0;
   }
   /* if current length is odd, consume the trailing byte now
      and remember that the next chunk must supply the high byte */
   if (len & 1){
      sum2 ^= (unsigned)(dptr[len - 1]);
      len--;
      fr->sum1 = 1;
   }
   /* XOR all remaining 16-bit words into the accumulator */
   const WORD *w = (const WORD *)dptr;
   unsigned n = len / 2;
   for (unsigned i = 0; i < n; i++)
      sum2 ^= w[i];
   fr->sum2 = (WORD)sum2;
}

WORD Fletcher (FREC *fr){
   return fr->sum2^0xA55A;
}

