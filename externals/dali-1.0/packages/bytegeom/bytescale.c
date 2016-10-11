/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * bytescale.c
 *
 * Functions that scale ByteImages
 *
 *----------------------------------------------------------------------
 */

#include "bytegeomInt.h"

int 
ByteShrink2x2 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int newWidthExtra, nw, nh;
    int srcHeightSkip, destHeightSkip;
    int i, sum;
    unsigned char *src1, *src2, *dest;

#if defined (ONLY_124_DEMUX_SAME_LENGTH)

    unsigned char *firstSrc1, *firstSrc2, *firstDest;
    int srcSkip, destSkip;
    int sStride, dStride;;
    /*
     * Copy from source to dest, averaging
     */
    firstSrc1 = srcBuf->firstByte;
    firstSrc2 = srcBuf->firstByte + srcBuf->parentWidth * srcBuf->rowSkip;
    firstDest = destBuf->firstByte;
    srcSkip = 2 * srcBuf->parentWidth * srcBuf->rowSkip;
    destSkip = destBuf->parentWidth * destBuf->rowSkip;

    *newWidth = srcBuf->width / 2;
    *newHeight = srcBuf->height / 2;
    newWidthExtra = srcBuf->width % 2;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    sStride = srcBuf->stride;
    dStride = destBuf->stride;
    switch (srcBuf->length) {
        case 1:
            for (i = 0; i < nh; i++) {
                src1 = firstSrc1;
                src2 = firstSrc2;
                dest = firstDest;
                DO_N_TIMES (nw,
                    sum = *src1;
                    sum += *src2;
                    src1 += sStride;
                    src2 += sStride;
                    sum += *src1;
                    sum += *src2;
                    src1 += sStride;
                    src2 += sStride;
                    *dest = (unsigned char)(sum >> 2);
                    dest += dStride;
                )
                firstSrc1 += srcSkip;
                firstSrc2 += srcSkip;
                firstDest += destSkip;
            }
            break;
        case 2:
            nw >>= 1;
            for (i = 0; i < nh; i++) {
                src1 = firstSrc1;
                src2 = firstSrc2;
                dest = firstDest;
                DO_N_TIMES (nw,
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1;
                    sum += *src2;
                    src1 += sStride;
                    src2 += sStride;
                    *dest++ = (unsigned char)(sum >> 2);
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1;
                    sum += *src2;
                    src1 += sStride;
                    src2 += sStride;
                    *dest = (unsigned char)(sum >> 2);
                    dest += destBuf->stride;
                )
                firstSrc1 += srcSkip;
                firstSrc2 += srcSkip;
                firstDest += destSkip;
            }
            break;
        case 4:
            nw >>= 2;
            for (i = 0; i < nh; i++) {
                src1 = firstSrc1;
                src2 = firstSrc2;
                dest = firstDest;
                DO_N_TIMES (nw,
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1++;
                    sum += *src2++;
                    *dest++ = (unsigned char)(sum >> 2);
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1;
                    sum += *src2;
                    src1 += sStride;
                    src2 += sStride;
                    *dest++ = (unsigned char)(sum >> 2);
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1++;
                    sum += *src2++;
                    *dest++ = (unsigned char)(sum >> 2);
                    sum = *src1++;
                    sum += *src2++;
                    sum += *src1;
                    sum += *src2;
                    src1 += srcBuf->stride;
                    src2 += srcBuf->stride;
                    *dest = (unsigned char)(sum >> 2);
                    dest += destBuf->stride;
                )
                firstSrc1 += srcSkip;
                firstSrc2 += srcSkip;
                firstDest += destSkip;
            }
            break;
    }
#else
    /*
     * Calculate the appropiate new width and height.
     */

    *newWidth = srcBuf->width / 2;
    *newHeight = srcBuf->height / 2;
    newWidthExtra = srcBuf->width % 2;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    /*
     * Copy from source to dest, averaging
     */
    srcHeightSkip = srcBuf->parentWidth*2 - nw*2 + newWidthExtra;
    src1 = srcBuf->firstByte;
    src2 = src1 + srcBuf->parentWidth;
    destHeightSkip = destBuf->parentWidth - nw;
    dest = destBuf->firstByte;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            sum = *src1++;
            sum += *src2++;
            sum += *src1++;
            sum += *src2++;
            *dest++ = (unsigned char)(sum >> 2);
        )
        src1 += srcHeightSkip;
        src2 += srcHeightSkip;
        dest += destHeightSkip;
    }
#endif
    return 0;
}

/* return DVM_BYTE_TOO_SMALL if destBuf not large enough */
int 
ByteShrink4x4 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int newWidthExtra;
    int srcHeightSkip, destHeightSkip;
    int i, j, sum;
    unsigned char *src1, *src2, *src3, *src4, *dest;

    *newWidth = srcBuf->width / 4;
    *newHeight = srcBuf->height / 4;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);
    newWidthExtra = srcBuf->width % 4;

    /*
     * Copy from source to dest, averaging
     */
    srcHeightSkip = srcBuf->parentWidth * 4 - nw*4 + newWidthExtra;
    src1 = srcBuf->firstByte;
    src2 = src1 + srcBuf->parentWidth;
    src3 = src2 + srcBuf->parentWidth;
    src4 = src3 + srcBuf->parentWidth;
    destHeightSkip = destBuf->parentWidth - nw;
    dest = destBuf->firstByte;
    for (i = 0; i < nh; i++) {
        for (j=0; j < nw; j++) {
            sum = *src1++;
            sum += *src1++;
            sum += *src1++;
            sum += *src1++;

            sum += *src2++;
            sum += *src2++;
            sum += *src2++;
            sum += *src2++;

            sum += *src3++;
            sum += *src3++;
            sum += *src3++;
            sum += *src3++;

            sum += *src4++;
            sum += *src4++;
            sum += *src4++;
            sum += *src4++;
            *dest++ = (unsigned char)(sum >> 4);
        }
        src1 += srcHeightSkip;
        src2 += srcHeightSkip;
        src3 += srcHeightSkip;
        src4 += srcHeightSkip;
        dest += destHeightSkip;
    }

    return 0;
}

/* return DVM_BYTE_TOO_SMALL if destBuf not large enough */
int 
ByteShrink1x2 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int newWidthExtra;
    int srcHeightSkip, destHeightSkip;
    int i, sum;
    unsigned char *src1, *src2, *dest;

    /*
     * Calculate the appropiate new height.
     * Width remains the same
     */

    *newWidth = srcBuf->width;
    *newHeight = srcBuf->height / 2;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);
    newWidthExtra = srcBuf->width % 2;

    /*
     * Copy from source to dest, averaging
     */
    srcHeightSkip = srcBuf->parentWidth*2 - nw + newWidthExtra;
    src1 = srcBuf->firstByte;
    src2 = src1 + srcBuf->parentWidth;
    destHeightSkip = destBuf->parentWidth - nw;
    dest = destBuf->firstByte;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            sum = *src1++;
            sum += *src2++;
            *dest++ = sum >> 1;
        )
        src1 += srcHeightSkip;
        src2 += srcHeightSkip;
        dest += destHeightSkip;
    }

    return 0;
}

/* return DVM_BYTE_TOO_SMALL if destBuf not large enough */
/* side effects : *newWidth and *newHeight updated with the 
 *                new image dimension */
int ByteShrink2x1 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int newWidthExtra;
    int srcHeightSkip, destHeightSkip;
    int i, sum;
    unsigned char *src1, *dest;

    /*
     * Calculate the appropiate new width.
     * Height remains the same
     */
    *newWidth = srcBuf->width / 2;
    *newHeight = srcBuf->height;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);
    newWidthExtra = srcBuf->width % 2;

    /*
     * Copy from source to dest, averaging
     */

    srcHeightSkip = srcBuf->parentWidth - nw*2 + newWidthExtra;
    src1 = srcBuf->firstByte;
    destHeightSkip = destBuf->parentWidth - nw;
    dest = destBuf->firstByte;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            sum = *src1++;
            sum += *src1++;
            *dest++ = sum >> 1;
        )
        src1 += srcHeightSkip;
        dest += destHeightSkip;
    }

    return 0;
}

/* side effects : *newWidth and *newHeight updated with the 
 *                new image dimension */
int 
ByteExpand2x2 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int srcWidth, srcHeightSkip, destHeightSkip;
    int i;
    unsigned char *src, *dest1, *dest2;

    /*
     * Calculate the appropiate new width and height.
     */
    srcWidth = srcBuf->width;

    *newWidth = srcWidth * 2;
    *newHeight = srcBuf->height * 2;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    /*
     * Copy from source to dest
     */
    src = srcBuf->firstByte;
    destHeightSkip = destBuf->parentWidth*2 - nw;
    dest1 = destBuf->firstByte;
    dest2 = dest1 + destBuf->parentWidth;
    nh >>= 1;
    nw >>= 1;
    srcHeightSkip = srcBuf->parentWidth - nw;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            *dest1++ = *src;
            *dest1++ = *src;
            *dest2++ = *src;
            *dest2++ = *src++;
        )
        src += srcHeightSkip;
        dest1 += destHeightSkip;
        dest2 += destHeightSkip;
    }

    return 0;
}

/* side effects : *newWidth and *newHeight updated with the 
 *                new image dimension */
int 
ByteExpand4x4 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int srcWidth, srcHeightSkip, destHeightSkip;
    int i, j;
    unsigned char *src, *dest1, *dest2, *dest3, *dest4;

    srcWidth = srcBuf->width;
    *newWidth = srcWidth * 4;
    *newHeight = srcBuf->height * 4;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    /*
     * Copy from source to dest
     */
    src = srcBuf->firstByte;
    destHeightSkip = destBuf->parentWidth*4 - nw;
    dest1 = destBuf->firstByte;
    dest2 = dest1 + destBuf->parentWidth;
    dest3 = dest2 + destBuf->parentWidth;
    dest4 = dest3 + destBuf->parentWidth;
    nw >>= 2;
    nh >>= 2;
    srcHeightSkip = srcBuf->parentWidth - nw;
    for (i = 0; i < nh; i++) {
        for (j=0; j < nw; j++) {
            *dest1++ = *src;
            *dest1++ = *src;
            *dest1++ = *src;
            *dest1++ = *src;

            *dest2++ = *src;
            *dest2++ = *src;
            *dest2++ = *src;
            *dest2++ = *src;

            *dest3++ = *src;
            *dest3++ = *src;
            *dest3++ = *src;
            *dest3++ = *src;
            
            *dest4++ = *src;
            *dest4++ = *src;
            *dest4++ = *src;
            *dest4++ = *src++;
        }
        src += srcHeightSkip;
        dest1 += destHeightSkip;
        dest2 += destHeightSkip;
        dest3 += destHeightSkip;
        dest4 += destHeightSkip;
    }

    return 0;
}

/* return DVM_BYTE_TOO_SMALL if destBuf not large enough */
/* side effects : *newWidth and *newHeight updated with the 
 *                new image dimension */
int 
ByteExpand1x2 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int srcWidth, srcHeightSkip, destHeightSkip;
    int i;
    unsigned char *src, *dest1, *dest2;

    /*
     * Calculate the appropiate new height.
     * Width remains the same
     */
    srcWidth = srcBuf->width;
    *newWidth = srcWidth;
    *newHeight = srcBuf->height * 2;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    /*
     * Copy from source to dest, averaging
     */
    srcHeightSkip = srcBuf->parentWidth - nw;
    src = srcBuf->firstByte;
    destHeightSkip = destBuf->parentWidth*2 - nw;
    dest1 = destBuf->firstByte;
    dest2 = dest1 + destBuf->parentWidth;

    nh >>= 1;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            *dest1++ = *src;
            *dest2++ = *src++;
        )
        src += srcHeightSkip;
        dest1 += destHeightSkip;
        dest2 += destHeightSkip;
    }

    return 0;
}

int 
ByteExpand2x1 (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int *newWidth;
    int *newHeight;
{
    int nw, nh;
    int srcWidth, srcHeightSkip, destHeightSkip;
    int i;
    unsigned char *src, *dest;

    /*
     * Calculate the appropiate new width.
     * Height remains the same
     */
    srcWidth = srcBuf->width;
    *newWidth = srcWidth * 2;
    *newHeight = srcBuf->height;
    nw = min(*newWidth, destBuf->width);
    nh = min(*newHeight, destBuf->height);

    /*
     * Copy from source to dest, averaging
     */
    src = srcBuf->firstByte;
    destHeightSkip = destBuf->parentWidth - nw;
    dest = destBuf->firstByte;
    nw >>= 1;
    srcHeightSkip = srcBuf->parentWidth - nw;
    for (i = 0; i < nh; i++) {
        DO_N_TIMES (nw,
            *dest++ = *src;
            *dest++ = *src++;
        )
        src += srcHeightSkip;
        dest += destHeightSkip;
    }

    return 0;
}

/* helper functions called by ByteScaleBilinear in bytescale.c */
void
BilinearHelper (newWidth, frac, skip, src, dest)
    int newWidth;           /* width of row[] */
    char *skip;             /* Precomputed skip[] array (see below) */
    double *frac;            /* Precomputed frac[] array (see below) */
    unsigned char *src;     /* Input row */
    unsigned char *dest;    /* Output row */
{
    int j, p0, p1;
    for (j=0; j<newWidth-1; j++) {
        p0 = *src;
        p1 = *(src+1);
        *dest++ = (unsigned char)(p0 + frac[j]*(p1 - p0));
        src += skip[j];
    }
    *dest++ = *src;
}

/* return DVM_BYTE_TOO_SMALL if destBuf not large enough */
int
ByteScaleBilinear (srcBuf, destBuf, newWidth, newHeight)
    ByteImage *srcBuf;
    ByteImage *destBuf;
    int newWidth;
    int newHeight;
{
    int destHeightSkip, srcHeightSkip;
    int i, j, p0, p1, old;
    unsigned char *src, *dest;
    char *skipx, *skipy;
    double *fracx, *fracy;
    unsigned char *row1, *row2, *r1, *r2, *rtmp;
    double s, d, factor, y;

    if ((newWidth > destBuf->width) || (newHeight > destBuf->height)) {
        return DVM_BYTE_TOO_SMALL;
    }

    /*
     * Copy from source to dest, using bilinear interpolation.  See
     * doc/blnotes.doc (an MS Word document) for a detailed description
     * of what's going on here.  Basically, we're precomputing everything
     * we can so the pass over the image is as fast as possible.
     */

    skipx = NEWARRAY(char, newWidth);
    fracx = NEWARRAY(double, newWidth);
    skipy = NEWARRAY(char, newHeight);
    fracy = NEWARRAY(double, newHeight);
    row1 = NEWARRAY(unsigned char, newWidth);
    row2 = NEWARRAY(unsigned char, newWidth);
    s = srcBuf->height-1;
    d = newHeight - 1;
    skipy[0] = 0;
    fracy[0] = 0;
    factor = s/d;
    old = 0;
    for (j=1; j<=d; j++) {
        double tmp = j*factor;
        int itmp = (int)(tmp);
        skipy[j] = itmp - old;
        fracy[j] = tmp - itmp;
        old = itmp;
    }
    s = srcBuf->width-1;
    d = newWidth - 1;
    skipx[0] = 0;
    fracx[0] = 0;
    factor = s/d;
    old = 0;
    for (j=1; j<=d; j++) {
        double tmp = j*factor;
        int itmp = (int)(tmp);
        skipx[j] = itmp - old;
        fracx[j] = tmp - itmp;
        old = itmp;
    }

    destHeightSkip = destBuf->parentWidth - newWidth;
    dest = destBuf->firstByte;

    /*
     * Linearly interpolate along first two rows.
     */
    srcHeightSkip = srcBuf->parentWidth;
    src = srcBuf->firstByte;
    BilinearHelper (newWidth, fracx, skipx+1, src, row1);
    BilinearHelper (newWidth, fracx, skipx+1, src+srcHeightSkip, row2);

    for (i = 0; i < newHeight-1; i++) {

        /*
         * this updating block has to be put at the front part of 
         * the outer for-loop will cause possible array out of bound 
         * if put at the end of for-loop
         */
        if (skipy[i] != 0) {
            /*
             * if skipy[i] == 0, then use the same rows to interpolate
             * otherwise either skip single rows, or skip multiple rows
             */
            if (skipy[i] == 1) {
                /*
                 * use the next 2 rows, while the new 1st row (row1) is
                 * just the current 2nd row (row2)
                 * so only compute the new 2nd row
                 */
                rtmp = row1;
                row1 = row2;
                row2 = rtmp;
                src += srcHeightSkip;
                BilinearHelper (newWidth, fracx, skipx+1, src, row2);
            } else {
                /* 
                 * skip (skipy[i]) rows, recompute the row1, row2
                 */
                src += srcHeightSkip * skipy[i];
                BilinearHelper (newWidth, fracx, skipx+1, src, row1);
                BilinearHelper (newWidth, fracx, skipx+1, src+srcHeightSkip, row2);
            }
        }
        
        /*
         * Interpolate along the y axis (between rows)
         */
        y = fracy[i];
        r1 = row1;
        r2 = row2;
        for (j=0; j<newWidth; j++) {
            p0 = *r1++;
            p1 = *r2++;
            *dest++ = (unsigned char)(p0 + y*(p1 - p0));
        }
        dest += destHeightSkip;
    }

    /*
     * copy the last row directly
     */
    r2 = row2;
    DO_N_TIMES(newWidth,
        *dest++ = *r2++;
    );

    FREE(skipx);
    FREE(fracx);
    FREE(skipy);
    FREE(fracy);
    FREE(row1);
    FREE(row2);

    return 0;
}
