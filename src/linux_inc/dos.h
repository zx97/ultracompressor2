/*
 * UltraCompressor II - dos.h
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
 * dos.h - minimal Borland <dos.h> shim for Linux port.
 *
 * Real-mode DOS / BIOS interrupts are not available on Linux.  The
 * int86() / geninterrupt() stubs are provided so the code compiles; they
 * return benign "not present" values.  Hardware-probing code paths
 * (EMS/XMS, real-mode dispatch) are reimplemented elsewhere on POSIX.
 */
#ifndef __UC2_DOS_H
#define __UC2_DOS_H

#include <portdefs.h>
#include <time.h>

/* Borland-only directory helpers (NOT in <unistd.h>); declared here so any
 * TU that includes <dos.h> sees them.  The standard ones (chdir/getcwd/
 * rmdir/mkdir) come from <unistd.h>/<sys/stat.h>. */
int   getdisk(void);
int   setdisk(int drive);
int   getdiskfree(unsigned drive, void *dtable);

struct WORDREGS {
    unsigned short ax, bx, cx, dx, si, di, cflag, flags;
};
struct BYTEREGS {
    unsigned char al, ah, bl, bh, cl, ch, dl, dh;
};
union REGS {
    struct WORDREGS x;
    struct BYTEREGS h;
};
struct SREGS {
    unsigned short es, cs, ss, ds;
};

/* Global "pseudo-register" used by _AX .. _DL and geninterrupt() */
extern union REGS _regs;

#define _AX _regs.x.ax
#define _BX _regs.x.bx
#define _CX _regs.x.cx
#define _DX _regs.x.dx
#define _SI _regs.x.si
#define _DI _regs.x.di
#define _FLAGS _regs.x.flags
#define _AL _regs.h.al
#define _AH _regs.h.ah
#define _BL _regs.h.bl
#define _BH _regs.h.bh
#define _CL _regs.h.cl
#define _CH _regs.h.ch
#define _DL _regs.h.dl
#define _DH _regs.h.dh

#define _ES _sregs.es
#define _DS _sregs.ds
#define _CS _sregs.cs
#define _SS _sregs.ss
extern struct SREGS _sregs;

/* Borland compatibility aliases */
#define AH_INREG(r)  ((r).h.ah)
#define AL_INREG(r)  ((r).h.al)

/* Call a (real-mode) interrupt.  Stubbed on Linux: does nothing useful
 * but keeps register contents so callers can test flags. */
int int86(int intno, union REGS *in, union REGS *out);
int int86x(int intno, union REGS *in, union REGS *out, struct SREGS *sregs);
#define geninterrupt(n) int86((n), &_regs, &_regs)
int bdos(int dosfn, unsigned dx, unsigned al);
int bdosptr(int dosfn, void *dx, unsigned al);

/* getvect / setvect -> not meaningful on Linux; return 0 / no-op */
void (*getvect(int intr))(void);
void setvect(int intr, void (*func)(void));

/* Date / time */
struct dosdate_t {
    unsigned char day;        /* 1-31 */
    unsigned char month;      /* 1-12 */
    unsigned short year;      /* 1980-2099 */
    unsigned char dayofweek;  /* 0=Sun .. 6=Sat */
};
struct dostime_t {
    unsigned char hour;       /* 0-23 */
    unsigned char minute;     /* 0-59 */
    unsigned char second;     /* 0-59 */
    unsigned char hsecond;    /* 0-99 (hundredths) */
};

/* Borland struct date / struct time */
struct date {
    int  da_year;            /* 1980..2099 */
    char da_day;             /* 1..31 */
    char da_mon;             /* 1..12 */
};
struct time {
    unsigned char ti_min;    /* 0..59 */
    unsigned char ti_hour;   /* 0..23 */
    unsigned char ti_hund;   /* 0..99 */
    unsigned char ti_sec;    /* 0..59 */
};

/* DOS file-attribute search flags (Borland FA_* live in dos.h) */
#define FA_NORMAL  0x00
#define FA_RDONLY  0x01
#define FA_HIDDEN  0x02
#define FA_SYSTEM  0x04
#define FA_LABEL   0x08
#define FA_DIREC   0x10
#define FA_ARCH    0x20

/* Borland extras (strupr, stricmp, intr, intdos, allocmem, ...) */
#include <borland.h>

/* Borland struct ftime (DOS file time, bit-packed) */
struct ftime {
    unsigned ft_tsec : 5;     /* 0-29 (two-second units) */
    unsigned ft_min  : 6;     /* 0-59 */
    unsigned ft_hour : 5;     /* 0-23 */
    unsigned ft_day  : 5;     /* 1-31 */
    unsigned ft_month: 4;     /* 1-12 */
    unsigned ft_year : 7;     /* 0-119 (1980 + year) */
};

void _dos_getdate(struct dosdate_t *d);
void _dos_gettime(struct dostime_t *t);

/* Borland getdate()/gettime() clash with glibc's getdate(); use these. */
void uc2_getdate(struct date *d);
void uc2_gettime(struct time *t);
#define getdate uc2_getdate
#define gettime uc2_gettime

/* Borland ioctl() is not meaningful on Linux; this shim returns 0. */
int uc2_ioctl(int handle, ...);
#define ioctl uc2_ioctl
/* set file date/time of an open or path file (best effort) */
unsigned _dos_setftime(int handle, unsigned date, unsigned time);
unsigned _dos_getftime(int handle, unsigned *date, unsigned *time);
unsigned _dos_setfileattr(const char *fname, WORD attrib);
unsigned _dos_getfileattr(const char *fname, WORD *attrib);

/* sleep in milliseconds-ish (Borland delay()) */
void delay(unsigned milliseconds);

#endif /* __UC2_DOS_H */
