/*
 * UltraCompressor II - compint.h
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
// COMPINT.H

extern WORD (far pascal *CReader)(BYTE far *pbBuffer, WORD wSize);
extern void (far pascal *CWriter)(BYTE far *pbBuffer, WORD wSize);

WORD pascal far Compressor (
   WORD wMethod,         // defined in SUPERMAN.H
   WORD (far pascal *Reader)(BYTE far *pbBuffer, WORD wSize),
   void (far pascal *Writer)(BYTE far *pbBuffer, WORD wSize),
   DWORD dwMaster
);

WORD pascal far Decompressor (
   WORD wMethod,
   WORD (far pascal *Reader)(BYTE far *pbBuffer, WORD wSize),
   void (far pascal *Writer)(BYTE far *pbBuffer, WORD wSize),
   DWORD dwMaster,
   DWORD len
);

#define CDRET_OK  0 // no problems
#define CDRET_MEM 1 // not enough memory
#define CDRET_DER 2 // decompression data error
#define CDRET_TER 3 // unknown method

/* function parameter description :
   Reader should read 1..pwSize bytes into pbBuffer. The number of actually
   read bytes should be returned. If there is no more input Reader should
   return 0. On decompression always excactly pwSize bytes should be read
   the decompressor makes sure this is possible.

   Writer writes :-)
*/

struct DeltaBlah {
   BYTE size;
   BYTE ctr;
   BYTE arra[8];
};

void InitDelta (DeltaBlah *db, BYTE type);
void Delta (DeltaBlah *db, BYTE far *pbData, WORD size);
void UnDelta (DeltaBlah *db, BYTE far *pbData, WORD size);
