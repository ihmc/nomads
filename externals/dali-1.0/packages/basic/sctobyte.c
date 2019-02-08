/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "basicInt.h"

/*
 * Interface to jrevdct.c routines (from JPEG decoder)
 */

#define DCTELEM short
#define DCTSIZE 8
#define DCTSIZE2 64

typedef DCTELEM DCTBLOCK[DCTSIZE2];
void DctToByte(DCTBLOCK, unsigned char *, int);
void DctToByteSparse(DCTBLOCK, unsigned char *, int);
void j_rev_dct(DCTBLOCK);
void j_rev_dct_sparse(DCTBLOCK, int);
static char zz[] =
{0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18,
    11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49,
    56, 57, 50, 43, 36, 29, 22, 15, 23, 30,
    37, 44, 51, 58, 59, 52, 45, 38, 31, 39,
    46, 53, 60, 61, 54, 47, 55, 62, 63};


extern unsigned char theCropTable[4096];

#define OFFSET(buf, row, col)   buf->firstByte + (row)*buf->parentWidth + col
#define CROP(n) (unsigned char)theCropTable[(n) + 2048]

    /*
       if ((block)->numOfAc == 1)\
       j_rev_dct_sparse(temp, (block)->data[1]);\
       else\
     */

/*
 * Santity check a block.  This function also gives us a hook
 * to stop in a debugger when something bad happens.
 */
#if 1
static void
CheckBlock(block)
    ScBlock *block;
{
    int k;

    if (block->numOfAC > 63) {
        block->numOfAC = 63;
    }
    if (block->dc > 2047) {
        block->dc = 2047;
    }
    if (block->dc < -2048) {
        block->dc = -2048;
    }
    for (k = 0; k < block->numOfAC; k++) {
        if (block->index[k] > 63) {
            block->numOfAC = k;
        }
    }
}
#else
#define CheckBlock(block)       /* Do nothing */
#endif

#define DO_IDCT(block, temp)\
{\
    char *_c;\
    short *_v;\
    CheckBlock (block); \
    memset(temp, 0, 64*sizeof(short));\
    temp[0] = (block)->dc; \
    _c = &((block)->index[0]);\
    _v = &((block)->value[0]);\
    if ((block)->numOfAC != 0) {\
        DO_N_TIMES((block)->numOfAC,\
            temp[(int)(zz[(int)*_c++])] = *_v++;\
        );\
    } \
    if ((block)->numOfAC == 1 && (block)->dc == 0) {\
        j_rev_dct_sparse(temp, (block)->index[0]);\
    } else if ((block)->numOfAC == 0 && (block)->dc != 0) {\
        j_rev_dct_sparse(temp, 0);\
    } else {\
        j_rev_dct(temp);\
    }\
}

#define SCBLOCK_TO_BYTE(block, temp, byte, w)\
{\
    char *_c;\
    short *_v;\
    CheckBlock (block); \
    memset(temp, 0, 64*sizeof(short));\
    temp[0] = (block)->dc; \
    _c = &((block)->index[0]);\
    _v = &((block)->value[0]);\
    if ((block)->numOfAC != 0) {\
        DO_N_TIMES((block)->numOfAC,\
            temp[(int)(zz[(int)*_c++])] = *_v++;\
        );\
    } \
    if ((block)->numOfAC == 0 && (block)->dc != 0) {\
        DctToByteSparse(temp, byte, w);\
    } else {\
        DctToByte(temp, byte, w);\
    }\
}


#define ShortToByteCopy(block, dest, skip) \
{\
    register int _i;\
    register short *_b;\
    register unsigned char *_d;\
    _b = block;\
    _d = dest;\
    for (_i = 0; _i < 8; _i++) {\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        *_d++ = CROP(*_b++);\
        _d += skip - 8;\
    }\
}\


#define BlockAverage(past, future, dest, pw, fw, dw)\
{\
    int i;\
    register unsigned char *_p, *_f, *_d;\
    _p = past; _f = future; _d = dest;\
    for (i = 0; i < 8; i++) {\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        *_d++ = CROP((*_p++ + *_f++)>>1);\
        _d += dw - 8;\
        _p += pw - 8;\
        _f += fw - 8;\
    }\
}

#define BlockCopy(past, dest, pw, dw)\
{\
    int i;\
    register unsigned char *_p, *_d;\
    _p = past; _d = dest;\
    for (i = 0; i < 8; i++) {\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        *_d++ = CROP(*_p++);\
        _d += dw - 8;\
        _p += pw - 8;\
    }\
}


#define BlockAverage3(block, src1, src2, dest, src1Skip, src2Skip, destSkip)\
{\
    int i;\
    register short *_b = block;\
    register unsigned char *_s1, *_s2, *_d;\
    _d = dest; _s1 = src1; _s2 = src2;\
    for (i = 0; i < 8; i++) {\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        *_d++ = CROP(*_b++ + ((*_s1++ + *_s2++)>>1));\
        _d  += destSkip - 8;\
        _s1 += src1Skip - 8;\
        _s2 += src2Skip - 8;\
    }\
}


void
BlockCopyToYuv2(src1, src2, dest, src_width, dest_width)
    unsigned char *src1;
    unsigned char *src2;
    unsigned char *dest;
    int src_width;
    int dest_width;
{
    int i;

    for (i = 0; i < 8; i++) {
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        *dest++ = CROP((*src1++ + *src2++ + 1) >> 1);
        dest += dest_width - 8;
        src1 += src_width - 8;
        src2 += src_width - 8;
    }
}

void
BlockCopyToYuv4(src1, src2, src3, src4, dest, src_width, dest_width)
    unsigned char *src1;
    unsigned char *src2;
    unsigned char *src3;
    unsigned char *src4;
    unsigned char *dest;
    int src_width;
    int dest_width;
{
    int i;

    for (i = 0; i < 8; i++) {
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        *dest++ = CROP((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2);
        dest += dest_width - 8;
        src1 += src_width - 8;
        src2 += src_width - 8;
        src3 += src_width - 8;
        src4 += src_width - 8;
    }
}

void
BlockMakeItBlack(dest, dest_width)
    unsigned char *dest;
    int dest_width;
{
    register int i;
    register unsigned char *d;

    d = dest;

    for (i = 0; i < 8; i++) {
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        *d++ = 0;
        d += dest_width - 8;
    }
}

void
BlockDiffToYuv1(block, src, dest, src_width, dest_width)
    short *block;
    unsigned char *src;
    unsigned char *dest;
    int src_width;
    int dest_width;
{
    register int i;

    for (i = 0; i < 8; i++) {
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        *dest++ = CROP(*block++ + *src++);
        dest += dest_width - 8;
        src += src_width - 8;
    }
}

void
BlockDiffToYuv2(block, src1, src2, dest, src_width, dest_width)
    short *block;
    unsigned char *src1;
    unsigned char *src2;
    unsigned char *dest;
    int src_width;
    int dest_width;
{
    register int i;

    for (i = 0; i < 8; i++) {
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + 1) >> 1));
        dest += dest_width - 8;
        src1 += src_width - 8;
        src2 += src_width - 8;
    }
}

void
BlockDiffToYuv4(block, src1, src2, src3, src4, dest, src_width, dest_width)
    short *block;
    unsigned char *src1;
    unsigned char *src2;
    unsigned char *src3;
    unsigned char *src4;
    unsigned char *dest;
    int src_width;
    int dest_width;
{
    int i;

    for (i = 0; i < 8; i++) {
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        *dest++ = CROP(*block++ + ((*src1++ + *src2++ + *src3++ + *src4++ + 2) >> 2));
        dest += dest_width - 8;
        src1 += src_width - 8;
        src2 += src_width - 8;
        src3 += src_width - 8;
        src4 += src_width - 8;
    }
}

int
ScIToByte(sc, byte)
    ScImage *sc;
    ByteImage *byte;
{
    int i, j, skip;
    unsigned char *curr_dest;
    ScBlock *curr_block;
    short temp[128];

#ifndef NODEBUG
    /* XXX: Hack for debugging */
    static int counter, limit;

    counter++;
    if (limit && (counter == limit)) {
        limit = counter;
    }
#endif

    curr_dest = byte->firstByte;
    curr_block = sc->firstBlock;
    skip = byte->parentWidth;
    memset(temp, 0, sizeof(temp));
    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {
            SCBLOCK_TO_BYTE(curr_block, temp, curr_dest, byte->parentWidth);
            // ShortToByteCopy(temp, curr_dest, byte->parentWidth);
            curr_block++;
            curr_dest += 8;
        }
        curr_dest += 8 * byte->parentWidth - byte->width;
        curr_block += sc->parentWidth - sc->width;
    }
    return 1;
}


int
ScPToY(sc, mv, prev, byte)
    ScImage *sc;
    VectorImage *mv;
    ByteImage *prev;
    ByteImage *byte;
{
    int i, j;
    ScBlock *block;
    Vector *v;
    unsigned char *pred, *src1, *src2, *src3, *src4;
    unsigned char *dest;
    short temp[64];
    char right_half, down_half, right, down;

#ifndef NODEBUG
    /* XXX: Hack for debugging */
    static int counter, limit;

    counter++;
    if (limit && (counter == limit)) {
        limit = counter;
    }
#endif

    block = sc->firstBlock;
    v = mv->firstVector;
    pred = prev->firstByte;
    dest = byte->firstByte;

    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {

            right = v->right >> 1;
            down = v->down >> 1;
            right_half = v->right & 0x1;
            down_half = v->down & 0x1;

            if (block->skipBlock || block->skipMB) {

                src1 = pred + right + down * (prev->parentWidth);
                if (!right_half && !down_half) {
                    BlockCopy(src1, dest, prev->parentWidth, byte->parentWidth);
                } else if (!right_half || !down_half) {
                    src2 = src1 + right_half + down_half * (prev->parentWidth);
                    BlockCopyToYuv2(src1, src2, dest, prev->parentWidth, byte->parentWidth);
                } else {
                    src2 = src1 + 1 + prev->parentWidth;
                    src3 = src1 + 1;
                    src4 = src1 + prev->parentWidth;
                    BlockCopyToYuv4(src1, src2, src3, src4, dest, prev->parentWidth, byte->parentWidth);
                }

            } else if (block->intracoded) {

                DO_IDCT(block, temp);
                ShortToByteCopy(temp, dest, byte->parentWidth);

            } else {

                src1 = pred + right + down * (prev->parentWidth);
                DO_IDCT(block, temp);

                if (!right_half && !down_half) {
                    BlockDiffToYuv1(temp, src1, dest, prev->parentWidth, byte->parentWidth);
                } else if (!right_half || !down_half) {
                    src2 = src1 + right_half + down_half * (prev->parentWidth);
                    BlockDiffToYuv2(temp, src1, src2, dest, prev->parentWidth, byte->parentWidth);
                } else {
                    src2 = src1 + 1 + prev->parentWidth;
                    src3 = src1 + 1;
                    src4 = src1 + prev->parentWidth;
                    BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, prev->parentWidth, byte->parentWidth);
                }
            }
            block++;
            if (j % 2)
                v++;
            pred += 8;
            dest += 8;
        }
        block += sc->parentWidth - sc->width;
        if (i % 2)
            v += mv->parentWidth - mv->width;
        else
            v -= mv->width;
        pred += 8 * prev->parentWidth - prev->width;
        dest += 8 * byte->parentWidth - byte->width;
    }
    return 1;
}

int
ScPToUV(sc, mv, prev, byte)
    ScImage *sc;
    VectorImage *mv;
    ByteImage *prev;
    ByteImage *byte;
{
    int i, j;
    ScBlock *block;
    Vector *v;
    unsigned char *pred, *src1, *src2, *src3, *src4;
    unsigned char *dest;
    short temp[64];
    char right_half, down_half, right, down;

#ifndef NODEBUG
    /* XXX: Hack for debugging */
    static int counter, limit;

    counter++;
    if (limit && (counter == limit)) {
        limit = counter;
    }
#endif

    block = sc->firstBlock;
    v = mv->firstVector;
    pred = prev->firstByte;
    dest = byte->firstByte;
    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {

            right = (v->right / 2) >> 1;
            down = (v->down / 2) >> 1;
            right_half = (v->right / 2) & 0x1;
            down_half = (v->down / 2) & 0x1;

            if (block->skipMB || block->skipBlock) {
                src1 = pred + right + down * (prev->parentWidth);
                if (!right_half && !down_half) {
                    BlockCopy(src1, dest, prev->parentWidth, byte->parentWidth);
                } else if (!right_half || !down_half) {
                    src2 = src1 + right_half + down_half * (prev->parentWidth);
                    BlockCopyToYuv2(src1, src2, dest, prev->parentWidth, byte->parentWidth);
                } else {
                    src2 = src1 + 1 + prev->parentWidth;
                    src3 = src1 + 1;
                    src4 = src1 + prev->parentWidth;
                    BlockCopyToYuv4(src1, src2, src3, src4, dest, prev->parentWidth, byte->parentWidth);
                }

            } else if (block->intracoded) {
                DO_IDCT(block, temp);
                ShortToByteCopy(temp, dest, byte->parentWidth);

            } else {
                src1 = pred + right + down * (prev->parentWidth);
                if (!right_half && !down_half) {
                    DO_IDCT(block, temp);
                    BlockDiffToYuv1(temp, src1, dest, prev->parentWidth, byte->parentWidth);
                } else if (!right_half || !down_half) {
                    DO_IDCT(block, temp);
                    src2 = src1 + right_half + down_half * (prev->parentWidth);
                    BlockDiffToYuv2(temp, src1, src2, dest, prev->parentWidth, byte->parentWidth);
                } else {
                    DO_IDCT(block, temp);
                    src2 = src1 + 1 + prev->parentWidth;
                    src3 = src1 + 1;
                    src4 = src1 + prev->parentWidth;
                    BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, prev->parentWidth, byte->parentWidth);
                }
            }

            block++;
            v++;
            pred += 8;
            dest += 8;
        }
        block += sc->parentWidth - sc->width;
        v += mv->parentWidth - mv->width;
        pred += 8 * prev->parentWidth - prev->width;
        dest += 8 * byte->parentWidth - byte->width;
    }
    return 1;
}


/*
 * Reminder : bwd vectors are references to _future_ frames.
 *            fwd vectors are references to _past_ frames.
 */

int
ScBToY(sc, fwdmv, bwdmv, prev, future, byte)
    ScImage *sc;
    VectorImage *fwdmv;
    VectorImage *bwdmv;
    ByteImage *prev;
    ByteImage *future;
    ByteImage *byte;
{
    int i, j;
    ScBlock *block;
    Vector *fv, *bv;
    Vector *prev_fv, *prev_bv;
    unsigned char *p, *f, *src1, *src2, *src3, *src4;
    unsigned char *dest;
    register int pw, fw, dw;
    char fwd_right_half, fwd_down_half, fwd_right, fwd_down;
    char bwd_right_half, bwd_down_half, bwd_right, bwd_down;
    char prev_fwd_right_half, prev_fwd_down_half, prev_fwd_right, prev_fwd_down;
    char prev_bwd_right_half, prev_bwd_down_half, prev_bwd_right, prev_bwd_down;
    short temp[64];

#ifndef NODEBUG
    /* XXX: Hack for debugging */
    static int counter, limit;

    counter++;
    if (limit && (counter == limit)) {
        limit = counter;
    }
#endif

    /* make compiler happy */
    prev_fv = NULL;
    prev_bv = NULL;
    prev_fwd_right_half = 0;
    prev_fwd_down_half = 0;
    prev_fwd_right = 0;
    prev_fwd_down = 0;
    prev_bwd_right_half = 0;
    prev_bwd_down_half = 0;
    prev_bwd_right = 0;
    prev_bwd_down = 0;

    block = sc->firstBlock;
    fv = fwdmv->firstVector;
    bv = bwdmv->firstVector;
    p = prev->firstByte;
    f = future->firstByte;
    dest = byte->firstByte;
    pw = prev->parentWidth;
    fw = future->parentWidth;
    dw = byte->parentWidth;

    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {

            fwd_right = fv->right >> 1;
            fwd_down = fv->down >> 1;
            fwd_right_half = fv->right & 0x1;
            fwd_down_half = fv->down & 0x1;
            bwd_right = bv->right >> 1;
            bwd_down = bv->down >> 1;
            bwd_right_half = bv->right & 0x1;
            bwd_down_half = bv->down & 0x1;

            if (block->skipMB) {

                /*
                 * Process Skipped Macroblock
                 */
                if (prev_fv->exists && prev_bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + prev_fwd_right + (prev_fwd_down) * (pw);
                    src2 = f + prev_bwd_right + (prev_bwd_down) * (fw);
                    BlockAverage(src1, src2, dest, pw, fw, dw);

                } else if (prev_fv->exists) {

                    src1 = p + prev_fwd_right + prev_fwd_down * (pw);
                    if (!prev_fwd_right_half && !prev_fwd_down_half) {
                        BlockCopy(src1, dest, pw, dw);
                    } else if (!prev_fwd_right_half || !prev_fwd_down_half) {
                        src2 = src1 + prev_fwd_right_half + prev_fwd_down_half * (pw);
                        BlockCopyToYuv2(src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (prev_bv->exists) {

                    src1 = f + prev_bwd_right + prev_bwd_down * (fw);
                    if (!prev_bwd_right_half && !prev_bwd_down_half) {
                        BlockCopy(src1, dest, fw, dw);
                    } else if (!prev_bwd_right_half || !prev_bwd_down_half) {
                        src2 = src1 + prev_bwd_right_half + prev_bwd_down_half * (fw);
                        BlockCopyToYuv2(src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }

            } else if (block->skipBlock) {

                if (fv->exists && bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + fwd_right + (fwd_down) * (pw);
                    src2 = f + bwd_right + (bwd_down) * (fw);
                    BlockAverage(src1, src2, dest, pw, fw, dw);

                } else if (fv->exists) {

                    src1 = p + fwd_right + fwd_down * (pw);

                    if (!fwd_right_half && !fwd_down_half) {
                        BlockCopy(src1, dest, pw, dw);
                    } else if (!fwd_right_half || !fwd_down_half) {
                        src2 = src1 + fwd_right_half + fwd_down_half * (pw);
                        BlockCopyToYuv2(src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (bv->exists) {

                    src1 = f + bwd_right + bwd_down * (fw);

                    if (!bwd_right_half && !bwd_down_half) {
                        BlockCopy(src1, dest, fw, dw);
                    } else if (!bwd_right_half || !bwd_down_half) {
                        src2 = src1 + bwd_right_half + bwd_down_half * (fw);
                        BlockCopyToYuv2(src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }
                prev_fv = fv;
                prev_bv = bv;
                prev_fwd_down = fwd_down;
                prev_fwd_down_half = fwd_down_half;
                prev_fwd_right = fwd_right;
                prev_fwd_right_half = fwd_right_half;
                prev_bwd_down = bwd_down;
                prev_bwd_down_half = bwd_down_half;
                prev_bwd_right = bwd_right;
                prev_bwd_right_half = bwd_right_half;

            } else if (block->intracoded) {

                DO_IDCT(block, temp);
                ShortToByteCopy(temp, dest, dw);

            } else {

                if (fv->exists && bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + fwd_right + (fwd_down) * (pw);
                    src2 = f + bwd_right + (bwd_down) * (fw);
                    DO_IDCT(block, temp);
                    BlockAverage3(temp, src1, src2, dest, pw, fw, dw);

                } else if (fv->exists) {

                    src1 = p + fwd_right + fwd_down * (pw);
                    DO_IDCT(block, temp);

                    if (!fwd_right_half && !fwd_down_half) {
                        BlockDiffToYuv1(temp, src1, dest, pw, dw);
                    } else if (!fwd_right_half || !fwd_down_half) {
                        src2 = src1 + fwd_right_half + fwd_down_half * (pw);
                        BlockDiffToYuv2(temp, src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (bv->exists) {

                    src1 = f + bwd_right + bwd_down * (fw);
                    DO_IDCT(block, temp);

                    if (!bwd_right_half && !bwd_down_half) {
                        BlockDiffToYuv1(temp, src1, dest, fw, dw);
                    } else if (!bwd_right_half || !bwd_down_half) {
                        src2 = src1 + bwd_right_half + bwd_down_half * (fw);
                        BlockDiffToYuv2(temp, src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }
                prev_fv = fv;
                prev_bv = bv;
                prev_fwd_down = fwd_down;
                prev_fwd_down_half = fwd_down_half;
                prev_fwd_right = fwd_right;
                prev_fwd_right_half = fwd_right_half;
                prev_bwd_down = bwd_down;
                prev_bwd_down_half = bwd_down_half;
                prev_bwd_right = bwd_right;
                prev_bwd_right_half = bwd_right_half;
            }

            block++;
            if (j % 2) {
                fv++;
                bv++;
            }
            p += 8;
            f += 8;
            dest += 8;

        }
        block += sc->parentWidth - sc->width;
        if (i % 2) {
            fv += fwdmv->parentWidth - fwdmv->width;
            bv += bwdmv->parentWidth - bwdmv->width;
        } else {
            fv -= fwdmv->width;
            bv -= bwdmv->width;
        }
        p += 8 * pw - prev->width;
        f += 8 * fw - future->width;
        dest += 8 * dw - byte->width;
    }
    return 1;
}

int
ScBToUV(sc, fwdmv, bwdmv, prev, future, byte)
    ScImage *sc;
    VectorImage *fwdmv;
    VectorImage *bwdmv;
    ByteImage *prev;
    ByteImage *future;
    ByteImage *byte;
{
    int i, j;
    ScBlock *block;
    Vector *fv, *bv;
    Vector *prev_fv, *prev_bv;
    unsigned char *p, *f, *src1, *src2, *src3, *src4;
    unsigned char *dest;
    register int pw, fw, dw;
    char fwd_right_half, fwd_down_half, fwd_right, fwd_down;
    char bwd_right_half, bwd_down_half, bwd_right, bwd_down;
    char prev_fwd_right_half, prev_fwd_down_half, prev_fwd_right, prev_fwd_down;
    char prev_bwd_right_half, prev_bwd_down_half, prev_bwd_right, prev_bwd_down;
    short temp[64];

#ifndef NODEBUG
    /* XXX: Hack for debugging */
    static int counter, limit;

    counter++;
    if (limit && (counter == limit)) {
        limit = counter;
    }
#endif

    /* make compiler happy */
    prev_fv = NULL;
    prev_bv = NULL;
    prev_fwd_right_half = 0;
    prev_fwd_down_half = 0;
    prev_fwd_right = 0;
    prev_fwd_down = 0;
    prev_bwd_right_half = 0;
    prev_bwd_down_half = 0;
    prev_bwd_right = 0;
    prev_bwd_down = 0;

    block = sc->firstBlock;
    fv = fwdmv->firstVector;
    bv = bwdmv->firstVector;
    p = prev->firstByte;
    f = future->firstByte;
    dest = byte->firstByte;
    pw = prev->parentWidth;
    fw = future->parentWidth;
    dw = byte->parentWidth;

    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {

            fwd_right = (fv->right / 2) >> 1;
            fwd_down = (fv->down / 2) >> 1;
            fwd_right_half = (fv->right / 2) & 0x1;
            fwd_down_half = (fv->down / 2) & 0x1;
            bwd_right = (bv->right / 2) >> 1;
            bwd_down = (bv->down / 2) >> 1;
            bwd_right_half = (bv->right / 2) & 0x1;
            bwd_down_half = (bv->down / 2) & 0x1;

            if (block->skipMB) {

                /*
                 * Process Skipped Macroblock
                 */
                if (prev_fv->exists && prev_bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + prev_fwd_right + (prev_fwd_down) * (pw);
                    src2 = f + prev_bwd_right + (prev_bwd_down) * (fw);
                    BlockAverage(src1, src2, dest, pw, fw, dw);

                } else if (prev_fv->exists) {

                    src1 = p + prev_fwd_right + prev_fwd_down * (pw);
                    if (!prev_fwd_right_half && !prev_fwd_down_half) {
                        BlockCopy(src1, dest, pw, dw);
                    } else if (!prev_fwd_right_half || !prev_fwd_down_half) {
                        src2 = src1 + prev_fwd_right_half + prev_fwd_down_half * (pw);
                        BlockCopyToYuv2(src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (prev_bv->exists) {

                    src1 = f + prev_bwd_right + prev_bwd_down * (fw);
                    if (!prev_bwd_right_half && !prev_bwd_down_half) {
                        BlockCopy(src1, dest, fw, dw);
                    } else if (!prev_bwd_right_half || !prev_bwd_down_half) {
                        src2 = src1 + prev_bwd_right_half + prev_bwd_down_half * (fw);
                        BlockCopyToYuv2(src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }

            } else if (block->skipBlock) {

                if (fv->exists && bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + fwd_right + (fwd_down) * (pw);
                    src2 = f + bwd_right + (bwd_down) * (fw);
                    BlockAverage(src1, src2, dest, pw, fw, dw);

                } else if (fv->exists) {

                    src1 = p + fwd_right + fwd_down * (pw);

                    if (!fwd_right_half && !fwd_down_half) {
                        BlockCopy(src1, dest, pw, dw);
                    } else if (!fwd_right_half || !fwd_down_half) {
                        src2 = src1 + fwd_right_half + fwd_down_half * (pw);
                        BlockCopyToYuv2(src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (bv->exists) {

                    src1 = f + bwd_right + bwd_down * (fw);

                    if (!bwd_right_half && !bwd_down_half) {
                        BlockCopy(src1, dest, fw, dw);
                    } else if (!bwd_right_half || !bwd_down_half) {
                        src2 = src1 + bwd_right_half + bwd_down_half * (fw);
                        BlockCopyToYuv2(src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockCopyToYuv4(src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }
                prev_fv = fv;
                prev_bv = bv;
                prev_fwd_down = fwd_down;
                prev_fwd_down_half = fwd_down_half;
                prev_fwd_right = fwd_right;
                prev_fwd_right_half = fwd_right_half;
                prev_bwd_down = bwd_down;
                prev_bwd_down_half = bwd_down_half;
                prev_bwd_right = bwd_right;
                prev_bwd_right_half = bwd_right_half;

            } else if (block->intracoded) {

                DO_IDCT(block, temp);
                ShortToByteCopy(temp, dest, dw);

            } else {

                if (fv->exists && bv->exists) {
                    /*
                     * Decode bi-directional blocks
                     */
                    src1 = p + fwd_right + (fwd_down) * (pw);
                    src2 = f + bwd_right + (bwd_down) * (fw);
                    DO_IDCT(block, temp);
                    BlockAverage3(temp, src1, src2, dest, pw, fw, dw);

                } else if (fv->exists) {

                    src1 = p + fwd_right + fwd_down * (pw);
                    DO_IDCT(block, temp);

                    if (!fwd_right_half && !fwd_down_half) {
                        BlockDiffToYuv1(temp, src1, dest, pw, dw);
                    } else if (!fwd_right_half || !fwd_down_half) {
                        src2 = src1 + fwd_right_half + fwd_down_half * (pw);
                        BlockDiffToYuv2(temp, src1, src2, dest, pw, dw);
                    } else {
                        src2 = src1 + 1 + pw;
                        src3 = src1 + 1;
                        src4 = src1 + pw;
                        BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, pw, dw);
                    }

                } else if (bv->exists) {

                    src1 = f + bwd_right + bwd_down * (fw);
                    DO_IDCT(block, temp);

                    if (!bwd_right_half && !bwd_down_half) {
                        BlockDiffToYuv1(temp, src1, dest, fw, dw);
                    } else if (!bwd_right_half || !bwd_down_half) {
                        src2 = src1 + bwd_right_half + bwd_down_half * (fw);
                        BlockDiffToYuv2(temp, src1, src2, dest, fw, dw);
                    } else {
                        src2 = src1 + 1 + fw;
                        src3 = src1 + 1;
                        src4 = src1 + fw;
                        BlockDiffToYuv4(temp, src1, src2, src3, src4, dest, fw, dw);
                    }

                } else {
                    fprintf(stderr, "predicted block(%d %d) : both vector does not exists.\n", i, j);
                }
                prev_fv = fv;
                prev_bv = bv;
                prev_fwd_down = fwd_down;
                prev_fwd_down_half = fwd_down_half;
                prev_fwd_right = fwd_right;
                prev_fwd_right_half = fwd_right_half;
                prev_bwd_down = bwd_down;
                prev_bwd_down_half = bwd_down_half;
                prev_bwd_right = bwd_right;
                prev_bwd_right_half = bwd_right_half;
            }

            block++;
            fv++;
            bv++;
            p += 8;
            f += 8;
            dest += 8;

        }
        block += sc->parentWidth - sc->width;
        fv += fwdmv->parentWidth - fwdmv->width;
        bv += bwdmv->parentWidth - bwdmv->width;
        p += 8 * pw - prev->width;
        f += 8 * fw - future->width;
        dest += 8 * dw - byte->width;
    }
    return 1;
}
