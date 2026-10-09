/*
 * UltraCompressor II - diverse.h
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
// DIVERSE.H

#define D_CON   1
#define D_NUL   2
#define D_DEV   3
#define D_FILE  4

void EatSpace (char *name);
BYTE *Name2Rep (char *pcName);  // hap.ext to rep
char *Rep2Name (BYTE *pbRep);   // rep to hap.ext
char *Rep2DName (BYTE *pbRep);  // rep to hap________ext (listings)

int StdOutType (void);

#define TMP  1  // temporarely (compression, booster)
#define PERM 2  // permanent (administration)
#define STMP 3  // super temporarely (closed cycle)
#define NTMP 4  // non regular TMP (pipelining, diverse)

#ifdef TRACE
   BYTE *xxmalloc (char *file, long line, DWORD len, BYTE type);
   void xxfree (char *file, long line, BYTE *address, BYTE type);
   #define xmalloc(q,r) xxmalloc(__FILE__,__LINE__,q,r);
   #define xfree(q,r) xxfree(__FILE__,__LINE__,q,r);
#else
   BYTE *yxmalloc (DWORD len);    // malloc or terminate program
   void yxfree (BYTE *);
   #define xmalloc(q,r) yxmalloc(q);
   #define xfree(q,r) yxfree(q);
#endif


BYTE *normalize (BYTE *base); // normalize pointer (offset to 0)

BYTE probits (void); // Determine processor type
   // 8  -> 8088/8086
   // 16 -> 286
   // 32 -> 386/486


void InternalError (char *, long);
#define IE() InternalError(__FILE__,__LINE__)

void far RapidCopy (void far *dst, void far *src, WORD wLen);
void DDump (char *pcFilename, BYTE *pbData, WORD wSize);

int pulsar (void);

char *neat (DWORD val);

// RegDat & UnRegDat register a usable TEMP file buffer
void RegDat (BYTE *dat, WORD len);
void UnRegDat (void);

// GetDat and UnGetDat get a buffer for speed I/O usage
void GetDat (BYTE **dat, WORD *len);
void UnGetDat (void);

void ssystem (char *);

void Critical (void);
void Normal (void);
void Reset (void);

void strrep (char *base, char *from, char *onto);

char* LocateF (char *name, int batch);
