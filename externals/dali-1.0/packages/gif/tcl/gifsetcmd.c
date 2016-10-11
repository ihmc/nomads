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
GifSeqHdrSetWidthCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr width", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetWidth(gifHdr, val);

    return TCL_OK;
}

int
GifSeqHdrSetHeightCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr height", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetHeight(gifHdr, val);

    return TCL_OK;
}

int
GifSeqHdrSetCtFlagCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr ctFlag", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetCtFlag(gifHdr, val);
    return TCL_OK;
}



int
GifSeqHdrSetCtSizeCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr ctSize", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetCtSize(gifHdr, val);
    return TCL_OK;
}

int
GifSeqHdrSetCtSortedCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr ctSorted", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetCtSorted(gifHdr, val);
    return TCL_OK;
}


int
GifSeqHdrSetResolutionCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr resolution", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetResolution(gifHdr, val);
    return TCL_OK;
}

int
GifSeqHdrSetBackgroundColorCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr background_color", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetBackgroundColor(gifHdr, val);
    return TCL_OK;
}

int
GifSeqHdrSetAspectRatioCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr aspectRatio", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifSeqHdrSetAspectRatio(gifHdr, val);
    return TCL_OK;
}


int
GifSeqHdrSetVersionCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s seqHdr version", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifSeqHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF seq header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    version = argv[2];
    if (strlen(version) != 3) {
        sprintf(interp->result, "%s: version must be exactly 3 characters long", argv[0]);
        return TCL_ERROR;
    }

    GifSeqHdrSetVersion(gifHdr, version);

    return TCL_OK;
}




/*
 *
 * Gif Img
 *
 */

int
GifImgHdrSetWidthCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr width", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetWidth(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetHeightCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr height", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetHeight(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetCtFlagCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr ctFlag", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetCtFlag(gifHdr, val);
    return TCL_OK;
}


int
GifImgHdrSetCtSizeCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr ctSize", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetCtSize(gifHdr, val);
    return TCL_OK;
}


int
GifImgHdrSetLeftPositionCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr leftPosition", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetLeftPosition(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetTopPositionCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr topPosition>", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetTopPosition(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetInterlacedCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr interlaced", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetInterlaced(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetGraphicControlFlagCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr graphicsControl_flag", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetGraphicControlFlag(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetDisposalMethodCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr disposalMethod", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetDisposalMethod(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetUserInputFlagCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr userInputFlag", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetUserInputFlag(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetTransparentColorFlagCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr transparent_color_flag", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetTransparentColorFlag(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetDelayTimeCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr delayTime", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetDelayTime(gifHdr, val);
    return TCL_OK;
}

int
GifImgHdrSetTransparentColorIndexCmd (clientData, interp, argc, argv)
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

    if (argc != 3) {
        sprintf(interp->result, "wrong # of args: should be %s imgHdr transparent_color_index", argv[0]);
        return TCL_ERROR;
    }

    gifHdr = GetGifImgHdr (argv[1]);
    if (gifHdr == NULL) {
        sprintf(interp->result, "%s: no such GIF img header %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    GifImgHdrSetTransparentColorIndex(gifHdr, val);
    return TCL_OK;
}


