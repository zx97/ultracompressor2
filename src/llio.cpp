/*
 * UltraCompressor II - llio.cpp
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
// LLIO.CPP - Linux POSIX file I/O for UC2.
// Replaces the original DOS-specific file I/O with standard POSIX calls.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <dirent.h>

#include "main.h"
#include "mem.h"
#include "llio.h"
#include "vmem.h"
#include <dir.h>

/*** AFD (Archive File Descriptor) - compatible with existing codebase ***/

#define NONE  0
#define UNKN  0xFFFFFFFFUL

struct AFD {
   char pcFileName[120];
   int iIntHandle;    /* POSIX file descriptor */
   DWORD dwPos;
   DWORD dwLen;
   VPTR cachet;       /* unused on Linux */
   int maxentry;
   WORD cmode;
   DWORD offset;
   DWORD size;
   DWORD vfptr;
};

static AFD afdlst[32];
static AFD* afdl[32];

int Han (int iHan){ return afdl[iHan]->iIntHandle; }

void diskfull (char *);

static void MacAcc (char *pcPath){ (void)pcPath; }

/*** Low-level I/O functions ***/

static void I_Tell (int fd){ (void)fd; }

static void I_Seek (int fd, DWORD pos){
   lseek (fd, (off_t)pos, SEEK_SET);
}

static void I_Read (int fd, BYTE *buf, WORD len){
   read (fd, buf, len);
}

static void I_Write (int fd, const BYTE *buf, WORD len){
   ssize_t r = write (fd, buf, len);
   (void)r;
}

static WORD I_GetLen (int fd){
    off_t cur = lseek (fd, 0, SEEK_CUR);
    off_t end = lseek (fd, 0, SEEK_END);
    lseek (fd, cur, SEEK_SET);
    return (WORD)(end);
}

static DWORD I_GetLen32 (int fd){
    off_t cur = lseek (fd, 0, SEEK_CUR);
    off_t end = lseek (fd, 0, SEEK_END);
    lseek (fd, cur, SEEK_SET);
    return (DWORD)(end);
}

/*** Cache functions - no-ops on Linux (write directly) ***/

void CacheTabSize (int han){ (void)han; }
void CacheR (int han){ (void)han; }
void CacheRT (int han){ (void)han; }
void CacheW (int han){ (void)han; }
void KillCache (int han){ (void)han; }
void UnCache (int han){ (void)han; }
void DFlush (char *why){ (void)why; }

int fcopy = 0;  /* flag checked by exit.cpp */
void exitcopy (void){ /* no deferred copies on Linux */ }

/*** File open/close/read/write ***/

static int exists_ci(char *path);

int Open (char *pcPath, int mode){
   int flags = 0;
   int osmode = 0;
   if (mode & RO) flags = O_RDONLY;
   if (mode & RW) flags = O_RDWR;
   if (mode & CR) flags = O_RDWR | O_CREAT | O_TRUNC;
   if (flags == 0) flags = O_RDWR;

   /* resolve case-insensitive match so open() works on case-sensitive fs */
   (void)exists_ci(pcPath);

   int fd = open (pcPath, flags, 0644);
   if (fd < 0) return -1;

   /* Find a free slot */
   for (int i = 1; i < 32; i++){
      if (afdl[i] == NULL){
         afdl[i] = &afdlst[i];
         afdl[i]->iIntHandle = fd;
         afdl[i]->dwPos = 0;
         afdl[i]->dwLen = UNKN;
         afdl[i]->cmode = NONE;
         afdl[i]->cachet = VNULL;
         afdl[i]->maxentry = 0;
         afdl[i]->offset = 0;
         afdl[i]->size = 0;
         afdl[i]->vfptr = 0;
         strncpy (afdl[i]->pcFileName, pcPath, 119);
         return i;
      }
   }
   close (fd);
   return -1;
}

void Close (int iHandle){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return;
   close (afdl[iHandle]->iIntHandle);
   afdl[iHandle] = NULL;
}

WORD Read (BYTE *bBuf, int iHandle, WORD wSize){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return 0;
   ssize_t r = read (afdl[iHandle]->iIntHandle, bBuf, wSize);
   if (r < 0) r = 0;
   afdl[iHandle]->dwPos += r;
   return (WORD)r;
}

void Write (BYTE *bBuf, int iHandle, WORD wSize){
    if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return;
    ssize_t written = write (afdl[iHandle]->iIntHandle, bBuf, wSize);
    if (written < 0) {
        diskfull("write error");
        return;
    }
    afdl[iHandle]->dwPos += written;
}

void Seek (int iHandle, DWORD dwPos){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return;
   lseek (afdl[iHandle]->iIntHandle, (off_t)dwPos, SEEK_SET);
   afdl[iHandle]->dwPos = dwPos;
}

DWORD Tell (int iHandle){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return 0;
   return afdl[iHandle]->dwPos;
}

DWORD GetFileSize (int iHandle){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return 0;
   return I_GetLen32 (afdl[iHandle]->iIntHandle);
}

void SetFileSize (int iHandle, DWORD dwNewSize){
   if (iHandle < 1 || iHandle >= 32 || !afdl[iHandle]) return;
   ftruncate (afdl[iHandle]->iIntHandle, (off_t)dwNewSize);
}

/*** Temp file names ***/

static int tmpcounter = 0;

char *TmpFile (char *pcLocat, int pure, char *useext){
   static char path[260];
   const char *basedir = pcLocat;
   if (!pure) {
      /* pcLocat is a file path: extract its directory */
      char drive[MAXDRIVE];
      char dir[MAXDIR];
      char file[MAXFILE];
      char ext[MAXEXT];
      fnsplit(pcLocat, drive, dir, file, ext);
      if (dir[0]) {
         /* use directory component, stripping trailing slash */
         size_t len = strlen(dir);
         if (len > 1 && (dir[len-1] == '/' || dir[len-1] == '\\'))
            dir[len-1] = '\0';
         basedir = dir;
      } else {
         basedir = ".";
      }
   }
   snprintf (path, sizeof(path), "%s/U$~%c%c%c%s",
             basedir,
             'A' + (tmpcounter / 676) % 26,
             'A' + (tmpcounter / 26) % 26,
             'A' + tmpcounter % 26,
             useext ? useext : ".TMP");
   tmpcounter++;
   return path;
}

/*** File operations ***/

static int exists_ci(char *path)
{
    if (access(path, F_OK) == 0) return 1;
    char *tmp = strdup(path);
    if (!tmp) return 0;
    char *base = basename(tmp);
    char *dir = dirname(tmp);
    char rdir[MAXPATH];
    if (!resolvedir_ci(dir, rdir, sizeof rdir)) { free(tmp); return 0; }
    DIR *d = opendir(rdir);
    if (!d) { free(tmp); return 0; }
    struct dirent *de;
    int found = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcasecmp(de->d_name, base) == 0) {
            /* correct path case in-place (DOS is case-insensitive) */
            if (rdir[0] == '.' && rdir[1] == 0) {
                strcpy(path, de->d_name);
            } else {
                strcpy(path, rdir);
                size_t dl = strlen(path);
                if (dl == 0 || path[dl-1] != '/') path[dl++] = '/';
                strcpy(path + dl, de->d_name);
            }
            found = 1;
            break;
        }
    }
    closedir(d);
    free(tmp);
    return found;
}

int Exists (char *pcPath){
   return exists_ci(pcPath);
}

void Rename (char *pcOld, char *pcNew){
   rename (pcOld, pcNew);
}

void Delete (char *pcPath){
   unlink (pcPath);
}

void CopyFile (char *from, char *onto){
   int fd_in = open (from, O_RDONLY);
   if (fd_in < 0) return;
   int fd_out = open (onto, O_WRONLY|O_CREAT|O_TRUNC, 0644);
   if (fd_out < 0) { close(fd_in); return; }
   char buf[8192];
   ssize_t r;
   while ((r = read (fd_in, buf, sizeof(buf))) > 0)
      write (fd_out, buf, r);
   close (fd_in);
   close (fd_out);
}
