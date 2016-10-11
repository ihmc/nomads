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
 * aviinit.c --
 *
 *      This file contains the code to make the RVM avi DLL for Tcl.
 *
 */

#include "tclAviInt.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmavi_Init) _ANSI_ARGS_((Tcl_Interp *interp));


static Commands avicmd[] =
{
    /* AVI File */

    { "avi_file_open", AviFileOpenCmd, NULL, NULL, },
    { "avi_file_close", AviFileCloseCmd, NULL, NULL, },
    { "avi_file_create", AviFileCreateCmd, NULL, NULL, },
    { "avi_file_get_num_streams", AviFileFieldCmd, (ClientData)AVIHDR_NUMSTREAMS, NULL, },
    { "avi_file_get_length", AviFileFieldCmd, (ClientData)AVIHDR_LENGTH, NULL, },

    /* AVI Stream Header */

    { "avi_stream_open", AviStreamOpenCmd, NULL, NULL, },
    { "avi_stream_close", AviStreamCloseCmd, NULL, NULL, },
    { "avi_video_stream_create", AviVideoStreamCreateCmd, NULL, NULL, },
    { "avi_audio_stream_create", AviAudioStreamCreateCmd, NULL, NULL, },
    { "avi_stream_start_decode", AviStreamStartDecodeCmd, NULL, NULL, },
    { "avi_stream_stop_decode", AviStreamStopDecodeCmd, NULL, NULL, },
    { "avi_stream_get_type", AviStreamFieldCmd, (ClientData)AVISHDR_TYPE, NULL, },
    { "avi_stream_get_codec", AviStreamFieldCmd, (ClientData)AVISHDR_COMP, NULL, },
    { "avi_stream_get_width", AviStreamFieldCmd, (ClientData)AVISHDR_WIDTH, NULL, },
    { "avi_stream_get_height", AviStreamFieldCmd, (ClientData)AVISHDR_HEIGHT, NULL, },
    { "avi_stream_get_fps", AviStreamFieldCmd, (ClientData)AVISHDR_FPS, NULL, },
    { "avi_stream_get_length", AviStreamFieldCmd, (ClientData)AVISHDR_LENGTH, NULL, },
    { "avi_stream_get_start", AviStreamFieldCmd, (ClientData)AVISHDR_START, NULL, },


    /* AVI Misc functions */

    { "avi_get_num_of_codecs", AviGetNumOfCodecsCmd, NULL, NULL, },
    { "avi_codec_info", AviCodecInfoCmd, NULL, NULL, },

    /* AVI Video Frames */

    { "avi_video_frame_read", AviVideoFrameReadCmd, NULL, NULL, },
    { "avi_video_frame_write", AviVideoFrameWriteCmd, NULL, NULL, },
    { "avi_video_frame_skip", AviVideoFramePosCmd, (ClientData)AVIFRAME_SKIP, NULL, },
    { "avi_video_frame_tell", AviVideoFramePosCmd, (ClientData)AVIFRAME_TELL, NULL, },
    { "avi_video_frame_rewind", AviVideoFramePosCmd, (ClientData)AVIFRAME_REWIND, NULL, },
    { "avi_video_frame_seek", AviVideoFrameSeekCmd, NULL, NULL, },

    /* AVI Audio Frames */
    { "avi_audio_frame_write", AviAudioFrameWriteCmd, NULL, NULL, },

    /* Audio conversion */
    { "audioconv_new", AudioConvNewCmd, NULL, NULL, },
    { "audioconv_free", AudioConvFreeCmd, NULL, NULL, },
    { "audioconv_encode", AudioConvEncodeCmd, NULL, NULL, },
    { "wave_hdr_new_from_audioconv", WaveHdrNewFromAudioConvCmd, NULL, NULL, },

};

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *      This wrapper function is used by Windows to invoke the
 *      initialization code for the DLL.  If we are compiling
 *      with Visual C++, this routine will be renamed to DllMain.
 *      routine.
 *
 * Results:
 *      Returns TRUE;
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;            /* Library instance handle. */
    DWORD reason;               /* Reason this function is being called. */
    LPVOID reserved;            /* Not used. */
{
    return TRUE;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Rvmavi_Init --
 *
 *      This procedure initializes the rvm avi library.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmavi_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading Tcldvmbasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, avicmd, sizeof(avicmd));

    return Tcl_PkgProvide(interp, "DvmAvi", "1.0");
}
