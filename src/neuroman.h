/*
 * UltraCompressor II - neuroman.h
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
// NEUROMAN.H

struct MASREC {
   MASMETA masmeta;
   COMPRESS compress;
   LOCATION location;
   VPTR vpChain[3];
   VPTR vpNext;
   BYTE bStatus;
   BYTE bCache; // master in cache?
   int ctab[4]; // cache block table
   BYTE cnum;
   DWORD dwVOffset; // offset in file keeping masters
   VPTR vpNextNtx;
   VPTR vpNextKey;
   char szName[256];
};

#define MS_CLR   0  // new MASREC
#define MS_NEW1  1  // new, never used master, single user
#define MS_NEW2  2  // new, never used master, multiple users
#define MS_NEWC  3  // new defined (but not stored) master
#define MS_OLD   4  // old unloaded (registered only) master
#define MS_OLDN  5  // old but needed master (ScanAdd)
#define MS_OLDC  6  // old master, resides in system
#define MS_WRT   7  // master written to new archive
#define MS_NEED  8  // needed master (extract)

void OpenNeuro ();
void CloseNeuro ();

DWORD ToKey (char *pcFileName);
DWORD ToHKey (char *pcFileName);


VPTR LocMacNtx (DWORD dwIndex);
void RegNtxKey (DWORD dwIndex, DWORD dwKey);
VPTR LocMacKey (DWORD dwIndex);
VPTR LocKey (DWORD dwIndex);

void AddAccess (BYTE fInc); // create new, read old, needed masters
void Transfer (BYTE *pbAddress, WORD *pwLen, DWORD dwIndex);

void ListMast (PIPE &p);
void ClearMasRefLen();

void TuneNeuro (WORD wItem, WORD wTotal);

int ValidMaster (DWORD dwIndex);

void AddToNtx (DWORD ntx, VPTR rev, DWORD size); // add revision to Ntx chain
VPTR GetFirstNtx (DWORD ntx, WORD index);        // locate first element of Ntx chain
void CleanNtx (void);                // clean general Ntx chains
