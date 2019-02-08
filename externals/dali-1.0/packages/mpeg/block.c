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

#include "mpegInt.h"
#include "tables.h"

#define BLOCK_TO_FRAME_COORD(bx1, bx2, x1, x2) {    \
        x1 = (bx1)*DCTSIZE;                         \
        x2 = (bx2)*DCTSIZE;                         \
    }

#define MOTION_TO_FRAME_COORD(bx1, bx2, mx1, mx2, x1, x2) { \
        x1 = (bx1)*DCTSIZE+(mx1);                           \
        x2 = (bx2)*DCTSIZE+(mx2);                           \
    }

#undef ABS
#define ABS(x)  ((x < 0) ? (-x) : x)
#define ABS_TABLE(x) absTable[(x) + 255]

#define TRUNCATE_UINT8(x)       ((x < 0) ? 0 : ((x > 255) ? 255 : x))

/* used only for U, V where half pixel data haven't been precomputed */
void
ComputeMotionBlock(prevFrame, by, bx, my, mx, motionBlock)
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

    xHalf = (ABS(mx) % 2 == 1);
    yHalf = (ABS(my) % 2 == 1);
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

void
ComputeBMotionBlock(prev, next, by, bx, fmy, fmx, bmy, bmx, mode, motionBlock)
    ByteImage *prev, *next;
    int bx, by;
    int fmx, fmy;               /* forward motion */
    int bmx, bmy;               /* backward motion */
    int mode;                   /* for/back/interpolate */
    Block motionBlock;
{
    Block prevBlock, nextBlock;
    register int i;

    switch (mode) {
    case MOTION_FORWARD:
        ComputeMotionBlock(prev, by, bx, fmy, fmx, motionBlock);
        break;
    case MOTION_BACKWARD:
        ComputeMotionBlock(next, by, bx, bmy, bmx, motionBlock);
        break;
    case MOTION_INTERPOLATE:
        ComputeMotionBlock(prev, by, bx, fmy, fmx, prevBlock);
        ComputeMotionBlock(next, by, bx, bmy, bmx, nextBlock);
        for (i = 0; i < 64; i++) {
            motionBlock[i] = (prevBlock[i] + nextBlock[i] + 1) >> 1;
        }
        break;
    }
}

/*===========================================================================*
 *
 * ComputeMotionLumBlock
 *
 *      compute the motion-compensated luminance block
 *
 * RETURNS:     motionBlock
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:       motion vector MUST be valid
 *
 * NOTE:  see ComputeMotionBlock
 *
 *===========================================================================*/
void
ComputeMotionLumBlock(prevFrame, intermediates, by, bx, my, mx, motionBlock)
    ByteImage *prevFrame;
    ByteImage **intermediates;
    int by;
    int bx;
    int my;
    int mx;
    LumBlock motionBlock;
{
    register unsigned char *across;
    register short *macross;
    register int i;
    unsigned char *prev;        /* actual pointer to bytes of previous frame */
    int width = prevFrame->parentWidth;
    int fy, fx;
    int rowStartPos;
    int xHalf, yHalf;

    xHalf = (ABS(mx) % 2 == 1);
    yHalf = (ABS(my) % 2 == 1);

    MOTION_TO_FRAME_COORD(by, bx, my / 2, mx / 2, fy, fx);

    if (xHalf) {
        if (mx < 0) {           // why ??????

            fx--;
        }
        if (yHalf) {
            if (my < 0) {
                fy--;
            }
            prev = intermediates[0]->firstByte;
        } else {
            prev = intermediates[1]->firstByte;
        }
    } else if (yHalf) {
        if (my < 0) {
            fy--;
        }
        prev = intermediates[2]->firstByte;
    } else {
        prev = prevFrame->firstByte;
    }

    rowStartPos = fy * width + fx;
    macross = motionBlock;
    for (i = 0; i < 16; i++) {
        across = &(prev[rowStartPos]);

        macross[0] = across[0];
        macross[1] = across[1];
        macross[2] = across[2];
        macross[3] = across[3];
        macross[4] = across[4];
        macross[5] = across[5];
        macross[6] = across[6];
        macross[7] = across[7];
        macross[8] = across[8];
        macross[9] = across[9];
        macross[10] = across[10];
        macross[11] = across[11];
        macross[12] = across[12];
        macross[13] = across[13];
        macross[14] = across[14];
        macross[15] = across[15];

        macross += 16;
        rowStartPos += width;
    }
}


void
ComputeBMotionLumBlock(prevY, nextY, interPrev, interNext, bx, by, fmx, fmy, bmx, bmy, mode, motionBlock)
    ByteImage *prevY, *nextY;
    ByteImage **interPrev, **interNext;
    int bx, by;
    int fmx, fmy;               /* forward motion */
    int bmx, bmy;               /* backward motion */
    int mode;                   /* for/back/interpolate */
    LumBlock motionBlock;
{
    LumBlock prevBlock, nextBlock;
    short *prev, *next, *motion;

    switch (mode) {
    case MOTION_FORWARD:
        ComputeMotionLumBlock(prevY, interPrev, by, bx, fmy, fmx, motionBlock);
        break;
    case MOTION_BACKWARD:
        ComputeMotionLumBlock(nextY, interNext, by, bx, bmy, bmx, motionBlock);
        break;
    case MOTION_INTERPOLATE:
        ComputeMotionLumBlock(prevY, interPrev, by, bx, fmy, fmx, prevBlock);
        ComputeMotionLumBlock(nextY, interNext, by, bx, bmy, bmx, nextBlock);

        prev = prevBlock;
        next = nextBlock;
        motion = motionBlock;
        DO_N_TIMES(16,
            motion[0] = (prev[0] + next[0] + 1) >> 1;
            motion[1] = (prev[1] + next[1] + 1) >> 1;
            motion[2] = (prev[2] + next[2] + 1) >> 1;
            motion[3] = (prev[3] + next[3] + 1) >> 1;
            motion[4] = (prev[4] + next[4] + 1) >> 1;
            motion[5] = (prev[5] + next[5] + 1) >> 1;
            motion[6] = (prev[6] + next[6] + 1) >> 1;
            motion[7] = (prev[7] + next[7] + 1) >> 1;
            motion[8] = (prev[8] + next[8] + 1) >> 1;
            motion[9] = (prev[9] + next[9] + 1) >> 1;
            motion[10] = (prev[10] + next[10] + 1) >> 1;
            motion[11] = (prev[11] + next[11] + 1) >> 1;
            motion[12] = (prev[12] + next[12] + 1) >> 1;
            motion[13] = (prev[13] + next[13] + 1) >> 1;
            motion[14] = (prev[14] + next[14] + 1) >> 1;
            motion[15] = (prev[15] + next[15] + 1) >> 1;

            motion += 16;
            prev += 16;
            next += 16;
            );
        break;
    }
}

/*===========================================================================*
 *
 * LumBlockMAD
 *
 *      return the MAD of two luminance blocks
 *
 * RETURNS:     the MAD, if less than bestSoFar, or
 *              some number bigger if not
 *
 * SIDE EFFECTS:    none
 *
 *===========================================================================*/
long
LumBlockMAD(currentBlock, motionBlock, bestSoFar)
    LumBlock currentBlock, motionBlock;
    long bestSoFar;
{
    register long diff = 0;     /* max value of diff is 255*256 = 65280 */
    register long localDiff = 0;
    register int y;
    register short *curr, *motion;

    curr = currentBlock;
    motion = motionBlock;
    for (y = 0; y < 16; y++) {
        localDiff = curr[0] - motion[0];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[1] - motion[1];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[2] - motion[2];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[3] - motion[3];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[4] - motion[4];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[5] - motion[5];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[6] - motion[6];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[7] - motion[7];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[8] - motion[8];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[9] - motion[9];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[10] - motion[10];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[11] - motion[11];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[12] - motion[12];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[13] - motion[13];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[14] - motion[14];
        diff += ABS_TABLE(localDiff);
        localDiff = curr[15] - motion[15];
        diff += ABS_TABLE(localDiff);

        if (diff > bestSoFar) {
            return diff;
        }
        curr += 16;
        motion += 16;
    }

    return diff;
}


/*===========================================================================*
 *
 * LumMotionError
 *
 *      return the MAD of the currentBlock and the motion-compensated block
 *
 * RETURNS:     the MAD, if less than bestSoFar, or
 *              some number bigger if not
 *
 * SIDE EFFECTS:    none
 *
 * PRECONDITIONS:  motion vector MUST be valid
 *
 * NOTES:  this is the procedure that is called the most, and should therefore
 *         be the most optimized!!!
 *
 *===========================================================================*/
long
LumMotionError(currentBlock, prevFrame, intermediates, by, bx, my, mx, bestSoFar)
    LumBlock currentBlock;
    ByteImage *prevFrame;
    ByteImage **intermediates;
    int by, bx;
    int my, mx;
    long bestSoFar;
{
    register long diff = 0;     /* max value of diff is 255*256 = 65280 */
    register long localDiff;
    register unsigned char *across;
    register short *cacross;
    register unsigned char *prev;       /* actual pointer to bytes of previous frame */
    int width = prevFrame->parentWidth;
    int fy, fx, i;
    int rowStartPos;
    int xHalf, yHalf;

    xHalf = (ABS(mx) % 2 == 1);
    yHalf = (ABS(my) % 2 == 1);

    MOTION_TO_FRAME_COORD(by, bx, my / 2, mx / 2, fy, fx);

    if (xHalf) {
        if (mx < 0) {
            fx--;
        }
        if (yHalf) {
            if (my < 0) {
                fy--;
            }
            prev = intermediates[0]->firstByte;
        } else {
            prev = intermediates[1]->firstByte;
        }
    } else if (yHalf) {
        if (my < 0) {
            fy--;
        }
        prev = intermediates[2]->firstByte;
    } else {
        prev = prevFrame->firstByte;
    }

    rowStartPos = fy * width + fx;
    cacross = currentBlock;
    for (i = 0; i < 16; i++) {
        across = &(prev[rowStartPos]);

        localDiff = across[0] - cacross[0];
        diff += ABS_TABLE(localDiff);
        localDiff = across[1] - cacross[1];
        diff += ABS_TABLE(localDiff);
        localDiff = across[2] - cacross[2];
        diff += ABS_TABLE(localDiff);
        localDiff = across[3] - cacross[3];
        diff += ABS_TABLE(localDiff);
        localDiff = across[4] - cacross[4];
        diff += ABS_TABLE(localDiff);
        localDiff = across[5] - cacross[5];
        diff += ABS_TABLE(localDiff);
        localDiff = across[6] - cacross[6];
        diff += ABS_TABLE(localDiff);
        localDiff = across[7] - cacross[7];
        diff += ABS_TABLE(localDiff);
        localDiff = across[8] - cacross[8];
        diff += ABS_TABLE(localDiff);
        localDiff = across[9] - cacross[9];
        diff += ABS_TABLE(localDiff);
        localDiff = across[10] - cacross[10];
        diff += ABS_TABLE(localDiff);
        localDiff = across[11] - cacross[11];
        diff += ABS_TABLE(localDiff);
        localDiff = across[12] - cacross[12];
        diff += ABS_TABLE(localDiff);
        localDiff = across[13] - cacross[13];
        diff += ABS_TABLE(localDiff);
        localDiff = across[14] - cacross[14];
        diff += ABS_TABLE(localDiff);
        localDiff = across[15] - cacross[15];
        diff += ABS_TABLE(localDiff);

        cacross += 16;
        rowStartPos += width;
        if (diff > bestSoFar) {
            return diff;
        }
    }
    return diff;
}
