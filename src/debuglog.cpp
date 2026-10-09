/*
 * UltraCompressor II - debuglog.cpp
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
#include "debuglog.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Light red ANSI for stderr errors */
#define ANSI_LIGHTRED "\x1B[1;31m"
#define ANSI_RESET    "\x1B[0m"

static FILE *debug_fp = NULL;
static int debug_opened = 0;

static void write_timestamp(FILE *fp)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fp, "[%s] ", buf);
}

static int is_tty_stderr(void)
{
    return isatty(fileno(stderr));
}

int OpenDebugLog(const char *path)
{
    if (debug_opened) return 0;
    if (!path) path = "/tmp/uc2_debug.log";
    debug_fp = fopen(path, "a");
    if (!debug_fp) {
        fprintf(stderr, "Failed to open debug log %s: %s\n", path, strerror(errno));
        return -1;
    }
    debug_opened = 1;
    write_timestamp(debug_fp);
    fprintf(debug_fp, "=== UC2 debug session started (pid %ld) ===\n", (long)getpid());
    fflush(debug_fp);
    atexit(CloseDebugLog);
    return 0;
}

void CloseDebugLog(void)
{
    if (debug_fp) {
        write_timestamp(debug_fp);
        fprintf(debug_fp, "=== UC2 debug session ended ===\n\n");
        fflush(debug_fp);
        fclose(debug_fp);
        debug_fp = NULL;
        debug_opened = 0;
    }
}

void DebugLog(const char *fmt, ...)
{
    if (!debug_fp) {
        /* Try opening default path if not already open */
        OpenDebugLog(NULL);
        if (!debug_fp) return;
    }
    va_list args;
    va_start(args, fmt);
    write_timestamp(debug_fp);
    fprintf(debug_fp, "DEBUG: ");
    vfprintf(debug_fp, fmt, args);
    va_end(args);
    fflush(debug_fp);
}

void UC2ErrorLog(const char *fmt, ...)
{
    va_list args;

    /* Always print to stderr with color if available */
    va_start(args, fmt);
    if (is_tty_stderr()) {
        fprintf(stderr, "%s", ANSI_LIGHTRED);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "%s", ANSI_RESET);
    } else {
        vfprintf(stderr, fmt, args);
    }
    va_end(args);

    /* Also log to file */
    if (!debug_fp) {
        OpenDebugLog(NULL);
    }
    if (debug_fp) {
        va_start(args, fmt);
        write_timestamp(debug_fp);
        fprintf(debug_fp, "ERROR: ");
        vfprintf(debug_fp, fmt, args);
        va_end(args);
        fflush(debug_fp);
    }
}
