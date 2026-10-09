/*
 * UltraCompressor II - vmem.h
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
// VMEM.H
#ifndef __UC2_VMEM_H
#define __UC2_VMEM_H

/* SUMMARY of the most important functions :

   VPTR                      virtual pointer type

   VNULL                     virtual NULL pointer
   IS_VNULL (VPTR)           test if pointer is VNULL

   void InitVmem (void)      Initialize (call once)
   VPTR Vmalloc (WORD size)  allocate virtual memory
   void Vfree (VPTR)         free virtual memory
   BYTE *A(VPTR)             get access to virtual memory (WATCH OUT!!)

   PIPE                      dynamic pipe type

   OpenPipe
   ReadPipe
   WritePipe
   ClosePipe

*/

#ifndef UC_MAIN_H // use VMEM as standalone package !!!

#include <stdint.h>
typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

#endif

struct VPTR {
   WORD wBlock;
   WORD wOffset;
};

struct PIPE {
   VPTR vpStart, vpCurrent, vpTail;
   WORD wOff;
   BYTE buf[1900];
   WORD wBufInUse;
};

struct PIPENODE {
   VPTR vpNext;
   WORD wLen;
   VPTR vpDat;
};

extern VPTR VNULL; // virtual NULL pointer

inline int IS_VNULL (VPTR me){
   return (me.wBlock==VNULL.wBlock);
}

inline int CMP_VPTR (VPTR a, VPTR b){
   return (a.wBlock==b.wBlock) && (a.wOffset==b.wOffset);
}

inline VPTR MK_VP(WORD block, WORD offset){
   VPTR tmp;
   tmp.wBlock = block;
   tmp.wOffset = offset;
   return tmp;
}

void InitVmem (void);
   // init VMEM software

VPTR LLVmalloc (WORD size);
   // malloc VMEM (max size BLOCK_SIZE - 6!)
   // succeeds or stops computer (unlikely)

void LLVfree (VPTR vpAdr);
   // free VMEM

BYTE *Acc (VPTR vpAdr);
   // get access to VMEM block (does lock block in memory)

void UnAcc (VPTR vpAdr);
   // unget access to VMEM block (unlock)

BYTE *V (VPTR vpAdr);
   // get access (automatic UnAcc after 'DEEP' succesive calls)

void MakePipe (PIPE &p);
   // create new PIPE

void WritePipe (PIPE &p, BYTE *pbData, WORD wLen);
   // write data to pipe

WORD ReadPipe (PIPE &p, BYTE *pbData, WORD wLen);
   // read data fom pipe

void ClosePipe (PIPE &p);
   // close down pipe

#ifdef UCPROX
   void Vforever_do();
   #define Vforever Vforever_do
#else
   #define Vforever()
#endif

//#define DEBUG_VMEM

#ifdef DEBUG_VMEM
   VPTR DBVmalloc (WORD size, char *file, long line);
   void DBVfree (VPTR vpAdr, char *file, long line);
   #define Vmalloc(a) DBVmalloc(a,__FILE__,__LINE__);
   #define Vfree(a) DBVfree(a,__FILE__,__LINE__);
#else
   #define Vmalloc LLVmalloc
   #define Vfree LLVfree
#endif
#endif /* __UC2_VMEM_H */
