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
 * overlap.c
 *
 * Steve Weiss November 97
 *
 * Routines for counting the number of "1" pixels in the intersection
 * of two byte images divided by the number of "1" pixels in the first image
 *
 *----------------------------------------------------------------------
 */

#include "visionInt.h"

extern unsigned char numBits[];
extern unsigned char abs_val[];

int 
BitCountOverlap (buf1, buf2, percent)
    BitImage *buf1;
    BitImage *buf2;
    int *percent;
{
    unsigned char *curr1, *curr2;
    int h, bw;
    int parW1, parW2;
    int totalOverlap, totalbit1s;
    unsigned char overlap;
    int bit1, bit2;
    unsigned char a, b;
    int leftOverBits;
    unsigned char *first1, *first2;
    register int i;

    h = buf1->height;
    bw = min(buf1->byteWidth, buf2->byteWidth);
    curr1 = buf1->firstByte;
    curr2 = buf2->firstByte;
    bit1 = buf1->firstBit;
    bit2 = buf2->firstBit;
    parW1 = buf1->parentWidth;
    parW2 = buf2->parentWidth;

    if (!bit1 && !bit2 && !buf1->lastBit && !buf2->lastBit)
        return DVM_USE_ALLIGN;


    /*
     * Compute the number of "full" bytes
     */
    if (buf2->firstBit == 0) leftOverBits = buf2->lastBit;
    else leftOverBits = 8 - buf2->firstBit + buf2->lastBit;

    if (leftOverBits >= 8) {
        bw++;
        leftOverBits -= 8;
    }


    totalOverlap=0;
    totalbit1s=0;

    bit1 = buf1->firstBit;
    bit2 = buf2->firstBit;

    /*
     * For each byte, compute the intersection
     */
    if (leftOverBits > 0) {
        for (i=0; i<h; i++) {
            first1 = curr1;
            first2 = curr2;

            DO_N_TIMES(bw,
                a = GET_NEXT_BYTE(curr1, bit1);
                b = GET_NEXT_BYTE(curr2, bit2);
                overlap = a&b;
                totalOverlap += numBits[overlap];
                totalbit1s += numBits[a];
            );

            a = GET_NEXT_BYTE(curr1, bit1);
            b = GET_NEXT_BYTE(curr2, bit2);
            overlap = (a&b) >> (8-leftOverBits);
            totalOverlap += numBits[overlap]; 
            totalbit1s += numBits[a];

            curr1 = first1 + parW1;
            curr2 = first2 + parW2;
        }
    } else {
        for(i=0; i<h; i++) {
            first1 = curr1;
            first2 = curr2;

            DO_N_TIMES(bw, 
                a = GET_NEXT_BYTE(curr1, bit1);
                b = GET_NEXT_BYTE(curr2, bit2);
                overlap = a&b;
                totalOverlap += numBits[overlap];
                totalbit1s += numBits[a];
            );

            curr1 = first1 + parW1;
            curr2 = first2 + parW2;
        }
    }

    if (totalbit1s == 0) {
        *percent = 0;
        return DVM_VISION_OK;
    }

    *percent = (short) (100 * (float)totalOverlap / (float)totalbit1s + 0.5);
    return DVM_VISION_OK;
}


int 
BitCountOverlap8 (buf1, buf2, percent)
    BitImage *buf1;
    BitImage *buf2;
    int *percent;
{
    unsigned char *curr1, *curr2;
    int h, bw;
    int parW1, parW2;
    int totalOverlap, totalbit1s;
    unsigned char overlap;
    unsigned char a, b;
    unsigned char *first1, *first2;
    register int i;

    h = buf1->height;
    bw = buf1->byteWidth;
    curr1 = buf1->firstByte;
    curr2 = buf2->firstByte;
    parW1 = buf1->parentWidth;
    parW2 = buf2->parentWidth;

    totalOverlap=0;
    totalbit1s=0;

    if (buf1->firstBit || buf2->firstBit || buf1->lastBit || buf2->lastBit)
        return DVM_NOT_BYTE_ALLIGNED;

    if (buf1->byteWidth != buf2->byteWidth)
        return DVM_DIFFERENT_SIZES;

    /*
     * For each byte, compute the intersection
     */
    for (i=0; i < h; i++) {
        first1 = curr1;
        first2 = curr2;

        DO_N_TIMES(bw, 
            a = *curr1;
            b = *curr2;
            curr1++;
            curr2++;
            overlap = a&b;
            totalOverlap += numBits[overlap];
            totalbit1s += numBits[a];
        );

        curr1 = first1 + parW1;
        curr2 = first2 + parW2;
    }

    if (totalbit1s == 0) {
        *percent = 0;
        return DVM_VISION_OK;
    }
    *percent = (short) (100 * (float)totalOverlap / (float)totalbit1s + 0.5);
    return DVM_VISION_OK;
}



