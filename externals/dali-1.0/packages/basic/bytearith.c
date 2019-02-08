/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * bytearith.c
 *
 * Functions that perform addition and multiplication on ByteImages.
 * This is originally implemented by Katen Patel (kpatel@cs.berkeley.edu)
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"
extern unsigned char theCropTable[];
#define CROP(n) (unsigned char)theCropTable[(n) + 2048]

void
ByteAdd (src1, src2, dest)
    ByteImage *src1;
    ByteImage *src2;
    ByteImage *dest;
{
    unsigned char *currSrc1, *currSrc2, *currDest;
    int w, h, i;

    w = min(src1->width, dest->width);
    w = min(src2->width, w);
    h = min(src1->height, dest->height);
    h = min(src2->height, h);

    currSrc1 = src1->firstByte;
    currSrc2 = src2->firstByte;
    currDest = dest->firstByte;
    for (i = 0; i < h; i++) {
        DO_N_TIMES ( w,
            *currDest++ = CROP(*currSrc1++ + *currSrc2++);
        );
        currSrc1 += src1->parentWidth - w;
        currSrc2 += src2->parentWidth - w;
        currDest += dest->parentWidth - w;
    }

    return;
}

void
ByteMultiply (src, k, dest)
    ByteImage *src;
    double k;
    ByteImage *dest;
{
    unsigned char *currSrc, *currDest;
    int w, h, i;

    w = min(src->width, dest->width);
    h = min(src->height, dest->height);

    currSrc = src->firstByte;
    currDest = dest->firstByte;
    for (i = 0; i < h; i++) {
        DO_N_TIMES ( w,
            *currDest++ = CROP((int)(k*(*currSrc++)));
        );
        currSrc += src->parentWidth - w;
        currDest += dest->parentWidth - w;
    }

    return;
}
