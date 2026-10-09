/*
 * UltraCompressor II - mem.h
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
/*
   MEM.H (C) 1992 Nico de Vries, released under the GNU LGPL. Unlimited
   use is also granted to Jean-loup Gailly and Robert Jung.
   Partial (C) 1993 Jan-Pieter Cornet.
*/

// 'extra' memory (UBM's, EMS emulated UMB)
void far* exmalloc (long);              // Allocate "extra" memory.
void exfree (void far*);                // Release it.
int extest (void far*);                 // Test if "extra" memory.
int coreleft4 (void);                   // Number of free 4k "extra" memory blocks.

void donate16(void far*);               // Donate a 16k base memory block.
// donate16 should be called BEFORE calls to other mem16 functions!

void* malloc16 (int grp);
void free16 (int grp, int idx = 0);
int coreleft16 (int);                  // Number of free 16k cache blocks in a group.
void to16 (int, unsigned, void far*, int);              // to cache
void from16 (void far*, int, unsigned, int);            // from cache


// Global variables that indicate maximum EMS, XMS, INT15 usage (in
// 16K blocks) and UMB usage (in paragraphs). Default value==0 ("do not use")
// Set to 0xFFFF to allow any usage. Notice these values should be
// specified BEFORE InitMem is called.
extern unsigned gmaxEMS;
extern unsigned gmaxXMS;
extern unsigned gmaxI15;
extern unsigned gmaxUMB;

// Return error codes for EMS, XMS and generic extended memory faults.
#define EMSFAIL 1
#define XMSFAIL 1
#define I15FAIL 1

// MEMORY GROUPS (current UltraCompressor settings)
//
// 1 supermaster storage
// 2 most recently used hash scheme
// 3 master storage
// 4 disk cache
// 5 vmem support
// 6 most recently use master cache
// 7 booster I/O temp buffer
