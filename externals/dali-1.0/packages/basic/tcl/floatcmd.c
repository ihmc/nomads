/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * floatcmd.c
 *
 * Tcl hooks to float image functions. 
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

int
FloatNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *new;
    int status, w, h;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s width height", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    new = FloatNew(w, h);
    PutFloatImage(interp, new);

    return TCL_OK;
}

int
FloatFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *img;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s floatImage", argv[0]);

    img = GetFloatImage(argv[1]);
    ReturnErrorIf2(img == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    FloatFree(img);
    return TCL_OK;
}

int
FloatClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *buf, *new;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s floatImage x y width height", argv[0]);

    buf = GetFloatImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    new = FloatClip(buf, x, y, w, h);
    PutFloatImage(interp, new);

    return TCL_OK;
}

int
FloatReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *buf, *clipped;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s floatImage x y width height clipped",
        argv[0]);

    buf = GetFloatImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    clipped = GetFloatImage(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s : no such float image %s\n", argv[0], argv[6]);

    FloatReclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}


int
FloatCopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcFloatImage destByte32Image", argv[0]);

    src = GetFloatImage(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    dest = GetFloatImage(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such float image %s\n", argv[0], argv[2]);

    FloatCopy(src, dest);
    return TCL_OK;
}


int
FloatGetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", FloatGetX(v));
    return TCL_OK;
}


int
FloatGetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", FloatGetY(v));
    return TCL_OK;
}


int
FloatGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", FloatGetWidth(v));
    return TCL_OK;
}


int
FloatGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", FloatGetHeight(v));
    return TCL_OK;
}


int
FloatGetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", FloatGetVirtual(v));
    return TCL_OK;
}


int
FloatInfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    FloatImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s floatImage", argv[0]);

    v = GetFloatImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such float image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d", v->x, v->y, v->width,
        v->height, v->isVirtual);

    return TCL_OK;
}
