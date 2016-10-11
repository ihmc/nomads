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
 * bitsetop.c
 *
 * Wei Tsang May 97
 *
 * Defines functions that perform set operations on regions of bitmap
 * (bit rectagular buffer) type.
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

int
BitUnion8(srcbuf1, srcbuf2, destBuf)
    BitImage *srcbuf1;
    BitImage *srcbuf2;
    BitImage *destBuf;
{
    unsigned char *dest, *src1, *src2;
    register int i, w, h;
    register int destSkip, src1Skip, src2Skip;

    if (!BitIsAligned(srcbuf1) ||
        !BitIsAligned(srcbuf2) ||
        !BitIsAligned(destBuf)) {
        return DVM_BIT_NOT_BYTE_ALIGN;
    }
    w = min(srcbuf1->unitWidth, srcbuf2->unitWidth);
    w = min(w, destBuf->unitWidth);
    h = min(srcbuf1->height, srcbuf2->height);
    h = min(h, destBuf->height);

    w >>= 3;
    dest = destBuf->firstByte;
    src1 = srcbuf1->firstByte;
    src2 = srcbuf2->firstByte;
    destSkip = destBuf->parentWidth - w;
    src1Skip = srcbuf1->parentWidth - w;
    src2Skip = srcbuf2->parentWidth - w;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *dest++ = *src1++ | *src2++;
            );
        dest += destSkip;
        src1 += src1Skip;
        src2 += src2Skip;
    }

    return DVM_BIT_OK;
}


int
BitUnion(srcbuf1, srcbuf2, destBuf)
    BitImage *srcbuf1;
    BitImage *srcbuf2;
    BitImage *destBuf;
{
    unsigned char *dest, *src1, *src2;
    register int i, w, h;
    int first, last;
    register int destSkip, src1Skip, src2Skip;

    if (BitIsAligned(srcbuf1) &&
        BitIsAligned(srcbuf2) &&
        BitIsAligned(destBuf)) {
        return DVM_BIT_IS_BYTE_ALIGN;
    }
    w = min(srcbuf1->unitWidth, srcbuf2->unitWidth);
    w = min(w, destBuf->unitWidth);
    w = (w - 8 + destBuf->firstBit) >> 3;
    h = min(srcbuf1->height, srcbuf2->height);
    h = min(h, destBuf->height);

    dest = destBuf->firstByte;
    src1 = srcbuf1->firstByte;
    src2 = srcbuf2->firstByte;
    destSkip = destBuf->parentWidth;
    src1Skip = srcbuf1->parentWidth;
    src2Skip = srcbuf2->parentWidth;

    first = 8 - destBuf->firstBit;
    last = destBuf->lastBit;

    for (i = 0; i < h; i++) {

        register unsigned char a, b;
        unsigned char *firstDest, *firstSrc1, *firstSrc2;
        int bit1, bit2;

        firstDest = dest;
        firstSrc1 = src1;
        firstSrc2 = src2;
        bit1 = srcbuf1->firstBit;
        bit2 = srcbuf2->firstBit;
        a = GetNextNBitsPutEnd(first, &src1, &bit1);
        b = GetNextNBitsPutEnd(first, &src2, &bit2);
        *dest = ZERO_LAST(first, *dest);
        *dest++ |= (a | b);
        DO_N_TIMES(w,
            a = GetNextByte(&src1, bit1);
            b = GetNextByte(&src2, bit2);
            *dest++ = a | b;
            );
        if (last != 0) {
            a = GetNextNBitsPutFront(last, &src1, &bit1);
            b = GetNextNBitsPutFront(last, &src2, &bit2);
            *dest = ZERO_FIRST(last, *dest);
            *dest++ |= (a | b);
        }
        dest = firstDest + destSkip;
        src1 = firstSrc1 + src1Skip;
        src2 = firstSrc2 + src2Skip;
    }

    return DVM_BIT_OK;
}


int
BitIntersect8(srcbuf1, srcbuf2, destBuf)
    BitImage *srcbuf1;
    BitImage *srcbuf2;
    BitImage *destBuf;
{
    unsigned char *dest, *src1, *src2;
    register int i, w, h;
    register int destSkip, src1Skip, src2Skip;

    if (!BitIsAligned(srcbuf1) ||
        !BitIsAligned(srcbuf2) ||
        !BitIsAligned(destBuf)) {
        return DVM_BIT_NOT_BYTE_ALIGN;
    }
    w = min(srcbuf1->unitWidth, srcbuf2->unitWidth);
    w = min(w, destBuf->unitWidth);
    h = min(srcbuf1->height, srcbuf2->height);
    h = min(h, destBuf->height);
    w >>= 3;
    dest = destBuf->firstByte;
    src1 = srcbuf1->firstByte;
    src2 = srcbuf2->firstByte;
    destSkip = destBuf->parentWidth - w;
    src1Skip = srcbuf1->parentWidth - w;
    src2Skip = srcbuf2->parentWidth - w;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *dest++ = *src1++ & *src2++;
            );
        dest += destSkip;
        src1 += src1Skip;
        src2 += src2Skip;
    }

    return DVM_BIT_OK;
}

int
BitIntersect(srcbuf1, srcbuf2, destBuf)
    BitImage *srcbuf1;
    BitImage *srcbuf2;
    BitImage *destBuf;
{
    unsigned char *dest, *src1, *src2;
    register int i, w, h;
    register int destSkip, src1Skip, src2Skip;

    if (BitIsAligned(srcbuf1) &&
        BitIsAligned(srcbuf2) &&
        BitIsAligned(destBuf)) {
        return DVM_BIT_IS_BYTE_ALIGN;
    }
    w = min(srcbuf1->unitWidth, srcbuf2->unitWidth);
    w = min(w, destBuf->unitWidth);
    h = min(srcbuf1->height, srcbuf2->height);
    h = min(h, destBuf->height);

    w = (w - 8 + destBuf->firstBit) >> 3;
    dest = destBuf->firstByte;
    src1 = srcbuf1->firstByte;
    src2 = srcbuf2->firstByte;
    destSkip = destBuf->parentWidth;
    src1Skip = srcbuf1->parentWidth;
    src2Skip = srcbuf2->parentWidth;

    for (i = 0; i < h; i++) {

        register unsigned char a, b;
        unsigned char *firstDest, *firstSrc1, *firstSrc2;
        int bit1, bit2;

        firstDest = dest;
        firstSrc1 = src1;
        firstSrc2 = src2;
        bit1 = srcbuf1->firstBit;
        bit2 = srcbuf2->firstBit;
        a = GetNextNBitsPutEnd(8 - destBuf->firstBit, &src1, &bit1);
        b = GetNextNBitsPutEnd(8 - destBuf->firstBit, &src2, &bit2);
        *dest = ZERO_LAST(8 - destBuf->firstBit, *dest);
        *dest++ |= (a & b);
        DO_N_TIMES(w,
            a = GetNextByte(&src1, bit1);
            b = GetNextByte(&src2, bit2);
            *dest++ = a & b;
            );
        if (destBuf->lastBit != 0) {
            a = GetNextNBitsPutFront(destBuf->lastBit, &src1, &bit1);
            b = GetNextNBitsPutFront(destBuf->lastBit, &src2, &bit2);
            *dest = ZERO_FIRST(destBuf->lastBit, *dest);
            *dest++ |= (a & b);
        }
        dest = firstDest + destSkip;
        src1 = firstSrc1 + src1Skip;
        src2 = firstSrc2 + src2Skip;
    }

    return DVM_BIT_OK;
}
