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
 * bytehomo.c
 *
 * Defines the homogeneous transformation for gray images
 *
 *----------------------------------------------------------------------
 */

#include "bytegeomInt.h"

int ByteHomoComputeMatrix (double w, double h, 
    double x1, double y1, double x2, double y2, double x3, double y3,
    double *a11, double *a12, double *a21, 
    double *a22, double *a31, double *a32)
{
    double deltax1, deltax3;
    double deltay1, deltay3;
    double maindet;

    double a31w, a32h;

    /* Now compute deltas */

    deltax1 = (x1 - x2);
    deltax3 = (x3 - x2);
    deltay1 = (y1 - y2);
    deltay3 = (y3 - y2);

    maindet = deltax1*deltay3 - deltay1*deltax3;

    if (maindet == 0) {
        return DVM_BYTE_BAD_MATRIX;
    }

    /* Now the a's */
    a31w = (y1*deltax3 - x1*deltay3)/maindet;
    a32h = (x3*deltay1 - y3*deltax1)/maindet;

    /* and the coeffecients */
    *a11 = (a31w+1)*x1/w;
    *a21 = (a31w+1)*y1/w;

    *a12 = (a32h+1)*x3/h;
    *a22 = (a32h+1)*y3/h;

    *a31 = a31w/w;
    *a32 = a32h/h;

    return 0;
}

int ByteHomoInvertMatrix ( double olda11, double olda12, double olda21, 
                           double olda22, double olda31, double olda32,
                           double *a11, double *a12, double *a21,
                           double *a22, double *a31, double *a32)
{
    double maindet;

    maindet = olda11*olda22 - olda12*olda21;
    if (fabs(maindet) < 0.0001)
        return DVM_BYTE_BAD_MATRIX;

    *a11 = olda22/maindet;
    *a22 = olda11/maindet;
    *a21 = -olda21/maindet;
    *a12 = -olda12/maindet;
    *a31 = (olda32*olda21 - olda31*olda22)/maindet;
    *a32 = (olda31*olda12 - olda32*olda11)/maindet;
    return 0;
}

void ByteHomo (ByteImage *srcBuf, ByteImage *destBuf, double a, double b,
                    double c, double d, double e, double f, double m, double n, double p)
{
    unsigned char *dest, *src;
    double acm, bcn, dfm, efn, dnm, det, xNom, yNom, Denom;
    double h11, h12, h13, h21, h22, h23, h31, h32, h33;
    double xh[4], yh[4], z, tmp;
    double xstartF, xendF;
    double srcXf, srcYf;
    double angle, minAngle;
    int tmpInt;
    int yend, ystart, xstartI, xendI, destX, destY, srcXi, srcYi;
    int x, y, p00, p01, p10, p11;
    int w, h;
    int order[4], startNextOrder, endNextOrder;
    int i, j, best, region, startP, startQ, endP, endQ, delta, minj;
    int h11Fixed, h21Fixed, h31Fixed, xNomFixed, yNomFixed, DenomFixed;
    int srcXfixed, srcYfixed, p00Fixed, p01Fixed;

    /* make compiler happy */
    startNextOrder = endNextOrder = startP = startQ = endP = endQ = minj = 0;
    
    /*
     * Calculate where the source rectangle lands on the destination image.
     * These will be four coordinates, xh[0]..xh[3], yh[0]..yh[3].
     */
    w = srcBuf->width-1;
    h = srcBuf->height-1;
    z = m * 0 + n * 0 + p;
    xh[0] = (a * 0 + b * 0 + c) / z;
    yh[0] = (d * 0 + e * 0 + f) / z;
    z = m * w + n * 0 + p;
    xh[1] = (a * w + b * 0 + c) / z;
    yh[1] = (d * w + e * 0 + f) / z;
    z = m * 0 + n * h + p;
    xh[2] = (a * 0 + b * h + c) / z;
    yh[2] = d * 0 + e * h + f;
    z = m * w + n * h + p;
    xh[3] = (a * w + b * h + c) / z;
    yh[3] = (d * w + e * h + f) / z;

    /*
     * calculate all the entries in the inverse matrix H
     */
    acm = a*p  -c*m;
    bcn = b*p  -c*n;
    dnm = d*n - e*m;
    dfm = d*p - f*m;
    efn = e*p - f*n;
    det = a * efn - b * dfm + c*dnm;
    h11 = efn / det;
    h12 = -bcn / det;
    h21 = -dfm / det;
    h22 = acm / det;
    h13 = (b*f - e*c)/ det;
    h23 = -(a*f - d*c)/det;
    h31 = dnm / det;
    h32 = -(a*n - b*m)/det;
    h33 = (a*e - b*d)/det;
    h11Fixed = FIX(h11);
    h21Fixed = FIX(h21);
    h31Fixed = FIX(h31);

    /*
     * Sort these 4 coordinates first by y coordinate, then by
     * x coordinate
     */
    for (i=0; i<4; i++) {
        best = i;
        for (j=i+1; j<4; j++) {
            if ((yh[j] < yh[best]) ||
                ((yh[j] == yh[best]) && (xh[j] < xh[best]))) {
                best = j;
            }
        }
        tmp = xh[i];
        xh[i] = xh[best];
        xh[best] = tmp;
        tmp = yh[i];
        yh[i] = yh[best];
        yh[best] = tmp;
    }

    /*
     * find the anti-clockwise order of the points
     * start from (x[order[0]], y[order[0]]
     * where (x[order[i+1]], y[order[i+1]]) is the next point of 
     *       (x[order[i]], y[order[i]]) in anti-clockwise direction
     */
    for (i=0; i<4; i++) {
        order[i] = i;
    }
    
    for (i=1; i<4; i++) {
        minAngle = 2*M_PI;
        
        /* starting from the horizontal line on the left of point(order[i-1]),
         * rotating the line anti-clockwise,
         * the first point being swept thru is the next point in 
         * anticlockwise direction
         */
        for (j=i; j<4; j++) {
            /* angle = anticlockwise angle from
             *         the horizontal line on the left of point(order[i-1]) to
             *         the line thru point(order[i-1]), point(order[j])
             */
            angle = M_PI - atan2(yh[order[j]] - yh[order[i-1]], 
                            xh[order[j]] - xh[order[i-1]]);
            if (angle < minAngle) {
                    minAngle = angle;
                    minj = j;
            }
        }
        tmpInt = order[i];
        order[i] = order[minj];
        order[minj] = tmpInt;
    }

    /*
     * We now have the coordinates to the quadrilateral that the
     * src image maps into on the destination image.  x[0],y[0] is
     * the top, x[3],y[3] is the bottom.  Divide it up into 3 regions:
     * the top (y[0]..y[1]), the middle (y[1]..y[2]) and the bottom
     * (y[2]..y[3]).  For each region, calculate the starting and
     * ending x coordinates for scanline y on the destination image.
     * These are the only pixels in the destination that are affected.
     */
    for (region=0; region<3; region++) {
        if (region == 0) {
            startNextOrder = 1;
            endNextOrder = 3;
            startP = 0;
            startQ = order[1];
            endP = 0;
            endQ = order[3];
        } else {
            if (yh[startQ] > yh[endQ]) {
                endP = endQ;
                endQ = order[--endNextOrder];
            } else {
                startP = startQ;
                startQ = order[++startNextOrder];
            }
        }
        ystart = max(0, (int)floor(yh[region]));
        yend = min(destBuf->height, (int)yh[region+1]);

        for (destY=ystart; destY < yend; destY++) {
        
            /*
             * We're working on scanline destY.  Compute the starting and
             * ending x coordinates (xstart, xend) on the destination
             * image of the region for which source pixels are defined.
             * xstart is given by the x coordinate of the line from
             * (x[startP], y[startP]) to x(x[startQ], y[startQ]) with
             * y coordinate destY.
             * xend is similarly computed from x/y[endP/endQ]
             * The starting coordinate is rounded down, the ending coordinate
             * rounded up.
             */
            xstartF = xh[startP] + 
                (destY-yh[startP])*(xh[startQ]-xh[startP])/
                (yh[startQ]-yh[startP]);
            xendF = xh[endP] + 
                (destY-yh[endP])*(xh[endQ]-xh[endP])/(yh[endQ]-yh[endP]);
            xstartF = max (0, xstartF);
            xendF = min (xendF, destBuf->width);
            xstartI = (int)ceil(xstartF);
            xendI = (int)(xendF);
            dest = destBuf->firstByte + (destY*destBuf->parentWidth + xstartI);

            /*
             * H is the inverse homogeneous matrix which maps destination points
             * back to source image
             * H(x,y,1) = (xNom, yNom, Denom)
             * srcX = xNom/Denom
             * srcY = yNom/Denom
             */
            xNom =  h11 * xstartI + h12 * destY + h13;
            yNom =  h21 * xstartI + h22 * destY + h23;
            Denom = h31 * xstartI + h32 * destY + h33;
            xNomFixed = FIX(xNom);
            yNomFixed = FIX(yNom);
            DenomFixed = FIX(Denom);
            srcXf = xNom / Denom;
            srcYf = yNom / Denom;
            srcXfixed = FIX(srcXf);
            srcYfixed = FIX(srcYf);

            for (destX = xstartI; destX < xendI; destX++, dest++) {

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
                 * interpolation routine.  On a Pentium 266, this routine will
                 * process about 2.02 Mpix/sec. For comparison, 320x240
                 * video at 30 fps is 2.3 Mpix/sec
                 */
                srcXi = IUNFIX(srcXfixed);
                srcYi = IUNFIX(srcYfixed);
                if ((srcXi >= 0) && (srcXi <= w) && (srcYi >= 0) && (srcYi <= h)) {
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
                    *dest = IUNFIX_ROUND(p00Fixed);
                }

                /*
                 * update the srcX, srcY
                 * H(x+1,y,1) = (xNom+h11, yNom+h21, Denom+h31)
                 */
                xNomFixed += h11Fixed;
                yNomFixed += h21Fixed;
                DenomFixed += h31Fixed;
                srcXfixed = FIXDIV(xNomFixed, DenomFixed);
                srcYfixed = FIXDIV(yNomFixed, DenomFixed);
            }
        }
    }
}

RectRegion ByteHomoRectRegion (ByteImage *srcBuf, double a, double b, double c, 
                               double d, double e, double f, double m, double n, double p)
{
    double xprime[4], yprime[4], z, xmin, xmax, ymin, ymax;
    int w, h;
    int i;
    RectRegion rect;

    /*
     * Calculate where the source rectangle lands on the destination image.
     * These will be four coordinates, xh[0]..xh[3], yh[0]..yh[3].
     * from the following formulas :
     *
     * destX = (a*srcX + b*srcY + c) / z;
     * destY = (d*srcX + e*srcY + f) / z;
     * where z = m*srcX + n*srcY + 1;
     */
    w = srcBuf->width-1;
    h = srcBuf->height-1;
    z = m * 0 + n * 0 + p;
    xprime[0] = (a * 0 + b * 0 + c) / z;
    yprime[0] = (d * 0 + e * 0 + f) / z;
    z = m * w + n * 0 + p;
    xprime[1] = (a * w + b * 0 + c) / z;
    yprime[1] = (d * w + e * 0 + f) / z;
    z = m * 0 + n * h + p;
    xprime[2] = (a * 0 + b * h + c) / z;
    yprime[2] = d * 0 + e * h + f;
    z = m * w + n * h + p;
    xprime[3] = (a * w + b * h + c) / z;
    yprime[3] = (d * w + e * h + f) / z;

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
