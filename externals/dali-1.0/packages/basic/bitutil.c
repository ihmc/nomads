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
 * bitutil.c
 *
 * Wei Tsang June 97
 *
 * Defines utilities functions that manipulate buffer of bit type.
 * Includes :
 *
 * - functions to copy a rows of non-byte-align bits to another
 *   row of non-byte-align bits.
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

/*
 *-----------------------------------------------------
 * The global variables thePreMask, thePostMask
 *
 * Predefined mask that is useful for masking bits, 
 * particularly when dealing with bit buffers.
 * 
 * ANDing with thePreMask[k] will zero out the first k
 * bits, while ANDing with thePostMask[k] will zero out
 * the last 8-k bits. (easier to think : ANDing with 
 * thePostMask[k] preserve the first k bit.)
 *
 * See also bit*.c or pbm*.c for example usage.
 *-----------------------------------------------------
 */
unsigned char thePreMask[8] =
{
    0xff,                       /* 11111111 */
    0x7f,                       /* 01111111 */
    0x3f,                       /* 00111111 */
    0x1f,                       /* 00011111 */
    0x0f,                       /* 00001111 */
    0x07,                       /* 00000111 */
    0x03,                       /* 00000011 */
    0x01,                       /* 00000001 */
};

unsigned char thePostMask[8] =
{
    0x00,                       /* 00000000 */
    0x80,                       /* 10000000 */
    0xc0,                       /* 11000000 */
    0xe0,                       /* 11100000 */
    0xf0,                       /* 11110000 */
    0xf8,                       /* 11111000 */
    0xfc,                       /* 11111100 */
    0xfe,                       /* 11111110 */
};


/*
 *----------------------------------------------------------------------
 *
 * CopyRow*() Functions
 *
 * copy a rows of non-byte-align bits to another row of non-byte-align 
 * bits.  
 * 
 * - destByte and srcByte points to the first bytes of the row 
 *   respectively.
 * - dest1st and src1st are the firstBit field of dest and src
 * - destLast and srcLast are the lastBit field of dest and src
 * - width are the number of "full"-bytes for both dest and src.
 *   (they should be the same)
 *
 *----------------------------------------------------------------------
 */
void
CopyRowSrcOnTheLeft(destByte, dest1st, destLast, srcByte, src1st, srcLast, width)
    unsigned char *destByte;
    int dest1st;
    int destLast;
    unsigned char *srcByte;
    int src1st;
    int srcLast;
    int width;
{
    register int k;


    *destByte = COMBINE_2_BYTES_FM(8 - dest1st, src1st, *destByte, *srcByte);
    destByte++;
    k = dest1st - src1st;
    DO_N_TIMES(width,
        *destByte = COMBINE_2_BYTES_LF(k, *srcByte, *(srcByte + 1));
        destByte++;
        srcByte++;
        );
    *destByte = COMBINE_3_BYTES_LFL(destLast - srcLast, srcLast, *srcByte,
        *(srcByte + 1), *destByte);
}


void
CopyRowSrcOnTheRight(destByte, dest1st, destLast, srcByte, src1st, srcLast, width)
    unsigned char *destByte;
    int dest1st;
    int destLast;
    unsigned char *srcByte;
    int src1st;
    int srcLast;
    int width;
{
    register int n;

    *destByte = COMBINE_3_BYTES_FLF(dest1st, 8 - src1st, *destByte, *srcByte,
        *(srcByte + 1));
    destByte++;
    srcByte++;
    n = 8 - src1st + dest1st;
    DO_N_TIMES(width,
        *destByte = COMBINE_2_BYTES_LF(n, *srcByte, *(srcByte + 1));
        destByte++;
        srcByte++;
        );
    *destByte = COMBINE_2_BYTES_ML(destLast, srcLast - destLast, *srcByte, *destByte);
}

void
CopyRowEqual(destByte, dest1st, destLast, srcByte, src1st, srcLast, width)
    unsigned char *destByte;
    int dest1st;
    int destLast;
    unsigned char *srcByte;
    int src1st;
    int srcLast;
    int width;
{
    *destByte = COMBINE_2_BYTES_FL(dest1st, *destByte, *srcByte);
    destByte++;
    srcByte++;
    memcpy(destByte, srcByte, width);
    destByte += width;
    srcByte += width;
    *destByte = COMBINE_2_BYTES_FL(destLast, *srcByte, *destByte);
}


/*
 *-------------------------------------------------------------------
 * Return the next n bits in the consecutive bytes pointed to by 
 * *byte, starting with bit *firstBit.  Advance *byte to the next 
 * byte if all bits in *byte have been look at.  If n < 8, then the 
 * byte return will have 8-n 0s as prefix. firstBit will be updated
 * for the next get operation.
 *-------------------------------------------------------------------
 */
unsigned char
GetNextNBitsPutEnd(int n, unsigned char **byte, int *firstBit)
{
    unsigned char result;

    if (n < 8 - *firstBit) {
        result = ZERO_FIRST(*firstBit, **byte) >> (8 - *firstBit - n);
        *firstBit += n;
    } else {
        result = (ZERO_FIRST(*firstBit, **byte) << (n - 8 + *firstBit)) |
            (KEEP_FIRST(n - 8 + *firstBit, *(*byte + 1)) >> (16 - n - *firstBit));
        *firstBit += n - 8;
        (*byte)++;
    }
    return result;
}


/*
 *-------------------------------------------------------------------
 * Same as above but last 8-n is 0 instead.
 *-------------------------------------------------------------------
 */
unsigned char
GetNextNBitsPutFront(int n, unsigned char **byte, int *firstBit)
{
    unsigned char result;

    if (n < 8 - *firstBit) {
        result = ZERO_LAST(8 - n - *firstBit, **byte) << *firstBit;
        *firstBit += n;
    } else {
        result = (ZERO_FIRST(*firstBit, **byte) << *firstBit) |
            (KEEP_FIRST(n - 8 + *firstBit, *(*byte + 1)) >> (8 - *firstBit));
        *firstBit += n - 8;
        (*byte)++;
    }
    return result;
}


/*
 *-------------------------------------------------------------------
 * Same as above but special case for n == 8
 *-------------------------------------------------------------------
 */
unsigned char
GetNextByte(unsigned char **byte, int firstBit)
{
    unsigned char result;

    result = (ZERO_FIRST(firstBit, **byte) << firstBit) |
        (KEEP_FIRST(firstBit, *(*byte + 1)) >> (8 - firstBit));
    (*byte)++;
    return result;
}
