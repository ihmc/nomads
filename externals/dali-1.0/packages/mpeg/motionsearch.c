/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */

/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 *--------------------------------------------------------------------
 * Interface functions for motion vector searching 
 *
 * Jiesang Song     Aug 98
 *---------------------------------------------------------------------
 */

#include "mpegInt.h"
#include "tables.h"

#define SET_VECTOR(curr_vector, d, r) {\
    (curr_vector)->down = d;\
    (curr_vector)->right = r;\
}

static void 
COPY_LUMBLOCK(currentBlock, blockStartPos, byteImage)
    LumBlock currentBlock;
    unsigned char *blockStartPos;
    ByteImage *byteImage;
{
    unsigned char *pByte = blockStartPos;
    short *currRow = currentBlock;

    DO_N_TIMES(16,
        currRow[0] = pByte[0];
        currRow[1] = pByte[1];
        currRow[2] = pByte[2];
        currRow[3] = pByte[3];
        currRow[4] = pByte[4];
        currRow[5] = pByte[5];
        currRow[6] = pByte[6];
        currRow[7] = pByte[7];
        currRow[8] = pByte[8];
        currRow[9] = pByte[9];
        currRow[10] = pByte[10];
        currRow[11] = pByte[11];
        currRow[12] = pByte[12];
        currRow[13] = pByte[13];
        currRow[14] = pByte[14];
        currRow[15] = pByte[15];
        pByte += byteImage->parentWidth;
        currRow += 16;
        );
}

/* Check if x is between 0 and 1 in Figure 2-D.33 of the MPEG-1 specification */
/* returns 1 if zero vector is sufficient */
static int
ZeroMotionSufficient(currentBlock, prev, blockStartPos, zeroDiff)
    LumBlock currentBlock;
    ByteImage *prev;
    unsigned char *blockStartPos;
    long *zeroDiff;             /* zeroDiff == Z */
{
    LumBlock motionBlock;       /* typedef int32 LumBlock[2*DCTSIZE][2*DCTSIZE]; */

    COPY_LUMBLOCK(motionBlock, blockStartPos, prev);
    *zeroDiff = LumBlockMAD(currentBlock, motionBlock, 0x7fffffff);

    return (*zeroDiff <= 256);
}

/* Remaining part of Figure 2-D.33 */
/* returns 1 if zero vector is better than (mx, my) */
static int
ZeroMotionBetter(motionDiff, zeroDiff)
    long motionDiff;            /* best MAD from motion search */
    long zeroDiff;              /* MAD of zero motion block */
{
    if (zeroDiff < 256 * 3) {   /* x < 3 */
        if (2 * motionDiff >= zeroDiff) {       /* y >= x/2 */
            return 1;
        }
    } else {
        if (11 * motionDiff >= 10 * zeroDiff) {         /* y >= x/1.1 */
            return 1;
        }
    }
    return 0;
}



#define SQUARE(x) squaresTable[(x) + 255]


/* Decide if intra coding is necessary */
/* Algorithm on page D-49 of the MPEG-1 specification  */
/* returns 1 if intra-block coding is better; 0 if not */
static int
DoPIntraCode(currentBlock, prev, intermediates, by, bx, motionY, motionX)
    LumBlock currentBlock;
    ByteImage *prev;
    ByteImage **intermediates;
    int by, bx;
    int motionY;
    int motionX;
{
    long sum = 0, vard = 0, varc = 0;
    short dif;
    short currPixel, prevPixel;
    LumBlock motionBlock;
    short *curr = currentBlock;
    short *motion = motionBlock;

    ComputeMotionLumBlock(prev, intermediates, by, bx, motionY, motionX, motionBlock);

    DO_N_TIMES(256,
        currPixel = *curr++;
        prevPixel = *motion++;

        sum += currPixel;
        varc += SQUARE(currPixel);

        dif = currPixel - prevPixel;
        vard += SQUARE(dif);
        );

    vard >>= 8;                 /* assumes mean is close to zero */
    varc = (varc >> 8) - (sum >> 8) * (sum >> 8);

    if (vard <= 64) {
        return 0;
    } else if (vard < varc) {
        return 0;
    } else {
        return 1;
    }
}


/* Produces a motion vector VectorImage and the Y, U, V error ByteImages */
void
BytePMotionVecSearch(picHdr, currY, prevY, intermediates, vec)
    MpegPicHdr *picHdr;
    ByteImage *currY, *prevY;
    ByteImage **intermediates;
    VectorImage *vec;
{
    int x, y;
    int stepSize = picHdr->full_pel_forward_vector + 1;         /* 1 (== 0.5) or 2 (== 1) */
    int searchRange = picHdr->forward_f * DCTSIZE * stepSize;
    int lastBlockX = (currY->width) / DCTSIZE;
    int lastBlockY = (currY->height) / DCTSIZE;
    int motionX = 0, motionY = 0;
    long zeroDiff, motionDiff;
    int skip = vec->parentWidth - vec->width;
    Vector *curVector = vec->firstVector;
    LumBlock currentBlock;
    unsigned char *currByte = currY->firstByte;
    unsigned char *prevByte = prevY->firstByte;
    int skip16 = (currY->parentWidth) * 2 * DCTSIZE - currY->width;

    for (y = 0; y < lastBlockY; y += 2) {
        for (x = 0; x < lastBlockX; x += 2) {

            COPY_LUMBLOCK(currentBlock, currByte, currY);

            if (ZeroMotionSufficient(currentBlock, prevY, prevByte, &zeroDiff)) {
                motionX = 0;
                motionY = 0;
                curVector->exists = 1;
                SET_VECTOR(curVector, 0, 0);
            } else {
                motionDiff = PMotionSearch(currentBlock, prevY, intermediates, stepSize, searchRange,
                    y, x, currY->height, currY->width, &motionY, &motionX);
                if (((motionX != 0) || (motionY != 0)) && ZeroMotionBetter(motionDiff, zeroDiff)) {
                    motionX = 0;
                    motionY = 0;
                }
                if (DoPIntraCode(currentBlock, prevY, intermediates, y, x, motionY, motionX)) {
                    curVector->exists = 0;
                } else {
                    curVector->exists = 1;
                    SET_VECTOR(curVector, motionY, motionX);
                }
            }
            curVector++;
            currByte += 16;
            prevByte += 16;
        }                       /* for x */
        currByte += skip16;
        prevByte += skip16;
        curVector += skip;
    }                           /* for y */
}


////////////////////////////////////////////
/* B-frame motion vector search functions */
static int
MotionSufficient(currBlock, prevY, nextY, interPrev, interNext, bx, by, fmx, fmy, bmx, bmy, mode)
    LumBlock currBlock;
    ByteImage *prevY, *nextY;
    ByteImage **interPrev, **interNext;
    int bx, by;
    int fmx, fmy;               /* forward motion */
    int bmx, bmy;               /* backward motion */
    int mode;                   /* forw/back/interpolate */
{
    LumBlock motionBlock;

    if (mode != MOTION_BACKWARD) {
        /* check forward motion for bounds */
        if ((by * DCTSIZE + (fmy - 1) / 2 < 0) || ((by + 2) * DCTSIZE + (fmy + 1) / 2 - 1 >= prevY->height)) {
            return 0;
        }
        if ((bx * DCTSIZE + (fmx - 1) / 2 < 0) || ((bx + 2) * DCTSIZE + (fmx + 1) / 2 - 1 >= prevY->width)) {
            return 0;
        }
    }
    if (mode != MOTION_FORWARD) {
        /* check backward motion for bounds */
        if ((by * DCTSIZE + (bmy - 1) / 2 < 0) || ((by + 2) * DCTSIZE + (bmy + 1) / 2 - 1 >= nextY->height)) {
            return 0;
        }
        if ((bx * DCTSIZE + (bmx - 1) / 2 < 0) || ((bx + 2) * DCTSIZE + (bmx + 1) / 2 - 1 >= nextY->width)) {
            return 0;
        }
    }
    ComputeBMotionLumBlock(prevY, nextY, interPrev, interNext, bx, by, fmx, fmy, bmx, bmy, mode, motionBlock);
    return (LumBlockMAD(currBlock, motionBlock, 0x7fffffff) <= 512);
}


/* Similar to P-FRAME */
static int
DoBIntraCode(currentBlock, prevY, nextY, interPrev, interNext, bx, by, fmx, fmy, bmx, bmy, mode)
    LumBlock currentBlock;
    ByteImage *prevY, *nextY;
    ByteImage **interPrev, **interNext;
    int bx, by;
    int fmx, fmy;               /* forward motion */
    int bmx, bmy;               /* backward motion */
    int mode;                   /* for/back/interpolate */
{
    long sum = 0, vard = 0, varc = 0;
    short currPixel, prevPixel;
    short dif;
    LumBlock motionBlock;
    short *curr = currentBlock;
    short *motion = motionBlock;

    ComputeBMotionLumBlock(prevY, nextY, interPrev, interNext, bx, by, fmx, fmy, bmx, bmy, mode, motionBlock);
    DO_N_TIMES(256,
        currPixel = *curr++;
        prevPixel = *motion++;

        sum += currPixel;
        varc += SQUARE(currPixel);

        dif = currPixel - prevPixel;
        vard += SQUARE(dif);
        );

    vard >>= 8;                 /* assumes mean is close to zero */
    varc = (varc >> 8) - (sum >> 8) * (sum >> 8);

    if (vard <= 64) {
        return 0;
    } else if (vard < varc) {
        return 0;
    } else {
        return 1;
    }
}

/* Produces forward and backward motion vector VectorImages and the Y, U, V error ByteImages */
void
ByteBMotionVecSearch(picHdr, currY, prevY, nextY, interPrev, interNext, sliceInfo, sliceInfoLen, fmv, bmv)
    MpegPicHdr *picHdr;
    ByteImage *currY, *prevY, *nextY;
    ByteImage **interPrev, **interNext;
    int *sliceInfo;
    int sliceInfoLen;
    VectorImage *fmv, *bmv;
{
    int lastIntra = 1;
    int result;
    int fStepSize = (picHdr->full_pel_forward_vector) ? 2 : 1;  /* 1 (== 0.5) or 2 (== 1) */
    int bStepSize = (picHdr->full_pel_backward_vector) ? 2 : 1;         /* 1 (== 0.5) or 2 (== 1) */
    int fSearchRange = picHdr->forward_f * DCTSIZE * fStepSize;
    int bSearchRange = picHdr->backward_f * DCTSIZE * bStepSize;
    int lastBlockX = currY->width / DCTSIZE;
    int lastBlockY = currY->height / DCTSIZE;
    int lastX = lastBlockX - 2;
    int lastY = lastBlockY - 2;
    int fmx = 0, fmy = 0;       /* forward motion */
    int bmx = 0, bmy = 0;       /* backward motion */
    int oldFmx = 0, oldFmy = 0;
    int oldBmx = 0, oldBmy = 0;
    int mode = MOTION_FORWARD;
    int oldMode = MOTION_FORWARD;
    int x, y;                   //, ix, iy, i;

    int curSliceNum, numMBInSlice = 0;
    int skipF = fmv->parentWidth - fmv->width;
    int skipB = bmv->parentWidth - bmv->width;
    Vector *curFVec = fmv->firstVector;
    Vector *curBVec = bmv->firstVector;
    LumBlock currentBlock;
    unsigned char *currByte = currY->firstByte;
    int skip16 = (currY->parentWidth) * 2 * DCTSIZE - currY->width;

    for (y = 0; y < lastBlockY; y += 2) {
        for (x = 0; x < lastBlockX; x += 2) {

            COPY_LUMBLOCK(currentBlock, currByte, currY);

            curSliceNum = 0;
            if (numMBInSlice == sliceInfo[curSliceNum]) {
                /* reset everything */
                oldFmx = 0;
                oldFmy = 0;
                oldBmx = 0;
                oldBmy = 0;
                oldMode = MOTION_FORWARD;
                lastIntra = 1;

                if (curSliceNum < sliceInfoLen) {
                    curSliceNum++;
                } else {
                    curSliceNum = 0;    /* wrap around in the sliceInfo array */
                }
                numMBInSlice = 0;
            }
            /*Check if the macroblock can be skipped:
             *  1)  not the last block in frame
             *  2)  not the last block in slice
             *  3)  not the first block in slice
             *  4)  previous block was not intra-coded
             */
            if (((y < lastY) || (x < lastX)) && (numMBInSlice < sliceInfo[curSliceNum]) && (numMBInSlice != 0) && (!lastIntra)) {
                result = (MotionSufficient(currentBlock, prevY, nextY, interPrev, interNext, x, y, oldFmx, oldFmy, oldBmx, oldBmy, oldMode));
            } else {
                result = 0;
            }

            if (result) {
                curFVec->exists = (curBVec->exists = 2);        /* skip this MB */

            } else {            /* do B mv search */
                if (picHdr->forward_f == 0) {
                    mode = MOTION_BACKWARD;
                    PMotionSearch(currentBlock, nextY, interNext, bStepSize, bSearchRange, y, x, currY->height, currY->width, &bmy, &bmx);
                } else if (picHdr->backward_f == 0) {
                    mode = MOTION_FORWARD;
                    PMotionSearch(currentBlock, prevY, interPrev, fStepSize, fSearchRange, y, x, currY->height, currY->width, &fmy, &fmx);
                } else {
                    mode = BMotionSearch(currentBlock, prevY, nextY, interPrev, interNext, fStepSize, fSearchRange,
                        bStepSize, bSearchRange, y, x, currY->height, currY->width, &fmy, &fmx, &bmy, &bmx, mode);
                }

                if (DoBIntraCode(currentBlock, prevY, nextY, interPrev, interNext, x, y, fmx, fmy, bmx, bmy, mode)) {
                    curFVec->exists = (curBVec->exists = 0);
                    lastIntra = 1;
                } else {
                    lastIntra = 0;
                    oldMode = mode;
                    switch (mode) {     /* clean this up */
                    case MOTION_FORWARD:
                        curBVec->exists = 0;
                        curFVec->exists = 1;
                        SET_VECTOR(curFVec, fmy, fmx);
                        oldFmx = fmx;
                        oldFmy = fmy;
                        break;
                    case MOTION_BACKWARD:
                        curFVec->exists = 0;
                        curBVec->exists = 1;
                        SET_VECTOR(curBVec, bmy, bmx);
                        oldBmx = bmx;
                        oldBmy = bmy;
                        break;
                    case MOTION_INTERPOLATE:
                        curFVec->exists = (curBVec->exists = 1);
                        SET_VECTOR(curFVec, fmy, fmx);
                        SET_VECTOR(curBVec, bmy, bmx);
                        oldFmx = fmx;
                        oldFmy = fmy;
                        oldBmx = bmx;
                        oldBmy = bmy;
                        break;
                    default:
                        fprintf(stderr, "Illegal mode: %d\n", mode);
                        exit(1);
                    }
                }
            }
            currByte += 16;
            curFVec++;
            curBVec++;
            numMBInSlice++;
        }                       /* for x */
        currByte += skip16;
        curFVec += skipF;
        curBVec += skipB;
    }                           /* for y */
}


void
ByteComputeIntermediates(original, intermediates)
    ByteImage *original;
    ByteImage **intermediates;
{
    int y, x;
    ByteImage *inter = intermediates[0];
    int skip1 = original->parentWidth;
    int skip2 = inter->parentWidth;
    unsigned char *src1 = original->firstByte;
    unsigned char *src2 = src1 + skip2;
    unsigned char *dest = inter->firstByte;

    /* intermediate x pixels */
    for (y = 0; y < inter->height; y++) {
        for (x = 0; x < inter->width; x++) {
            dest[x] = (src1[x] + src1[x + 1] + 1) >> 1;
        }
        src1 += skip1;
        dest += skip2;
    }

    /* intermediate y pixels */
    inter = intermediates[1];
    skip2 = inter->parentWidth;
    src1 = original->firstByte;
    dest = inter->firstByte;
    for (y = 0; y < inter->height; y++) {
        for (x = 0; x < inter->width; x++) {
            dest[x] = (src1[x] + src2[x] + 1) >> 1;
        }
        src1 += skip1;
        src2 += skip1;
        dest += skip2;
    }

    /* intermediate xy pixels */
    inter = intermediates[2];
    skip2 = inter->parentWidth;
    src1 = original->firstByte;
    src2 = src1 + skip1;
    dest = inter->firstByte;
    for (y = 0; y < inter->height; y++) {
        for (x = 0; x < inter->width; x++) {
            dest[x] = (src1[x] + src1[x + 1] + src2[x] + src2[x + 1] + 2) >> 2;
        }
        src1 += skip1;
        src2 += skip1;
        dest += skip2;
    }
}
