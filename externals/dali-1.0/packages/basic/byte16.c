
/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * byte16.c
 *
 * Functions that create/free/manipulate Byte16Images.
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */


#include "basicInt.h"

Byte16Image *
Byte16New(width, height)
    int width, height;
{
    Byte16Image *buf;

    buf = NEW(Byte16Image);
    buf->x = 0;
    buf->y = 0;
    buf->width = width;
    buf->height = height;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstByte = NEWARRAY(unsigned short, (buf->width) * (buf->height));
    return buf;
}


void
Byte16Free(Byte16Image * buf)
{
    if (!buf->isVirtual) {
        FREE(buf->firstByte);
    }
    FREE(buf);
}


Byte16Image *
Byte16Clip(buf, x, y, w, h)
    Byte16Image *buf;
    int x;
    int y;
    int w;
    int h;
{
    Byte16Image *newBuf;
    int pw;
    int offset;

    newBuf = NEW(Byte16Image);
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
Byte16Reclip(buf, x, y, w, h, clipped)
    Byte16Image *buf;
    int x;
    int y;
    int w;
    int h;
    Byte16Image *clipped;
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
Byte16Copy(src, dest)
    Byte16Image *src;
    Byte16Image *dest;
{
    register int w, h;
    unsigned short *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstByte;
    currDest = dest->firstByte;

    DO_N_TIMES (h,
        memcpy(currDest, currSrc, sizeof(unsigned short)*w);
        currSrc += src->parentWidth;
        currDest += dest->parentWidth;
    );
}
