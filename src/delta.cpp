/*
 * UltraCompressor II - delta.cpp
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
#include <alloc.h>
#include <mem.h>
#include "main.h"
#include "compint.h"
#include "vmem.h"
#include "superman.h"
#include "comp_tt.h"
#include "video.h"
#include "diverse.h"
#include "bitio.h"
#include "ultracmp.h"
#include "fletch.h"
#include "tree.h"
#include "llio.h"

/* BASIC FUNCTIONS

void Delta (DeltaBlah *db, BYTE far *pbData, WORD size){
   Out (7,"{D%u}",size);
   for (WORD i=0;i<size;i++){
      BYTE tmp=pbData[i];
      pbData[i] = tmp - db->arra[db->ctr];
      db->arra[db->ctr] = tmp;
      if (++db->ctr==db->size) db->ctr=0;
   }
}

void UnDelta (DeltaBlah *db, BYTE far *pbData, WORD size){
   Out (7,"{U%u}",size);
   for (WORD i=0;i<size;i++){
      pbData[i] += db->arra[db->ctr];
      db->arra[db->ctr] = pbData[i];
      if (++db->ctr==db->size) db->ctr=0;
   }
}

*/

//#define Delta DUMMY1
//#define UnDelta DUMMY2

static BYTE SDelta (BYTE a, WORD size, BYTE far* pbData){
   for (WORD i=0;i<size;i++){
      BYTE tmp=pbData[i];
      pbData[i] = tmp - a;
      a = tmp;
   }
   return a;
}

static BYTE SUnDelta (BYTE a, WORD size, BYTE far *pbData){
   for (WORD i=0;i<size;i++){
      pbData[i] += a;
      a = pbData[i];
   }
   return a;
}

void Delta (DeltaBlah *db, BYTE far *pbData, WORD size){
   BYTE arra[8];
   arra[0] = db->arra[0];
   arra[1] = db->arra[1];
   arra[2] = db->arra[2];
   arra[3] = db->arra[3];
   arra[4] = db->arra[4];
   arra[5] = db->arra[5];
   arra[6] = db->arra[6];
   arra[7] = db->arra[7];
   WORD ctr=db->ctr;
   WORD siz=db->size;

   if (siz==1){
      arra[0] = SDelta (arra[0],size,pbData);
   } else {
      for (WORD i=0;i<size;i++){
	 BYTE tmp=pbData[i];
	 pbData[i] = tmp - arra[ctr];
	 arra[ctr] = tmp;
	 if (++ctr==siz) ctr=0;
      }
   }

   db->ctr = ctr;
   db->arra[0] = arra[0];
   db->arra[1] = arra[1];
   db->arra[2] = arra[2];
   db->arra[3] = arra[3];
   db->arra[4] = arra[4];
   db->arra[5] = arra[5];
   db->arra[6] = arra[6];
   db->arra[7] = arra[7];
}

void UnDelta (DeltaBlah *db, BYTE far *pbData, WORD size){
   BYTE arra[8];
   arra[0] = db->arra[0];
   arra[1] = db->arra[1];
   arra[2] = db->arra[2];
   arra[3] = db->arra[3];
   arra[4] = db->arra[4];
   arra[5] = db->arra[5];
   arra[6] = db->arra[6];
   arra[7] = db->arra[7];
   WORD ctr=db->ctr;
   WORD siz=db->size;

   if (siz==1){
      arra[0] = SUnDelta (arra[0], size, pbData);
   } else {
      for (WORD i=0;i<size;i++){
	 pbData[i] += arra[ctr];
	 arra[ctr] = pbData[i];
	 if (++ctr==siz) ctr=0;
      }
   }

   db->ctr = ctr;
   db->arra[0] = arra[0];
   db->arra[1] = arra[1];
   db->arra[2] = arra[2];
   db->arra[3] = arra[3];
   db->arra[4] = arra[4];
   db->arra[5] = arra[5];
   db->arra[6] = arra[6];
   db->arra[7] = arra[7];
}