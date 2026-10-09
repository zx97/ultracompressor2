/*
 * UltraCompressor II - llio.h
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
// LLIO.H

// file open error mode
#define MAY  1   // ignore critical errors
#define TRY  2   // ask at critical errors (with skip option)
#define MUST 4   // succeed completely or FatalError

// file open I/O mode
#define RO   8   // read only
#define RW   16  // read & write
#define CR   32  // create

// file open CACHE mode
#define NOC  64  // no caching
#define CRI  128 // cache reads (incremental)
#define CRT  256 // cache reads (total)
#define CWR  512 // cache writes

int Open (char *pcName, int mode);         // open file
void Close (int iHandle);                  // close file

WORD Read (BYTE *bBuf, int iHan, WORD wSiz);  // read data from file
void Write (BYTE *bBuf, int iHan, WORD wSiz); // write data to file

void Seek (int iHandle, DWORD dwPos);          // set file pointer
DWORD Tell (int iHandle);                     // read file pointer

DWORD GetFileSize (int iHan);              // get file size
void SetFileSize (int iHan, DWORD dwSiz);  // set file size

char *TmpFile (char *pcLocat, int pure, char *useext);   // return TMP name, pure->path only

int Exists (char *pcPath);                 // does file exist ?

void Rename (char *pcOld, char *pcNew);    // rename file (Win31 proof)
void Delete (char *pcPath);                // delete file
void CopyFile (char *from, char *onto);    // copy file

void CacheW (int han);   // faster write
void CacheR (int han);   // faster read
void CacheRT (int han);  // maximal speed read

int Han (int iHan);  // convert internal to external handle

void DFlush (char *why); // flush (external) defered write cache