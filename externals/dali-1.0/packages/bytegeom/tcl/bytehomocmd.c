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
 * bytehomocmd.c
 *
 * Tcl commands to Byte Image Homogeneous Transformations
 * Haye Oct 97
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmByteGeom.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_homo_compute_matrix <w> <h> <x1> <y1> <x2> <y2> <x3> <y3>
 *
 * precond 
 *     none
 *
 * return 
 *     a list of six floats a11 a12 a21 a22 a31 a32
 *
 * side effect :
 *     The basic idea is to compute the projection matrix for this
 *     special case, that a rectangular image gets projectively
 *     transformed into a quadrilateral, with the simplifying 
 *     assumption that (0,0) maps to (0,0)
 *
 *(0,0)------------------           (0,0)  --------------  (x1,y1)
 *     |      w         |                   \            /
 *     |                |        ------>     \          /
 *     |h               |             (x3,y3) \--------- (x2,y2)
 *     |                |
 *     ------------------
 *
 *     So, the problem is to find six out of eight coordinates of
 *     the general projective transform matrix such that:
 *      -            -     - -        -  -
 *     |  a11  a12  0 |   | u |      | x' |
 *     |  a21  a22  0 | * | v |   =  | y' |
 *     |  a31  a32  1 |   | 1 |      | w  |
 *      -            -     - -        -  -
 *
 * (sugata - 12/2/97)
 *----------------------------------------------------------------------
 */
int
ByteHomoComputeMatrixCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    double w, h, x1, y1, x2, y2, x3, y3;
    double a11, a12, a21, a22, a31, a32;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 9, "wrong # args: should be %s w h x1 y1 x2 y2 x3 y3", argv[0]);
    ReturnErrorIf ((Tcl_GetDouble(interp, argv[1], &w) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[2], &h) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[3], &x1) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[4], &y1) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[5], &x2) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[6], &y2) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[7], &x3) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[8], &y3) != TCL_OK));

    status = ByteHomoComputeMatrix(w, h, x1, y1, x2, y2, x3, y3, 
        &a11, &a12, &a21, &a22, &a31, &a32);

    ReturnErrorIf1(status == DVM_BYTE_BAD_MATRIX, "%s: not a projective transform", argv[0]);

    sprintf(interp->result,"%f %f %f %f %f %f", a11, a12, a21, a22, a31, a32);
    return TCL_OK;
}

int
ByteHomoInvertMatrixCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    double a11, a12, a21, a22, a31, a32;
    double na11, na12, na21, na22, na31, na32;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 7, "wrong # args: should be %s x1 y1 x2 y2 x3 y3", argv[0]);
    ReturnErrorIf ((Tcl_GetDouble(interp, argv[1], &a11) != TCL_OK) ||
           (Tcl_GetDouble(interp, argv[2], &a12) != TCL_OK) ||
           (Tcl_GetDouble(interp, argv[3], &a21) != TCL_OK) ||
           (Tcl_GetDouble(interp, argv[4], &a22) != TCL_OK) ||
           (Tcl_GetDouble(interp, argv[5], &a31) != TCL_OK) ||
           (Tcl_GetDouble(interp, argv[6], &a32) != TCL_OK));

    status = ByteHomoInvertMatrix(a11, a12, a21, a22, a31, a32,
                                  &na11, &na12, &na21, &na22, &na31, &na32);

    ReturnErrorIf1(status == DVM_BYTE_BAD_MATRIX, "%s: matrix is singular", argv[0]);

    sprintf(interp->result,"%f %f %f %f %f %f", na11, na12, na21, na22, na31, na32);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     byte_homo <src> <dest> <a> <b> <c> <d> <e> <f> <m> <n> <p>
 *
 * precond
 *     buf is a valid buffer
 *     the destination quadrilateral must be convex
 *
 * return
 *     none
 * 
 * side effect :
 *     The content of _src_ get homo transformed and the results 
 *     goes into _dest_  The equation is
 *
 *           x' = (ax+by+c) / z
 *           y' = (dx+ey+f) / z
 *           where
 *           z = mx+ny+1
 *
 *     Only those pixels lying within the region of the source
 *     buffer are affected.  Border pixels in the destination image are
 *     skipped.
 *
 * ----------------------------------------------------------------------
 */
int
ByteHomoCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    double a, b, c, d, e, f, m, n, p;
    
    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 12, "wrong # args: should be %s src dest a b c d e f m n p", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);
    ReturnErrorIf ((Tcl_GetDouble(interp, argv[3], &a) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[4], &b) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[5], &c) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[6], &d) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[7], &e) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[8], &f) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[9], &m) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[10], &n) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[11], &p) != TCL_OK));

    ByteHomo(srcBuf, destBuf, a, b, c, d, e, f, m, n, p);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_homo_rect_region <buf> <a> <b> <c> <d> <e> <f> <m> <n> <p>
 *
 * precond 
 *     buf is a valid buffer
 *
 * return 
 *     the bounding box of the source buffer in destination
 *     image coordinates, as (x,y,w,h).  These four values give the
 *     four offset and dimensions of where the output buffer would
 *     be written in the destination image space.  These values
 *     are useful for allocating buffers for the destination image.
 * 
 * side effect :
 *     None
 *
 * ----------------------------------------------------------------------
 */

int
ByteHomoRectRegionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;
    double a, b, c, d, e, f, m, n, p;
    RectRegion rect;

        /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 11, "wrong # args: should be %s src a b c d e f m n p", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    ReturnErrorIf ((Tcl_GetDouble(interp, argv[2], &a) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[3], &b) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[4], &c) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[5], &d) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[6], &e) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[7], &f) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[8], &m) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[9], &n) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[10], &p) != TCL_OK));

    rect = ByteHomoRectRegion(srcBuf, a, b, c, d, e, f, m, n, p);
    
    sprintf(interp->result, "%d %d %d %d", rect.x, rect.y, rect.w, rect.h);

    return TCL_OK;
}
