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
 * byteaffinecmd.c
 *
 * Tcl commands to Byte Image Affine Transformation
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmByteGeom.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_affine <src> <dest> <a> <b> <c> <d> <e> <f>
 *
 * precond 
 *     buf is a valid buffer
 *
 * return 
 *     none
 * 
 * side effect :
 *     The content of _src_ get affine transformed and the results 
 *     goes into _dest_  The equation is
 *
 *           x' = ax+by+c
 *           y' = dx+ey+f
 *
 *     Only those pixels lying within the region of the source
 *     buffer are affected.  Border pixels in the destination image are
 *     skipped.
 *
 * ----------------------------------------------------------------------
 */

int
ByteAffineCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    double a, b, c, d, e, f;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 9, "wrong # args: should be %s src dest a b c d e f", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    ReturnErrorIf ((Tcl_GetDouble(interp, argv[3], &a) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[4], &b) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[5], &c) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[6], &d) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[7], &e) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[8], &f) != TCL_OK));

    ByteAffine(srcBuf, destBuf, a, b, c, d, e, f);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_affine_rect_region <src> <a> <b> <c> <d> <e> <f>
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
ByteAffineRectRegionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;
    double a, b, c, d, e, f;
        RectRegion rect;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 8, "wrong # args: should be %s src a b c d e f", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);

    ReturnErrorIf ((Tcl_GetDouble(interp, argv[2], &a) != TCL_OK) ||
                (Tcl_GetDouble(interp, argv[3], &b) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[4], &c) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[5], &d) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[6], &e) != TCL_OK) ||
        (Tcl_GetDouble(interp, argv[7], &f) != TCL_OK));

        rect = ByteAffineRectRegion(srcBuf, a, b, c, d, e, f);

    sprintf(interp->result, "%d %d %d %d", rect.x, rect.y, rect.w, rect.h);

    return TCL_OK;
}
