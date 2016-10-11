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
 * byterotate.c
 *
 * Functions that rotate ByteImages
 *
 *----------------------------------------------------------------------
 */

#include "bytegeomInt.h"

void
ByteRotate90a (srcBuf, destBuf)
    ByteImage *srcBuf;
    ByteImage *destBuf;
{
    int w, h, srcSkip, destSkip;
    int i, j;
    unsigned char *currSrc, *currDest;

    w = srcBuf->width;
    h = srcBuf->height;

    srcSkip = srcBuf->parentWidth + w;
    destSkip = destBuf->parentWidth;
    currSrc = srcBuf->firstByte + w - 1;
    for (i = 0; i < h; i++) {
        currDest = destBuf->firstByte + i;
        for (j = 0; j < w; j++) {
            *currDest = *currSrc--;
            currDest += destSkip;
        }
        currSrc += srcSkip;
    }
}

void
ByteRotate90c (srcBuf, destBuf)
    ByteImage *srcBuf;
    ByteImage *destBuf;
{
    int w, h, srcSkip;
    int i, j;
    unsigned char *currSrc, *currDest;
    
    w = srcBuf->width;
    h = srcBuf->height;

    srcSkip = srcBuf->parentWidth - w;
    currSrc = srcBuf->firstByte;
    for (i = h - 1; i >= 0; i--) {
        currDest = destBuf->firstByte + i;
        for (j = 0; j < w; j++) {
               *currDest = *currSrc++;
            currDest += destBuf->parentWidth;
        }
        currSrc += srcSkip;
    }
}

void
ByteRotateOrig (srcBuf, destBuf, theta)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    double theta;
{
    int w, h;
    int i, j, srcXInt, srcYInt;
    unsigned char *temp, *currSrc, *currDest;
    float srcXFloat, srcYFloat;
    float cosf, sinf, K, dK;
    double cosd, sind, cosinv, radians;

    /*
     * Compute the angle, cosine and sine value.
     */
    radians = -M_PI*(double)theta/180.0;
    cosd = cos(radians);
    sind = sin(radians);
    cosinv = 1.0/cosd;
    cosf = (float)cosd;
    sinf = (float)sind;
    w = srcBuf->width;
    h = srcBuf->height;

    /*
     * Allocate a temporary buffer for intermediate image.
     * First pass. y stays the same.  x "sheared".
     */

    temp = NEWARRAY(unsigned char, w*h);
    currSrc = srcBuf->firstByte;
    currDest = temp;
    K = 0;
    dK = (float) (sinf*cosinv);
    for (i = 0; i < h; i++) {
        srcXFloat = K;
        for (j = 0; j < w; j++) {
            ROUNDDOWN(srcXInt, srcXFloat);
            if (srcXInt >= 0 && srcXInt < w) {
                *currDest = *(currSrc + srcXInt);
            }
            srcXFloat += (float) cosinv;
            currDest++;
        }
        currSrc += srcBuf->parentWidth;
        K += dK;
    }

    /*
     * Second pass.  Now shear the y
     */

    currSrc = temp;
    K = 0;
    for (i = 0; i < w; i++) {
        currDest = destBuf->firstByte + i;
        srcYFloat = K;
        for (j = 0; j < h; j++) {
            ROUNDDOWN(srcYInt, srcYFloat);
            if (srcYInt >= 0 && srcYInt < h) {
                *currDest = *(currSrc + srcYInt*w);
            }
            currDest += destBuf->parentWidth;
            srcYFloat += cosf;
        }
        currSrc ++;
        K -= sinf;
    }

    FREE(temp);
}

void
ByteRotate (srcBuf, destBuf, theta, x, y)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    double theta;
    int x;
    int y;
{
    int w, h, pw;
    int i, j, srcXInt, srcYInt;
    unsigned char *temp, *currSrc, *currDest, *firstByte;
    float srcXFloat, srcYFloat;

    float cosf, sinf, K, dK;
    double cosd, sind, cosinv, radians;
    double A, B;

    /*
     * Compute the angle, cosine and sine value.
     */
    radians = -M_PI*(double)theta/180.0;
    cosd = cos(radians);
    sind = sin(radians);
    cosinv = 1.0/cosd;
    cosf = (float)cosd;
    sinf = (float)sind;
    w = srcBuf->width;
    pw = srcBuf->parentWidth;
    h = srcBuf->height;
    A = -(float)x*cosf + (float)y*sinf + (float)x;
    B = -(float)x*sinf - (float)y*cosf + (float)y;

    /*
     * Allocate a temporary buffer for intermediate image.
     * First pass. y stays the same.  x "sheared".
     */
    temp = NEWARRAY(unsigned char, w*h);
    memset(temp, 0, w*h);
    firstByte = srcBuf->firstByte;
    currSrc = firstByte;
    currDest = temp;
    K = -(float) A*(float)cosinv;
    dK = (float) (sinf*cosinv);
    for (i = 0; i < h; i++) {
        srcXFloat = K;
        for (j = 0; j < w; j++) {
            ROUNDDOWN(srcXInt, srcXFloat);
            if (srcXInt >= 0 && srcXInt < w) {
                *currDest = *(currSrc + srcXInt);
            }
            srcXFloat += (float) cosinv;
            currDest++;
        }
        currSrc += pw;
        K += dK;
    }

    /*
     * Second pass.  Now shear the y
     */
    currSrc = temp;
    firstByte = destBuf->firstByte;
    pw = destBuf->parentWidth;
    K = (float) (A*sinf - B*cosf);
    for (i = 0; i < w; i++) {
        currDest = firstByte + i;
        srcYFloat = K;
        for (j = 0; j < h; j++) {
            ROUNDDOWN(srcYInt, srcYFloat);
            if (srcYInt >= 0 && srcYInt < h) {
                *currDest = *(currSrc + srcYInt*w);
            }
            currDest += pw;
            srcYFloat += cosf;
        }
        currSrc ++;
        K -= sinf;
    }

    FREE(temp);
}
