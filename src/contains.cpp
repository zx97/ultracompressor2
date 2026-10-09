/*
 * UltraCompressor II - contains.cpp
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
/**********************************************************************/
/*$ INCLUDES */

#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <alloc.h>
#include <conio.h>
#include <stdio.h>
#include <io.h>
#include <share.h>
#include "main.h"
#include "video.h"
#include "vmem.h"
#include "superman.h"
#include "llio.h"
#include "archio.h"
#include "views.h"
#include "diverse.h"
#include "test.h"
#include "mem.h"
#include "compint.h"
#include "dirman.h"
#include "menu.h"
#include "comoterp.h"
#include <ctype.h>

static long found=0;
static int offset=0;

void StartSearch (void){
   found=0;
   offset=0;
}

void Investigate (BYTE *data, WORD len){
   for (int i=0;i<len;i++){
      if (toupper(data[i])==MODE.szContains[offset]){
	 int j=offset;
	 int k=i;
	 while (MODE.szContains[j+1]!=0){
	    k++;
	    j++;
	    if (k==len){
	       offset=j;
	       return;
	    }
	    if (toupper(data[k])!=MODE.szContains[j]) goto next;
	 }
	 found++;
      } else {
	 offset=0;
      }
next:
   }
}

/*
void Investigate (BYTE *data, WORD len){
   while (len){
      int trn=3;
      if (trn>len) trn=len;
      IInvestigate (data, trn);
      data+=trn;
      len-=trn;
   }
}
*/

long Found (void){
   return found;
}

int SearchFile (char *szFileName){
   int iHan = Open (szFileName, TRY|RO|CRT);
   if (iHan==-1) return 0;
   StartSearch();
   BYTE buf[512];
   DWORD todo=GetFileSize (iHan);
   while (todo){
      int trn=512;
      if (trn>todo) trn=(int)todo;
      Read (buf, iHan, trn);
      Investigate (buf, trn);
      todo-=trn;
   }
   Close (iHan);
   return Found()!=0;
}