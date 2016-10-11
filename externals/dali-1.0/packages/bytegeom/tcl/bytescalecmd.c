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
 * bytescalecmd.c
 *
 * Tcl commands to Byte Image Scaling
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmByteGeom.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_shrink_4x4 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled down by 4 in both directions
 *     the width and height of new image is updated
 *
 * special :
 *     Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteShrink4x4Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteShrink4x4(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_shrink_2x2 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled down by 2 in both directions
 *     the width and height of new image is updated
 *
 * special :
 *     Overlap-safe
 *
 *----------------------------------------------------------------------
 */

int
ByteShrink2x2Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteShrink2x2(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL, 
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_shrink_1x2 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled down by 2 in y direction (height),
 *     but x (width) remains the same.
 *     the width and height of new image is updated
 *
 * special :
 *     Overlap-safe
 *
 *----------------------------------------------------------------------
 */

int
ByteShrink1x2Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteShrink1x2(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_shrink_2x1 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled down by 2 in x direction (width),
 *           but y (height) remains the same.
 *     the width and height of new image is updated
 *
 * special :
 *     Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteShrink2x1Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteShrink2x1(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_expand_4x4 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled up by 4 in both directions
 *
 * special :
 *     not Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteExpand4x4Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteExpand4x4(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_expand_2x2 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled up by 2 in both directions
 *
 * special :
 *     not Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteExpand2x2Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteExpand2x2(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_expand_1x2 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled up by 2 in y direction (height),
 *     but x (width) remains the same.
 *
 * special :
 *     not Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteExpand1x2Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteExpand1x2(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_expand_2x1 <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     width and height of new image
 *
 * side effect :
 *     the destination buffer is scaled down by 2 in x direction (width),
 *           but y (height) remains the same.
 *
 * special :
 *     not Overlap-safe
 *
 *----------------------------------------------------------------------
 */
int
ByteExpand2x1Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = ByteExpand2x1(srcBuf, destBuf, &newWidth, &newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    sprintf(interp->result, "%d %d", newWidth, newHeight);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_scale_bilinear <src> <dest> <newWidth> <newHeight>
 *
 * precond 
 *     bufs exist.  newWidth and newHeight are smaller than
 *     src image width and height
 *
 * return 
 *     None
 *
 * side effect :
 *     the source buffer is scaled down to fit into image that is
 *     newWidth by newHeight using bilinear interpolation.  The result
 *     is written into starting at the upper left corner of the
 *     destination buffer.
 *
 * special :
 *     Overlap-safe if shrinking image only
 *
 *----------------------------------------------------------------------
 */
int
ByteScaleBilinearCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int newWidth, newHeight;
    int status;

    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 5, 
        "wrong # args: should be %s src dest newWidth newHeight", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &newWidth);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &newHeight);
    ReturnErrorIf(status != TCL_OK);
        
    status = ByteScaleBilinear(srcBuf, destBuf, newWidth, newHeight);
    ReturnErrorIf4(status == DVM_BYTE_TOO_SMALL,
        "%s: destination image %s is too small.  Minimum size is %d by %d",
        argv[0], argv[2], newWidth, newHeight);

    return TCL_OK;
}
