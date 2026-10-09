/*
 * UltraCompressor II - vol.h
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
char* getvol(char drive);
// Returns the volume label of the specified drive, NULL if no volume
// label found. Returns a pointer to an internal static object.
// Drives are always 0 = default, 1 = A: etc...

int setvol(char drive, char* label);
// Sets the volume label of the specified drive. Returns -1 on error or
// 0 on success.

int rmvol(char drive);
// Removes the volume label on the specified drive. Returns -1 on error or
// 0 on success.

