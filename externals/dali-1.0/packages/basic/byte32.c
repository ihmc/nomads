
/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * byte32.c
 *
 * Functions that create/free/manipulate Byte32Images.
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */


#include "basicInt.h"

Byte32Image *
Byte32New(width, height)
    int width, height;
{
    Byte32Image *buf;

    buf = NEW(Byte32Image);
    buf->x = 0;
    buf->y = 0;
    buf->width = width;
    buf->height = height;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstByte = NEWARRAY(unsigned int, (buf->width) * (buf->height));
    return buf;
}


void
Byte32Free(Byte32Image * buf)
{
    if (!buf->isVirtual) {
        FREE(buf->firstByte);
    }
    FREE(buf);
}


Byte32Image *
Byte32Clip(buf, x, y, w, h)
    Byte32Image *buf;
    int x;
    int y;
    int w;
    int h;
{
    Byte32Image *newBuf;
    int pw;
    int offset;

    newBuf = NEW(Byte32Image);
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
Byte32Reclip(buf, x, y, w, h, clipped)
    Byte32Image *buf;
    int x;
    int y;
    int w;
    int h;
    Byte32Image *clipped;
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
Byte32Copy(src, dest)
    Byte32Image *src;
    Byte32Image *dest;
{
    register int w, h;
    unsigned int *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstByte;
    currDest = dest->firstByte;

    DO_N_TIMES (h,
        memcpy(currDest, currSrc, sizeof(unsigned int)*w);
        currSrc += src->parentWidth;
        currDest += dest->parentWidth;
    );
}
