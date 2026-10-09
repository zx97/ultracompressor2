/*
 * UltraCompressor II - uc2_decomp.cpp
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
/* UC2_DECOMP.CPP - Extracted decompression core from libunuc2.c
 * Adapted for the Linux UC2 port callback system.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "main.h"
#include "superman.h"
#include "neuroman.h"
#include "fletch.h"
#include "compint.h"
#include "ultracmp.h"

/* --- Types matching libunuc2.c --- */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* Supermaster compressed data (bootstraps the decompressor) */
static const u8 supermaster_compressed[] = {
#include "super.inc"
};

/* --- Master constants (must match superman.h and libunuc2.c) --- */
#define SM_SUPERMASTER 0
#define SM_NOMASTER    1
#define SM_FIRSTMASTER 2

/* --- Supermaster (49152 bytes, decompressed once at init) --- */
u8 *g_supermaster = NULL;

/* --- Little-endian helpers --- */
typedef struct { u8 b[2]; } u16le;
typedef struct { u8 b[4]; } u32le;

static u16 get16(u16le v) { return (u16)v.b[0] | (u16)v.b[1]<<8; }
static u32 get32(u32le v) { return (u32)v.b[0] | (u32)v.b[1]<<8 | (u32)v.b[2]<<16 | (u32)v.b[3]<<24; }

/* --- Callback readers/writers (mirror libunuc2 structs) --- */
struct reader {
    void *context;
    int (*read)(void *context, void *buffer, unsigned size);
};

struct writer {
    void *context;
    int (*write)(void *context, const void *buffer, unsigned size);
};

struct range {
    u8 *ptr, *end;
};

static unsigned range_len(struct range *r) { return (unsigned)(r->end - r->ptr); }

static void *range_get(struct range *r, unsigned n)
{
    unsigned l = range_len(r);
    if (l < n) return 0;
    u8 *p = r->ptr;
    r->ptr += n;
    return p;
}

static int buf_read(void *context, void *ptr, unsigned size)
{
    struct range *br = context;
    unsigned have = range_len(br);
    if (have < size) {
        if (!have) return 0;
        size = have;
    }
    memcpy(ptr, br->ptr, size);
    br->ptr += size;
    return size;
}

static int buf_write(void *context, const void *ptr, unsigned size)
{
    struct range *bw = context;
    unsigned free = range_len(bw);
    if (free < size) {
        if (free == 0) return 0;
        size = free;
    }
    memcpy(bw->ptr, ptr, size);
    bw->ptr += size;
    return 0;
}

/* --- Bits --- */
struct bits {
    u32 bits;
    unsigned have_bits;
    unsigned head, tail;
    struct reader *rd;
    u8 buffer[4 << 10];
};

static int bits_init(struct bits *bi, struct reader *rd)
{
    bi->head = 0;
    bi->tail = 0;
    bi->bits = 0;
    bi->have_bits = 0;
    bi->rd = rd;
    return 0;
}

static void bits_skip(struct bits *bi, unsigned n)
{
    assert(bi->have_bits >= n);
    bi->have_bits -= n;
}

static int bits_feed(struct bits *bi, unsigned n)
{
    assert(n <= 16);
    if (bi->have_bits < n) {
        unsigned have = bi->tail - bi->head;
        if (have <= 1) {
            if (have == 1)
                bi->buffer[0] = bi->buffer[bi->tail - 1];
            bi->tail = have;
            int r = bi->rd->read(bi->rd->context, bi->buffer + have, sizeof bi->buffer - have);
            if (r <= 0)
                return r ? r : -5; // Truncated
            bi->head = 0;
            bi->tail += r;
        }
        bi->bits = bi->bits << 16 | bi->buffer[bi->head] | bi->buffer[bi->head + 1] << 8;
        bi->head += 2;
        bi->have_bits += 16;
    }
    return 0;
}

static int bits_peek(struct bits *bi, unsigned n)
{
    int r = bits_feed(bi, n);
    if (r < 0) return r;
    return bi->bits >> (bi->have_bits - n) & ((1 << n) - 1);
}

static int bits_get(struct bits *bi, unsigned n)
{
    int r = bits_peek(bi, n);
    if (r >= 0) bits_skip(bi, n);
    return r;
}

static void bits_destroy(struct bits *bi) {}

/* --- Fletcher checksum --- */
struct csum {
    u32 value;
};

static void csum_init(struct csum *cs)
{
    cs->value = 0xA55A;
}

static void csum_update(struct csum *cs, const u8 *p, unsigned n)
{
    if (!n) return;
    u32 v = cs->value;
    const u8 *e = p + n - 1;
    if (v > 0xffff)
        v ^= *p++ << 8;
    while (p < e) {
        v ^= p[0] | p[1]<<8;
        p += 2;
    }
    v &= 0xffff;
    if (p == e)
        v ^= *p | 0x10000;
    cs->value = v;
}

static u16 csum_get(struct csum *cs)
{
    return (u16)cs->value;
}

/* --- Delta --- */
struct delta {
    u8 size;
    u8 index;
    u8 val[8];
};

static void delta_init(struct delta *db, u8 type)
{
    struct delta d = {.size = type};
    *db = d;
}

static void delta_apply(struct delta *db, u8 *p, unsigned size)
{
    struct delta d = *db;
    while (size--) {
        u8 v = *p;
        *p++ = v - d.val[d.index];
        d.val[d.index] = v;
        if (++d.index == d.size)
            d.index = 0;
    }
    *db = d;
}

static void delta_revert(struct delta *db, u8 *dst, const u8 *src, unsigned size)
{
    struct delta d = *db;
    while (size--) {
        u8 v = *src++ + d.val[d.index];
        d.val[d.index] = *dst++ = v;
        if (++d.index == d.size)
            d.index = 0;
    }
    *db = d;
}

/* --- Circular output buffer --- */
struct cbuffer {
    u16 head, tail;
    unsigned limit;
    struct csum csum;
    u8 data[0x10000];
};

static unsigned cbuf_have(struct cbuffer *cb)
{
    return (u16)(cb->tail - cb->head);
}

static unsigned cbuf_space(struct cbuffer *cb)
{
    return sizeof cb->data - cbuf_have(cb) - 1;
}

static int cbuf_flush(struct writer *wr, struct cbuffer *cb, struct delta *db, u8 *dbuf)
{
    for (;;) {
        unsigned n = cbuf_have(cb);
        if (!n) return 0;
        unsigned u = 0x10000 - cb->head;
        if (n > u) n = u;
        if (cb->limit < n) {
            n = cb->limit;
        }
        u8 *p = cb->data + cb->head;
        csum_update(&cb->csum, p, n);
        if (dbuf) {
            delta_revert(db, dbuf, p, n);
            p = dbuf;
        }
        int r = wr->write(wr->context, p, n);
        if (r < 0)
            return r;
        cb->head += n;
        cb->limit -= n;
        if (!cb->limit)
            break;
    }
    return 0;
}

/* --- Huffman --- */
enum {
    MaxCodeBits = 13,
    LookupSize = 1 << MaxCodeBits
};

static int huff(u32 table[LookupSize], struct bits *bi)
{
    int b = bits_peek(bi, 13);
    if (b < 0) return b;
    u32 c = table[b];
    bits_skip(bi, c >> 24);
    return c & 0xffffff;
}

enum {
    NumByteSym = 256,
    NumDistSym = 60,
    NumLenSym = 28,
    NumSymbols = NumByteSym + NumDistSym + NumLenSym,

    NumLoAsciiSym = 28,
    NumHiByteSym = 128
};

struct dcinfo {
    u8 symprev[NumSymbols];
};

static void dc_init(struct dcinfo *dc);
static int ht_dec(u8 lengths[NumSymbols], struct dcinfo *dc, struct bits *bi, u32 table[LookupSize]);
static int ht_mktree(u32 table[LookupSize], const u8 *lengths, int nlit, int ncodes, const u32 *codes);

enum {
    NumDeltaCodes = MaxCodeBits + 1,
    NumExtraCodes = 1,
    NumLenCodes = NumDeltaCodes + NumExtraCodes,
};

static const u8 vval[NumDeltaCodes][NumDeltaCodes] = {
    { 0,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1},
    { 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0},
    { 2, 1, 3, 4, 5, 6, 7, 8, 9,10,11,12,13, 0},
    { 3, 2, 4, 1, 5, 6, 7, 8, 9,10,11,12,13, 0},
    { 4, 3, 5, 2, 6, 1, 7, 8, 9,10,11,12,13, 0},
    { 5, 4, 6, 3, 7, 2, 8, 1, 9,10,11,12,13, 0},
    { 6, 5, 7, 4, 8, 3, 9, 2,10, 1,11,12,13, 0},
    { 7, 6, 8, 5, 9, 4,10, 3,11, 2,12, 1,13, 0},
    { 8, 7, 9, 6,10, 5,11, 4,12, 3,13, 2, 0, 1},
    { 9, 8,10, 7,11, 6,12, 5,13, 4, 0, 3, 2, 1},
    {10, 9,11, 8,12, 7,13, 6, 0, 5, 4, 3, 2, 1},
    {11,10,12, 9,13, 8, 0, 7, 6, 5, 4, 3, 2, 1},
    {12,11,13,10, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1},
    {13,12, 0,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1}
};

static void default_lengths(u8 d[NumSymbols])
{
    static const u8 rle[] = {
        10,9, 1,7, 1,9, 1,7, 19,9, 1,7, 13,8, 1,7, 11,8, 1,7, 33,8, 1,7, 35,8, 128,10, 16,6, 12,7, 6,8, 10,9, 16,10, 9,4, 9,5, 10,6, 0
    };
    const u8 *s = rle;
    u8 n = s[0];
    do {
        u8 v = s[1];
        s += 2;
        do {
            *d++ = v;
        } while (--n);
        n = *s;
    } while (n);
}

static void dc_init(struct dcinfo *dc)
{
    default_lengths(dc->symprev);
}

enum {
    RepeatCode = MaxCodeBits + 1,
    MinRepeat = 6
};

static int ht_dec(u8 lengths[NumSymbols], struct dcinfo *dc, struct bits *bi, u32 table[LookupSize])
{
    int t = bits_get(bi, 1);
    if (t <= 0) {
        if (t == 0) {
            default_lengths(dc->symprev);
            default_lengths(lengths);
        }
        return t;
    }

    t = bits_get(bi, 2);
    if (t < 0)
        return t;

    u8 tlengths[NumLenCodes];
    for (int i = 0; i < NumLenCodes; i++) {
        int b = bits_get(bi, 3);
        if (b < 0)
            return b;
        tlengths[i] = (u8)b;
    }

    ht_mktree(table, tlengths, NumLenCodes, 0, 0);

    u8 stream[NumSymbols];
    u8 *symp = stream;
    u8 *syme = stream + NumSymbols - NumLoAsciiSym - NumHiByteSym;
    if (t & 1)
        syme += NumLoAsciiSym;
    if (t & 2)
        syme += NumHiByteSym;

    u8 val = 0;
    do {
        int c = huff(table, bi);
        if (c < 0)
            return c;
        if (c == RepeatCode) {
            c = huff(table, bi);
            if (c < 0)
                return c;
            int n = c + MinRepeat - 1;
            for (; n > 0; n--)
                *symp++ = val;
        } else {
            val = c;
            *symp++ = c;
        }
    } while (symp < syme);

    static const u16 rle[][8] = {
        {0x009, 0x202, 0x1, 0x202, 0x12, 0x260, 0x80, 0x258},
        {0x280, 0x80, 0x258},
        {0x009, 0x202, 0x1, 0x202, 0x12, 0x338},
        {0x358}
    };
    const u16 *p = rle[t];
    int i = 0;
    symp = stream;
    do {
        u16 v = *p++;
        int e = i + (v & 0x1ff);
        do {
            lengths[i] = v & 0x200 ? vval[dc->symprev[i]][*symp++] : 0;
        } while (++i < e);
    } while (symp < syme);

    for (int i = 0; i < NumSymbols; i++) {
        assert(lengths[i] <= 13);
        dc->symprev[i] = lengths[i];
    }

    return 0;
}

static int ht_mktree(u32 table[LookupSize], const u8 *lengths, int nlit, int ncodes, const u32 *codes)
{
    int nsym = nlit + ncodes;
    u32 *p = table;
    u32 *e = table + LookupSize;

    for (int l = 1; l <= MaxCodeBits; l++) {
        for (int i = 0; i < nsym; i++) {
            if (lengths[i] == l) {
                int n = 1 << (MaxCodeBits - l);
                if (p + n > e)
                    return -4; // Damaged
                u32 c = i < nlit ? i : codes[i - nlit];
                c |= l << 24;
                do {
                    *p++ = c;
                } while (--n);
            }
        }
    }

    while (p < e)
        *p++ = 1 << 24;

    return 0;
}

/* --- Decompressor core --- */
struct ultra {
    struct bits bi;
    struct dcinfo dc;
    struct cbuffer cb;

    u32 bd_table[LookupSize];
    u32 l_table[LookupSize];
};

static int decode_ht(struct ultra *ultra);
static int decompress_block(struct ultra *ultra);

enum {
    End,
    More
};

/* Use master: returns size, fills buffer if non-NULL */
static int uc2_use_master(BYTE *buffer, unsigned master)
{
    switch (master) {
    case SM_SUPERMASTER:
        if (!g_supermaster) return -1;
        if (buffer)
            memcpy(buffer, g_supermaster, 49152);
        return 49152;
    case SM_NOMASTER:
        if (buffer)
            memset(buffer, 0, 512);
        return 512;
    default: {
        WORD len = 0;
        Transfer(NULL, &len, (DWORD)master);
        if (buffer) {
            if (len > 65535) len = 65535;
            Transfer(buffer, &len, (DWORD)master);
        }
        return len;
    }
    }
}

static int decompressor_ultra(unsigned master, unsigned delta,
                               struct reader *rd, struct writer *wr,
                               unsigned limit, u16 *csum)
{
    int ret;
    u8 *dbuf = NULL;
    struct delta db;

    struct ultra *ultra = (struct ultra *)malloc(sizeof *ultra);
    if (!ultra)
        return -2; // UserFault

    ret = uc2_use_master(ultra->cb.data, master);
    if (ret < 0)
        goto done;
    ultra->cb.limit = limit;
    ultra->cb.head = ultra->cb.tail = ret;
    csum_init(&ultra->cb.csum);

    if (delta) {
        if (master != SM_SUPERMASTER) {
            delta_init(&db, delta);
            delta_apply(&db, ultra->cb.data, ultra->cb.tail);
        }
        dbuf = (u8 *)malloc(sizeof ultra->cb.data);
        ret = -2;
        if (!dbuf)
            goto done;
        delta_init(&db, delta);
    }

    ret = bits_init(&ultra->bi, rd);
    if (ret < 0)
        goto done;

    dc_init(&ultra->dc);
    for (;;) {
        ret = decode_ht(ultra);
        if (ret <= 0)
            break;
        for (;;) {
            int o = decompress_block(ultra);
            ret = cbuf_flush(wr, &ultra->cb, &db, dbuf);
            if (ret < 0)
                goto done;
            if (o != More)
                break;
        }
    }
    bits_destroy(&ultra->bi);
    if (csum)
        *csum = csum_get(&ultra->cb.csum);
    ret = limit - ultra->cb.limit;
done:
    free(dbuf);
    free(ultra);
    return ret;
}

static int decode_ht(struct ultra *ultra)
{
    int ret = bits_get(&ultra->bi, 1);
    if (ret > 0) {
        u8 lengths[NumSymbols];
        u32 *tmp = ultra->bd_table;
        ret = ht_dec(lengths, &ultra->dc, &ultra->bi, tmp);
        if (ret < 0)
            return ret;

        #define D(V,B) ((B)<<20|1<<16|(V))
        static const u32 d_codes[NumDistSym] = {
            D(1,0),     D(2,0),     D(3,0),     D(4,0),     D(5,0),     D(6,0),     D(7,0),     D(8,0),
            D(9,0),     D(10,0),    D(11,0),    D(12,0),    D(13,0),    D(14,0),    D(15,0),    D(16,4),
            D(32,4),    D(48,4),    D(64,4),    D(80,4),    D(96,4),    D(112,4),   D(128,4),   D(144,4),
            D(160,4),   D(176,4),   D(192,4),   D(208,4),   D(224,4),   D(240,4),   D(256,8),   D(512,8),
            D(768,8),   D(1024,8),  D(1280,8),  D(1536,8),  D(1792,8),  D(2048,8),  D(2304,8),  D(2560,8),
            D(2816,8),  D(3072,8),  D(3328,8),  D(3584,8),  D(3840,8),  D(4096,12), D(8192,12), D(12288,12),
            D(16384,12),D(20480,12),D(24576,12),D(28672,12),D(32768,12),D(36864,12),D(40960,12),D(45056,12),
            D(49152,12),D(53248,12),D(57344,12),D(61440,12)
        };
        #undef D
        ret = ht_mktree(ultra->bd_table, lengths, NumByteSym, NumDistSym, d_codes);
        if (ret < 0)
            return ret;

        #define L(V,B) ((B)<<20|(V))
        static const u32 l_codes[NumLenSym] = {
            L(3,0),     L(4,0),     L(5,0),     L(6,0),     L(7,0),     L(8,0),     L(9,0),     L(10,0),
            L(11,1),    L(13,1),    L(15,1),    L(17,1),    L(19,1),    L(21,1),    L(23,1),    L(25,1),
            L(27,3),    L(35,3),    L(43,3),    L(51,3),    L(59,3),    L(67,3),    L(75,3),    L(83,3),
            L(91,6),    L(155,9),   L(667,11),  L(2715,15)
        };
        #undef L
        ret = ht_mktree(ultra->l_table, lengths + NumByteSym + NumDistSym, 0, NumLenSym, l_codes);
        if (ret < 0)
            return ret;
        ret = 1;
    }
    return ret;
}

static int decompress_block(struct ultra *ultra)
{
    const unsigned EOB_MARK = 125*512+1;

    do {
        int c = huff(ultra->bd_table, &ultra->bi);
        if (c < 0)
            return c;
        if (!(c & 1<<16))
            ultra->cb.data[ultra->cb.tail++] = (u8)c;
        else {
            unsigned dist = c & 0xffff;
            c = c >> 20 & 0xf;
            if (c)
                dist += bits_get(&ultra->bi, c);

            c = huff(ultra->l_table, &ultra->bi);
            if (c < 0)
                return c;

            if (dist == EOB_MARK)
                return End;

            unsigned len = c & 0xffff;
            c = c >> 20 & 0xf;
            if (c)
                len += bits_get(&ultra->bi, c);
            assert(cbuf_space(&ultra->cb) >= len);
            do {
                ultra->cb.data[ultra->cb.tail] = ultra->cb.data[(u16)(ultra->cb.tail - dist)];
                ultra->cb.tail++;
            } while (--len);
        }

    } while (cbuf_space(&ultra->cb) >= 35482);

    return More;
}

/* --- Top-level decompressor dispatcher --- */
static int decompressor(int method, unsigned master, unsigned delta,
                          struct reader *rd, struct writer *wr,
                          unsigned len, u16 *csum)
{
    int ret = -4; // Damaged

    if (method >= 1 && method <= 9) {
        delta = 0;
        goto ultra;
    } else if (method >= 30 && method <= 39) {
        delta = method - 29;
        goto ultra;
    } else if (method >= 40 && method <= 49) {
        delta = method - 39;
        goto ultra;
    } else if (method >= 21 && method <= 29) {
        delta = 1;
        goto ultra;
    } else if (method == 80) {
        return -6; // Unimplemented
    }
    return ret;

ultra:
    ret = decompressor_ultra(master, delta, rd, wr, len, csum);
    return ret;
}

/* --- Public API wrappers matching our callback system --- */

extern WORD (far pascal *CReader)(BYTE far *pbBuffer, WORD wSize);
extern void (far pascal *CWriter)(BYTE far *pbBuffer, WORD wSize);

static int wrap_creader(void *ctx, void *buf, unsigned len)
{
    (void)ctx;
    return CReader((BYTE *)buf, (WORD)len);
}

static int wrap_cwriter(void *ctx, const void *buf, unsigned len)
{
    (void)ctx;
    CWriter((BYTE *)buf, (WORD)len);
    FletchUpdate(&Fout, (BYTE *)buf, (WORD)len);
    return 0;
}

/* Initialize supermaster (call once before any decompression) */
void UC2DecompInit(void)
{
    if (g_supermaster) return;

    g_supermaster = (u8 *)malloc(49152);
    if (!g_supermaster) return;

    struct range br = {
        .ptr = (u8 *)supermaster_compressed,
        .end = (u8 *)supermaster_compressed + sizeof(supermaster_compressed)
    };
    struct range bw = {
        .ptr = g_supermaster,
        .end = g_supermaster + 49152
    };
    struct reader rd = {.context = &br, .read = buf_read};
    struct writer wr = {.context = &bw, .write = buf_write};
    u16 cs;
    int r = decompressor(4, SM_NOMASTER, 0, &rd, &wr, 49152, &cs);
    if (r < 0 || cs != 0x1E55) {
        free(g_supermaster);
        g_supermaster = NULL;
    }
}

/* Decompress a single file/stream using original UC2 algorithm */
int UC2Decompress(int method, DWORD master, BYTE bDelta, DWORD len, WORD *csum)
{
    struct reader rd = {.context = NULL, .read = wrap_creader};
    struct writer wr = {.context = NULL, .write = wrap_cwriter};

    unsigned umaster = (unsigned)master;
    unsigned ulen = (unsigned)len;
    unsigned udelta = bDelta;

    /* Map our master IDs to libunuc2 values */
    if (master == SUPERMASTER) umaster = SM_SUPERMASTER;
    else if (master == NOMASTER) umaster = SM_NOMASTER;

    return decompressor(method, umaster, udelta, &rd, &wr, ulen, csum);
}
