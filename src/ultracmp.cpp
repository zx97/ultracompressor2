/*
 * UltraCompressor II - ultracmp.cpp
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
// ultracmp.cpp - UltraCompressor / UltraDecompressor for Linux UC2 port.
// Compression engine ported from original t/ULTRACMP.CPP (Borland C++ / DOS)
// with assembly hash/match finders rewritten in plain C.
// Decompression uses the extracted original engine in uc2_decomp.cpp.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "video.h"
#include "diverse.h"
#include "compint.h"
#include "bitio.h"
#include "ultracmp.h"
#include "vmem.h"
#include "superman.h"
#include "neuroman.h"
#include "handle.h"
#include "mem.h"
#include "fletch.h"
#include "llio.h"
#include "dos.h"
#include "comoterp.h"
#include "tree.h"
#include "debuglog.h"

FREC Fin, Fout;

/* --- Decompression interface (defined in uc2_decomp.cpp) --- */
extern void UC2DecompInit(void);
extern int UC2Decompress(int method, DWORD master, BYTE bDelta, DWORD len, WORD *csum);

/* --- Globals referenced by superman.cpp / compint.cpp --- */
BYTE bCBar = 0;
BYTE bDBar = 0;
WORD hhint = 0, llen = 0, ddst = 0;

/* --- External declarations --- */
extern DWORD dwLM1;
extern void SecureSmast(void);
void RPutLenDst(WORD len, WORD dst);
extern struct MODE MODE;

/**********************************************************************/
/*  Compression tuning                                                  */
/**********************************************************************/
static WORD wMd = 1400;   // normal max search depth
static WORD wLd = 900;    // lazy max search depth
static WORD wLl = 200;    // limit for lazy evaluation
static WORD wGu = 180;    // give up chain search length

void TuneComp(WORD wMaxSearch, WORD wMaxLazySearch,
              WORD wLimitLazy, WORD wLimitSearch)
{
    wMd = wMaxSearch;
    wLd = wMaxLazySearch;
    wLl = wLimitLazy;
    wGu = wLimitSearch;
}

/**********************************************************************/
/*  Constants                                                           */
/**********************************************************************/
#define EOB_MARK   (125*512+1U)
#define MAX_LEN    200
#define MAX_XLEN   32760U
unsigned max_dist = 125 * 512U;
#define MAX_DIST   max_dist
#define READ_SIZE  512U
#define MAX_DEPTH  wMd
#define LAZY_DEPTH wLd
#define LAZY_LIMIT wLl
#define GIVE_UP    wGu

#define BUF_SIZE   (MAX_DIST + 2 * READ_SIZE)
#define HASH_SIZE  8192

/**********************************************************************/
/*  Hash-chain buffers                                                  */
/**********************************************************************/
static BYTE *mal_pwPrev;
static BYTE *mal_pwHash;
static BYTE *mal_pwLen;
static BYTE *mal_pbDataC;

static WORD *pwPrevL;
static WORD *pwPrevH;
static WORD *pwHash;
static WORD *pwLen;
BYTE        *pbDataC;

static WORD wLH;   // last byte referred to in hash chains
static WORD wLR;   // last byte read
static WORD wLP;   // last byte to be processed in current batch
WORD        wTOE;  // next byte to be encoded

/**********************************************************************/
/*  Init / exit for hash-chain buffers                                  */
/**********************************************************************/
static BYTE specStat = 0;
static BYTE *spp1;
static BYTE *spp2;

void InitPrevL(void)
{
    if (specStat)
        mal_pwPrev = spp1;
    else
        mal_pwPrev = (BYTE *)xmalloc(32768L * 2 + (BUF_SIZE - 32768L) * 2 + 15L, TMP);
    pwPrevL = (WORD *)normalize(mal_pwPrev);
    if (!specStat) {
        RegDat((BYTE *)pwPrevL, 65534U);
    }
}

void InitAlloc1(void)
{
    if (specStat)
        mal_pbDataC = spp2;
    else {
        mal_pbDataC = (BYTE *)exmalloc(BUF_SIZE + 15L);
        if (!mal_pbDataC)
            mal_pbDataC = (BYTE *)xmalloc(BUF_SIZE + 15L, TMP);
    }
    pbDataC = normalize(mal_pbDataC);
}

void InitAlloc2(void)
{
    pwPrevH = (WORD *)MK_FP(FP_SEG(pwPrevL) + 4096, 0);

    mal_pwHash = (BYTE *)xmalloc(HASH_SIZE * 2L + 15L, TMP);
    pwHash = (WORD *)normalize(mal_pwHash);

    mal_pwLen = (BYTE *)xmalloc(HASH_SIZE * 2L + 15L, TMP);
    pwLen = (WORD *)normalize(mal_pwLen);
}

void ExitAlloc(void)
{
    specStat = 1;
    spp1 = mal_pwPrev;
    spp2 = mal_pbDataC;

    xfree(mal_pwHash, TMP);
    xfree(mal_pwLen, TMP);
}

void NoSpec(void)
{
    InvHashC();
    if (specStat) {
        xfree(spp1, TMP);
        xfree(spp2, TMP);
        specStat = 0;
        UnRegDat();
    }
}

/**********************************************************************/
/*  Hash macros                                                         */
/**********************************************************************/
#define HL1 3
#define HL2 6

#define HASH(p) ((WORD)(((pbDataC[p]) ^                     \
                        ((pbDataC[p + 1]) << HL1) ^          \
                        ((0x7F & (pbDataC[p + 2])) << HL2))))

#define PutPrev(wIndex, wValue) {          \
    if ((wIndex) < 32768U) {              \
        pwPrevL[(wIndex)] = (wValue);      \
    } else {                               \
        pwPrevH[(wIndex) ^ 0x8000] = (wValue); \
    }                                      \
}

#define GetPrev(wIndex) \
    (((wIndex) < 32768U) ? pwPrevL[(wIndex)] : pwPrevH[(wIndex) ^ 0x8000])

#define Enter(wPos) {                       \
    WORD hash = HASH(wPos);                 \
    WORD tmp = pwHash[hash];                \
    pwHash[hash] = (wPos);                  \
    PutPrev((wPos), tmp);                   \
    pwLen[hash]++;                          \
}

#define Remove(wPos) pwLen[HASH(wPos)]--

/**********************************************************************/
/*  Hash-chain management (C only, no asm)                              */
/**********************************************************************/
static void EnterMuch(WORD wFrom, WORD wOnto)
{
    for (WORD i = wFrom; i <= wOnto; i++)
        Enter(i);
}

static void RemoveMuch(WORD wFrom, WORD wOnto)
{
    for (WORD i = wFrom; i <= wOnto; i++)
        Remove(i);
}

/**********************************************************************/
/*  File input                                                          */
/**********************************************************************/
static BYTE bCtr;

WORD ReadBlock(void)
{
    if (bCBar)
        Hint();
    else
        bCtr = 0;

    WORD ret;
    WORD backup = wLR;
    BrkQ();
    if (++wLR == MAX_DIST + READ_SIZE)
        wLR = 0;

    ret = CReader(pbDataC + wLR, READ_SIZE);

    FletchUpdate(&Fin, pbDataC + wLR, ret);

    if (wLR == 0)
        RapidCopy(pbDataC + MAX_DIST + READ_SIZE, pbDataC, READ_SIZE);
    if (ret == 0) {
        wLR = backup;
    } else {
        wLR += ret - 1;
    }
    return ret;
}

/**********************************************************************/
/*  EMS/XMS cache helpers -- no-op on Linux                             */
/**********************************************************************/
static char fCache = 0;
static char fCheck = 0;

void OptInit(void)
{
    if (!fCheck) {
        fCache = 0;   /* disable on Linux: avoids malloc16 handle bugs */
        fCheck = 1;
    }
}

void Store(void)   { /* no-op */ }
void ReStore(int)  { /* no-op */ }

/**********************************************************************/
/*  Master / file init                                                  */
/**********************************************************************/
static DWORD dwLasMas = 4000000000L;
DWORD dwLM = 4000000000L;

void InvHashC(void)
{
    dwLM = 4000000000L;
    dwLM1 = 4000000000L;
}

BYTE bDDelta;
BYTE bRel;

void InitANA1(DWORD dwIndex)
{
    bCtr = 0;
    OptInit();

    WORD wLen;
    Transfer(NULL, &wLen, dwIndex);

    wLH = MAX_DIST + READ_SIZE - wLen;
    if (dwLM == dwIndex && dwLM == dwLasMas && MODE.bCompressor < 4) {
        bRel = 1;
    } else {
        if (MODE.bCompressor > 3) {
            InvHashC();
            dwLasMas = 4000000000L;
        }
        Transfer((BYTE *)(pbDataC + wLH), &wLen, dwIndex);
        dwLasMas = dwIndex;
        bRel = 0;
    }
}

BYTE InitANA2(DWORD dwIndex, BYTE bDelta)
{
    static BYTE bLDelta = 255;
    if ((dwLasMas == dwIndex) && fCache) {
        if (dwLM == dwIndex && dwLM == dwLasMas && bLDelta == bDelta)
            ReStore(0);
        else
            ReStore(1);
    } else {
        memset(pwHash, 0, HASH_SIZE * 2);
        memset(pwLen, 0, HASH_SIZE * 2);
        EnterMuch(wLH, MAX_DIST + READ_SIZE - 10);
        if (fCache)
            Store();
    }
    bLDelta = bDelta;
    dwLM = dwLasMas = dwIndex;

    wLR = MAX_DIST + READ_SIZE - 1;
    WORD b1, b2 = 0;
    if (((b1 = ReadBlock()) == READ_SIZE) && ((b2 = ReadBlock()) == READ_SIZE)) {
        wLP = READ_SIZE - 1;
        wLR = READ_SIZE * 2 - 1;
    } else {
        if (b1 + b2 == 0)
            return 0;
        wLP = wLR = b1 + b2 - 1;
    }
    wTOE = 0;

    EnterMuch(MAX_DIST + READ_SIZE - 9, MAX_DIST + READ_SIZE - 1);
    return 1;
}

void ReadMore(void)
{
    if ((wLR + 1) % (MAX_DIST + READ_SIZE) == wLH) {
        RemoveMuch(wLH, wLH + READ_SIZE - 1);
        wLH = (wLH + READ_SIZE) % (MAX_DIST + READ_SIZE);
    }
    wLP = wLR;
    if (ReadBlock() != READ_SIZE) {
        if (wLR < 512)
            wLR += MAX_DIST + READ_SIZE;
        wLP = wLR;
        if (wTOE < 512)
            wTOE += MAX_DIST + READ_SIZE;
        if (wLP < 2000 && wTOE > 2000)
            wTOE -= MAX_DIST + READ_SIZE;
    }
}

/**********************************************************************/
/*  Match management (C rewrites of original asm)                       */
/**********************************************************************/
static WORD MatchLen(WORD p1, WORD p2)
{
    WORD len = 0;
    while (len < MAX_LEN && pbDataC[p1 + len] == pbDataC[p2 + len])
        len++;
    return len;
}

static void FindMaxLen(WORD pos, WORD *len, WORD *mpos,
                       WORD upper, WORD min, WORD hash, WORD wlen)
{
    WORD maxlen = 0;
    WORD maxpos = 0;
    WORD gup = GIVE_UP;

    WORD count = wlen;
    if (count > upper)
        count = upper;

    WORD candidate = pwHash[hash];

    while (count--) {
        if (pbDataC[candidate + min] == pbDataC[pos + min]) {
            WORD ml = MatchLen(pos, candidate);
            if (ml > maxlen) {
                maxlen = ml;
                maxpos = candidate;
                if (maxlen > gup)
                    break;
            }
        }

        candidate = GetPrev(candidate);
    }

    *len = maxlen;
    *mpos = maxpos;

    /* add current position to hash chains */
    WORD tmp = pwHash[hash];
    pwHash[hash] = pos;
    PutPrev(pos, tmp);
    pwLen[hash]++;
}

/**********************************************************************/
/*  Huffman / high-speed buffer globals                                 */
/**********************************************************************/
WORD wLDFreq[2 * (256 + 60)];
WORD wLFreq[2 * 28];
BYTE bLen[256 + 60 + 28];
WORD wCode[256 + 60 + 28];

/**********************************************************************/
/*  HBUFF system                                                        */
/**********************************************************************/
WORD wBNo = 28;
#define MAXBNO 50
#define HIBUFS 490

WORD *ppwBufs[MAXBNO];
WORD wSpSiz = 0;
VPTR vpBufs[MAXBNO];
WORD wLimit[MAXBNO];

WORD *pwIBuf;
WORD bbptr;
WORD biptr;
WORD bgptr;

void SetIBuf(int index)
{
    if (bbptr == index)
        return;
    if (bbptr != 255) {
        if (bbptr + 1 > wSpSiz)
            UnAcc(vpBufs[bbptr]);
    }
    bbptr = index;
    if (index != 255) {
        if (bbptr + 1 > wSpSiz)
            pwIBuf = (WORD *)Acc(vpBufs[bbptr]);
        else
            pwIBuf = ppwBufs[bbptr];
    }
}

void ClearFreq(void)
{
    for (int i = 0; i < 256 + 60; i++)
        wLDFreq[i] = 0;
    for (int i = 0; i < 28; i++)
        wLFreq[i] = 0;
}

void BufInit(void)
{
    static int first = 1;
    if (first)
        first = 0;
    else
        return;
    for (int i = 0; i < MAXBNO; i++) {
        if (i + 1 > wSpSiz)
            vpBufs[i] = Vmalloc(HIBUFS * 2 + 20);
        else
            vpBufs[i] = VNULL;
    }
    bbptr = 255;
    SetIBuf(0);
    char *env = getenv("UC2_HUFBUF");
    if (env) {
        wBNo = (WORD)(atol(env) / HIBUFS);
        if (wBNo < 5)  wBNo = 5;
        if (wBNo > MAXBNO - 3) wBNo = MAXBNO - 3;
    }
}

void InitPut(void)
{
    BufInit();
    biptr = 0;
    bgptr = 0;
    SetIBuf(bgptr);
    ClearFreq();
    TreeInit();
}

/**********************************************************************/
/*  Block flush & symbol output                                         */
/**********************************************************************/
#define HPutLen(val) PUTBITS(wCode[(val) + 256 + 60], bLen[(val) + 256 + 60])
#define HPutLD(val)  PUTBITS(wCode[(val)], bLen[(val)])

void FlushLLD(void)
{
    if ((biptr == 0) && (bgptr == 0))
        return;
    RPutLenDst(3, EOB_MARK);

    TreeGen(wLDFreq, 256 + 60, 13, bLen);
    TreeGen(wLFreq, 28, 13, bLen + 256 + 60);

    PUTBITS(1, 1);
    if ((bgptr == 0) && (biptr < 256))
        TreeEnc(bLen, 1);
    else
        TreeEnc(bLen, 0);
    CodeGen(256 + 60, bLen, wCode);
    CodeGen(28, bLen + 256 + 60, wCode + 256 + 60);

    wLimit[bgptr] = biptr;

    for (WORD j = 0; j < bgptr + 1; j++) {
        WORD lim = wLimit[j];
        SetIBuf(j);
        if (lim) {
            for (WORD i = 0; i < lim; i++) {
                if (pwIBuf[i] < 256) {
                    HPutLD(pwIBuf[i]);
                } else {
                    WORD dst = pwIBuf[i] - 256;
                    if (dst < 16) {
                        HPutLD(dst + 255);
                    } else if (dst < 256) {
                        HPutLD(dst / 16 + 14 + 256);
                        PUTBITS(dst % 16, 4);
                    } else if (dst < 4096) {
                        HPutLD(dst / 256 + 29 + 256);
                        PUTBITS(dst % 256, 8);
                    } else {
                        HPutLD(dst / 4096 + 44 + 256);
                        PUTBITS(dst % 4096, 12);
                    }
                    WORD len = pwIBuf[++i];
                    if (len < 11) {
                        HPutLen(len - 3);
                    } else if (len < 27) {
                        HPutLen((len - 11) / 2 + 8);
                        PUTBITS((len - 11) % 2, 1);
                    } else if (len < 91) {
                        HPutLen((len - 27) / 8 + 16);
                        PUTBITS((len - 27) % 8, 3);
                    } else if (len < 155) {
                        HPutLen(24);
                        PUTBITS(len - 91, 6);
                    } else if (len < 667) {
                        HPutLen(25);
                        PUTBITS(len - 155, 9);
                    } else if (len < 2715) {
                        HPutLen(26);
                        PUTBITS(len - 667, 11);
                    } else {
                        HPutLen(27);
                        PUTBITS(len - 2715, 15);
                    }
                }
            }
        }
    }
    biptr = 0;
    bgptr = 0;
    SetIBuf(bgptr);
}

void MFlush(void)
{
    if (bgptr == wBNo - 1) {
        FlushLLD();
        ClearFreq();
    } else {
        wLimit[bgptr++] = biptr;
        biptr = 0;
        SetIBuf(bgptr);
    }
}

void PutLit(BYTE lit)
{
    pwIBuf[biptr++] = lit;
    wLDFreq[lit]++;
    if (biptr > HIBUFS)
        MFlush();
}

void RPutLenDst(WORD len, WORD dst)
{
    pwIBuf[biptr++] = dst + 256;
    pwIBuf[biptr++] = len;
    if (dst < 16) {
        wLDFreq[dst + 255]++;
    } else if (dst < 256) {
        wLDFreq[dst / 16 + 14 + 256]++;
    } else if (dst < 4096) {
        wLDFreq[dst / 256 + 29 + 256]++;
    } else {
        wLDFreq[dst / 4096 + 44 + 256]++;
    }
    if (len < 11) {
        wLFreq[len - 3]++;
    } else if (len < 27) {
        wLFreq[(len - 11) / 2 + 8]++;
    } else if (len < 91) {
        wLFreq[(len - 27) / 8 + 16]++;
    } else if (len < 155) {
        wLFreq[24]++;
    } else if (len < 667) {
        wLFreq[25]++;
    } else if (len < 2715) {
        wLFreq[26]++;
    } else {
        wLFreq[27]++;
    }
}

void PutLenDst(WORD len, WORD dst)
{
    RPutLenDst(len, dst);
    if (biptr > HIBUFS)
        MFlush();
}

void FlushPut(void)
{
    FlushLLD();
    PUTBITS(0, 1);
    FlushBitsOut();
}

/**********************************************************************/
/*  Compressor core                                                     */
/**********************************************************************/
void SpComp(WORD len, WORD dst)
{
    InitBitsOut();
    InitPut();
    while (len) {
        if (len > 30000) {
            PutLenDst(30000, dst);
            len -= 30000;
        } else {
            PutLenDst(len, dst);
            len = 0;
        }
    }
    Hint();
    FlushPut();
}

void UltraCompressor(DWORD dwMaster, BYTE bDelta)
{
    if (dwMaster != NOMASTER)
        SecureSmast();

    bDDelta = bDelta;

    if (hhint) {
        SpComp(llen, ddst);
        SetIBuf(255);
        return;
    }

    WORD len, pos, dst;
    WORD tlen = 0, tpos = 0;
    BYTE flag = 0;

    FletchInit(&Fin);
    FletchInit(&Fout);

    WORD wLen;

    InitAlloc1();
    InitANA1(dwMaster);

    FREC SFin = Fin;
    FREC SFout = Fout;
    Transfer(NULL, &wLen, dwMaster);
    Fin = SFin;
    Fout = SFout;

    InitPrevL();

    if (!bRel && bDelta && (dwMaster != SUPERMASTER)) {
        DeltaBlah db;
        InitDelta(&db, bDelta);
        Delta(&db, (BYTE *)(pbDataC + wLH), wLen);
    }

    InitBitsOut();
    InitPut();
    InitAlloc2();
    if (InitANA2(dwMaster, bDelta)) {
        for (;;) {
            if (flag) {
                len = tlen;
                pos = tpos;
                flag = 0;
            } else {
                WORD hash = HASH(wTOE);
                WORD wlen = pwLen[hash];
                FindMaxLen(wTOE, &len, &pos, MAX_DEPTH, 2, hash, wlen);
            }
            if (len > 2) {
                if (wTOE > pos)
                    dst = wTOE - pos;
                else
                    dst = wTOE - pos + MAX_DIST + READ_SIZE;
                if (dst > MAX_DIST)
                    goto skip;

                if (len < LAZY_LIMIT) {
                    WORD hash = HASH(wTOE + 1);
                    WORD wlen = pwLen[hash];
                    FindMaxLen(wTOE + 1, &tlen, &tpos, LAZY_DEPTH, len, hash, wlen);
                    if (tlen > len) {
                        flag = 1;
                        goto skip;
                    }
                } else {
                    Enter(wTOE + 1);
                }
                if (len > 3) {
                    EnterMuch(wTOE + 2, wTOE + len - 1);
                } else {
                    Enter(wTOE + 2);
                }

                wTOE += len;
            even_more:
                if (wTOE > wLP) {
                    if (wLP == wLR) {
                        len -= wTOE - wLP - 1;
                        wTOE = wLP + 1;
                        if (len > 2) {
                            PutLenDst(len, dst);
                            goto done;
                        } else {
                            wTOE -= len;
                            PutLit(pbDataC[wTOE]);
                            if (len == 2)
                                PutLit(pbDataC[wTOE + 1]);
                        }
                        goto done;
                    } else {
                        ReadMore();
                        if (wLP != wLR)
                            wTOE %= (MAX_DIST + READ_SIZE);
                    }
                }

                if ((len >= MAX_LEN) && (dst < 124 * 512U)) {
                    if (len < MAX_XLEN) {
                        WORD other;
                        if (wTOE < dst)
                            other = wTOE + MAX_DIST + READ_SIZE - dst;
                        else
                            other = wTOE - dst;
                        WORD tmp = MatchLen(wTOE, other);
                        if (tmp + len > MAX_XLEN)
                            tmp = MAX_XLEN - len;
                        if (tmp) {
                            EnterMuch(wTOE, wTOE + tmp - 1);
                            wTOE += tmp;
                            len += tmp;
                            goto even_more;
                        }
                    }
                }

                PutLenDst(len, dst);
            } else {
            skip:
                PutLit(pbDataC[wTOE]);
                wTOE++;
                if (wTOE > wLP) {
                    if (wLP == wLR)
                        goto done;
                    else {
                        ReadMore();
                        if (wLP != wLR)
                            wTOE %= (MAX_DIST + READ_SIZE);
                    }
                }
            }
        }
    }
done:
    FlushPut();
    ExitAlloc();
    SetIBuf(255);
}

/**********************************************************************/
/*  Decompression adapter                                               */
/**********************************************************************/
void UltraDecompressor(DWORD dwMaster, BYTE bDelta, DWORD len)
{
    FletchInit(&Fout);
    WORD csum = 0;
    int ret = UC2Decompress(4, dwMaster, bDelta, len, &csum);
    if (ret < 0) {
        UC2ErrorLog("UC2Decompress failed: master=%lu delta=%d len=%lu ret=%d\n",
                    (unsigned long)dwMaster, (int)bDelta, (unsigned long)len, ret);
    }
}
