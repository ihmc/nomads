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
 * tcl/avistream.c
 *
 * Tcl interface to functions that manipulate avi streams
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_stream_open <aviFile> <num>
 *
 * precond 
 *     AVI File aviFile exists
 *     num lies between 0 and numstreams-1
 *      
 * return 
 *     A avistream handle, to be used to read frames from the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use avi_stream_close to free this.
 *
 *----------------------------------------------------------------------
 */

int
AviStreamOpenCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    AviStream *str;
    int streamnum;
    int error;

    /*
     * Check args
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s <aviFile> <num>", argv[0]);
    aviFile = GetAviFile(argv[1]);
    ReturnErrorIf2 (aviFile == NULL,
        "%s: no such avi header %s", argv[0], argv[1]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[2], &streamnum) != TCL_OK,
        "%s: expected int got %s", argv[0], argv[2]);

    /*
     * Do the work, check return code
     */
    error = AviStreamOpen (aviFile, streamnum, &str);
    ReturnErrorIf3 (error, "%s: bad stream number %d -- %s",
                    argv[0], streamnum, AviTranslateError (error));
    PutAviStream (interp, str);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_stream_close <stream>
 *
 * precond 
 *      avi stream <stream> exists
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the memory allocated for the avi stream <stream> is freed
 *
 *----------------------------------------------------------------------
 */

int
AviStreamCloseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;

    /*
     * Check args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <stream>", argv[0]);
    str = RemoveAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL,
        "%s: no such avi stream %s", argv[0], argv[1]);

    AviStreamClose(str);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_stream_start_decode <stream>
 *
 * precond 
 *     <stream> exists
 *      
 * return 
 *     The stream decode is readied
 * 
 * side effect :
 *     memory is allocated for a a getframe object.
 *     Use avi_stream_stop_decode to finish up
 *
 *----------------------------------------------------------------------
 */

int
AviStreamStartDecodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    int error;

    /*
     * Check args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <stream>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL,
        "%s: no such avi stream %s", argv[0], argv[1]);

    /*
     * Do the work, check return code 
     */
    error = AviStreamStartDecode (str);
    ReturnErrorIf2 (error, "%s: %s", argv[0], AviTranslateError (error));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_stream_stop_decode <stream>
 *
 * precond 
 *     <stream> exists
 *      
 * return 
 *     The stream decode is unreadied
 * 
 * side effect :
 *     memory allocated for the getframe is freed
 *
 *----------------------------------------------------------------------
 */

int
AviStreamStopDecodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <stream>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL,
        "%s: no such avi stream %s", argv[0], argv[1]);

    AviStreamStopDecode (str);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_stream_type <stream>
 *     avi_stream_comp <stream>
 *     avi_stream_width <stream>
 *     avi_stream_height <stream>
 *     avi_stream_fps <stream>
 *
 * precond 
 *     AVI stream <stream> exists.
 *
 * return 
 *     The corresponding field in the stream
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
AviStreamFieldCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    int field, value;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <stream>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL,
        "%s: no such avi stream %s", argv[0], argv[1]);

    field = (int)clientData;
    switch (field) {
    case AVISHDR_TYPE: 
        if (str->type == AVI_STREAM_VIDEO)
            Tcl_SetResult(interp,"video",TCL_STATIC);
        else if (str->type == AVI_STREAM_AUDIO)
            Tcl_SetResult(interp,"audio",TCL_STATIC);
        else
            Tcl_SetResult(interp,"other",TCL_STATIC);
        return TCL_OK;
    case AVISHDR_COMP: {
        char codec[5];
        codec[0] = (str->codec & 0xff);
        codec[1] = (str->codec & 0xff00)>>8;
        codec[2] = (str->codec & 0xff0000)>>16;
        codec[3] = (str->codec & 0xff000000)>>24;
        codec[4] = '\0';
        Tcl_SetResult(interp, codec, TCL_VOLATILE);
        return TCL_OK;
    }
    case AVISHDR_LENGTH: 
        sprintf(interp->result, "%d", str->length);
        return TCL_OK;
    case AVISHDR_START: 
        sprintf(interp->result, "%d", str->start);
        return TCL_OK;
    default:
        break;
    }

    if (str->type == AVI_STREAM_VIDEO) {
        AviVideoStream *viddata;
        viddata = (AviVideoStream *)(str->data);
        switch (field) {
        case AVISHDR_FPS:
            value = viddata->fps;
            break;
        case AVISHDR_WIDTH:
            value = viddata->width;
            break;
        case AVISHDR_HEIGHT:
            value = viddata->height;
            break;
        default:
            ReturnErrorIf2(1, "%s: stream %s is a video stream",
                    argv[0], argv[1]);
        }
        sprintf(interp->result, "%d", value);
        return TCL_OK;
    } else { /* type audio */
        /* We dont support audio right now */
        ReturnErrorIf2(1, "%s: stream %s is an audio stream", argv[0], argv[1]);
    }
    return TCL_OK;  /* Just to quiet warnings -- we can't get here */
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_video_stream_create <avifile> <codec> <w> <h> <fps> <keyinterval>
 *                             <quality> <bitrate>
 *
 * precond 
 *     AVI File file exists
 *     type is either audio or video
 *      
 * return 
 *     A avistream handle, to be used to write frames to the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use avi_stream_free to free this.
 *
 *----------------------------------------------------------------------
 */
int
AviVideoStreamCreateCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    AviStream *str;
    int w, h, codec, fps;
        int keyinterval, quality, bitrate;
        int error;

    ReturnErrorIf1 (argc != 9,
        "wrong # args: should be %s <avifile> <codec> <w> <h> <fps> <keyinterval> <quality> <bitrate>", argv[0]);

    aviFile = GetAviFile(argv[1]);
    ReturnErrorIf2 (aviFile == NULL, "%s: no such avi file %s", argv[0], argv[1]);

    ReturnErrorIf2 (strlen(argv[2]) != 4,
         "%s: bad codec specifier %s", argv[0], argv[3]);
    codec = mmioFOURCC(argv[2][0], argv[2][1], argv[2][2], argv[2][3]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[3], &w) != TCL_OK,
        "%s: bad width %s", argv[0], argv[3]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[4], &h) != TCL_OK,
        "%s: bad height %s", argv[0], argv[4]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[5], &fps) != TCL_OK,
        "%s: bad frame rate %s", argv[0], argv[5]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[6], &keyinterval) != TCL_OK,
        "%s: bad key interval %s", argv[0], argv[6]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[7], &quality) != TCL_OK,
        "%s: bad quality specifier %s", argv[0], argv[7]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[8], &bitrate) != TCL_OK,
        "%s: bad bitrate %s", argv[0], argv[8]);

    /*
     * Create new stream header
     */
    error = AviVideoStreamCreate (aviFile, codec, w, h, fps,
                               keyinterval, quality, bitrate, &str);
    ReturnErrorIf2 (error, "%s: failed to create new stream -- %s",
                    argv[0], AviTranslateError (error));
    PutAviStream(interp, str);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_audio_stream_create <avifile>
 *
 * precond 
 *     AVI File file exists
 *      
 * return 
 *     A avistream handle, to be used to write frames to the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use avi_stream_free to free this.
 *
 *----------------------------------------------------------------------
 */
int
AviAudioStreamCreateCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    AviStream *str;
    int error;
    int numChan, bitsPerSample, samplesPerSec;


    ReturnErrorIf1 (argc != 5,
                    "wrong # args: should be %s <avifile> <numchan> <bits_per_sample> <samples_per_sec>", argv[0]);

    aviFile = GetAviFile(argv[1]);
    ReturnErrorIf2 (aviFile == NULL, "%s: no such avi file %s", argv[0], argv[1]);

    ReturnErrorIf2 (Tcl_GetInt(interp, argv[2], &numChan) != TCL_OK,
            "%s: bad numchannels %s", argv[0], argv[2]);

    ReturnErrorIf2 (Tcl_GetInt(interp, argv[3], &bitsPerSample) != TCL_OK,
            "%s: bad bitsPerSample %s", argv[0], argv[3]);

    ReturnErrorIf2 (Tcl_GetInt(interp, argv[4], &samplesPerSec) != TCL_OK,
            "%s: bad samplesPerSec %s", argv[0], argv[4]);


    /*
     * Create new stream header
     */
    error = AviAudioStreamCreate (aviFile, &str, (short)numChan, 
	    (short)bitsPerSample, samplesPerSec);
    ReturnErrorIf2 (error, "%s: failed to create new stream -- %s",
                    argv[0], AviTranslateError (error));
    PutAviStream(interp, str);
    return TCL_OK;
}
