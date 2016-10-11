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
 * edgedetect.c
 *
 * Steve Weiss October 97
 *
 * Sobel edge detection
 *
 *----------------------------------------------------------------------
 */


#include "visionInt.h"

unsigned char numBits[] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

unsigned char abs_val[] =
{255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241,
 240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226,
 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211,
 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196,
 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181,
 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166,
 165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151,
 150, 149, 148, 147, 146, 145, 144, 143, 142, 141, 140, 139, 138, 137, 136,
 135, 134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121,
 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106,
 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88,
 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 
 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50,
 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31,
 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 
 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 
 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 
 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 
 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135,
 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 
 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 
 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};


int
ByteEdgeDetectSobel (srcBuf, dest1Buf, dest2Buf, percent, thresh1, thresh2)
    ByteImage *srcBuf, *dest1Buf, *dest2Buf;
    int percent;
    int *thresh1, *thresh2;
{
    register int i;
    int w, h;
    unsigned char *currSrc, *currDest1, *currDest2, *nextSrc;
    int srcParW, dest1ParW, dest2ParW;
    unsigned char *firstDest1, *firstDest2, *firstSrc;
    int spw2;
    int *currHor, *currVert;

    unsigned char borderVal;
    int goal, numPixelsBelow;
    unsigned char* abs_v;
    int horPixelBuf[256];
    int vertPixelBuf[256];


    if ((srcBuf->width != dest1Buf->width) || (srcBuf->height != dest1Buf->height) || 
        (srcBuf->width != dest2Buf->width) || (srcBuf->height != dest2Buf->height))
    {
      return DVM_DIFFERENT_SIZES;
    } 

    if (percent < 0 || percent > 100) return DVM_BAD_PERCENT;

    abs_v = abs_val+255;

    w = srcBuf->width;
    h = srcBuf->height;
    srcParW = srcBuf->parentWidth;
    borderVal = (char)0;
       
    /*
     * Create two arrays of 256 values to represent the number of horizontal and vertical pixel pairs for
     * each difference
     */

    currHor = horPixelBuf;
    currVert = vertPixelBuf;
    memset(horPixelBuf, 0, 256*sizeof(int));
    memset(vertPixelBuf, 0, 256*sizeof(int));

    firstSrc = srcBuf->firstByte;
    dest1ParW = dest1Buf->parentWidth;
    dest2ParW = dest2Buf->parentWidth;
    firstDest1 = dest1Buf->firstByte;
    firstDest2 = dest2Buf->firstByte;
    spw2 = srcParW<<1;

    /*
     * Top row
     */
    memset(firstDest2, 0, w);
    firstDest2 += dest2ParW;
    /*
     * Compute the horizontal and vertical differences, and keep 
     * track of how many of each difference are found
     */

    for (i = 0; i < h-2; i++) {
        currSrc = firstSrc;
        currDest1 = firstDest1;
        currDest2 = firstDest2;

        *currDest1++ = 0;
        nextSrc = currSrc + spw2;
        if (w > 2) {
            DO_N_TIMES(w-2,
                *currDest1 = abs_v[*currSrc - *(currSrc+2)];
                vertPixelBuf[*currDest1]++;
                currDest1++;

                *currDest2 = abs_v[*currSrc - *nextSrc];
                horPixelBuf[*currDest2]++;
                currDest2++;
                nextSrc++;

                currSrc++;
            );
        }

        /*
         * Last two source columns
         */

        *currDest2 = abs_v[*currSrc - *nextSrc];
        *currDest1 = 0;
        horPixelBuf[*currDest2]++;
        currDest2++;
        nextSrc++;
        currSrc++;

        *currDest2 = abs_v[*currSrc - *nextSrc];
        horPixelBuf[*currDest2]++;

        firstSrc += srcParW;
        firstDest1 += dest1ParW;
        firstDest2 += dest2ParW;
    }

    /*
     * Last two rows
     */ 

    currSrc = firstSrc;
    currDest1 = firstDest1;
    currDest2 = firstDest2;

    *currDest1 = *currDest2 = 0;
    currDest1++;
    currDest2++;

    if (w > 2) {
        DO_N_TIMES(w-2,
            *currDest1 = abs_v[*currSrc - *(currSrc+2)];
            *currDest2 = 0;
            vertPixelBuf[*currDest1]++;
            currDest1++;
            currDest2++;
            currSrc++;
        );
    }

    *currDest1 = *currDest2 = 0;

    firstSrc += srcParW;
    firstDest1 += dest1ParW;
    firstDest2 += dest2ParW;
    currDest1 = firstDest1;
    currSrc = firstSrc;

    *currDest1 = 0;
    currDest1++;

    if (w > 2) {
        DO_N_TIMES(w-2,
            *currDest1 = abs_v[*currSrc - *(currSrc+2)];
            vertPixelBuf[*currDest1]++;
            currDest1++;
            currSrc++;
        );
    }

    *currDest1 = 0;

    /*
     * Find the horizontal and vertical thresholds
     */

    vertPixelBuf[0] += 2*h;
    horPixelBuf[0] += 2*w;

    goal = (int)((float)percent/100.0 * (float)(w*h) + 0.5);
    numPixelsBelow=0;

    while (numPixelsBelow < goal) {
        numPixelsBelow += *currHor;
        currHor++;
    }

    numPixelsBelow=0;
    while (numPixelsBelow < goal) {
        numPixelsBelow += *currVert;
        currVert++;
    }

    *thresh1 = (int)(currVert - vertPixelBuf);
    *thresh2 = (int)(currHor - horPixelBuf);

    return 1;
}








