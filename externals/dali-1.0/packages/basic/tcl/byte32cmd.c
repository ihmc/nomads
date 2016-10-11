/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * byte32cmd.c
 *
 * Tcl hooks to byte32 image functions. 
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

int
Byte32NewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *new;
    int status, w, h;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s width height", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    new = Byte32New(w, h);
    PutByte32Image(interp, new);

    return TCL_OK;
}

int
Byte32FreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *img;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s byte32Image", argv[0]);

    img = GetByte32Image(argv[1]);
    ReturnErrorIf2(img == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    Byte32Free(img);
    return TCL_OK;
}

int
Byte32ClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *buf, *new;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s byte32Image x y width height", argv[0]);

    buf = GetByte32Image(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    new = Byte32Clip(buf, x, y, w, h);
    PutByte32Image(interp, new);

    return TCL_OK;
}

int
Byte32ReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *buf, *clipped;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s byte32Image x y width height clipped",
        argv[0]);

    buf = GetByte32Image(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    clipped = GetByte32Image(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[6]);

    Byte32Reclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}


int
Byte32CopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcByte32Image destByte32Image", argv[0]);

    src = GetByte32Image(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    dest = GetByte32Image(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[2]);

    Byte32Copy(src, dest);
    return TCL_OK;
}


int
Byte32GetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte32GetX(v));
    return TCL_OK;
}


int
Byte32GetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte32GetY(v));
    return TCL_OK;
}


int
Byte32GetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte32GetWidth(v));
    return TCL_OK;
}


int
Byte32GetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte32GetHeight(v));
    return TCL_OK;
}


int
Byte32GetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte32GetVirtual(v));
    return TCL_OK;
}


int
Byte32InfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte32Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte32Image", argv[0]);

    v = GetByte32Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte32 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d", v->x, v->y, v->width,
        v->height, v->isVirtual);

    return TCL_OK;
}
