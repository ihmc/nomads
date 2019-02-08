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
 * bitmake.c
 *
 * Wei Tsang May 97
 *
 * Defines functions that create a bit region from chroma keys
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

/*
 * Static functions declarations.
 */
static void MakeBitRowFromByteRow(unsigned char *, int, int,
    unsigned char *, unsigned char,
    unsigned char, int);

static void
MakeBitRowFromByteRow(src, firstBit, lastBit, dest, low, high, w)
    unsigned char *src;
    int firstBit;
    int lastBit;
    unsigned char *dest;
    unsigned char low;
    unsigned char high;
    int w;
{
    switch (firstBit) {
    case 1:
        if (*src <= high && *src >= low)
            *dest |= 0x40;
        src++;
    case 2:
        if (*src <= high && *src >= low)
            *dest |= 0x20;
        src++;
    case 3:
        if (*src <= high && *src >= low)
            *dest |= 0x10;
        src++;
    case 4:
        if (*src <= high && *src >= low)
            *dest |= 0x08;
        src++;
    case 5:
        if (*src <= high && *src >= low)
            *dest |= 0x04;
        src++;
    case 6:
        if (*src <= high && *src >= low)
            *dest |= 0x02;
        src++;
    case 7:
        if (*src <= high && *src >= low)
            *dest |= 0x01;
        src++;
        dest++;
    }


    /*
     * Do 8 bytes from src at a time, compute a 8 bit value in dest.
     */
    DO_N_TIMES(w,
        *dest = 0;
        if (*src <= high && *src >= low)
        * dest |= 0x80;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x40;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x20;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x10;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x08;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x04;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x02;
        src++;
        if (*src <= high && *src >= low)
        * dest |= 0x01;
        src++;
        dest++;
        );

    switch (lastBit) {
    case 7:
        if (*src <= high && *src >= low)
            *dest |= 0x80;
        src++;
    case 6:
        if (*src <= high && *src >= low)
            *dest |= 0x40;
        src++;
    case 5:
        if (*src <= high && *src >= low)
            *dest |= 0x20;
        src++;
    case 4:
        if (*src <= high && *src >= low)
            *dest |= 0x10;
        src++;
    case 3:
        if (*src <= high && *src >= low)
            *dest |= 0x08;
        src++;
    case 2:
        if (*src <= high && *src >= low)
            *dest |= 0x04;
        src++;
    case 1:
        if (*src <= high && *src >= low)
            *dest |= 0x02;
    }
}

void
BitMakeFromKey(bytebuf, low, high, bitbuf)
    ByteImage *bytebuf;
    unsigned char low;
    unsigned char high;
    BitImage *bitbuf;
{
    unsigned char *src, *dest;
    int w, firstBit, lastBit;
    int h;

    /*
     * Map 8 bytes at a time into 8 bit (one byte) in bitbuf.
     */

    w = min(bitbuf->unitWidth, bytebuf->width);
    h = min(bitbuf->height, bytebuf->height);

    w = (w + bitbuf->firstBit - 8) / 8;
    if (bitbuf->lastBit == 0)
        w++;

    src = bytebuf->firstByte;
    dest = bitbuf->firstByte;
    firstBit = bitbuf->firstBit;
    lastBit = bitbuf->lastBit;
    DO_N_TIMES(h,
        MakeBitRowFromByteRow(src, firstBit, lastBit, dest, low, high, w);
        src += bytebuf->parentWidth;
        dest += bitbuf->parentWidth;
        );
}
