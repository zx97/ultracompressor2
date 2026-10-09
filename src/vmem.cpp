/*
 * UltraCompressor II - vmem.cpp
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
// VMEM.CPP - Linux RAM-only virtual memory for UC2.
// Ultra-simple handle-based allocator using malloc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "mem.h"
#include "vmem.h"
#include "debuglog.h"

#define MAX_BLOCKS 65535U

/* Handle table: alloc_table[handle] = malloc'd pointer.
 * VPTR.wOffset stores the handle index. */
static void *alloc_table[MAX_BLOCKS];
static WORD alloc_count = 0;
int fCloseVmem = 0;

VPTR VNULL = { 65535U, 0 };

void InitVmem (void){
   memset(alloc_table, 0, sizeof(alloc_table));
   alloc_count = 1;   /* 0 is reserved */
   fCloseVmem = 1;
}

void CloseVmem (void){
   for (WORD i = 1; i < alloc_count; i++){
      if (alloc_table[i]) free(alloc_table[i]);
   }
   memset(alloc_table, 0, sizeof(alloc_table));
   alloc_count = 1;
}

VPTR LLVmalloc (WORD size){
   if (size == 0) size = 1;
   if (alloc_count >= MAX_BLOCKS) { UC2ErrorLog("VMEM: out of handles\n"); return VNULL; }
   WORD handle = alloc_count++;
   alloc_table[handle] = malloc(size);
   if (!alloc_table[handle]) { UC2ErrorLog("VMEM: malloc failed (%u bytes)\n",size); return VNULL; }
   memset(alloc_table[handle], 0, size);
   return MK_VP(0, handle);
}

void LLVfree (VPTR vpAdr){
   if (IS_VNULL(vpAdr)) return;
   WORD handle = vpAdr.wOffset;
   if (handle > 0 && handle < MAX_BLOCKS && alloc_table[handle]){
      free(alloc_table[handle]);
      alloc_table[handle] = NULL;
   }
}

BYTE *V (VPTR vpAdr){
   if (IS_VNULL(vpAdr)) return NULL;
   WORD handle = vpAdr.wOffset;
   if (handle >= MAX_BLOCKS || !alloc_table[handle]) return NULL;
   return (BYTE *)alloc_table[handle];
}

BYTE *Acc (VPTR vpAdr){ return V(vpAdr); }
void UnAcc (VPTR vpAdr){ (void)vpAdr; }

/* ---------- Pipe ---------- */

void MakePipe (PIPE &p){
   p.vpStart = p.vpCurrent = p.vpTail = VNULL;
   p.wOff = 0;
   p.wBufInUse = 0;
}

static void FlushPipe (PIPE &p){
   if (!p.wBufInUse) return;
   if (IS_VNULL(p.vpStart)){
      p.vpStart = LLVmalloc(sizeof(PIPENODE));
      p.vpCurrent = p.vpStart;
      p.vpTail = p.vpStart;
   } else {
      VPTR n = LLVmalloc(sizeof(PIPENODE));
      ((PIPENODE *)V(p.vpCurrent))->vpNext = n;
      p.vpTail = n;
   }
   PIPENODE *pnp = (PIPENODE *)V(p.vpTail);
   pnp->vpNext = VNULL;
   pnp->wLen = p.wBufInUse;
   pnp->vpDat = LLVmalloc(p.wBufInUse);
   memcpy(V(pnp->vpDat), p.buf, p.wBufInUse);
   UnAcc(p.vpTail);
   p.wBufInUse = 0;
   p.wOff = 0;
}

void WritePipe (PIPE &p, BYTE *pbData, WORD wLen){
   WORD pos = 0;
   while (pos < wLen){
      WORD space = 1900 - p.wBufInUse;
      WORD chunk = (wLen - pos < space) ? (wLen - pos) : space;
      memcpy(p.buf + p.wBufInUse, pbData + pos, chunk);
      p.wBufInUse += chunk;
      pos += chunk;
      if (p.wBufInUse >= 1900) FlushPipe(p);
   }
}

WORD ReadPipe (PIPE &p, BYTE *pbData, WORD wLen){
    WORD pos = 0;
    while (pos < wLen){
       if (p.wOff == 0){
          if (IS_VNULL(p.vpCurrent)){
             /* If there are no nodes but the write buffer still holds data,
                flush it to a node so ReadPipe can consume it. */
             if (p.wBufInUse > 0){
                FlushPipe(p);
             } else
                break;
          }
          p.vpCurrent = p.vpStart;
          p.vpStart = ((PIPENODE *)V(p.vpCurrent))->vpNext;
       }
       PIPENODE *pnp = (PIPENODE *)V(p.vpCurrent);
       WORD avail = pnp->wLen - p.wOff;
       WORD chunk = (wLen - pos < avail) ? (wLen - pos) : avail;
       memcpy(pbData + pos, (BYTE *)V(pnp->vpDat) + p.wOff, chunk);
       p.wOff += chunk;
       pos += chunk;
       if (p.wOff >= pnp->wLen){
          p.wOff = 0;
          p.vpCurrent = pnp->vpNext;
          if (IS_VNULL(p.vpCurrent)) p.vpStart = VNULL;
       }
    }
    return pos;
}

void ClosePipe (PIPE &p){
   VPTR walk = p.vpStart;
   while (!IS_VNULL(walk)){
      PIPENODE *pnp = (PIPENODE *)V(walk);
      VPTR next = pnp->vpNext;
      LLVfree(pnp->vpDat);
      LLVfree(walk);
      walk = next;
   }
   p.vpStart = p.vpCurrent = p.vpTail = VNULL;
   p.wOff = 0;
   p.wBufInUse = 0;
}
