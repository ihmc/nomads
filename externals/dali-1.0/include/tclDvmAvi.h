/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef TCL_DVMAVI_H_
#define TCL_DVMAVI_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>
#undef WIN32_LEAN_AND_MEAN

#include "tclDvmbasic.h"
#include "tclDvmwave.h"
#include "dvmavi.h"
#ifdef __cplusplus
extern "C" {
#endif

/* AVI Header */
int AviFileOpenCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviFileCloseCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviFileCreateCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviFileFieldCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* AVI Stream Header */
int AviStreamOpenCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviStreamCloseCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviVideoStreamCreateCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviAudioStreamCreateCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviStreamStartDecodeCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviStreamStopDecodeCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviStreamFieldCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* AVI Misc functions */
int AviGetNumOfCodecsCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviCodecInfoCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* AVI Video Frames */
int AviVideoFrameReadCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviVideoFrameWriteCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviVideoFramePosCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AviVideoFrameSeekCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* AVI Audio Frames */
int AviAudioFrameWriteCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* AudioConv routines */
int AudioConvNewCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AudioConvFreeCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int AudioConvEncodeCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
int WaveHdrNewFromAudioConvCmd (ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#define AVI_FILE_PREFIX "aviFile__"
#define GetAviFile(s) (!strncmp(s,AVI_FILE_PREFIX,9)? GetBuf(s):NULL)
#define RemoveAviFile(s) (!strncmp(s,AVI_FILE_PREFIX,9)? RemoveBuf(s):NULL)
#define PutAviFile(interp, buf) PutBuf(interp, AVI_FILE_PREFIX, buf)

#define AVI_STREAM_PREFIX "aviStream"
#define GetAviStream(s) (!strncmp(s,AVI_STREAM_PREFIX,9)? GetBuf(s):NULL)
#define RemoveAviStream(s) (!strncmp(s,AVI_STREAM_PREFIX,9)?RemoveBuf(s):NULL)
#define PutAviStream(interp, buf) PutBuf(interp, AVI_STREAM_PREFIX, buf)

#define AUDIO_CONV_PREFIX "audioConv"
#define GetAudioConv(s) (!strncmp(s,AUDIO_CONV_PREFIX,9)? GetBuf(s):NULL)
#define RemoveAudioConv(s) (!strncmp(s,AUDIO_CONV_PREFIX,9)?RemoveBuf(s):NULL)
#define PutAudioConv(interp, buf) PutBuf(interp, AUDIO_CONV_PREFIX, buf)

#ifdef __cplusplus
extern "C" {
#endif
#endif

