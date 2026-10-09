/*
 * UltraCompressor II - conio.h
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
/* conio.h - Borland <conio.h> shim, backed by ncurses for the Linux port. */
#ifndef __UC2_CONIO_H
#define __UC2_CONIO_H
#include <portdefs.h>

struct text_info {
    unsigned char winleft, wintop, winright, winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight, screenwidth;
    unsigned char curx, cury;
};
#define MONO  1
#define BW40  2
#define C40   3
#define BW80  4
#define C80   5
#define LASTMODE 6

/* color constants (Borland compatible) */
#define BLACK       0
#define BLUE        1
#define GREEN       2
#define CYAN        3
#define RED         4
#define MAGENTA     5
#define BROWN       6
#define LIGHTGRAY   7
#define DARKGRAY    8
#define LIGHTBLUE   9
#define LIGHTGREEN  10
#define LIGHTCYAN   11
#define LIGHTRED    12
#define LIGHTMAGENTA 13
#define YELLOW      14
#define WHITE       15
#define BLINK       0x80

extern int directvideo;    /* Borland conio global (0 = BIOS, 1 = direct) */

/* text mode constants (Borland conio) */
#define C40   1
#define C80   3
#define C4350 3
#define C0    0

#ifdef __cplusplus
extern "C" {

#endif

void conio_init(void);     /* start ncurses (no-op if not a tty) */
void conio_restore(void);  /* end ncurses */
void clrscr(void);
void clreol(void);
void delline(void);
void insline(void);
void highvideo(void);
void lowvideo(void);
void normvideo(void);
void textcolor(int c);
void textbackground(int c);
void textattr(int c);
void gotoxy(int x, int y);
int  wherex(void);
int  wherey(void);
void window(int x1, int y1, int x2, int y2);
int  getch(void);
int  kbhit(void);
int  cprintf(const char *fmt, ...);
int  cputs(const char *s);
int  putch(int c);
void gettextinfo(struct text_info *t);

#ifdef __cplusplus
}

#endif

#endif
