/*
 * UltraCompressor II - ultracmp.h
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
// ULTRACMP.H

void InvHashC (void);

void TuneComp (WORD wMaxSearch, WORD wMaxLazySearch,
	       WORD wLimitLazy, WORD wLimitSearch);

void pascal far UltraCompressor (DWORD dwMaster, BYTE bDelta);
void pascal far UltraDecompressor (DWORD dwMaster, BYTE bDelta, DWORD len);

/* Init original UC2 decompression engine (decompresses supermaster) */
void UC2DecompInit(void);


   void far pascal TreeGen(WORD far *pwFreqCounters,   // input
                           WORD wNrSymbols,            // input
                           WORD wMaxLenCode,           // input
                           BYTE far *pbLengths         // output
                          );


   void far pascal CodeGen(WORD wNrSymbols,            // input
                           BYTE far *pbLengths,        // input
                           WORD far *pwCodes           // output
                          );


   void far pascal DCodeGen(WORD wNrSymbols,           // input
                            BYTE far *pbLengths,       // input
                            WORD far *pwTable,         // output
                            BYTE far *pbTableLengths   // output
                           );


   void far pascal TreeInit(void);


   void far pascal TreeEnc(BYTE far *pbLengths,    // input
			   BYTE bFlag              // input
                          );                       // output is I/O


   void far pascal TreeDec(BYTE far *pbLengths     // output
                          );                       // input is I/O



