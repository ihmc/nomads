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
 * byterotatecmd.c
 *
 * Tcl commands to Byte Image Rotations
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmByteGeom.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_rotate_90a <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     None
 *
 * side effect :
 *     the buffer and it's content is rotated at center by 90
 *     degree anti-clockwise.  the dimension of the destination 
 *     buffer changes.
 *
 *----------------------------------------------------------------------
 */
int
ByteRotate90aCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    
    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    ByteRotate90a(srcBuf, destBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_rotate_90c <src> <dest>
 *
 * precond 
 *     buf exists.
 *
 * return 
 *     None
 *
 * side effect :
 *     the buffer and it's content is rotated at center by 90
 *     degree clockwise.
 *
 *----------------------------------------------------------------------
 */
int
ByteRotate90cCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    
    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s src dest", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);

    ByteRotate90c(srcBuf, destBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_rotate_orig <src> <dest> <theta> 
 *
 * precond 
 *     buf exists.
 *     theta in degrees (can be fraction)
 *
 * return 
 *     None
 *
 * side effect :
 *     the content of _src_ is rotated at (0,0) by _theta_
 *     degree, and the result is written into _dest_.
 *     
 *----------------------------------------------------------------------
 */
int
ByteRotateOrigCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    double theta;
    
    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 4, "wrong # args: should be %s src dest theta", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);
    ReturnErrorIf(Tcl_GetDouble(interp, argv[3], &theta) != TCL_OK);

    ByteRotateOrig(srcBuf, destBuf, theta);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_rotate <src> <dest> <theta> <x> <y>
 *
 * precond 
 *     buf exists.
 *     theta in degrees
 *
 * return 
 *     None
 *
 * side effect :
 *     the content of _src_ is rotated at (x,y) by _theta_
 *     degree and the result is dumped into _dest_. 
 *
 *----------------------------------------------------------------------
 */
int
ByteRotateCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    double theta;
    int x, y;
    
    /*
     * Check args, retrieve buffer from hash table.
     */
    ReturnErrorIf1 (argc != 6, "wrong # args: should be %s src dest theta x y", argv[0]);
    srcBuf  = GetByteImage(argv[1]);
    ReturnErrorIf2 (srcBuf == NULL, "%s: no such byte image %s", argv[0], argv[1]);
    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2 (destBuf == NULL, "%s: no such byte image %s", argv[0], argv[2]);
    ReturnErrorIf(Tcl_GetDouble(interp, argv[3], &theta) != TCL_OK);
    ReturnErrorIf(Tcl_GetInt(interp, argv[4], &x) != TCL_OK);
    ReturnErrorIf(Tcl_GetInt(interp, argv[5], &y) != TCL_OK);

    ByteRotate(srcBuf, destBuf, theta, x, y);

    return TCL_OK;
}
