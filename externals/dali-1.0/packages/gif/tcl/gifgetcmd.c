/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmGif.h"

int
GifSeqHdrGetWidthCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;

    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetWidth(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetHeightCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;
    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetHeight(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetCtFlagCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetCtFlag(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}


int
GifSeqHdrGetCtSizeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetCtSize(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetCtSortedCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetCtSorted(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}


int
GifSeqHdrGetResolutionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetResolution(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetBackgroundColorCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetBackgroundColor(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetAspectRatioCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifSeqHdrGetAspectRatio(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifSeqHdrGetVersionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifSeqHdr* gifHdr;
    char *version;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    version = GifSeqHdrGetVersion(gifHdr);

    sprintf(interp->result, "%s", version);
    return TCL_OK;
}




/*
 *
 * Gif Img
 *
 */

int
GifImgHdrGetWidthCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;

    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetWidth(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetHeightCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;
    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetHeight(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetCtFlagCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetCtFlag(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}


int
GifImgHdrGetCtSizeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetCtSize(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetLeftPositionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetLeftPosition(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetTopPositionCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetTopPosition(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetInterlacedCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetInterlaced(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetGraphicControlFlagCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetGraphicControlFlag(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetDisposalMethodCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetDisposalMethod(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetUserInputFlagCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetUserInputFlag(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetTransparentColorFlagCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetTransparentColorFlag(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetDelayTimeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetDelayTime(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

int
GifImgHdrGetTransparentColorIndexCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    GifImgHdr* gifHdr;
    int val;

    /*
     * Retrive the file and buffer from the hashtables
     */

    if (argc != 2) {
        sprintf(interp->result, "wrong # of args: should be %s <gif hdr>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    val = GifImgHdrGetTransparentColorIndex(gifHdr);

    sprintf(interp->result, "%d", val);
    return TCL_OK;
}

