/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * byte.c
 *
 * Functions that manipulate ByteImages.
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

ByteImage *
ByteNew(w, h)
    int w;
    int h;
{
    ByteImage *buf;

    buf = NEW(ByteImage);
    buf->x = 0;
    buf->y = 0;
    buf->width = w;
    buf->height = h;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstByte = NEWARRAY(unsigned char, (buf->width) * (buf->height));

    return buf;
}

void
ByteFree(buf)
    ByteImage *buf;
{
    if (!buf->isVirtual) {
        FREE(buf->firstByte);
    }
    FREE(buf);
}

ByteImage *
ByteClip(buf, x, y, width, height)
    ByteImage *buf;
    int x;
    int y;
    int width;
    int height;
{
    ByteImage *newBuf;

    width = min(width, buf->width);
    height = min(height, buf->height);

    newBuf = NEW(ByteImage);
    newBuf->x = x;
    newBuf->y = y;
    newBuf->width = width;
    newBuf->height = height;
    newBuf->isVirtual = 1;
    newBuf->parentWidth = buf->parentWidth;
    newBuf->firstByte = buf->firstByte + y * (newBuf->parentWidth) + x;

    return newBuf;
}


void
ByteReclip(buf, x, y, width, height, clipped)
    ByteImage *buf;
    int x;
    int y;
    int width;
    int height;
    ByteImage *clipped;
{
    width = min(width, buf->width);
    height = min(height, buf->height);

    clipped->x = x;
    clipped->y = y;
    clipped->width = width;
    clipped->height = height;
    clipped->isVirtual = 1;
    clipped->parentWidth = buf->parentWidth;
    clipped->firstByte = buf->firstByte + y * (clipped->parentWidth) + x;

    return;
}


void
ByteCopy(srcBuf, destBuf)
    ByteImage *srcBuf;
    ByteImage *destBuf;
{
    register unsigned char *currSrc, *currDest;
    int srcSkip, destSkip, w, h;

    w = min(srcBuf->width, destBuf->width);
    h = min(srcBuf->height, destBuf->height);
    currSrc = srcBuf->firstByte;
    currDest = destBuf->firstByte;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    DO_N_TIMES(h,
        memcpy(currDest, currSrc, w);
        currDest += destSkip;
        currSrc += srcSkip;
        );
}


void
ByteCopyMux1(srcBuf, srcOffset, srcStride, destBuf, destOffset, destStride)
    ByteImage *srcBuf;
    int srcOffset;
    int srcStride;
    ByteImage *destBuf;
    int destOffset;
    int destStride;
{
    register unsigned char *currSrc, *currDest;
    int w, h;
    int i, j;
    unsigned char *srcFirst, *destFirst;
    int srcSkip, destSkip;

    /* Calculate number of bytes to copy per row */
    w = min(srcBuf->width/srcStride, destBuf->width/destStride);
    h = min(srcBuf->height, destBuf->height);
    srcFirst = srcBuf->firstByte + srcOffset;
    destFirst = destBuf->firstByte + destOffset;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    for ( i = 0; i < h; i++ ) {
        currSrc = srcFirst;
        currDest = destFirst;
        for ( j = 0; j < w; j++ ) {
            *currDest = *currSrc;
             currSrc += srcStride;
             currDest += destStride;
        }
        srcFirst += srcSkip;
        destFirst += destSkip;
    }
}


void
ByteCopyMux2(srcBuf, srcOffset, srcStride, destBuf, destOffset, destStride)
    ByteImage *srcBuf;
    int srcOffset;
    int srcStride;
    ByteImage *destBuf;
    int destOffset;
    int destStride;
{
    register unsigned char *currSrc, *currDest;
    int w, h;
    int i, j;
    unsigned char *srcFirst, *destFirst;
    int srcSkip, destSkip;

    /* Calculate number of segments to copy per row */
    w = min(srcBuf->width/(srcStride + 1), destBuf->width/(destStride + 1));
    h = min(srcBuf->height, destBuf->height);
    srcFirst = srcBuf->firstByte + srcOffset;
    destFirst = destBuf->firstByte + destOffset;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    for ( i = 0; i < h; i++ ) {
        currSrc = srcFirst;
        currDest = destFirst;
        for ( j = 0; j < w; j++ ) {
             *currDest++ = *currSrc++;
             *currDest = *currSrc;
             currSrc += srcStride;
             currDest += destStride;
        }
        srcFirst += srcSkip;
        destFirst += destSkip;
    }
}


void
ByteCopyMux4(srcBuf, srcOffset, srcStride, destBuf, destOffset, destStride)
    ByteImage *srcBuf;
    int srcOffset;
    int srcStride;
    ByteImage *destBuf;
    int destOffset;
    int destStride;
{
    register unsigned char *currSrc, *currDest;
    int w, h;
    int i, j;
    unsigned char *srcFirst, *destFirst;
    int srcSkip, destSkip;

    w = min(srcBuf->width/(srcStride + 3), destBuf->width/(destStride + 3));
    h = min(srcBuf->height, destBuf->height);
    srcFirst = srcBuf->firstByte + srcOffset;
    destFirst = destBuf->firstByte + destOffset;
    srcSkip = srcBuf->parentWidth;
    destSkip = destBuf->parentWidth;
    for ( i = 0; i < h; i++) {
        currSrc = srcFirst;
        currDest = destFirst;
        for ( j = 0; j < w; j++ ) {
             *currDest++ = *currSrc++;
             *currDest++ = *currSrc++;
             *currDest++ = *currSrc++;
             *currDest = *currSrc;
             currSrc += srcStride;
             currDest += destStride;
        }
        srcFirst += srcSkip;
        destFirst += destSkip;
    }
}


void
ByteCopyWithMask(srcBuf, bitmask, destBuf)
    ByteImage *srcBuf;
    BitImage *bitmask;
    ByteImage *destBuf;
{
    unsigned char *currSrc, *currDest;
    int w, h;
    int startx, len, y;
    BitImageScan *scan;

    w = min(srcBuf->width, destBuf->width);
    h = min(srcBuf->height, destBuf->height);
    scan = BitImageOpenScan(bitmask, w, h);
    while (BitImageGetNextExtent(scan, &startx, &len, &y) != SCAN_DONE) {
        currSrc = srcBuf->firstByte + srcBuf->parentWidth * y + startx;
        currDest = destBuf->firstByte + destBuf->parentWidth * y + startx;
        memcpy(currDest, currSrc, len);
    }
    BitImageCloseScan(scan);
}

void
ByteSet(buf, value)
    ByteImage *buf;
    unsigned char value;
{
    register unsigned char *src;
    register int w, h, skip;

    w = buf->width;
    h = buf->height;
    src = buf->firstByte;
    skip = buf->parentWidth;
    if (buf->isVirtual) {
        DO_N_TIMES(h,
            memset(src, value, w);
            src += skip;
            )
    } else {
        memset(src, value, w * h);
    }
}


void
ByteSetMux1(buf, offset, stride, value)
    ByteImage *buf;
    int offset;
    int stride;
    unsigned char value;
{
    register unsigned char *curr, *first;
    register int i, w, h, skip;

    w = buf->width/stride;
    h = buf->height;
    first = buf->firstByte + offset;
    skip = buf->parentWidth;
    for (i = 0; i < h; i++) {
        curr = first;
        DO_N_TIMES(w,
            *curr = value;
            curr += stride;
            );
        first += skip;
    }
}


void
ByteSetMux2(buf, offset, stride, value)
    ByteImage *buf;
    int offset;
    int stride;
    unsigned char value;
{
    register unsigned char *curr, *first;
    register int i, w, h, skip;

    w = buf->width/(stride + 1);
    h = buf->height;
    first = buf->firstByte + offset;
    skip = buf->parentWidth;
    for (i = 0; i < h; i++) {
        curr = first;
        DO_N_TIMES(w,
            *curr++ = value;
            *curr = value;
            curr += stride;
            );
        first += skip;
    }
}


void
ByteSetMux4(buf, offset, stride, value)
    ByteImage *buf;
    int offset;
    int stride;
    unsigned char value;
{
    register unsigned char *curr, *first;
    register int i, w, h, skip;

    w = buf->width/(stride + 3);
    h = buf->height;
    first = buf->firstByte + offset;
    skip = buf->parentWidth;
    for (i = 0; i < h; i++) {
        curr = first;
        DO_N_TIMES(w,
            *curr++ = value;
            *curr++ = value;
            *curr++ = value;
            *curr = value;
            curr += stride;
            );
        first += skip;
    }
}


void
ByteSetWithMask(buf, mask, value)
    ByteImage *buf;
    BitImage *mask;
    unsigned char value;
{
    BitImageScan *scan;
    register unsigned char *src;
    int startx, len, y;
    register int w, h;

    w = buf->width;
    h = buf->height;

    scan = BitImageOpenScan(mask, w, h);
    while (BitImageGetNextExtent(scan, &startx, &len, &y) != SCAN_DONE) {
        src = buf->firstByte + buf->parentWidth * y + startx;
        memset(src, value, len);
    }
    BitImageCloseScan(scan);
}

void
ByteExtend(buf, bw, bh)
    ByteImage *buf;
    int bw, bh;
{
    int w, h, bw1, bh1, bw2, bh2, rowSkip;
    unsigned char *curRow, *firstRow, *lastRow, *topRow, *bottomRow, *curPtr;

    w = buf->width;
    h = buf->height;

    bw = min(w, bw);
    bh = min(h, bh);

    bw1 = bw / 2;
    bh1 = bh / 2;
    bw2 = bw - bw1;
    bh2 = bh - bh1;

    /*
     * Initialize special row pointers
     */

    rowSkip = buf->parentWidth;
    topRow = buf->firstByte;
    bottomRow = topRow + (h - 1) * rowSkip;
    firstRow = topRow + bh1 * rowSkip;
    lastRow = topRow + (h - bh2 - 1) * rowSkip;

    /*
     * Copy pixels out towards the sides.
     */

    for (curRow = firstRow; curRow <= lastRow; curRow += rowSkip) {
        if (bw1 > 0) {
            curPtr = curRow;
            DO_N_TIMES(bw1,
                *(curPtr++) = *(curRow + bw1);
                );
        }
        if (bw2 > 0) {
            curPtr = curRow + w - bw2;
            DO_N_TIMES(bw2,
                *(curPtr++) = *(curRow + w - bw2 - 1);
                );
        }
    }

    /*
     * Copy pixels up and down
     */

    if (bh1 > 0) {
        curRow = topRow;
        DO_N_TIMES(bh1,
            memcpy(curRow, firstRow, w);
            curRow += rowSkip;
            );
    }
    if (bh2 > 0) {
        curRow = lastRow + rowSkip;
        DO_N_TIMES(bh2,
            memcpy(curRow, lastRow, w);
            curRow += rowSkip;
            );
    }
}
