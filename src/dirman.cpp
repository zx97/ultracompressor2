/*
 * UltraCompressor II - dirman.cpp
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
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dir.h>
#include <borland.h>
#include "main.h"
#include "vmem.h"
#include "comoterp.h"
#include "dirman.h"
#include "video.h"
#include "handle.h"
#include "llio.h"
#include "handle.h"
#include "diverse.h"
#include "menu.h"

struct lloo {
   char bDisk;
   char pcPath[260];
   VPTR next;
};

VPTR root=VNULL;

unsigned char gbDisk=2; // default c:
char gpcPath[260];

static unsigned char sbDisk;
static char spcPath[260];

static unsigned char sbBDisk;
static char spcBPath[260];

static unsigned char sbEDisk;
static char spcEPath[260];

void Keep (void){
   char pcPath[260];
   CSB;
      getcwd (pcPath,99);
   CSE;
   VPTR tmp = Vmalloc (sizeof (lloo));
   ((lloo*)V(tmp))->bDisk = 0;
   strcpy (((lloo*)V(tmp))->pcPath, pcPath);
   ((lloo*)V(tmp))->next = root;
   root = tmp;
}

void Back (void){
   if (IS_VNULL(root)) IE();
   char pcPath[260];
   strcpy (pcPath,((lloo*)V(root))->pcPath);
   CSB;
      chdir (pcPath);
   CSE;
   VPTR tmp = root;
   root = ((lloo*)V(root))->next;
   Vfree (tmp);
}

char szTPath[260];

void GKeep (void){
   CSB;
      getcwd (gpcPath,199);
      strcpy (szTPath, gpcPath);
   CSE;
}

void GKeep2 (void){
   static int first=1;
   if (!first) return;
   first=0;
   CSB;
      getcwd (szTPath, 199);
   CSE;
}

void GBack (void){
   CSB;
      chdir (gpcPath);
      chdir (szTPath);
   CSE;
}

void SKeep (void){
   CSB;
      getcwd (spcPath,199);
   CSE;
}

void SBack (void){
   CSB;
      chdir (spcPath);
   CSE;
}

void BKeep (void){
   CSB;
      getcwd (spcBPath,199);
   CSE;
}

void BBack (void){
   CSB;
      chdir (spcBPath);
   CSE;
}

void EKeep (void){
   CSB;
      getcwd (spcEPath,199);
   CSE;
}

void EBack (void){
   CSB;
      chdir (spcEPath);
   CSE;
}

void ChPath (char *path){
   /* Linux: case-sensitive, no drive letters */
   int ret;
   CSB;
      ret = chdir (path);
   CSE;
   if (ret!=0)
      FatalError (185,"failed to change directory into %s",path);
}

int TstPath (char *path){
   /* Linux: simple path existence check */
   if (path[0]==0) return 1;
   if (path[0]=='.' && path[1]==0) return 1;
   if (access(path, 0)==0) return 1;
   return 1;
}
int RCMD (char *path){
   if (0==strcmp(path,".")) return 1;
   int ret;
   CSB;
      ret = mkdir(path, 0755)==0;
   CSE;
   if (!ret){
      for (int i=strlen(path);i>0;i--){
	 if ((path[i]=='\\' || path[i]=='/') && path[i-1]!=':'){
	    char sep = path[i];
	    path[i]=0;
	    if (!TstPath(path)){
		if (RCMD (path)){
		   path[i]=sep;
                   if (0==strcmp(path,".")) return 1;
		   CSB;
		      ret = mkdir(path, 0755)==0;
		   CSE;
		   if (ret)
		      return 1;
		   else {
		      goto giveup;
		   }
		} else {
		   path[i]=sep;
		   goto giveup;
		}
	    }
	 }
      }
giveup:
      if (0==strcmp(path,".")) return 1;
      Error (85,"cannot create directory %s",path);
   }
   return ret;
}

int MkPath (char *path){
   if (MODE.bMKDIR!=3){ // not NEVER
      if (MODE.bMKDIR!=2){ // not ALWAYS
	 Menu ("\x6Create directory %s ?",path);
	 Option ("",'Y',"es");
	 Option ("",'N',"o");
	 Option ("",'A',"lways create directories");
	 Option ("N",'e',"ver create directories");
	 switch(Choice()){
	    case 1:
	       goto doit;
	    case 2:
	       return 0;
	    case 3:
	       MODE.bMKDIR = 2;
	       goto doit;
	    case 4:
	       MODE.bMKDIR = 3;
	       return 0;
	 }
      }
doit:
      return RCMD (path);
   } else
      return 0;
}

char tmppath[260]="*************************";

int tmpp=0;
void ktmp (void){
   if (tmpp==1){
      Out (1,"\x7Removing temporary files/directories ");
      StartProgress (-1, 1);
      KillTmpPath();
      EndProgress();
      Out (1,"\n\r");
   }
}

int fktmp=0;

char* MkTmpPath (void){
   if (tmpp==0) fktmp=1;
   tmpp=1;
   strcpy (tmppath, (char *)CONFIG.pbTPATH);
   strcpy (tmppath, TmpFile (tmppath,1,".TMP"));
   int f = Open (tmppath, CR|MAY|NOC);
   if (f==-1){
      strcpy (tmppath, TmpFile ((char *)CONFIG.pbTPATH,1,".TMP"));
      f = Open (tmppath, CR|MAY|NOC);
      if (f==-1){
	 strcpy (tmppath, TmpFile ((char *)CONFIG.pbTPATH,1,".TMP"));
	 f = Open (tmppath, CR|MUST|NOC);
      }
   }
   Close (f);
   Delete (tmppath);
   CSB;
      mkdir(tmppath, 0755);
   CSE;
   return tmppath;
}

static int ctr;

void blk (void){
   Hint();
}

static int special;
static int mmask;

void KillPath (char *p){
   int sp=special;
   special=0;
   tmpp=2;
   struct ffblk ffblk;
   int ret;
   char nam[15];
   Keep();
   ChPath (p);
   int done;
again:
   CSB;
      done = findfirst ("*.*",&ffblk,0xF7);
   CSE;
sppwb:
   CSB;
sppw:
      if (!done && strcmp(ffblk.ff_name,".")==0)
	 done = findnext (&ffblk);
      if (!done && strcmp(ffblk.ff_name,"..")==0){
	 done = findnext (&ffblk);
	 goto sppw;
      }
      if (!done && mmask && (strstr(ffblk.ff_name, ".U~K")!=NULL)){
	 strupr (ffblk.ff_name);
	 done = findnext (&ffblk);
	 goto sppw;
      }
   CSE;
   if (!done){
      BrkQ();
      blk();
      strcpy (nam, ffblk.ff_name);
      if (ffblk.ff_attrib&FA_DIREC){
	 KillPath (nam);
      } else {
	 Delete (nam);
	 done = findnext (&ffblk);
	 goto sppwb;
      }
      goto again;
   }
delfail:
   if (sp) return;
   ChPath ("..");
   CSB;
      ret = rmdir (p);
   CSE;
   Back();
   if (ret!=0){
      if (!TstPath(p)) return; // some glitch in OS/2 2.x ???
      Error (60,"failed to delete directory %s",p);
      static int derr=0;
      derr++;
      if (derr==100) FatalError (180,"attempt to delete directory failed too often");
   }
}

void SKillPath (char *p){
   mmask=1;
   special=1;
   KillPath (p);
   mmask=0;
   special=0;
}

void KillTmpPath (void){
   ctr=0;
   KillPath (tmppath);
}
