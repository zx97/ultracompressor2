/*
 * UltraCompressor II - io.h
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
/* io.h - Borland low-level I/O shim.  Maps to POSIX open/read/write/... */
#ifndef __UC2_IO_H
#define __UC2_IO_H
#include <portdefs.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define O_BINARY 0
#define O_TEXT   0

/* These are already declared by fcntl.h / unistd.h:
 *   open, close, read, write, lseek, tell(our wrapper), access, chmod,
 *   dup, dup2, creat.  Add the Borland-specific extras below. */
#define _read  read
#define _write write

long tell(int handle);
int  eof(int handle);
long filelength(int handle);
int  setmode(int handle, int mode);
int  chsize(int handle, long size);   /* truncate/grow file */
int  _chmod(const char *path, int func, int attrib = 0); /* Borland _chmod */

#endif
