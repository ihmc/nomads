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
 * waveinit.c --
 *
 *      This file contains the code to make the Dali DV wave DLL for Tcl.
 *
 */

#include "tclDvmWave.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmwave_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands wavecmd[] =
{
    /*
     * wave header commands
     */
    { "wave_hdr_new", WaveHdrNewCmd, NULL, NULL, },
    { "wave_hdr_parse", WaveHdrParseCmd, NULL, NULL, },
    { "wave_hdr_free", WaveHdrFreeCmd, NULL, NULL, },
    { "wave_hdr_encode", WaveHdrEncodeCmd, NULL, NULL, },

    { "wave_hdr_get_format", WaveHdrGetFormatCmd, NULL, NULL, },
    { "wave_hdr_get_num_of_chan", WaveHdrGetNumOfChanCmd, NULL, NULL, },
    { "wave_hdr_get_samples_per_sec", WaveHdrGetSamplesPerSecCmd, NULL, NULL, },
    { "wave_hdr_get_bytes_per_sec", WaveHdrGetBytesPerSecCmd, NULL, NULL, },
    { "wave_hdr_get_block_align", WaveHdrGetBlockAlignCmd, NULL, NULL, },
    { "wave_hdr_get_bits_per_sample", WaveHdrGetBitsPerSampleCmd, NULL, NULL, },
    { "wave_hdr_get_data_len", WaveHdrGetDataLenCmd, NULL, NULL, },

    { "wave_hdr_set_format", WaveHdrSetFormatCmd, NULL, NULL, },
    { "wave_hdr_set_num_of_chan", WaveHdrSetNumOfChanCmd, NULL, NULL, },
    { "wave_hdr_set_samples_per_sec", WaveHdrSetSamplesPerSecCmd, NULL, NULL, },
    { "wave_hdr_set_bytes_per_sec", WaveHdrSetBytesPerSecCmd, NULL, NULL, },
    { "wave_hdr_set_block_align", WaveHdrSetBlockAlignCmd, NULL, NULL, },
    { "wave_hdr_set_bits_per_sample", WaveHdrSetBitsPerSampleCmd, NULL, NULL, },
    { "wave_hdr_set_data_len", WaveHdrSetDataLenCmd, NULL, NULL, },
    
#ifdef WIN32
    /*
     * wave output device functions
     */
    { "wave_out_open", WaveOutOpenCmd, NULL, NULL, },
    { "wave_out_close", WaveOutCloseCmd, NULL, NULL, },
    { "wave_audio_prep_play", WaveAudioPrepPlayCmd, NULL, NULL, },
    { "wave_audio_play", WaveAudioPlayCmd, NULL, NULL, },
    { "wave_out_done", WaveOutDoneCmd, NULL, NULL, },
#endif  //WIN32

    /*
     * wave i/o functions == audio i/o functions,
     * since after the header part is read, 
     * only raw audio data is left
     */
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
 * Dvmwave_Init --
 *
 *      This procedure initializes the rvm command.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int, Tcldvmwave_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, wavecmd, sizeof(wavecmd));
    return Tcl_PkgProvide(interp, "DvmWave", "1.0");
}
