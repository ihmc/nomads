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
 * byteaffine.c
 *
 * Defines the affine transformation for gray images
 *
 *----------------------------------------------------------------------
 */

#include "bytegeomInt.h"

void ByteAffine (ByteImage *srcBuf, ByteImage *destBuf,
                 double a, double b, double c, double d, double e, double f)
{
    double srcXf, srcYf;
    double aebd, dx, dy, xbase, ybase, dxbase, dybase;
    int srcXi, srcYi, destX, destY;
    unsigned char *dest, *src;
    double xprime[4], yprime[4];
    int x, y;
    int p00, p01, p10, p11;
    int w, h, best, i, j;
    double tmp;
    int region, startP, startQ, endP, endQ;
    int yend, ystart, xstartI, xendI;
    double xstartF, xendF;
    int srcXfixed, srcYfixed, dxFixed, dyFixed;
    int p00Fixed, p01Fixed, delta;

    /* make compiler happy */
    startP = startQ = endP = endQ = 0;
    /*
     * srcX = [ e(destX - c) - b(destY - f)]/(ae - bd)     
     * srcY = [-d(destX - c) + a(destY - f)]/(ae - bd)     
     */
    aebd = a*e - b*d;
    xbase = (b*f - e*c)/aebd;
    ybase = (d*c - a*f)/aebd;
    dx = e/aebd;
    dy = -d/aebd;
    dxbase = -b/aebd;
    dybase = a/aebd;
        
    /*
     * Calculate where the source rectangle lands on the destination image.
     * These will be four coordinates, xprime[0]..xprime[3], yprime[0]..yprime[3].
     */
    w = srcBuf->width-1;
    h = srcBuf->height-1;
    xprime[0] = a * 0 + b * 0 + c;
    yprime[0] = d * 0 + e * 0 + f;
    xprime[1] = a * w + b * 0 + c;
    yprime[1] = d * w + e * 0 + f;
    xprime[2] = a * 0 + b * h + c;
    yprime[2] = d * 0 + e * h + f;
    xprime[3] = a * w + b * h + c;
    yprime[3] = d * w + e * h + f;

    /*
     * Sort these 4 coordinates first by y coordinate, then by
     * x coordinate
     */
    for (i=0; i<4; i++) {
        best = i;
        for (j=i+1; j<4; j++) {
            if ((yprime[j] < yprime[best]) ||
                ((yprime[j] == yprime[best]) && (xprime[j] < xprime[best]))) {
                best = j;
            }
        }
        tmp = xprime[i];
        xprime[i] = xprime[best];
        xprime[best] = tmp;
        tmp = yprime[i];
        yprime[i] = yprime[best];
        yprime[best] = tmp;
    }

    /*
     * We now have the coordinates to the parallelogram that the
     * src image maps into on the destination image.  x[0],y[0] is
     * the top, x[3],y[3] is the bottom.  Divide it up into 3 regions:
     * the top (y[0]..y[1]), the middle (y[1]..y[2]) and the bottom
     * (y[2]..y[3]).  For each region, calculate the starting and
     * ending x coordinates for scanline y on the destination image.
     * These are the only pixels in the destination that are affected.
     */
    for (region=0; region<3; region++) {
        
        /*
         * Let L(p,q) be the line 
         * from (xprime[p],yprime[q]) to (xprime[p], yprime[q])
         * then the parallelogram after affine trasnformation 
         * will have 2 cases:
         * 1) xprime[1] > xprime[2]
         * 2) xprime[1] < xprime[2]
         * the case of xprime[1] = xprime[2] can be categorized
         * into either (1) or (2) arbitrarily
         * for case (1), the 3 triangular regions are 
         * bounded on the left and right by these lines :
         *              left            right
         * region 1:    L(0,2)          L(0,1)
         * region 2:    L(0,2)          L(1,3)
         * region 3:    L(2,3)          L(1,3)
         * for cases (2) :
         *              left            right
         * region 1:    L(0,1)          L(0,2)
         * region 2:    L(1,3)          L(0,2)
         * region 3:    L(1,3)          L(2,3)
         */
        if (xprime[1] > xprime[2]) {
            if (region == 0) {
                startP = 0;
                startQ = 2;
                endP = 0;
                endQ = 1;
            } else if (region == 1) {
                endP = 1;
                endQ = 3;
            } else {
                startP = 2;
                startQ = 3;
            }
        } else {
            if (region == 0) {
                startP = 0;
                startQ = 1;
                endP = 0;
                endQ = 2;
            } else if (region == 1) {
                startP = 1;
                startQ = 3;
            } else {
                endP = 2;
                endQ = 3;
            }
        }

        ystart = max(0, (int)floor(yprime[region]));
        yend = min((int)yprime[region+1], destBuf->height);
        for (destY=ystart; destY < yend; destY++) {

            /*
             * We're working on scanline destY.  Compute the starting and
             * ending x coordinates (xstart, xend) on the destination
             * image of the region for which source pixels are defined.
             * xstart is given by the x coordinate of the line from
             * (x[startP], y[startP]) to x(x[startQ], y[startQ]) with
             * y coordinate destY.
                         *
             * xend is similarly computed from x/y[endP/endQ]
             * The starting coordinate is rounded down, the ending coordinate
             * rounded up.
             */
            xstartF = xprime[startP] + 
                (destY-yprime[startP])*(xprime[startQ]-xprime[startP])/
                (yprime[startQ]-yprime[startP]);
            xendF = xprime[endP] + 
                (destY-yprime[endP])*(xprime[endQ]-xprime[endP])/
                (yprime[endQ]-yprime[endP]);
            xstartF = max (0, xstartF);
            xendF = min (xendF, destBuf->width);
            xstartI = (int)ceil(xstartF);
            xendI = (int)(xendF);
            dest = destBuf->firstByte + (destY*destBuf->parentWidth + xstartI);
            srcXf = (e*(xstartI-c) - b*(destY-f))/aebd;
            srcYf = (-d*(xstartI-c) + a*(destY-f))/aebd;
            srcXfixed = FIX(srcXf);
            srcYfixed = FIX(srcYf);
            dxFixed = FIX(dx);
            dyFixed = FIX(dy);
            
            for (destX = xstartI; destX < xendI; destX++) {

                /*
                 * The following code computes the value of a pixel in
                 * the destination image at (destX, destY).  It has been
                 * optimized by using fixed point arithmetic (see macro.h
                 * for a description of the fixed point operators).
                 * src[XY]fixed are the fixed-point coordinates of the
                 * corresponding location in the source image.  We compute
                 * the integer (truncated) value of that location in src[XY]i,
                 * and the remainder in x and y.  p00..p11 are the four closest
                 * pixels to src[XY], and we use them in the bilinear
                 * interpolation routine.  On a Pentium 133, this routine will
                 * process about 1.37 Mpix/sec. For comparison, 320x240
                 * video at 30 fps is 2.3 Mpix/sec
                 */
                srcXi = IUNFIX(srcXfixed);
                srcYi = IUNFIX(srcYfixed);
                src = srcBuf->firstByte + (srcYi*srcBuf->parentWidth + srcXi);

                x = srcXfixed - IFIX(srcXi);
                y = srcYfixed - IFIX(srcYi);
                p00 = *src++;
                p10 = *src;
                src += srcBuf->parentWidth;
                p11 = *src--;
                p01 = *src;

                p00Fixed = IFIX(p00) + x*(p10-p00);
                p01Fixed = IFIX(p01) + x*(p11-p01);
                delta = IUNFIX_ROUND(p01Fixed - p00Fixed);
                p00Fixed = p00Fixed + y*delta;
                *dest++ = IUNFIX_ROUND(p00Fixed);

                srcXfixed += dxFixed;
                srcYfixed += dyFixed;
            }
        }
    }
}

RectRegion ByteAffineRectRegion (ByteImage *srcBuf, 
                                 double a, double b, double c, 
                                 double d, double e, double f)
{
    RectRegion rect;
    double xprime[4], yprime[4], xmax, ymax, xmin, ymin;
    int w, h, i;


    /*
     * Calculate where the source rectangle lands on the destination image.
     * These will be four coordinates, xprime[0]..xprime[3], yprime[0]..yprime[3].
     * from the following formulas :
     *
     * destX = a*srcX + b*srcY + c;
     * destY = d*srcX + e*srcY + f;
     */

    w = srcBuf->width-1;
    h = srcBuf->height-1;
    xprime[0] = a * 0 + b * 0 + c;
    yprime[0] = d * 0 + e * 0 + f;
    xprime[1] = a * w + b * 0 + c;
    yprime[1] = d * w + e * 0 + f;
    xprime[2] = a * 0 + b * h + c;
    yprime[2] = d * 0 + e * h + f;
    xprime[3] = a * w + b * h + c;
    yprime[3] = d * w + e * h + f;

    /*
     * Find the min, max X and Y coordinates.
     */
    xmax = xmin = xprime[0];
    ymax = ymin = yprime[0];
    for (i=1; i<4; i++) {
        if (xprime[i] > xmax) {xmax = xprime[i];}
        if (xprime[i] < xmin) {xmin = xprime[i];}
        if (yprime[i] > ymax) {ymax = yprime[i];}
        if (yprime[i] < ymin) {ymin = yprime[i];}
    }
    xmin = ceil(xmin);
    xmax = floor(xmax);
    ymin = floor(ymin);
    ymax = floor(ymax);

    rect.x = (int) xmin;
    rect.y = (int) ymin;
    rect.w = (int) (xmax - xmin);
    rect.h = (int) (ymax-ymin);
    
    return rect;
}
