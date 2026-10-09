/*
 * UltraCompressor II - archio.h
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
// ARCHIO.H

#define MAX_AREA 2 // number of allowed concurrent accessible archives

extern int iArchArea;    // current active archive area

void SetArea (int a);    // define work area
int NewArch ();          // true if current archive is NEW (created)

int SetArchive (char *pcPath, BYTE mode);
   // mode 0=read only 1=update 2=incremental update

void CloseArchiveFile (void);                     // close link to archive
void ARSeek (DWORD dwVolume, DWORD dwOffset);      // multi volume seek
int ARTell (DWORD *pdwVolume, DWORD *pdwOffset);  // multi volume tell
void AWSeek (DWORD dwVolume, DWORD dwOffset);      // multi volume seek
int AWTell (DWORD *pdwVolume, DWORD *pdwOffset);  // multi volume tell

void AWCut (void); // cut end of archive off                                // cut off write file
void AWEnd (void); // seek to end of archive

// (AR is for the ARead function, AW for the AWrite function)

void far pascal AWrite (BYTE *bBuf, WORD wSize);
WORD far pascal ARead (BYTE *bBuf, WORD wSize);

// measure (compressor) output
void ResetOutCtr ();
DWORD GetOutCtr ();

void Old2New (DWORD dwLen); // copy form old to new archive