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
 * dvmbytegeom.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_BYTEGEOM_H_
#define _DVM_BYTEGEOM_H_

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *---------------------------------------------------------------------
 *
 * Error codes for ByteGeom package
 *
 *---------------------------------------------------------------------
 */
#define DVM_BYTE_OK 0
#define DVM_BYTE_TOO_SMALL -1
#define DVM_BYTE_BAD_MATRIX -2

/*
 *---------------------------------------------------------------------
 *
 * This is the main C library interface header file for BYTEGEOM package.
 * It contains functions for common geometric operations on byte images
 *
 *---------------------------------------------------------------------
 */

    typedef struct RectRegion {
        int x;
        int y;
        int w;
        int h;
    } RectRegion;

/* geometric functions (bytescale.c) */
    int ByteShrink2x2(ByteImage *, ByteImage *, int *, int *);
    int ByteShrink4x4(ByteImage *, ByteImage *, int *, int *);
    int ByteShrink1x2(ByteImage *, ByteImage *, int *, int *);
    int ByteShrink2x1(ByteImage *, ByteImage *, int *, int *);
    int ByteExpand2x2(ByteImage *, ByteImage *, int *, int *);
    int ByteExpand4x4(ByteImage *, ByteImage *, int *, int *);
    int ByteExpand1x2(ByteImage *, ByteImage *, int *, int *);
    int ByteExpand2x1(ByteImage *, ByteImage *, int *, int *);
    int ByteScaleBilinear(ByteImage *, ByteImage *, int, int);

/* Rotation functions (byterotate.c) */
    void ByteRotate90a(ByteImage * srcBuf, ByteImage * destBuf);
    void ByteRotate90c(ByteImage * srcBuf, ByteImage * destBuf);
    void ByteRotateOrig(ByteImage * srcBuf, ByteImage * destBuf, double theta);
    void ByteRotate(ByteImage * srcBuf, ByteImage * destBuf,
        double theta, int x, int y);

/* General affine transform (byteaffine.c) */
    void ByteAffine(ByteImage * srcBuf, ByteImage * destBuf,
        double a, double b, double c, double d, double e, double f);
    RectRegion ByteAffineRectRegion(ByteImage * srcBuf,
        double a, double b, double c,
        double d, double e, double f);

/* General Homogeneous transform (bytehomo.c) */
    int ByteHomoComputeMatrix(double w, double h,
        double x1, double y1, double x2, double y2, double x3, double y3,
        double *a11, double *a12, double *a21,
        double *a22, double *a31, double *a32);
    int ByteHomoInvertMatrix(double a11, double a12, double a21,
        double a22, double a31, double a32,
        double *na11, double *na12, double *na21,
        double *na22, double *na31, double *na32);
    void ByteHomo(ByteImage * srcBuf, ByteImage * destBuf, double a, double b, double c,
        double d, double e, double f, double m, double n, double p);
    RectRegion ByteHomoRectRegion(ByteImage * srcBuf, double a, double b, double c,
        double d, double e, double f, double m, double n, double p);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
