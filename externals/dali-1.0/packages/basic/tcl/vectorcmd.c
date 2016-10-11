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
 * vectorcmd.c
 *
 * Tcl hooks to vector image functions. 
 *
 * weitsang Nov 97
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

int
VectorNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *new;
    int status, w, h;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s width height", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    new = VectorNew(w, h);
    PutVectorImage(interp, new);

    return TCL_OK;
}

int
VectorFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *img;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s vectorImage", argv[0]);

    img = GetVectorImage(argv[1]);
    ReturnErrorIf2(img == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    VectorFree(img);
    return TCL_OK;
}

int
VectorClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *buf, *new;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s vectorImage x y width height", argv[0]);

    buf = GetVectorImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    new = VectorClip(buf, x, y, w, h);
    PutVectorImage(interp, new);

    return TCL_OK;
}

int
VectorReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *buf, *clipped;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s vectorImage x y width height clipped",
        argv[0]);

    buf = GetVectorImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    clipped = GetVectorImage(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s : no such vector image %s\n", argv[0], argv[6]);

    VectorReclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}


int
VectorCopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcVectorImage destVectorImage", argv[0]);

    src = GetVectorImage(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    dest = GetVectorImage(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such vector image %s\n", argv[0], argv[2]);

    VectorCopy(src, dest);
    return TCL_OK;
}


int
VectorGetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", VectorGetX(v));
    return TCL_OK;
}


int
VectorGetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", VectorGetY(v));
    return TCL_OK;
}


int
VectorGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", VectorGetWidth(v));
    return TCL_OK;
}


int
VectorGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", VectorGetHeight(v));
    return TCL_OK;
}


int
VectorGetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", VectorGetVirtual(v));
    return TCL_OK;
}


int
VectorInfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VectorImage *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s vectorImage", argv[0]);

    v = GetVectorImage(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such vector image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d", v->x, v->y, v->width,
        v->height, v->isVirtual);

    return TCL_OK;
}
