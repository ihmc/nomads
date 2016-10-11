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
 * avivideo.c
 *
 * Functions that deal with avi video
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_video_frame_read <stream> <r> <g> <b>
 *
 * precond 
 *     AVI stream stream exists
 *     The bytebuffers rplane,gplane,bplane exist and have the right
 *     dimensions.
 *      
 * return 
 *     the rgb data in the buffers
 * 
 * side effect :
 *     the frame pointer is advanced, so the next call will retrieve
 *     the next frame
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFrameReadCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    ByteImage *redBuf,*greenBuf,*blueBuf;
    int error;

    ReturnErrorIf1 (argc != 5,
                 "wrong # args: should be %s <stream> <r> <g> <b>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL, "%s: no such avi stream %s", argv[0], argv[1]);
    redBuf = GetByteImage (argv[2]);
    greenBuf = GetByteImage (argv[3]);
    blueBuf = GetByteImage (argv[4]);
    ReturnErrorIf2 (redBuf==NULL,   "%s: invalid ByteImage %s", argv[0], argv[1]);
    ReturnErrorIf2 (greenBuf==NULL, "%s: invalid ByteImage %s", argv[0], argv[1]);
    ReturnErrorIf2 (blueBuf==NULL,  "%s: invalid ByteImage %s", argv[0], argv[1]);

    error = AviVideoFrameRead (str, redBuf, greenBuf, blueBuf);
    ReturnErrorIf2 (error, "%s: error reading video frame -- %s",
                    argv[0], AviTranslateError (error));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_video_frame_write <stream> <r> <g> <b>
 *
 * precond 
 *     AVI stream stream exists
 *     The bytebuffers rplane,gplane,bplane exist and have the right
 *     dimensions.
 *      
 * return 
 *     none
 * 
 * side effect :
 *     The data is written at the next position in the stream
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFrameWriteCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    ByteImage *redBuf,*greenBuf,*blueBuf;
    int error;

    ReturnErrorIf1 (argc != 5,
                 "wrong # args: should be %s <stream> <r> <g> <b>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL, "%s: no such avi stream %s", argv[0], argv[1]);
    redBuf = GetByteImage (argv[2]);
    greenBuf = GetByteImage (argv[3]);
    blueBuf = GetByteImage (argv[4]);
    ReturnErrorIf2 (redBuf==NULL,   "%s: invalid ByteImage %s", argv[0], argv[1]);
    ReturnErrorIf2 (greenBuf==NULL, "%s: invalid ByteImage %s", argv[0], argv[1]);
    ReturnErrorIf2 (blueBuf==NULL,  "%s: invalid ByteImage %s", argv[0], argv[1]);

    error = AviVideoFrameWrite (str, redBuf, greenBuf, blueBuf);
    ReturnErrorIf2 (error, "%s: error writing video frame -- %s",
                    argv[0], AviTranslateError (error));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_video_frame_skip <stream>
 *     avi_video_frame_rewind <stream>
 *     avi_video_frame_tell <stream>
 *
 * precond 
 *      avi stream <stream> exists
 *      
 * return 
 *     none
 * 
 * side effect :
 *      just sets the frame indicator. returns the new value.
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFramePosCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    int field,value;

    ReturnErrorIf1 (argc != 2, "wrong # args: should be %s <stream>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL, "%s: no such avi stream %s", argv[0], argv[1]);

    field = (int)clientData;
    switch (field) {
    case AVIFRAME_SKIP:
        value = AviVideoFrameSkip (str);
        break;
    case AVIFRAME_REWIND:
        value = AviVideoFrameRewind (str);
        break;
    case AVIFRAME_TELL:
        value = AviVideoFrameTell (str);
        break;
    default:
        ReturnErrorIf1 (1, "%s: internal errror", argv[0]);
    }

    sprintf(interp->result,"%d",value);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_video_frame_seek <stream> <fnum>
 *
 * precond 
 *     <stream> exists
 *      
 * return 
 *     The new frame position
 * 
 * side effect :
 *     The value of the frame position is set to <fnum>
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFrameSeekCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    int fnum;

    ReturnErrorIf1 (argc != 3, "wrong # args: should be %s stream framenum", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL, "%s: no such avi stream %s", argv[0], argv[1]);
    ReturnErrorIf2 ((Tcl_GetInt(interp, argv[2], &fnum) != TCL_OK) || (fnum < 0),
                    "%s: expected positive int got %s", argv[0], argv[2]);
    AviVideoFrameSeek (str, fnum);
    sprintf (interp->result, "%d", fnum);
    return TCL_OK;
}
