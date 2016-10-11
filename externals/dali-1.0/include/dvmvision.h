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
 * dvmvision.h
 *
 * This file contains prototypes for functions to perform
 * vision operation on byte images
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_VISION_
#define _DVM_VISION_

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

#define GET_NEXT_BYTE(byte, firstBit) (*byte)<<firstBit | *(byte+1)>>(8-firstBit);byte++

#define BIT7(x) (((x)&0x80)>>7)
#define BIT6(x) (((x)&0x40)>>6)
#define BIT5(x) (((x)&0x20)>>5)
#define BIT4(x) (((x)&0x10)>>4)
#define BIT3(x) (((x)&0x08)>>3)
#define BIT2(x) (((x)&0x04)>>2)
#define BIT1(x) (((x)&0x02)>>1)
#define BIT0(x) ((x)&0x01)

#define DVM_VISION_OK 0
#define DVM_NOT_BYTE_ALLIGNED -1
#define DVM_DIFFERENT_SIZES -2
#define DVM_BAD_HEIGHT -3
#define DVM_ALLOC_ERROR -4
#define DVM_USE_ALLIGN -5
#define DVM_BAD_BLOCK_SIZE -6
#define DVM_BAD_THRESHOLD -7
#define DVM_BAD_NUM_PASSES -8
#define DVM_BAD_PERCENT -9
#define DVM_SCRATCH_SIZE_TOO_SMALL -10
#define DVM_BAD_LOW_VAL -11

#define SCRATCHSIZE 10000

    extern int BitDilate8(BitImage * srcBuf, BitImage * destBuf);
    extern int ByteSmooth(ByteImage * srcBuf, ByteImage * destBuf, int numPasses);
    extern int ByteEdgeDetectSobel(ByteImage *, ByteImage *, ByteImage *, int, int *, int *);
    extern int ByteEdgeDetectCanny(ByteImage *, ByteImage *, int, int);
    extern int BitMakeFromThreshold8(ByteImage * srcBuf, BitImage * destBuf, int threshold, int lowVal);
    extern int ByteComputeThreshold(ByteImage * srcBuf, int percent);
    extern int BitCompareBlocks(BitImage * srcBuf1, BitImage * srcBuf2, int size);
    extern int BitCountOverlap(BitImage * buf1, BitImage * buf2, int *percent);
    extern int BitCountOverlap8(BitImage * buf1, BitImage * buf2, int *percent);

    extern int ByteSmoothGaussian(ByteImage *, ByteImage *, float);
    extern int BitAdaptiveThreshold8(ByteImage *, BitImage *, int, int, int, int, int);
    extern int ByteMakeFromThreshold8(ByteImage *, ByteImage *, int, int);
    extern int BitErode8(BitImage *, BitImage *);

/* Slide */
    extern void ByteMakeFromBitIntersect(ByteImage * rBuf, ByteImage * gBuf, ByteImage * bBuf, BitImage * srcBuf1, BitImage * srcBuf2);
    extern float BitCompare(BitImage * buf1, BitImage * buf2);
    extern int BitAllWhite(BitImage * buf);
    extern void BitFindCentroid(BitImage * buf, int val, int *xmean, int *ymean);
    extern void ByteFindBoundingBox(ByteImage * buf, int px, int py, int *x0, int *y0, int *x1, int *y1, int *x2, int *y2, int *x3, int *y3);
    extern void ByteFindOuterCorners(ByteImage * buf, int ix0, int iy0, int ix1, int iy1, int ix2, int iy2, int ix3, int iy3,
        int *x0, int *y0, int *x1, int *y1, int *x2, int *y2, int *x3, int *y3);
    extern int ByteFindBackgroundIntensity(ByteImage * buf);
    extern int ByteMakeFromBit8(BitImage * srcBuf, ByteImage * destBuf);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
