/*
 * UltraCompressor II - bitio.cpp
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
// BITIO.CPP

#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include "main.h"
#include "compint.h"
#include "video.h"
#include "diverse.h"
#include "fletch.h"

/**********************************************************************/
/*$ FUNCTION BLOCK  // method(s), function(s)
    PURPOSE OF FUNCTION BLOCK:
       "Gate" to outside world.
       Either PutWord OR GetWord is used.
*/

// copy in nuke !!!, supercopy in nuke+ultracmp !!!!!
#define IBUFS 1024U // input buffer size, decompression
#define OBUFS 2048U // output buffer size (LLIO cached as well), compr

WORD *buffer;
int specbuf;
WORD ptr = 0;

static void near _fastcall PutWord (WORD w){
   buffer[ptr++] = w;
   if (ptr==OBUFS/2){
   CWriter ((BYTE *)buffer, OBUFS);
#ifdef UCPROX
      if (debug) Out (7,"{CW:%u}",OBUFS);
#endif
      ptr=0;
   }
}


WORD _fastcall GetWord (void){
   if (ptr==IBUFS/2-1){
      WORD ret = buffer[IBUFS/2-1];
      CReader ((BYTE *)buffer, IBUFS);
//      FletchUpdate (&Fin, (BYTE *)buffer, no);
      ptr=0;
      return ret;
   } else {
      return buffer[ptr++];
   }
}


/**********************************************************************/
/*$ FUNCTION BLOCK  // method(s), function(s)
    PURPOSE OF FUNCTION BLOCK:
       Bit oriented output.
*/

static WORD wOutBuf;
static BYTE bOutLeft;

// initialize bit output
void _fastcall InitBitsOut (void){
   bOutLeft = 16;
   wOutBuf = 0;
   ptr=0;
   buffer = (WORD *)xmalloc (OBUFS, TMP);
//   printf ("%ld ",farcoreleft());
}

// flush bit output
void _fastcall FlushBitsOut (void){
   if (bOutLeft!=16){
      PutWord (wOutBuf);
   }
   if (ptr!=0){
      CWriter ((BYTE *)buffer, ptr*2);
//      FletchUpdate (&Fout, (BYTE *)buffer, ptr*2);
#ifdef UCPROX
      if (debug) Out (7,"{CW:%u}",ptr*2);
#endif
   }
   xfree ((BYTE *)buffer, TMP);
}

// put bits to output
void _fastcall PUTBITS (WORD val, BYTE len){
   if (len<bOutLeft){                  // more than enough space left
      bOutLeft -= len;                 // less space left
      wOutBuf |= val<<(bOutLeft);      // bits to OutBuf
   } else if (len!=bOutLeft){          // less than enough space left
      wOutBuf |= val>>(len-bOutLeft);  // as much as possible to OutBuf
      PutWord (wOutBuf);               // export OutBuf
      bOutLeft += 16-len;              // recalculate OutLeft
      wOutBuf = val<<bOutLeft;         // remaining bits to "new" OutBuf
   } else {                            // exactly enough space left
      wOutBuf |= val;                  // exact put
      PutWord (wOutBuf);               // export OutBuf
      bOutLeft = 16;                   // clean...
      wOutBuf = 0;                     // ...OutBuf
   }
}


/**********************************************************************/
/*$ FUNCTION BLOCK  // method(s), function(s)
    PURPOSE OF FUNCTION BLOCK:
       Bit oriented output.
*/

WORD wInBuf = 0;
BYTE bInLeft = 16;
WORD wIn13 = 0;

// advance bit pointer 'len' bits (HARD!)
void _fastcall SKIP (BYTE len){
   wIn13 = (wIn13<<len) & 0x1FFF;        // eat 'len' bits and clean start
   if (bInLeft<len){                     // less in InBuf than needed
      wIn13 |= wInBuf<<(len-bInLeft);    // all of InBuf to In13
      wInBuf = GetWord();                // get new word
      bInLeft = 16-len+bInLeft;          // new InLeft
      wIn13 |= wInBuf>>(bInLeft);        // add rest of bits
      wInBuf &= 0xFFFF>>(16-bInLeft);    // make used bits 0
   } else if (bInLeft!=len){             // more in InBuf than needed
      wIn13 |= wInBuf>>(bInLeft-len);    // part of InBuf to In13
      bInLeft -= len;                    // new InLeft (len less)
      wInBuf &= 0xFFFF>>(16-bInLeft);    // make used bits 0
   } else {                              // "perfect match"
      wIn13 |= wInBuf;                   // use all
      wInBuf = GetWord();                // replace all
      bInLeft = 16;                      // 16 bits left
   }
}

// initialize bit input
void _fastcall InitBitsIn (WORD *spec){
   if (spec){
      buffer = spec;
      specbuf=1;
   } else {
      buffer = (WORD *)xmalloc (IBUFS, TMP);
      specbuf=0;
   }
   CReader ((BYTE *)buffer, IBUFS);
//   FletchUpdate (&Fin, (BYTE *)buffer, no);
   ptr=0;
   wInBuf = GetWord();
   bInLeft = 16;
   wIn13 = 0;
   SKIP (13);
}

void _fastcall ExitBitsIn (){
   if (!specbuf) xfree ((BYTE *)buffer, TMP);
}

// read 'len' bits
WORD _fastcall GETBITS (BYTE len){
   WORD ret;
   if (len<14){
      ret = wIn13 >> (13-len);
      SKIP (len);
      return ret;
   } else {
      return (GETBITS(len-8)<<8) | GETBITS(8);
   }
}

// snoop 13 bits (needed for superfast Huffman decoder)
WORD _fastcall SNOOP13 (void){
   return wIn13;
}
