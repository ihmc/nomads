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
 * sc.c
 *
 * Functions that create/free/manipulate ScImages.
 *
 * weitsang Nov 97
 *
 *----------------------------------------------------------------------
 */


#include "basicInt.h"

ScImage *
ScNew(width, height)
    int width, height;
{
    ScImage *buf;

    buf = NEW(ScImage);
    buf->x = 0;
    buf->y = 0;
    buf->width = width;
    buf->height = height;
    buf->parentWidth = buf->width;
    buf->isVirtual = 0;
    buf->firstBlock = NEWARRAY(ScBlock, (buf->width) * (buf->height));
    return buf;
}


void
ScFree(ScImage * buf)
{
    if (!buf->isVirtual) {
        FREE(buf->firstBlock);
    }
    FREE(buf);
}


ScImage *
ScClip(buf, x, y, w, h)
    ScImage *buf;
    int x;
    int y;
    int w;
    int h;
{
    ScImage *newBuf;
    int pw;
    int offset;

    newBuf = NEW(ScImage);
    newBuf->x = x;
    newBuf->y = y;
    newBuf->width = w;
    newBuf->height = h;
    newBuf->isVirtual = 1;
    pw = newBuf->parentWidth = buf->parentWidth;
    offset = y * pw + x;
    newBuf->firstBlock = buf->firstBlock + offset;

    return newBuf;
}

void
ScReclip(buf, x, y, w, h, clipped)
    ScImage *buf;
    int x;
    int y;
    int w;
    int h;
    ScImage *clipped;
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
    clipped->firstBlock = buf->firstBlock + offset;
}


void
ScCopy(src, dest)
    ScImage *src;
    ScImage *dest;
{
    register int i, w, h;
    ScBlock *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstBlock;
    currDest = dest->firstBlock;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            {
                currDest->intracoded = currSrc->intracoded;
                currDest->skipMB = currSrc->skipMB;
                currDest->skipBlock = currSrc->skipBlock;
                currDest->dc = currSrc->dc;
                currDest->numOfAC = currSrc->numOfAC;
                memcpy(currDest->index, currSrc->index,
                    sizeof(char) * currSrc->numOfAC);

                memcpy(currDest->value, currSrc->value,
                    sizeof(short) * currSrc->numOfAC);

                currSrc++;
                currDest++;
            }
        );
        currSrc += src->parentWidth - w;
        currDest += dest->parentWidth - w;
    }
}


void
ScCopyDcAc(src, dest)
    ScImage *src;
    ScImage *dest;
{
    register int i, w, h;
    ScBlock *currSrc, *currDest;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstBlock;
    currDest = dest->firstBlock;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            currDest->dc = currSrc->dc;
            currDest->numOfAC = currSrc->numOfAC;
            memcpy(currDest->index, currSrc->index,
                sizeof(char) * currSrc->numOfAC);

            memcpy(currDest->value, currSrc->value,
                sizeof(short) * currSrc->numOfAC);

            currSrc++;
            currDest++;
            );
        currSrc += src->parentWidth - w;
        currDest += dest->parentWidth - w;
    }
}
