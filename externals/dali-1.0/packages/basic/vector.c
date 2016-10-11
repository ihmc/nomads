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
 * vector.c
 *
 * Functions that create/free/manipulate VectorImages.
 *
 * weitsang Nov 97
 *
 *----------------------------------------------------------------------
 */


#include "basicInt.h"

VectorImage *
VectorNew(width, height)
    int width, height;
{
    VectorImage *buf;

    buf = NEW(VectorImage);
    buf->x = 0;
    buf->y = 0;
    buf->width = width;
    buf->height = height;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstVector = NEWARRAY(Vector, (buf->width) * (buf->height));

    return buf;
}


void
VectorFree(VectorImage * buf)
{
    if (!buf->isVirtual) {
        FREE(buf->firstVector);
    }
    FREE(buf);
}


VectorImage *
VectorClip(buf, x, y, w, h)
    VectorImage *buf;
    int x;
    int y;
    int w;
    int h;
{
    VectorImage *newBuf;
    int pw;
    int offset;

    newBuf = NEW(VectorImage);
    newBuf->x = x;
    newBuf->y = y;
    newBuf->width = w;
    newBuf->height = h;
    newBuf->isVirtual = 1;
    pw = newBuf->parentWidth = buf->parentWidth;
    offset = y * pw + x;
    newBuf->firstVector = buf->firstVector + offset;

    return newBuf;
}

void
VectorReclip(buf, x, y, w, h, clipped)
    VectorImage *buf;
    int x;
    int y;
    int w;
    int h;
    VectorImage *clipped;
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
    clipped->firstVector = buf->firstVector + offset;
}

void
VectorCopy(src, dest)
    VectorImage *src;
    VectorImage *dest;
{
    int i, w, h;
    Vector *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstVector;
    currDest = dest->firstVector;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            currDest->exists = currSrc->exists;
            currDest->down = currSrc->down;
            currDest->right = currSrc->right;
            currDest++;
            currSrc++;
            );
        currDest += dest->parentWidth - w;
        currSrc += src->parentWidth - w;
    }
}
