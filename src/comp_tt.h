/*
 * UltraCompressor II - comp_tt.h
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

   Comp_TT.h                                     Danny Bezemer




*/

#define HASH_TABLE_SIZE                32768U
#define BLOCK_SIZE                     25000U
#define MAX_EXTRA_BYTES                (BLOCK_SIZE/8)

//#define Hash(c1,c2)                    ((((c1)&0x7F)<<6) ^ (c2&0x7F))
#define Hash(c1,c2) (((c1)<<7) ^ c2)


/*
   Prototypes
*/
void far pascal TurboCompressor (DWORD dwMaster);
void far pascal TurboDecompressor (DWORD dwMaster);
