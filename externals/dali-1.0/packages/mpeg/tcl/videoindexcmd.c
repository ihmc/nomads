/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmMpeg.h"
#include "dvmmpeg.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_new size
 * return 
 *     a new mpeg video index object
 * side effect 
 *     memory is allocated for mpeg b\video index
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int size;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s tableSize", argv[0]);


    if ((Tcl_GetInt(interp, argv[1], &size) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = MpegVideoIndexNew(size);
    PutMpegVideoIndex(interp, vi);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_free video_index
 * precond 
 *     video_index is an MPEG video index
 * return
 *     none
 * side effect :
 *     video index is destroyed
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s videoIndex", argv[0]);

    vi = RemoveMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    MpegVideoIndexFree(vi);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_parse video_index bitparser
 * precond 
 *     video_index is an MPEG video index
 * return
 *     none
 * side effect :
 *     video index is read into a bitstream
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexParseCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    BitParser *bp;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser videoIndex", argv[0]);

    vi = GetMpegVideoIndex(argv[2]);
    bp = GetBitParser(argv[1]);

    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    MpegVideoIndexParse(bp, vi);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_encode video_index bitparser
 * precond 
 *     video_index is an MPEG video index
 *     bitparser is at the head of an MPEG video index
 * return
 *     none
 * side effect :
 *     video index is written to a bitstream
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexEncodeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    BitParser *bp;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex FILE", argv[0]);

    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    MpegVideoIndexEncode(vi, bp);
    return TCL_OK;
}

/*
 *-----------------------------------------------------------------------
 *
 * usage
 *     mpeg_video_index_numrefs video_index framenum
 * precond 
 *     video_index is an MPEG video index
 * return
 *     returns the number of frames needed to decode a given frame n
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexNumRefsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegVideoIndexNumRefs(vi, frameNum));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_findrefs video_index1 video_index2 framenum
 * precond 
 *     video_index1 is an MPEG video index 
 *     video_index2 is an empty MPEG video index
 * return
 *     none
 * side effect :
 *     All necessary frames to decode framenum are copied from 
 *     video_index1 to video_index2
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexFindRefsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi1, *vi2;
    int frameNum;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s inVideoIndex outVideoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[3], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi1 = GetMpegVideoIndex(argv[1]);
    vi2 = GetMpegVideoIndex(argv[2]);
    ReturnErrorIf2(vi1 == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);
    ReturnErrorIf2(vi2 == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[2]);

    MpegVideoIndexFindRefs(vi1, vi2, frameNum);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_get_type video_index framenum
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     type (p, b, i) of framenum
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexGetTypeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%c", MpegVideoIndexGetType(vi, frameNum));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_get_offset video_index framenum
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     bit offset of bitparsr at current location
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexGetOffsetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegVideoIndexGetOffset(vi, frameNum));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_get_length video_index framenum
 * precond 
 *     video_index is an MPEG video index 
 *     assume bitparser is located at the beginning of a pic hdr
 * return
 *     length of current pic frame 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexGetLengthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegVideoIndexGetLength(vi, frameNum));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_table_add video_index displaynum bitOffset
 *                                     type length pastFrame futFrame
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     none
 * side effect :
 *     video_index is appended with data from an MPEG file
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexTableAddCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum, offset, len, past, next;

    ReturnErrorIf1(argc != 8,
        "wrong # args: should be %s videoIndex frameNum bitOffset type len pastoffset nextoffset", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &offset) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[5], &len) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[6], &past) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[7], &next) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    if (!strcmp(argv[4], "i")) {
        MpegVideoIndexTableAdd(vi, frameNum, offset, I_FRAME, len, past, next);
    } else if (!strcmp(argv[4], "p")) {
        MpegVideoIndexTableAdd(vi, frameNum, offset, P_FRAME, len, past, next);
    } else if (!strcmp(argv[4], "b")) {
        MpegVideoIndexTableAdd(vi, frameNum, offset, B_FRAME, len, past, next);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_resize video_index newSize
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     none
 * side effect :
 *     video_index is resized to accomodate more elements to it's table
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexResizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int newsize;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex newSize", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &newsize) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    MpegVideoIndexResize(vi, newsize);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_get_past video_index framenum
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     past offset of framenum
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexGetPastCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegVideoIndexGetPast(vi, frameNum));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_video_index_get_next video_index framenum
 * precond 
 *     video_index is an MPEG video index 
 * return
 *     nextoffset of framenum
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
MpegVideoIndexGetNextCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegVideoIndex *vi;
    int frameNum;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s videoIndex frameNum", argv[0]);

    if ((Tcl_GetInt(interp, argv[2], &frameNum) != TCL_OK)) {
        return TCL_ERROR;
    }
    vi = GetMpegVideoIndex(argv[1]);
    ReturnErrorIf2(vi == NULL,
        "%s: no such MpegVideoIndex %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegVideoIndexGetNext(vi, frameNum));
    return TCL_OK;
}
