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
 * waveoutcmd.c --
 *
 *      Tcl command to Wave Output Device control
 *
 */

#include "tclDvmWave.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_out_open <wavehdr>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     wave output audio device is opened for use
 *
 *----------------------------------------------------------------------
 */
int WaveOutOpenCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr* hdr;
    int status;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s wavehdr", argv[0]);
    hdr = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[2]);
        
    status = WaveOutOpen(hdr);
    ReturnErrorIf1(status == DVM_WAVE_ERROR,
        "%s: error initializing wave output device", argv[0]);
    ReturnErrorIf1(status == DVM_WAVE_ALREADY_OPEN,
        "%s: wave output device already open", argv[0]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_out_close
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     wave output audio device will be closed
 *
 *----------------------------------------------------------------------
 */
int WaveOutCloseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int status;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    status = WaveOutClose();
    ReturnErrorIf1(status == DVM_WAVE_ERROR,
        "%s: unable to close wave audio device", argv[0]);
    ReturnErrorIf1(status == DVM_WAVE_NOT_OPEN,
        "%s: wave audio device not open", argv[0]);
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_audio_prep_play <audio_buf>
 *
 * precond 
 *     wave_out_open has to be called prior to it
 *
 * return 
 *     none
 * 
 * side effect :
 *     prepare a wave header for playing <audio_buf>
 *
 *----------------------------------------------------------------------
 */
int WaveAudioPrepPlayCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    Audio *audio;
    int status;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <audio_buf>", argv[0]);

    audio = GetAudio(argv[1]);
    ReturnErrorIf1(audio == NULL,
        "no such audio %s\n", argv[1]);
  
    status = WaveAudioPrepPlay(audio);
    ReturnErrorIf1(status == DVM_WAVE_NOT_OPEN,
        "%s: wave audio device not open", argv[0]);
    ReturnErrorIf1(status == DVM_WAVE_ERROR,
        "%s: audio preparation unsuccessful\n", argv[0]);
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_audio_play <length>
 *
 * precond 
 *     nSamples an integer, representing the number of samples of
 *     the prepared audio buffer to play
 *
 * return 
 *     none
 * 
 * side effect :
 *     play the prepared buffer for <length> samples
 *
 *----------------------------------------------------------------------
 */
int WaveAudioPlayCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int status, length;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <length>", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &length);
    ReturnErrorIf(status != TCL_OK);
  
    status = WaveAudioPlay(length);
    ReturnErrorIf1(status == DVM_WAVE_ERROR,
        "%s: unable to close wave audio device", argv[0]);
    ReturnErrorIf1(status == DVM_WAVE_NOT_OPEN,
        "%s: wave audio device not open", argv[0]);
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_audio_done
 *
 * precond 
 *     none
 *
 * return 
 *     true (non-zero) if the wave audio played by previous call
 *     to wave_audio_play is done, false otherwise
 * 
 * side effect :
 *
 *----------------------------------------------------------------------
 */
int WaveOutDoneCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    sprintf(interp->result, "%d", WaveOutDone());
    return TCL_OK;
}
