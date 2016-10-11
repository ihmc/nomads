/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * byte16cmd.c
 *
 * Tcl hooks to byte16 image functions. 
 *
 * weitsang Jan 99
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

int
Byte16NewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *new;
    int status, w, h;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s width height", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    new = Byte16New(w, h);
    PutByte16Image(interp, new);

    return TCL_OK;
}

int
Byte16FreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *img;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s byte16Image", argv[0]);

    img = GetByte16Image(argv[1]);
    ReturnErrorIf2(img == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    Byte16Free(img);
    return TCL_OK;
}

int
Byte16ClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *buf, *new;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s byte16Image x y width height", argv[0]);

    buf = GetByte16Image(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    new = Byte16Clip(buf, x, y, w, h);
    PutByte16Image(interp, new);

    return TCL_OK;
}

int
Byte16ReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *buf, *clipped;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s byte16Image x y width height clipped",
        argv[0]);

    buf = GetByte16Image(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    clipped = GetByte16Image(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[6]);

    Byte16Reclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}


int
Byte16CopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcByte16Image destByte16Image", argv[0]);

    src = GetByte16Image(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    dest = GetByte16Image(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[2]);

    Byte16Copy(src, dest);
    return TCL_OK;
}


int
Byte16GetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte16GetX(v));
    return TCL_OK;
}


int
Byte16GetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte16GetY(v));
    return TCL_OK;
}


int
Byte16GetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte16GetWidth(v));
    return TCL_OK;
}


int
Byte16GetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte16GetHeight(v));
    return TCL_OK;
}


int
Byte16GetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", Byte16GetVirtual(v));
    return TCL_OK;
}


int
Byte16InfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Byte16Image *v;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byte16Image", argv[0]);

    v = GetByte16Image(argv[1]);
    ReturnErrorIf2(v == NULL,
        "%s : no such byte16 image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d", v->x, v->y, v->width,
        v->height, v->isVirtual);

    return TCL_OK;
}
