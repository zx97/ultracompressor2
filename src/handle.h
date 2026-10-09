/*
 * UltraCompressor II - handle.h
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
// MENU.H

#include <setjmp.h>

void BrkQ ();  // test for ctrl-C/Break

void ceask (void);

int CeAskOpen (char *name, char *why, int skip); // smart open ask, 1 -> try again

extern jmp_buf jbCritRep;
extern int ceflag;
   // 0=no problems found
   // 1=critical error ocured
extern int cestat;
   // 0=handle local
   // 1=let external handler take care of it
   // 2=during handling of critical error, abort NOW
   // 3=always retry
   // 4=optionat turboterminate mode (like 0 except for fatal)
extern int nobreak;

// Critical section management:
//    ... inocent code
//    CSR;
//    ... code to be used in retry's (e.g. reposition file pointer)
//    CSS;
//    ... critical code
//    CSE;
//    ... inocent code
//
// In case of no retry code CSR+CSS can be replaced by CSB.
// In case the error handle should ALWAYS call retry, replace CSR with CSRA
//
// Linux port: the original implemented these with setjmp/longjmp so that a
// DOS critical error (INT 24) could restart the section.  That mechanism is
// DOS-specific and was a source of infinite retry loops on Linux, so the
// critical section now simply executes its body once.

#define CSR  {
#define CSS
#define CSE  }
#define CSB  {
#define CSRA {
