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
 * aviaudiocmd.c
 *
 * Functions that deal with avi audio (tcl interface)
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_audio_frame_write <stream> <audiobuf>
 *
 * precond 
 *     AVI stream stream exists
 *     The audio buffer audiobuf has the right sample size 
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
AviAudioFrameWriteCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviStream *str;
    Audio *audio;
    int error, wsamples;

    ReturnErrorIf1 (argc != 3,
                 "wrong # args: should be %s <stream> <audiobuf>", argv[0]);
    str = GetAviStream(argv[1]);
    ReturnErrorIf2 (str == NULL, "%s: no such avi stream %s", argv[0], argv[1]);
    audio = GetAudio (argv[2]);
    ReturnErrorIf2 (audio==NULL,  "%s: invalid Audio %s", argv[0], argv[2]);

    error = AviAudioFrameWrite (str, audio, &wsamples);
    ReturnErrorIf2 (error, "%s: error writing audio frame -- %s",
                    argv[0], AviTranslateError (error));

    sprintf(interp->result, "%d", wsamples);
    return TCL_OK;}
