/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * float.c
 *
 * Functions that create/free/manipulate FloatImages.
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */


#include "basicInt.h"

FloatImage *
FloatNew(width, height)
    int width, height;
{
    FloatImage *buf;

    buf = NEW(FloatImage);
    buf->x = 0;
    buf->y = 0;
    buf->width = width;
    buf->height = height;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstByte = NEWARRAY(float, (buf->width) * (buf->height));
    return buf;
}


void
FloatFree(FloatImage * buf)
{
    if (!buf->isVirtual) {
        FREE(buf->firstByte);
    }
    FREE(buf);
}


FloatImage *
FloatClip(buf, x, y, w, h)
    FloatImage *buf;
    int x;
    int y;
    int w;
    int h;
{
    FloatImage *newBuf;
    int pw;
    int offset;

    newBuf = NEW(FloatImage);
    newBuf->x = x;
    newBuf->y = y;
    newBuf->width = w;
    newBuf->height = h;
    newBuf->isVirtual = 1;
    pw = newBuf->parentWidth = buf->parentWidth;
    offset = y * pw + x;
    newBuf->firstByte = buf->firstByte + offset;

    return newBuf;
}

void
FloatReclip(buf, x, y, w, h, clipped)
    FloatImage *buf;
    int x;
    int y;
    int w;
    int h;
    FloatImage *clipped;
{
    int pw;
    int offset;

    clipped->x = x;
    clipped->y = y;
    clipped->width = w;
    clipped->height = h;
    clipped->isVirtual = 1;
    pw = clipped->parentWidth = buf->parentWidth;
    offset = y * pw + x;
    clipped->firstByte = buf->firstByte + offset;
}


void
FloatCopy(src, dest)
    FloatImage *src;
    FloatImage *dest;
{
    register int w, h;
    float *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstByte;
    currDest = dest->firstByte;

    DO_N_TIMES (h,
        memcpy(currDest, currSrc, sizeof(float)*w);
        currSrc += src->parentWidth;
        currDest += dest->parentWidth;
    );
}
