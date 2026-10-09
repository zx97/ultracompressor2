/*
 * UltraCompressor II - conio_linux.cpp
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
/**
 * conio_linux.cpp - Linux ANSI implementation of the Borland <conio.h> shim.
 *
 * We used to use ncurses, but that switches to the alternate screen buffer
 * on many terminals, causing all output to vanish when the program exits.
 * For a command-line tool we want persistent output, so we use direct ANSI
 * escape sequences instead.
 */
#include <portdefs.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

int directvideo = 0;
int factor = 1;

static int conio_started = 0;
static int conio_tty = 0;
static int cur_fg = LIGHTGRAY;
static int cur_bg = BLACK;
static int cur_bold = 0;
static int cur_blink = 0;
static int win_x1 = 1, win_y1 = 1, win_x2 = 80, win_y2 = 25;

/* saved terminal settings for raw mode */
static struct termios saved_tios;
static int tios_saved = 0;

extern "C" {

static void apply_attr(void);

void conio_init(void)
{
    if (conio_started) return;
    conio_started = 1;
    conio_tty = isatty(fileno(stdout)) && isatty(fileno(stdin));
    cur_fg = LIGHTGRAY; cur_bg = BLACK; cur_bold = 0; cur_blink = 0;
}

void conio_restore(void)
{
    if (!conio_started) return;
    if (conio_tty) {
        /* reset terminal attributes */
        fputs("\033[0m", stdout);          /* reset colors */
        fputs("\033[?25h", stdout);        /* show cursor */
        fflush(stdout);
        if (tios_saved) {
            tcsetattr(fileno(stdin), TCSAFLUSH, &saved_tios);
            tios_saved = 0;
        }
    }
    conio_started = 0;
}

/* map DOS color (0-15) to ANSI foreground code */
static int ansi_fg(int c)
{
    int base = c & 7;
    int bright = (c & 8) ? 60 : 0;   /* 30 vs 90, etc. */
    switch (base) {
        case 0: return 30 + bright; /* black/grey  */
        case 1: return 34 + bright; /* blue        */
        case 2: return 32 + bright; /* green       */
        case 3: return 36 + bright; /* cyan        */
        case 4: return 31 + bright; /* red         */
        case 5: return 35 + bright; /* magenta     */
        case 6: return 33 + bright; /* brown/yellow*/
        case 7: return 37 + bright; /* light grey  */
    }
    return 37;
}

/* map DOS color (0-7) to ANSI background code */
static int ansi_bg(int c)
{
    switch (c & 7) {
        case 0: return 40; /* black   */
        case 1: return 44; /* blue    */
        case 2: return 42; /* green   */
        case 3: return 46; /* cyan    */
        case 4: return 41; /* red     */
        case 5: return 45; /* magenta */
        case 6: return 43; /* brown   */
        case 7: return 47; /* grey    */
    }
    return 40;
}

static void apply_attr(void)
{
    if (!conio_tty) return;
    int fg = ansi_fg(cur_fg);
    int bg = ansi_bg(cur_bg);
    fprintf(stdout, "\033[%d;%d", fg, bg);
    if (cur_bold)  fputs(";1", stdout);
    if (cur_blink) fputs(";5", stdout);
    fputs("m", stdout);
}

void clrscr(void)
{
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
}

void clreol(void)
{
    fputs("\033[K", stdout);
    fflush(stdout);
}

void delline(void)
{
    fputs("\033[M", stdout);
    fflush(stdout);
}

void insline(void)
{
    fputs("\033[L", stdout);
    fflush(stdout);
}

void highvideo(void) { cur_bold = 1; }
void lowvideo(void)  { cur_bold = 0; }
void normvideo(void) { cur_bold = 0; cur_blink = 0; }

void textcolor(int c)
{
    cur_fg = c & 0x8F;
    cur_blink = (c & BLINK) ? 1 : 0;
}

void textbackground(int c)
{
    cur_bg = c & 0x07;
}

void textattr(int c)
{
    cur_fg = c & 0x8F;
    cur_bg = (c >> 4) & 0x07;
    cur_bold = (c & 0x08) ? 1 : 0;
    cur_blink = (c & BLINK) ? 1 : 0;
}

void gotoxy(int x, int y)
{
    if (!conio_tty) return;
    fprintf(stdout, "\033[%d;%dH", y, x);
    fflush(stdout);
}

int wherex(void)
{
    if (!conio_tty) return 1;
    /* ANSI terminals don't provide a reliable query without reading
       the response.  Return 1 as a safe default. */
    return 1;
}

int wherey(void)
{
    if (!conio_tty) return 1;
    return 1;
}

void window(int x1, int y1, int x2, int y2)
{
    win_x1 = x1; win_y1 = y1; win_x2 = x2; win_y2 = y2;
}

/* set raw mode for single-character input */
static void set_raw(void)
{
    if (!conio_tty || tios_saved) return;
    tcgetattr(fileno(stdin), &saved_tios);
    tios_saved = 1;
    struct termios t = saved_tios;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSAFLUSH, &t);
}

int getch(void)
{
    if (!conio_tty) return getchar();
    set_raw();
    int c = getchar();
    return c;
}

int kbhit(void)
{
    if (!conio_tty) return 0;
    set_raw();
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fileno(stdin), &fds);
    struct timeval tv = { 0, 0 };
    return select(fileno(stdin) + 1, &fds, NULL, NULL, &tv) > 0;
}

int cprintf(const char *fmt, ...)
{
    char buf[8192];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (!conio_tty) {
        fputs(buf, stdout);
    } else {
        apply_attr();
        fputs(buf, stdout);
    }
    fflush(stdout);
    return n;
}

int cputs(const char *s)
{
    if (!conio_tty) {
        fputs(s, stdout);
    } else {
        apply_attr();
        fputs(s, stdout);
    }
    fflush(stdout);
    return 0;
}

int putch(int c)
{
    char s[2] = { (char)c, 0 };
    return cputs(s);
}

void gettextinfo(struct text_info *t)
{
    t->winleft   = (unsigned char)win_x1;
    t->wintop    = (unsigned char)win_y1;
    t->winright  = (unsigned char)win_x2;
    t->winbottom = (unsigned char)win_y2;
    t->attribute = (unsigned char)((cur_bg << 4) | (cur_fg & 0x0F));
    t->normattr  = t->attribute;
    t->currmode  = conio_tty ? C80 : MONO;
    t->screenheight = (unsigned char)(conio_tty ? 25 : 25);
    t->screenwidth  = (unsigned char)(conio_tty ? 80 : 80);
    t->curx = (unsigned char)wherex();
    t->cury = (unsigned char)wherey();
}

} /* extern "C" */
