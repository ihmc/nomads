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
 * threshold.c
 *
 * Steve Weiss January 98
 *
 * Threshold algorithms
 *
 *----------------------------------------------------------------------
 */


#include "visionInt.h"

static void FindMeanAndVar(unsigned char* data, int n1, int n2, 
                           int *ave, int *var);

int
ByteComputeThreshold (srcBuf, percent)
    ByteImage *srcBuf;
    int percent;
{
    int w, h;
    unsigned char *currSrc;
    int srcParW;
    unsigned char *firstSrc;
    int goal, numPixelsBelow;
    int *currPixel;
    int pixelBuf[256];


    if (percent < 0 || percent > 100) 
        return DVM_BAD_PERCENT;

    w = srcBuf->width;
    h = srcBuf->height;
    srcParW = srcBuf->parentWidth;
    firstSrc = srcBuf->firstByte;

    memset(pixelBuf, 0, 256*sizeof(int));

    /*
     * Count the number of pixels with each distinct value
     */
    
    DO_N_TIMES(h,
        currSrc = firstSrc;
        DO_N_TIMES(w,
            pixelBuf[*currSrc]++;
            currSrc++;
        );
        firstSrc += srcParW;
    );

    /*
     * Find the threshold
     */

    goal = (int)((float)percent/100.0 * (float)(w*h) + 0.5);
    numPixelsBelow=0;

    currPixel = pixelBuf;
    while (numPixelsBelow < goal) {
        numPixelsBelow += *currPixel;
        currPixel++;
    }

    return currPixel - pixelBuf;
}


int
BitMakeFromThreshold8 (srcBuf, destBuf, threshold, lowVal)
    ByteImage *srcBuf;
    BitImage *destBuf;
    int threshold;
    int lowVal;
{
    int w, h, bw;
    unsigned char *currSrc, *currDest;
    int srcParW, destParW;
    unsigned char *firstSrc, *firstDest;
    unsigned char destVal, mask;
    unsigned char maskBuf[256];
    register int i;

    if (threshold < 0 || threshold > 256)
        return DVM_BAD_THRESHOLD;

    if (destBuf->firstBit || destBuf->lastBit || (srcBuf->width%8))
        return DVM_NOT_BYTE_ALLIGNED;

    if (lowVal != 0 && lowVal != 1)
        return DVM_BAD_LOW_VAL;

    w = srcBuf->width;
    h = srcBuf->height;
    bw = destBuf->byteWidth;
    srcParW = srcBuf->parentWidth;
    destParW = destBuf->parentWidth;
    firstSrc = srcBuf->firstByte;
    firstDest = destBuf->firstByte;

    /*
     * Create a mask buffer such that the first threshold bytes are 0 and the rest are 1
     */
    memset(maskBuf, lowVal, threshold);
    memset(maskBuf+threshold, 1-lowVal, (256-threshold));

    for (i=0; i < h; i++) {
        currSrc = firstSrc;
        currDest = firstDest;

        DO_N_TIMES(bw,
            destVal = 0;
            
            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;
            destVal <<= 1;

            mask = maskBuf[*currSrc];
            currSrc++;
            destVal |= mask;

            *currDest = destVal;
            currDest++;
        );
        firstSrc += srcParW;
        firstDest += destParW;
    }

    return DVM_VISION_OK;

}

#define ALLWHITE 100

int
BitAdaptiveThreshold8 (srcBuf, destBuf, blockw, blockh, 
                       maxVariance, allWhite, lowVal)
    ByteImage *srcBuf;
    BitImage *destBuf;
    int blockw;
    int blockh;
    int maxVariance;
    int allWhite;
    int lowVal;
{
    int w, h, bw, x, y, val;
    unsigned char *currSrc, *currDest;
    int srcParW, destParW;
    unsigned char *firstSrc, *firstDest;
    unsigned char destVal, mask;
    unsigned char maskBuf[256];
    unsigned char pelVals[256];
    int blockx, blocky, t1, t2;
    int mean1, mean2, var1, var2, mean, var;
    int thresh, q;
    int block_widths[100], block_heights[100];
    register int i;
    int iMin, iMax;
    int prevThresh;
#ifdef DO_CHECK
    unsigned char* src1, *src2, *dest1, *dest2;
#endif

    if (destBuf->firstBit || destBuf->lastBit || (srcBuf->width%8))
        return DVM_NOT_BYTE_ALLIGNED;

    if (lowVal != 0 && lowVal != 1)
        return DVM_BAD_LOW_VAL;

    if (srcBuf->width != destBuf->unitWidth) 
        return DVM_DIFFERENT_SIZES;

    w = srcBuf->width;
    h = srcBuf->height;
    bw = destBuf->byteWidth;
    srcParW = srcBuf->parentWidth;
    destParW = destBuf->parentWidth;
    firstSrc = srcBuf->firstByte;
    firstDest = destBuf->firstByte;

#ifdef DO_CHECK
    src1 = firstSrc;
    src2 = firstSrc + srcParW*h;
    dest1 = firstDest;
    dest2 = firstDest + destParW*h;
#endif

    blockx = w / blockw;
    blocky = h / blockh;

    assert(blockx <= 100);
    assert(blocky <= 100);

    for (i = 0; i < blockx; i++)
        block_widths[i] = blockw;

    block_widths[i] = w - blockw*blockx;
    if (block_widths[i] > 5)
        blockx++;

    for (i = 0; i < blocky; i++)
        block_heights[i] = blockh;

    block_heights[i] = h - blockh*blocky;
    if (block_heights[i] > 5)
        blocky++;

    for (t1 = 0; t1 < blocky; t1++) {
        firstSrc = srcBuf->firstByte + srcParW*t1*blockh;
        firstDest = destBuf->firstByte + destParW*t1*blockh;

        blockh = block_heights[t1];
        for (t2 = 0; t2 < blockx; t2++) {
            memset(pelVals, 0, 256);
            blockw = block_widths[t2];
            currSrc = firstSrc;
            for (y=0; y < blockh; y++) {
                for (x=0; x < blockw; x++) {
                    pelVals[*currSrc]++;
                    currSrc++;
                }
                currSrc += srcParW - blockw;
            }

            FindMeanAndVar(pelVals, 0, 256, &mean, &var);
            if (var < maxVariance) { /* One mean */
                if (mean < allWhite) { /* All black */
                    val = lowVal*255;
                } else {
                    val = (1 - lowVal)*255;
                }
                currDest = firstDest;
                for (y=0; y < blockh; y++) {
                    memset(currDest, val, blockw>>3);
                    currDest += destParW;
                }
            } else {

                for (iMin=0; pelVals[iMin]==0; iMin++);
                for(iMax=255; pelVals[iMax]==0; iMax--);

                prevThresh = -1;
     
                thresh = (iMin+iMax)/2;

                while (thresh != prevThresh) {
                    FindMeanAndVar(pelVals, iMin, thresh, &mean1, &var1);
                    FindMeanAndVar(pelVals, thresh, iMax, &mean2, &var2);

                    prevThresh = thresh;
                    thresh = (mean1 + mean2) / 2;
                }
                
                assert(thresh < 256);
                memset(maskBuf, lowVal, thresh);
                memset(maskBuf+thresh, 1-lowVal, (256-thresh));

                for (i=0; i < blockh; i++) {
                    currSrc = firstSrc + i*srcParW;
                    currDest = firstDest + i*destParW;

                    for (q=0; q < (blockw>>3); q++) {
                        destVal = 0;

                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;

                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;

                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;
                        
                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;
                        
                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;
                        
                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;
                        
                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        destVal <<= 1;
                        
                        mask = maskBuf[*currSrc];
                        currSrc++;
                        destVal |= mask;
                        
                        *currDest = destVal;
                        currDest++;
                    }
                }
            }
            firstSrc += blockw;
            firstDest += (blockw>>3);
        }
    }
    return DVM_VISION_OK;
}


void FindMeanAndVar(unsigned char* data, int n1, int n2, int *ave, int *var)
{
    int j, numPixels;
    int ep=0, s;

    s = 0;
    numPixels = 0;

    for (j=n1; j < n2; j++) {
        s += data[j]*j;
        numPixels += data[j];
    }

    if (numPixels < 2) {
        *var = 0;
        *ave = s;
        return;
    }
    *ave = s/numPixels;
    *var = 0;

    for (j=n1; j < n2; j++) {
        s = j-(*ave);
        ep += s*data[j];
        *var += s*s*data[j];
    }

    *var = (*var-ep*ep/numPixels)/(numPixels-1);

}




int
ByteMakeFromThreshold8 (srcBuf, destBuf, threshold, lowVal)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int threshold;
    int lowVal;
{
    int w, h;
    unsigned char *currSrc, *currDest;
    int srcParW, destParW;
    unsigned char *firstSrc, *firstDest;
    unsigned char maskBuf[256];
    register int i;

    if (threshold < 0 || threshold > 256)
        return DVM_BAD_THRESHOLD;

    if (lowVal != 0 && lowVal != 1)
        return DVM_BAD_LOW_VAL;

    w = srcBuf->width;
    h = srcBuf->height;
    srcParW = srcBuf->parentWidth;
    destParW = destBuf->parentWidth;
    firstSrc = srcBuf->firstByte;
    firstDest = destBuf->firstByte;

    /*
     * Create a mask buffer such that the first threshold bytes are 0 and the rest are 1
     */
    memset(maskBuf, lowVal, threshold);
    memset(maskBuf+threshold, 1-lowVal, (256-threshold));

    for (i=0; i < h; i++) {
        currSrc = firstSrc;
        currDest = firstDest;

        DO_N_TIMES(w,
            *currDest++ = maskBuf[*currSrc++];
        );
        firstSrc += srcParW;
        firstDest += destParW;
    }

    return DVM_VISION_OK;

}
