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
 *----------------------------------------------------------------------
 *
 * smooth.c
 *
 * Steve Weiss October 97
 *
 * Smoothing an image
 *
 *----------------------------------------------------------------------
 */

#include "visionInt.h"

extern unsigned char abs_val[];
extern unsigned char numBits[];

int
BitCompareBlocks (srcBuf1, srcBuf2, size)
    BitImage *srcBuf1;
    BitImage *srcBuf2;
    int size;
{
    int h, bw;
    unsigned char *currSrc1, *currSrc2;
    int src1ParW, src2ParW;
    unsigned char *firstSrc1, *firstSrc2, *secSrc1, *secSrc2;
    int srcBlock1, srcBlock2, totalBlockDiffs, totalSrcBlock1s, totalSrcBlock2s;
    int i, j;
    int avg;
    unsigned char* abs_v = abs_val+255;

    avg = 0;
    if (size != 8 && size != 16) 
        return DVM_BAD_BLOCK_SIZE;

    h = srcBuf1->height;
    bw = srcBuf1->byteWidth;
    src1ParW = srcBuf1->parentWidth;
    src2ParW = srcBuf2->parentWidth;
    firstSrc1 = srcBuf1->firstByte;
    firstSrc2 = srcBuf2->firstByte;

    totalBlockDiffs = totalSrcBlock1s = totalSrcBlock2s = 0;

    /*
     * For a block size of 8, sum the bits in 8 consecutive bytes in a column
     * and take their absolute difference
     */
    if (size == 8) {
        for (i=0; i < h/8; i++) {
            secSrc1 = firstSrc1;
            secSrc2 = firstSrc2;

            DO_N_TIMES(bw,
                currSrc1 = secSrc1;
                currSrc2 = secSrc2;

                srcBlock1 = srcBlock2 = 0;
                DO_N_TIMES(8, 
                    srcBlock1 += numBits[*currSrc1];
                    srcBlock2 += numBits[*currSrc2];
                    currSrc1 += src1ParW;
                    currSrc2 += src2ParW;
                );
                totalBlockDiffs += abs(srcBlock1 - srcBlock2);
                totalSrcBlock1s += srcBlock1;
                totalSrcBlock2s += srcBlock2;

                secSrc1++;
                secSrc2++;
            );

            firstSrc1 += 8*src1ParW;
            firstSrc2 += 8*src2ParW;
        }
        avg = (int)((float)totalBlockDiffs/(float)(bw*h/8));
    }

    /* 
     * For a block size of 16, take the sums of 16 consecutive bytes in two adjacent columns 
     */
    else if (size == 16) {
        for (i=0; i < h/16; i++) {
            secSrc1 = firstSrc1;
            secSrc2 = firstSrc2;

            for (j=0; j < bw/2; j++) {
                currSrc1 = secSrc1;
                currSrc2 = secSrc2;

                srcBlock1 = srcBlock2 = 0;
                DO_N_TIMES(16, 
                    srcBlock1 += numBits[*currSrc1] + numBits[*(currSrc1+1)];
                    srcBlock2 += numBits[*currSrc2] + numBits[*(currSrc2+1)];
                    currSrc1 += src1ParW;
                    currSrc2 += src2ParW;
                );
                totalBlockDiffs += abs_v[srcBlock1 - srcBlock2];
                totalSrcBlock1s += srcBlock1;
                totalSrcBlock2s += srcBlock2;

                secSrc1+=2;
                secSrc2+=2;
            }

            firstSrc1 += 16*src1ParW;
            firstSrc2 += 16*src2ParW;
        }
        avg = (int)((float)totalBlockDiffs/(float)(bw*h/32));
    }

    return avg;
}
