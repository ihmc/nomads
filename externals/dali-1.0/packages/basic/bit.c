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
 * bitbuf.c
 *
 * Wei Tsang May 97
 *
 * Defines functions that manipulate buffer of bit type.
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

/*
 * two global table defined in bitutil.c
 */
extern unsigned char thePreMask[8];
extern unsigned char thePostMask[8];

BitImage *
BitNew(w, h)
    int w;
    int h;
{
    BitImage *buf;

    buf = NEW(BitImage);
    buf->x = 0;
    buf->y = 0;
    buf->isVirtual = 0;
    buf->unitWidth = w;
    buf->firstBit = 0;
    buf->lastBit = w & 0x07;
    buf->byteWidth = w >> 3;
    buf->height = h;

    buf->parentWidth = (buf->lastBit != 0) ? buf->byteWidth + 1 : buf->byteWidth;
    buf->firstByte = NEWARRAY(unsigned char, (buf->parentWidth) * (buf->height));

    return buf;
}

void
BitFree(buf)
    BitImage *buf;
{
    if (buf->isVirtual == 0) {
        FREE(buf->firstByte);
    }
    FREE(buf);
}

BitImage *
BitClip(buf, x, y, w, h)
    BitImage *buf;
    int x;
    int y;
    int w;
    int h;
{
    BitImage *newBuf;
    register int offset, width;

    w = min(w, buf->unitWidth);
    h = min(h, buf->height);

    newBuf = NEW(BitImage);
    newBuf->x = x;
    newBuf->y = y;
    newBuf->unitWidth = w;
    newBuf->height = h;
    newBuf->isVirtual = 1;

    newBuf->firstBit = (x + buf->firstBit) & 0x07;
    width = newBuf->unitWidth - 8 + newBuf->firstBit;
    newBuf->byteWidth = width >> 3;
    if (newBuf->firstBit == 0)
        newBuf->byteWidth++;
    newBuf->lastBit = width & 0x07;

    newBuf->parentWidth = buf->parentWidth;
    offset = newBuf->y * newBuf->parentWidth + ((newBuf->x + buf->firstBit) >> 3);
    newBuf->firstByte = buf->firstByte + offset;

    return newBuf;
}

void
BitReclip(buf, x, y, w, h, clipped)
    BitImage *buf;
    int x;
    int y;
    int w;
    int h;
    BitImage *clipped;
{
    register int offset, width;

    w = min(w, buf->unitWidth);
    h = min(h, buf->height);

    clipped->x = x;
    clipped->y = y;
    clipped->unitWidth = w;
    clipped->height = h;
    clipped->isVirtual = 1;

    clipped->firstBit = (x + buf->firstBit) & 0x07;
    width = clipped->unitWidth - 8 + clipped->firstBit;
    clipped->byteWidth = width >> 3;
    if (clipped->firstBit == 0)
        clipped->byteWidth++;
    clipped->lastBit = width & 0x07;

    clipped->parentWidth = buf->parentWidth;
    offset = clipped->y * clipped->parentWidth + ((clipped->x + buf->firstBit) >> 3);
    clipped->firstByte = buf->firstByte + offset;
}


int
BitCopy8(srcBuf, destBuf)
    BitImage *srcBuf;
    BitImage *destBuf;
{
    int width;
    int height;
    register int srcSkip, destSkip, w;
    register unsigned char *currSrc, *currDest;

    if (!BitIsAligned(srcBuf) || !BitIsAligned(destBuf)) {
        return DVM_BIT_NOT_BYTE_ALIGN;
    }
    width = min(srcBuf->unitWidth, destBuf->unitWidth);
    height = min(srcBuf->height, destBuf->height);

    /*
     * Now copy the src buffer into the destination buffer, row by
     * row.
     */

    currSrc = srcBuf->firstByte;
    currDest = destBuf->firstByte;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    w = width / 8;

    DO_N_TIMES(height,
        memcpy(currDest, currSrc, w);
        currDest += destSkip;
        currSrc += srcSkip;
        );

    return DVM_BIT_OK;
}

int
BitCopy(srcBuf, destBuf)
    BitImage *srcBuf;
    BitImage *destBuf;
{
    int width;
    int height;
    register int srcSkip, destSkip, w, h;
    register int srcLast, destLast;
    register unsigned char *currSrc, *currDest;

    if (BitIsAligned(srcBuf) && BitIsAligned(destBuf)) {
        return DVM_BIT_IS_BYTE_ALIGN;
    }
    width = min(srcBuf->unitWidth, destBuf->unitWidth);
    height = min(srcBuf->height, destBuf->height);

    /*
     * Now copy the src buffer into the destination buffer, row by
     * row.
     */

    currSrc = srcBuf->firstByte;
    currDest = destBuf->firstByte;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    w = width - 8 + srcBuf->firstBit;
    srcLast = w & 0x07;
    w = width - 8 + destBuf->firstBit;
    destLast = w & 0x07;
    if (destBuf->firstBit == 0)
        w = (w >> 3) + 1;
    else
        w >>= 3;

    h = height;
    if (srcBuf->firstBit > destBuf->firstBit) {
        DO_N_TIMES(h,
            CopyRowSrcOnTheRight(currDest, destBuf->firstBit,
                destLast, currSrc, srcBuf->firstBit,
                srcLast, w);
            currDest += destSkip;
            currSrc += srcSkip;
            );
    } else if (srcBuf->firstBit < destBuf->firstBit) {
        DO_N_TIMES(h,
            CopyRowSrcOnTheLeft(currDest, destBuf->firstBit,
                destLast, currSrc, srcBuf->firstBit,
                srcLast, w);
            currDest += destSkip;
            currSrc += srcSkip;
            );
    } else {
        DO_N_TIMES(h,
            CopyRowEqual(currDest, destBuf->firstBit,
                destLast, currSrc, srcBuf->firstBit,
                srcLast, w);
            currDest += destSkip;
            currSrc += srcSkip;
            );
    }

    return DVM_BIT_OK;
}

int
BitSet8(buf, value)
    BitImage *buf;
    unsigned char value;
{
    register int w, h, srcSkip;
    register unsigned char *currSrc;

    if (!BitIsAligned(buf)) {
        return DVM_BIT_NOT_BYTE_ALIGN;
    }
    if (value != 0)
        value = 255;
    /*
     * Now set all the src buffer content to value
     */
    currSrc = buf->firstByte;
    w = buf->byteWidth;
    h = buf->height;
    srcSkip = buf->parentWidth;

    DO_N_TIMES(h,
        memset(currSrc, value, w);
        currSrc += srcSkip;
        );

    return DVM_BIT_OK;
}

int
BitSet(buf, value)
    BitImage *buf;
    unsigned char value;
{
    unsigned char preMask, postMask;
    register int w, h, srcSkip;
    register unsigned char *currSrc, *first;

    if (BitIsAligned(buf)) {
        return DVM_BIT_IS_BYTE_ALIGN;
    }
    /*
     * Now set all the src buffer content to value
     */
    currSrc = buf->firstByte;
    w = buf->byteWidth;
    h = buf->height;
    srcSkip = buf->parentWidth;

    if (value == 0) {
        if (buf->lastBit != 0) {
            DO_N_TIMES(h,
                first = currSrc;
                *currSrc = KEEP_FIRST(buf->firstBit, *currSrc);
                currSrc++;
                memset(currSrc, 0, w);
                currSrc += w;
                *currSrc = ZERO_FIRST(buf->lastBit, *currSrc);
                currSrc = first + srcSkip;
                );
        } else {
            DO_N_TIMES(h,
                first = currSrc;
                *currSrc = KEEP_FIRST(buf->firstBit, *currSrc);
                currSrc++;
                memset(currSrc, 0, w);
                currSrc = first + srcSkip;
                );
        }
    } else {
        preMask = thePreMask[buf->firstBit];
        postMask = thePostMask[buf->lastBit];
        if (buf->lastBit != 0) {
            DO_N_TIMES(h,
                first = currSrc;
                *currSrc |= preMask;
                currSrc++;
                memset(currSrc, 0xff, w);
                currSrc += w;
                *currSrc |= postMask;
                currSrc = first + srcSkip;
                );
        } else {
            DO_N_TIMES(h,
                first = currSrc;
                *currSrc |= preMask;
                currSrc++;
                memset(currSrc, 0xff, w);
                currSrc = first + srcSkip;
                );
        }
    }

    return DVM_BIT_OK;
}

int
BitGetSize(buf)
    BitImage *buf;
{
    int w;

    w = buf->byteWidth;
    if (buf->firstBit != 0)
        w++;
    if (buf->lastBit != 0)
        w++;

    return w * buf->height;
}
