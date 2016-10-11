/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _TCL_DVM_WAVE_H_
#define _TCL_DVM_WAVE_H_

#include "tclDvmBasic.h"
#include "dvmwave.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for WAVE package.
 * It only contains a set of wave header commands for reading header
 * chunk of wave file
 *
 * Haye Chan Jan 98
 *---------------------------------------------------------------------
 */

/* header i/o and memory management commands */
    int WaveHdrNewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrParseCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrFreeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrEncodeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* slot access commands */
    int WaveHdrGetFormatCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetNumOfChanCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetSamplesPerSecCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetBytesPerSecCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetBlockAlignCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetBitsPerSampleCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrGetDataLenCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* slot set commands */
    int WaveHdrSetFormatCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetNumOfChanCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetSamplesPerSecCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetBytesPerSecCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetBlockAlignCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetBitsPerSampleCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveHdrSetDataLenCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#ifdef WIN32
/* wave audio output commands */
    int WaveOutOpenCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveOutCloseCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveAudioPrepPlayCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveAudioPlayCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int WaveOutDoneCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
#endif                          //WIN32

#define WAVE_HDR_PREFIX "dvmWavHdr"
#define GetWaveHdr(s) (!strncmp(s, WAVE_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveWaveHdr(s) (!strncmp(s, WAVE_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutWaveHdr(interp, buf) PutBuf(interp, WAVE_HDR_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
