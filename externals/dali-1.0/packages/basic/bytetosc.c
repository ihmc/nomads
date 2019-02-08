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
#include "mfwddct.h"

#define DCTSIZE 8
#define BLOCK_BOUND 128

typedef short Block[64];

extern void DCTBlock(short *);

static int absTable[512] =
{
    255, 254, 253, 252, 251, 250, 249, 248,
    247, 246, 245, 244, 243, 242, 241, 240,
    239, 238, 237, 236, 235, 234, 233, 232,
    231, 230, 229, 228, 227, 226, 225, 224,
    223, 222, 221, 220, 219, 218, 217, 216,
    215, 214, 213, 212, 211, 210, 209, 208,
    207, 206, 205, 204, 203, 202, 201, 200,
    199, 198, 197, 196, 195, 194, 193, 192,
    191, 190, 189, 188, 187, 186, 185, 184,
    183, 182, 181, 180, 179, 178, 177, 176,
    175, 174, 173, 172, 171, 170, 169, 168,
    167, 166, 165, 164, 163, 162, 161, 160,
    159, 158, 157, 156, 155, 154, 153, 152,
    151, 150, 149, 148, 147, 146, 145, 144,
    143, 142, 141, 140, 139, 138, 137, 136,
    135, 134, 133, 132, 131, 130, 129, 128,
    127, 126, 125, 124, 123, 122, 121, 120,
    119, 118, 117, 116, 115, 114, 113, 112,
    111, 110, 109, 108, 107, 106, 105, 104,
    103, 102, 101, 100, 99, 98, 97, 96,
    95, 94, 93, 92, 91, 90, 89, 88,
    87, 86, 85, 84, 83, 82, 81, 80,
    79, 78, 77, 76, 75, 74, 73, 72,
    71, 70, 69, 68, 67, 66, 65, 64,
    63, 62, 61, 60, 59, 58, 57, 56,
    55, 54, 53, 52, 51, 50, 49, 48,
    47, 46, 45, 44, 43, 42, 41, 40,
    39, 38, 37, 36, 35, 34, 33, 32,
    31, 30, 29, 28, 27, 26, 25, 24,
    23, 22, 21, 20, 19, 18, 17, 16,
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 4, 3, 2, 1,
    0,
    1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87,
    88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135,
    136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167,
    168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183,
    184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215,
    216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231,
    232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247,
    248, 249, 250, 251, 252, 253, 254, 255};

#define ABS_TABLE(x) absTable[(x) + 255]


/* REMOVE THIS LATER - included in MPEG block.c */
#define ABS2(x) (((x) < 0) ? -(x) : (x))
#define MOTION_TO_FRAME_COORD(bx1, bx2, mx1, mx2, x1, x2) { \
    x1 = (bx1)*DCTSIZE+(mx1);               \
    x2 = (bx2)*DCTSIZE+(mx2);               \
    }

static void
GetMotionBlock(prevFrame, by, bx, my, mx, motionBlock)
    ByteImage *prevFrame;
    int by;
    int bx;
    int my;
    int mx;
    Block motionBlock;
{
    int fy, fx;
    int i, rowStartPos;
    int width = prevFrame->parentWidth;
    short *destPtr = motionBlock;
    unsigned char *srcPtr;
    unsigned char *srcPtr2;
    unsigned char *prev = prevFrame->firstByte;
    int xHalf, yHalf;

    xHalf = (ABS2(mx) % 2 == 1);
    yHalf = (ABS2(my) % 2 == 1);
    MOTION_TO_FRAME_COORD(by, bx, (my / 2), (mx / 2), fy, fx);

    if (xHalf && yHalf) {
        /* really should be fy+y-1 and fy+y so do (fy-1)+y = fy+y-1 and
           (fy-1)+y+1 = fy+y */

        if (my < 0) {
            fy--;
        }
        if (mx < 0) {
            fx--;
        }
        rowStartPos = fy * width + fx;

        for (i = 0; i < DCTSIZE; i++) {
            srcPtr = &(prev[rowStartPos]);
            srcPtr2 = &(prev[rowStartPos + width]);

            destPtr[0] = (srcPtr[0] + srcPtr[1] + srcPtr2[0] + srcPtr2[1] + 2) >> 2;
            destPtr[1] = (srcPtr[1] + srcPtr[2] + srcPtr2[1] + srcPtr2[2] + 2) >> 2;
            destPtr[2] = (srcPtr[2] + srcPtr[3] + srcPtr2[2] + srcPtr2[3] + 2) >> 2;
            destPtr[3] = (srcPtr[3] + srcPtr[4] + srcPtr2[3] + srcPtr2[4] + 2) >> 2;
            destPtr[4] = (srcPtr[4] + srcPtr[5] + srcPtr2[4] + srcPtr2[5] + 2) >> 2;
            destPtr[5] = (srcPtr[5] + srcPtr[6] + srcPtr2[5] + srcPtr2[6] + 2) >> 2;
            destPtr[6] = (srcPtr[6] + srcPtr[7] + srcPtr2[6] + srcPtr2[7] + 2) >> 2;
            destPtr[7] = (srcPtr[7] + srcPtr[8] + srcPtr2[7] + srcPtr2[8] + 2) >> 2;

            destPtr += 8;
            rowStartPos += width;
        }

    } else if (xHalf) {
        if (mx < 0) {
            fx--;
        }
        rowStartPos = fy * width + fx;

        for (i = 0; i < DCTSIZE; i++) {
            srcPtr = &(prev[rowStartPos]);

            destPtr[0] = (srcPtr[0] + srcPtr[1] + 1) >> 1;
            destPtr[1] = (srcPtr[1] + srcPtr[2] + 1) >> 1;
            destPtr[2] = (srcPtr[2] + srcPtr[3] + 1) >> 1;
            destPtr[3] = (srcPtr[3] + srcPtr[4] + 1) >> 1;
            destPtr[4] = (srcPtr[4] + srcPtr[5] + 1) >> 1;
            destPtr[5] = (srcPtr[5] + srcPtr[6] + 1) >> 1;
            destPtr[6] = (srcPtr[6] + srcPtr[7] + 1) >> 1;
            destPtr[7] = (srcPtr[7] + srcPtr[8] + 1) >> 1;

            destPtr += 8;
            rowStartPos += width;
        }

    } else if (yHalf) {
        if (my < 0) {
            fy--;
        }
        rowStartPos = fy * width + fx;

        for (i = 0; i < DCTSIZE; i++) {
            srcPtr = &(prev[rowStartPos]);
            srcPtr2 = &(prev[rowStartPos + width]);

            destPtr[0] = (srcPtr[0] + srcPtr2[0] + 1) >> 1;
            destPtr[1] = (srcPtr[1] + srcPtr2[1] + 1) >> 1;
            destPtr[2] = (srcPtr[2] + srcPtr2[2] + 1) >> 1;
            destPtr[3] = (srcPtr[3] + srcPtr2[3] + 1) >> 1;
            destPtr[4] = (srcPtr[4] + srcPtr2[4] + 1) >> 1;
            destPtr[5] = (srcPtr[5] + srcPtr2[5] + 1) >> 1;
            destPtr[6] = (srcPtr[6] + srcPtr2[6] + 1) >> 1;
            destPtr[7] = (srcPtr[7] + srcPtr2[7] + 1) >> 1;

            destPtr += 8;
            rowStartPos += width;
        }

    } else {
        rowStartPos = fy * width + fx;

        for (i = 0; i < DCTSIZE; i++) {
            srcPtr = &(prev[rowStartPos]);

            destPtr[0] = srcPtr[0];
            destPtr[1] = srcPtr[1];
            destPtr[2] = srcPtr[2];
            destPtr[3] = srcPtr[3];
            destPtr[4] = srcPtr[4];
            destPtr[5] = srcPtr[5];
            destPtr[6] = srcPtr[6];
            destPtr[7] = srcPtr[7];

            destPtr += 8;
            rowStartPos += width;
        }
    }
}
/* REMOVE THIS LATER */
/*********************/


static void
Blockify(block, byteImage, by, bx)
    Block block;
    ByteImage *byteImage;
    int by, bx;
{
    int n = (by) * DCTSIZE * byteImage->parentWidth + (bx) * DCTSIZE;
    register unsigned char *row = &(byteImage->firstByte[n]);
    register short *dest = block;

    DO_N_TIMES(DCTSIZE,
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        *dest++ = (unsigned char) (*row++);
        \
        row += byteImage->parentWidth - 8;
        );
}


static int
BlockifyDiff(block, curr, prev, y, x, my, mx)
    Block block;
    ByteImage *curr, *prev;
    int y, x, my, mx;
{
    register int j;
    register int diff = 0;
    Block currBlock, prevBlock;
    register short *_b = block;
    register short *_c = currBlock;
    register short *_p = prevBlock;

    GetMotionBlock(curr, y, x, 0, 0, currBlock);        // optimize this later

    GetMotionBlock(prev, y, x, my, mx, prevBlock);

    for (j = 0; j < DCTSIZE; j++) {
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
        *_b = *_c++ - *_p++;
        diff += ABS_TABLE(*_b);
        _b++;
    }
    return (diff < BLOCK_BOUND);        /* BLOCK_BOUND == 128 */
}


int
BBlockifyDiff(block, curr, prev, next, y, x, uvBlock, fv, bv)
    Block block;
    ByteImage *curr, *prev, *next;
    int y, x;
    int uvBlock;                /* boolean flag; need to divided vectors by 2 for U or V blocks */
    Vector *fv, *bv;
{
    register int i, j;
    int n = 0, diff = 0;
    Block currBlock, prevBlock, nextBlock;

    if (!bv->exists)
        return BlockifyDiff(block, curr, prev, y, x, (fv->down) >> uvBlock, (fv->right) >> uvBlock);
    if (!fv->exists)
        return BlockifyDiff(block, curr, next, y, x, (bv->down) >> uvBlock, (bv->right) >> uvBlock);

    GetMotionBlock(curr, y, x, 0, 0, currBlock);        // optimize this later

    GetMotionBlock(prev, y, x, (fv->down) >> uvBlock, (fv->right) >> uvBlock, prevBlock);
    GetMotionBlock(next, y, x, (bv->down) >> uvBlock, (bv->right) >> uvBlock, nextBlock);

    for (j = 0; j < DCTSIZE; j++) {
        for (i = 0; i < DCTSIZE; i++, n++) {
            block[n] = currBlock[n] - (prevBlock[n] + nextBlock[n] + 1) / 2;
            diff += ABS_TABLE(block[n]);
        }
    }
    return (diff < BLOCK_BOUND);        /* BLOCK_BOUND == 128 */
}


/* This function is used for P-frame and B-frame intra coding only.
   The I-frame coding uses ScQuantize() which takes in an ScImage instead of a block */
static void
BlockIQuantZigRLE(block, scBlock, qScale, qTable)
    Block block;
    ScBlock *scBlock;
    int qScale;
    int *qTable;
{
    register int i, j = 0;
    int qentry, temp;

    scBlock->intracoded = 1;

    /* the DC coefficient is handled specially
       it's not sensitive to qscale, but everything else is */
    if (block[0] < 0) {
        scBlock->dc = -((-block[0] + 4) >> 3);  // out = in / 8; add 4 to round up

    } else {
        scBlock->dc = (block[0] + 4) >> 3;
    }

    /* now the AC coefficients */
    for (i = 1; i < 64; i++) {
        temp = block[ZAG[i]];
        qentry = qTable[ZAG[i]] * qScale;

        /* see 1993 MPEG doc, section D.6.3.4 */
        if (temp < 0) {
            temp = -((((-temp) << 3) + (qentry >> 1)) / qentry);
        } else {
            temp = ((temp << 3) + (qentry >> 1)) / qentry;
        }

        /* Record only the nonzero entries in the ScBlock.  The difference
           between the zig zag order indicies in the index[63] are the Run
           Lengths (plus one) */
        if (temp != 0) {
            scBlock->index[j] = (char) i;
            scBlock->value[j++] = temp;
        }
    }
    scBlock->numOfAC = j;
}


/* quantize intra coded ScImages only */
void
ScQuantize(scIn, qScale, qTable, scOut)
    ScImage *scIn;
    ByteImage *qScale;
    int *qTable;
    ScImage *scOut;
{
    register int i, j, y, x;
    register int qentry;
    register int temp;
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *inBlock = scIn->firstBlock;
    ScBlock *outBlock = scOut->firstBlock;
    int inSkip = scIn->parentWidth - scIn->width;
    int outSkip = scOut->parentWidth - scOut->width;
    int qScaleSkip = qScale->parentWidth - qScale->width;
    int isScUV = 2 - (scOut->width / qScale->width);    /* If scOut is U or V image then isScUV = 1 */

    for (y = 0; y < scIn->height; y++) {
        for (x = 0; x < scIn->width; x++) {

            /* the DC coefficient is handled specially
               it's not sensitive to qscale, but everything else is */
            if (inBlock->dc < 0) {
                outBlock->dc = -((-inBlock->dc + 4) >> 3);      // out = in / 8 (add 4 to round up)

            } else {
                outBlock->dc = (inBlock->dc + 4) >> 3;
            }

            /* now the AC coefficients */
            for (i = 0, j = 0; i < inBlock->numOfAC; i++) {
                temp = inBlock->value[i];
                qentry = qTable[(int)ZAG[(int)inBlock->index[i]]] * (*curQScale);
		/* index[i] has the index in zigzag order */

                /* see 1993 MPEG doc, section D.6.3.4 */
                if (temp < 0) {
                    temp = -((((-temp) << 3) + (qentry >> 1)) / qentry);

                } else {
                    temp = ((temp << 3) + (qentry >> 1)) / qentry;
                }

                if (temp != 0) {
                    outBlock->index[j] = inBlock->index[i];
                    outBlock->value[j++] = temp;
                }
            }
            outBlock->numOfAC = j;
            inBlock++;
            outBlock++;
            if (isScUV || (x % 2))
                curQScale++;
        }
        inBlock += inSkip;
        outBlock += outSkip;
        if (isScUV || (y % 2))
            curQScale += qScaleSkip;
        else
            curQScale -= qScale->width;
    }
}

/* Quantize, zig zag scan, RLE a non-intra coded block */
static void
BlockNonIQuantZigRLE(in, out, qScale, qTable)
    Block in;
    ScBlock *out;
    int qScale;
    int *qTable;
{
    register int i, j = 0;
    int temp;
    int nonZero = 0;

    if (out->skipBlock)
        return;
    out->intracoded = 0;

    /* DC coefficient treated same as AC but separate field in ScBlock */
    out->dc = (in[0] << 3) / (qTable[0] * qScale);
    if (out->dc) {
        nonZero = 1;
    }
    /* Now the AC coefficients */
    for (i = 1; i < 64; i++) {
        /* dead-zone quantizer - truncate */
        temp = (in[ZAG[i]] << 3) / (qTable[ZAG[i]] * qScale);

        if (temp != 0) {
            out->index[j] = (char) i;
            out->value[j++] = temp;
            nonZero = 1;
        }
    }
    out->numOfAC = j;

    if (!nonZero) {
        out->skipBlock = 1;
    }
}


/* Output ScImage is DCT'd, zig-zag, RLE'd, but not quantized */
void
ByteToSc(byte, sc)
    ByteImage *byte;
    ScImage *sc;
{
    int i, j, k, y;
    int skip = sc->parentWidth - sc->width;
    int skip8 = (byte->parentWidth) * DCTSIZE - byte->width;
    ScBlock *curBlock = sc->firstBlock;
    unsigned char *curSrc = byte->firstByte;
    unsigned char *curBlockSrc;
    short *curDest;
    short block[64];

    for (i = 0; i < sc->height; i++) {
        for (j = 0; j < sc->width; j++) {
            /* copy into block[64] (DCTBlock takes an array and destructively modifies it) */
            curBlockSrc = curSrc;
            curDest = block;
            DO_N_TIMES(8,
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                *curDest++ = (short) (*curBlockSrc++);
                curBlockSrc += byte->parentWidth - 8;
                );

            /* DCT transform */
            DCTBlock(block);

            /* copy into ScBlock */
            curBlock->dc = block[0];
            k = 0;
            y = 1;
            DO_N_TIMES(63,
                if (block[ZAG[y]]) {
                    curBlock->index[k] = (char) y;
                    curBlock->value[k++] = block[ZAG[y]];
                }
                y++;);
            curBlock->numOfAC = k;
            curBlock++;
            curSrc += DCTSIZE;
        }
        curSrc += skip8;
        curBlock += skip;
    }
}


void
ByteToScI(byte, qScale, qTable, sc)
    ByteImage *byte;
    ByteImage *qScale;
    int *qTable;
    ScImage *sc;
{
    ByteToSc(byte, sc);
    ScQuantize(sc, qScale, qTable, sc);
}


void
ByteYToScP(curr, prev, fmv, qScale, qTable, niqTable, sc)
    ByteImage *curr;
    ByteImage *prev;
    VectorImage *fmv;
    ByteImage *qScale;
    int *qTable, *niqTable;
    ScImage *sc;
{
    int x, y;
    int blkSkip = sc->parentWidth - sc->width;
    int vecSkip = fmv->parentWidth - fmv->width;
    Block block;
    ScBlock *curBlock = sc->firstBlock;
    Vector *curVector = fmv->firstVector;
    unsigned char *curQScale = qScale->firstByte;

    for (y = 0; y < sc->height; y++) {
        for (x = 0; x < sc->width; x++) {

            if (curVector->exists) {
                /* USE MOTION VECTORS */
                curBlock->skipBlock = BlockifyDiff(block, curr, prev, y, x, curVector->down, curVector->right);
                if (!curBlock->skipBlock) {
                    DCTBlock(block);
                    BlockNonIQuantZigRLE(block, curBlock, *curQScale, niqTable);
                }
            } else {
                /* ENCODE INTRA BLOCK */
                Blockify(block, curr, y, x);
                DCTBlock(block);
                BlockIQuantZigRLE(block, curBlock, *curQScale, qTable);
            }
            curBlock++;
            if (x % 2)
                curVector++;
        }                       /* for x */
        curBlock += blkSkip;
        if (y % 2)
            curVector += vecSkip;
        else
            curVector -= fmv->width;
    }                           /* for y */
}

void
ByteUVToScP(curr, prev, fmv, qScale, qTable, niqTable, sc)
    ByteImage *curr;
    ByteImage *prev;
    VectorImage *fmv;
    ByteImage *qScale;
    int *qTable, *niqTable;
    ScImage *sc;
{
    int x, y;
    int blkSkip = sc->parentWidth - sc->width;
    int vecSkip = fmv->parentWidth - fmv->width;
    int qScaleSkip = qScale->parentWidth - qScale->width;
    Block block;
    ScBlock *curBlock = sc->firstBlock;
    Vector *curVector = fmv->firstVector;
    unsigned char *curQScale = qScale->firstByte;

    for (y = 0; y < sc->height; y++) {
        for (x = 0; x < sc->width; x++) {

            if (curVector->exists) {
                /* USE MOTION VECTORS */
                curBlock->skipBlock = BlockifyDiff(block, curr, prev, y, x, (curVector->down >> 1), (curVector->right >> 1));
                if (!curBlock->skipBlock) {
                    DCTBlock(block);
                    BlockNonIQuantZigRLE(block, curBlock, *curQScale, niqTable);
                }
            } else {
                /* ENCODE INTRA BLOCK */
                Blockify(block, curr, y, x);
                DCTBlock(block);
                BlockIQuantZigRLE(block, curBlock, *curQScale, qTable);
            }
            curBlock++;
            curVector++;
            curQScale++;
        }                       /* for x */
        curBlock += blkSkip;
        curVector += vecSkip;
        curQScale += qScaleSkip;
    }                           /* for y */
}


void
ByteYToScB(curr, prev, next, fmv, bmv, qScale, qTable, niqTable, sc)
    ByteImage *curr, *prev, *next;
    VectorImage *fmv, *bmv;
    ByteImage *qScale;
    int *qTable, *niqTable;
    ScImage *sc;
{
    int x, y;
    int blkSkip = sc->parentWidth - sc->width;
    int fmvSkip = fmv->parentWidth - fmv->width;
    int bmvSkip = fmv->parentWidth - bmv->width;
    int qScaleSkip = qScale->parentWidth - qScale->width;
    Block block;
    ScBlock *curBlock = sc->firstBlock;
    Vector *curFVec = fmv->firstVector;
    Vector *curBVec = bmv->firstVector;
    unsigned char *curQScale = qScale->firstByte;

    for (y = 0; y < sc->height; y++) {
        for (x = 0; x < sc->width; x++) {

            if (((curFVec->exists != 2) || (curBVec->exists != 2))) {   /* if not skip (exists == 2 for skip) */

                if ((curFVec->exists == 1) || (curBVec->exists == 1)) {
                    /* USE MOTION VECTORS */
                    curBlock->skipBlock = BBlockifyDiff(block, curr, prev, next, y, x, 0, curFVec, curBVec);
                    if (!curBlock->skipBlock) {
                        DCTBlock(block);
                        BlockNonIQuantZigRLE(block, curBlock, *curQScale, niqTable);
                    }
                } else {
                    /* ENCODE INTRA BLOCK */
                    Blockify(block, curr, y, x);
                    DCTBlock(block);
                    BlockIQuantZigRLE(block, curBlock, *curQScale, qTable);
                }
            }
            curBlock++;
            if (x % 2) {
                curFVec++;
                curBVec++;
                curQScale++;
            }
        }                       /* for x */
        curBlock += blkSkip;
        if (y % 2) {
            curFVec += fmvSkip;
            curBVec += bmvSkip;
            curQScale += qScaleSkip;
        } else {
            curFVec -= fmv->width;
            curBVec -= bmv->width;
            curQScale -= qScale->width;
        }
    }                           /* for y */
}

void
ByteUVToScB(curr, prev, next, fmv, bmv, qScale, qTable, niqTable, sc)
    ByteImage *curr, *prev, *next;
    VectorImage *fmv, *bmv;
    ByteImage *qScale;
    int *qTable, *niqTable;
    ScImage *sc;
{
    int x, y;
    int blkSkip = sc->parentWidth - sc->width;
    int fmvSkip = fmv->parentWidth - fmv->width;
    int bmvSkip = bmv->parentWidth - bmv->width;
    int qScaleSkip = qScale->parentWidth - qScale->width;
    Block block;
    ScBlock *curBlock = sc->firstBlock;
    Vector *curFVec = fmv->firstVector;
    Vector *curBVec = bmv->firstVector;
    unsigned char *curQScale = qScale->firstByte;

    for (y = 0; y < sc->height; y++) {
        for (x = 0; x < sc->width; x++) {

            if (!((curFVec->exists == 2) && (curBVec->exists == 2))) {  /* if not skip (exists == 2 for skip) */

                if ((curFVec->exists == 1) || (curBVec->exists == 1)) {
                    /* USE MOTION VECTORS */
                    curBlock->skipBlock = BBlockifyDiff(block, curr, prev, next, y, x, 1, curFVec, curBVec);
                    if (!curBlock->skipBlock) {
                        DCTBlock(block);
                        BlockNonIQuantZigRLE(block, curBlock, *curQScale, niqTable);
                    }
                } else {
                    /* ENCODE INTRA BLOCK */
                    Blockify(block, curr, y, x);
                    DCTBlock(block);
                    BlockIQuantZigRLE(block, curBlock, *curQScale, qTable);
                }
            }
            curBlock++;
            curFVec++;
            curBVec++;
            curQScale++;
        }                       /* for x */
        curBlock += blkSkip;
        curFVec += fmvSkip;
        curBVec += bmvSkip;
        curQScale += qScaleSkip;
    }                           /* for y */
}

void
ScDequantize(scIn, qScale, qTable, scOut)
    ScImage *scIn;
    ByteImage *qScale;
    int *qTable;
    ScImage *scOut;
{
    register int i, y, x;
    int qentry;
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *inBlock = scIn->firstBlock;
    ScBlock *outBlock = scOut->firstBlock;
    int isScUV = 2 - (scOut->width / qScale->width);    /* If scOut is U or V image then isScUV = 1 */

    for (y = 0; y < scIn->height; y++) {
        for (x = 0; x < scIn->width; x++) {
            outBlock->dc = inBlock->dc << 3;
            if (outBlock->dc < 0) {
                outBlock->dc += (1 - (outBlock->dc & 1));
            } else {
                outBlock->dc -= (1 - (outBlock->dc & 1));
            }

            for (i = 0; i < inBlock->numOfAC; i++) {
                qentry = qTable[(int)ZAG[(int)inBlock->index[i]]] * (*curQScale);         /* index[i] has the index in zigzag order */
                outBlock->value[i] = (inBlock->value[i] * qentry) >> 3;
                if (outBlock->value[i] < 0) {
                    outBlock->value[i] += (1 - (outBlock->value[i] & 1));
                } else {
                    outBlock->value[i] -= (1 - (outBlock->value[i] & 1));
                }
            }
            outBlock->numOfAC = inBlock->numOfAC;
            inBlock++;
            outBlock++;
            if (isScUV || (x % 2))
                curQScale++;
        }
        inBlock += scIn->parentWidth - scIn->width;
        outBlock += scOut->parentWidth - scOut->width;
        if (isScUV || (y % 2))
            curQScale += qScale->parentWidth - qScale->width;
        else
            curQScale -= qScale->width;
    }
}


/* This needs a little cleaning up */
void
ScNonIDequantize(scIn, qScale, qTable, niqTable, scOut)         // need to have two qTables
     ScImage *scIn;
    ByteImage *qScale;
    int *qTable, *niqTable;
    ScImage *scOut;
{
    register int i, y, x;
    int qentry;
    unsigned char *curQScale = qScale->firstByte;
    ScBlock *inBlock = scIn->firstBlock;
    ScBlock *outBlock = scOut->firstBlock;
    int isScUV = 2 - (scOut->width / qScale->width);    /* If scOut is U or V image then isScUV = 1 */

    for (y = 0; y < scIn->height; y++) {
        for (x = 0; x < scIn->width; x++) {

            if (inBlock->intracoded) {
                outBlock->dc = inBlock->dc << 3;
                if (outBlock->dc < 0) {
                    outBlock->dc += (1 - (outBlock->dc & 1));
                } else {
                    outBlock->dc -= (1 - (outBlock->dc & 1));
                }

                for (i = 0; i < inBlock->numOfAC; i++) {
                    qentry = qTable[(int)ZAG[(int)inBlock->index[i]]] * (*curQScale);
		    /* index[i] has the index in zigzag order */
                    outBlock->value[i] = (inBlock->value[i] * qentry) >> 3;
                    if (outBlock->value[i] < 0) {
                        outBlock->value[i] += (1 - (outBlock->value[i] & 1));
                    } else {
                        outBlock->value[i] -= (1 - (outBlock->value[i] & 1));
                    }
                }
                outBlock->numOfAC = inBlock->numOfAC;

            } else if ((inBlock->skipMB != 1) && (inBlock->skipBlock != 1)) {
                qentry = niqTable[0] * (*curQScale);
                //outBlock->dc = (inBlock->dc >> 3) * qentry;
                if (inBlock->dc < 0) {
                    outBlock->dc = (((inBlock->dc << 1) - 1) * qentry) >> 4;
                    outBlock->dc += (1 - (outBlock->dc & 1));
                } else {
                    outBlock->dc = (((inBlock->dc << 1) + 1) * qentry) >> 4;
                    outBlock->dc -= (1 - (outBlock->dc & 1));
                }

                for (i = 0; i < inBlock->numOfAC; i++) {
                    qentry = niqTable[(int)ZAG[(int)inBlock->index[i]]] * (*curQScale);
                    //outBlock->value[i] = ((inBlock->value[i]) >> 3) * qentry;
                    if (inBlock->value[i] < 0) {
                        outBlock->value[i] = (((inBlock->value[i] << 1) - 1) * qentry) >> 4;
                        outBlock->value[i] += (1 - (outBlock->value[i] & 1));
                    } else {
                        outBlock->value[i] = (((inBlock->value[i] << 1) + 1) * qentry) >> 4;
                        outBlock->value[i] -= (1 - (outBlock->value[i] & 1));
                    }

                    if (outBlock->value[i] > 2047) {
                        outBlock->value[i] = 2047;
                    } else if (outBlock->value[i] < -2048) {
                        outBlock->value[i] = -2048;
                    }
                }
                outBlock->numOfAC = inBlock->numOfAC;
            }
            inBlock++;
            outBlock++;
            if (isScUV || (x % 2))
                curQScale++;
        }
        inBlock += scIn->parentWidth - scIn->width;
        outBlock += scOut->parentWidth - scOut->width;
        if (isScUV || (y % 2))
            curQScale += qScale->parentWidth - qScale->width;
        else
            curQScale -= qScale->width;
    }
}
