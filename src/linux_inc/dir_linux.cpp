/*
 * UltraCompressor II - dir_linux.cpp
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
 * dir_linux.cpp - POSIX implementation of Borland <dir.h> directory scan
 * and path splitting for the UC2 Linux port.
 */
#include <portdefs.h>
#include <dir.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* -------- DOS date/time packing -------- */
unsigned dos_fdate(time_t t)
{
    struct tm *tm = localtime(&t);
    int year = tm->tm_year + 1900 - 1980;
    if (year < 0) year = 0;
    return (unsigned)(((year & 0x7F) << 9) |
                      (((tm->tm_mon + 1) & 0x0F) << 5) |
                      (tm->tm_mday & 0x1F));
}
unsigned dos_ftime(time_t t)
{
    struct tm *tm = localtime(&t);
    return (unsigned)(((tm->tm_hour & 0x1F) << 11) |
                      (((tm->tm_min) & 0x3F) << 5) |
                      ((tm->tm_sec / 2) & 0x1F));
}

/* -------- wildcard match (DOS semantics, case-insensitive) -------- */
static int dos_match(const char *name, const char *pat)
{
    while (*pat) {
        if (*pat == '*') {
            if (!*++pat) return 1;
            while (*name) {
                if (dos_match(name, pat)) return 1;
                name++;
            }
            return 0;
        }
        if (*pat == '?') {
            if (!*name) return 0;
        } else if (toupper((unsigned char)*pat) != toupper((unsigned char)*name)) {
            return 0;
        }
        pat++; name++;
    }
    return (*name == 0);
}

/* -------- scan state keyed by ffblk pointer -------- */
struct DirState {
    DIR *dir;
    char dirpart[1024];
    char pat[256];
    int  attrib;
};
#define MAXSTATES 16
static DirState states[MAXSTATES];
static int find_free_state(void)
{
    for (int i = 0; i < MAXSTATES; i++)
        if (states[i].dir == NULL) return i;
    return -1;
}

static int fill_entry(DirState *st, struct ffblk *ffblk)
{
    struct dirent *de;
    while ((de = readdir(st->dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        if (!dos_match(de->d_name, st->pat))
            continue;
        /* build full path for stat */
        char full[2048];
        snprintf(full, sizeof(full), "%s/%s",
                 st->dirpart[0] ? st->dirpart : ".", de->d_name);
        struct stat stt;
        if (stat(full, &stt) != 0)
            continue;
        int isdir = S_ISDIR(stt.st_mode);
        /* attribute filtering */
        if (isdir) {
            if (!(st->attrib & _A_SUBDIR)) continue;
        } else {
            /* skip hidden/system unless requested */
            if ((st->attrib & (_A_HIDDEN | _A_SYSTEM)) == 0) {
                if (de->d_name[0] == '.') continue; /* hide dotfiles like DOS hidden */
            }
        }
        /* Store as "name.ext" (with dot) — this is what DOS findfirst returns
         * and what Name2Rep/fit expect. */
        strncpy(ffblk->ff_name, de->d_name, 255);
        ffblk->ff_name[255] = 0;
        ffblk->ff_fsize = (long)stt.st_size;
        ffblk->ff_fdate = dos_fdate(stt.st_mtime);
        ffblk->ff_ftime = dos_ftime(stt.st_mtime);
        ffblk->ff_attrib = isdir ? (char)_A_SUBDIR : (char)_A_ARCH;
        if (stt.st_mode & 0) { /* map readonly */ }
        if (!(stt.st_mode & 0200)) ffblk->ff_attrib |= (char)_A_RDONLY;
        return 1;
    }
    return 0;
}

/* Resolve every component of a directory path case-insensitively (DOS semantics).
 * Returns 1 with out[] set to an existing path, or 0 if not resolvable. */
int resolvedir_ci(const char *in, char *out, size_t outsz)
{
    char work[1024];
    snprintf(work, sizeof work, "%s", in);
    size_t rl = 0;
    if (work[0] == '/') {
        if (outsz < 2) return 0;
        out[rl++] = '/';
    }
    char *save = NULL;
    for (char *tok = strtok_r(work, "/", &save); tok; tok = strtok_r(NULL, "/", &save)) {
        char found[256];
        int ok = 0;
        if (!strcmp(tok, ".") || !strcmp(tok, "..")) {
            snprintf(found, sizeof found, "%s", tok);
            ok = 1;
        } else {
            char cur[1024];
            if (rl == 0) { cur[0] = '.'; cur[1] = 0; }
            else { memcpy(cur, out, rl); cur[rl] = 0; }
            DIR *d = opendir(cur);
            if (d) {
                struct dirent *de;
                while ((de = readdir(d)) != NULL) {
                    if (strcasecmp(de->d_name, tok) == 0) {
                        snprintf(found, sizeof found, "%s", de->d_name);
                        ok = 1;
                        break;
                    }
                }
                closedir(d);
            }
        }
        if (!ok) return 0;
        if (rl && out[rl - 1] != '/') {
            if (rl + 1 >= outsz) return 0;
            out[rl++] = '/';
        }
        size_t n = strlen(found);
        if (rl + n >= outsz) return 0;
        memcpy(out + rl, found, n);
        rl += n;
    }
    if (rl == 0) {
        if (outsz < 2) return 0;
        out[0] = '.'; out[1] = 0;
    } else {
        out[rl] = 0;
    }
    return 1;
}

int findfirst(const char *path, struct ffblk *ffblk, int attrib)
{
    char dp[1024];
    char pat[256];
    /* split directory part and pattern */
    const char *slash = strrchr(path, '/');
    if (slash) {
        int n = (int)(slash - path);
        if (n >= (int)sizeof(dp)) n = sizeof(dp) - 1;
        memcpy(dp, path, n); dp[n] = 0;
        strncpy(pat, slash + 1, sizeof(pat) - 1); pat[sizeof(pat)-1] = 0;
    } else {
        dp[0] = 0;
        strncpy(pat, path, sizeof(pat) - 1); pat[sizeof(pat)-1] = 0;
    }
    if (dp[0] == 0) strcpy(dp, ".");
    if (!resolvedir_ci(dp, dp, sizeof dp)) return -1;

    int idx = find_free_state();
    if (idx < 0) return -1;
    DirState *st = &states[idx];
    st->dir = opendir(dp);
    if (!st->dir) { st->dir = NULL; return -1; }
    strncpy(st->dirpart, dp, sizeof(st->dirpart) - 1); st->dirpart[sizeof(st->dirpart)-1]=0;
    strncpy(st->pat, pat, sizeof(st->pat) - 1); st->pat[sizeof(st->pat)-1]=0;
    st->attrib = attrib;
    /* store index in reserved so findnext can find it */
    {
        int *p = (int *)ffblk->ff_reserved;
        *p = idx;
    }
    if (fill_entry(st, ffblk)) return 0;
    closedir(st->dir); st->dir = NULL;
    return -1;
}

int findnext(struct ffblk *ffblk)
{
    if (!ffblk) return -1;
    int *p = (int *)ffblk->ff_reserved;
    int idx = *p;
    if (idx < 0 || idx >= MAXSTATES) return -1;
    DirState *st = &states[idx];
    if (!st->dir) return -1;
    if (fill_entry(st, ffblk)) return 0;
    closedir(st->dir); st->dir = NULL;
    return -1;
}

int fnsplit(const char *path, char *drive, char *dir, char *name, char *ext)
{
    int bits = 0;
    const char *p = path;
    const char *lastslash = NULL, *dot = NULL;
    const char *colon = strchr(path, ':');
    if (colon) { bits |= WILDDRIVE; if (drive) { int n=colon-path; if(n>3)n=3; memcpy(drive,path,n); drive[n]=0; } } else if (drive) drive[0]=0;
    const char *base = colon ? colon + 1 : path;
    for (const char *q = base; *q; q++) {
        if (*q == '/' || *q == '\\') lastslash = q;
        if (*q == '.') dot = q;
    }
    const char *dirs = base;
    const char *nm = lastslash ? lastslash + 1 : base;
    const char *xp = dot && dot > nm ? dot : NULL;
    if (lastslash) {
        bits |= WILDDIR;
        if (dir) {
            int n = (int)(lastslash - base) + 1;
            if (n > 1023) n = 1023;
            memcpy(dir, base, n); dir[n] = 0;
        }
    } else if (dir) dir[0] = 0;
    if (xp) {
        bits |= WILDFILE | WILDEXT;
        int nl = (int)(xp - nm);
        if (name) { memcpy(name, nm, nl); name[nl] = 0; }
        if (ext) strcpy(ext, xp);
    } else {
        bits |= WILDFILE;
        if (name) strcpy(name, nm);
        if (ext) ext[0] = 0;
    }
    (void)p;
    return bits;
}

void fnmerge(char *path, const char *drive, const char *dir,
             const char *name, const char *ext)
{
    char *p = path;
    if (drive && *drive) { strcpy(p, drive); p += strlen(p); }
    if (dir && *dir) { strcpy(p, dir); p += strlen(p); if (p > path && p[-1] != '/' && p[-1] != '\\') *p++ = '/'; }
    if (name && *name) { strcpy(p, name); p += strlen(p); }
    if (ext && *ext) { if (*ext != '.') *p++ = '.'; strcpy(p, ext); p += strlen(p); }
}

char *searchpath(const char *name)
{
    static char buf[1024];
    const char *path = getenv("PATH");
    if (!path) return NULL;
    char *dup = strdup(path);
    char *tok = strtok(dup, ":");
    while (tok) {
        snprintf(buf, sizeof(buf), "%s/%s", tok, name);
        if (access(buf, X_OK) == 0) { free(dup); return buf; }
        tok = strtok(NULL, ":");
    }
    free(dup);
    return NULL;
}
