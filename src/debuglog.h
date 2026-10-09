/*
 * UltraCompressor II - debuglog.h
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
#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Open the debug log file (typically /tmp/uc2_debug.log).
 * Safe to call multiple times; subsequent calls are no-ops.
 * Returns 0 on success, -1 on failure.
 */
int OpenDebugLog(const char *path);

/* Close the debug log file. Registered with atexit() automatically. */
void CloseDebugLog(void);

/* Write a debug message to the log file only.
 * Format is: [TIMESTAMP] DEBUG: <message>
 */
void DebugLog(const char *fmt, ...);

/* Write an error message to stderr (with light red color if terminal)
 * AND to the debug log file.
 * Format is: [TIMESTAMP] ERROR: <message>
 */
void UC2ErrorLog(const char *fmt, ...);

/* Convenience macro for conditional debug logging */
#define DBGLOG(...) do { if (debug) DebugLog(__VA_ARGS__); } while(0)

#ifdef __cplusplus
}
#endif

#endif /* DEBUGLOG_H */
