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
 * rvmaudiomap.h
 *
 * This file contains the definition of buffers for audio maps of audio
 * buffers.
 *
 *----------------------------------------------------------------------
 */

#ifndef _TCL_DVM_AUDIOMAP_H_
#define _TCL_DVM_AUDIOMAP_H_

#include "tclDvmBasic.h"
#include "dvmamap.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for AudioMap package.
 * It contains a set of commmands for AudioMaps manipulation
 * and some built-in mappings
 * These maps can be applied to Audio buffer
 *
 * Haye Chan Jan 98
 *---------------------------------------------------------------------
 */

/* create a new map (AudioMapNewCmd.c) */
    int AudioMap8To8NewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16NewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8NewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16NewCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* free up a map variable (AudioMapFreeCmd.c) */
    int AudioMapFreeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* duplicate a map variable (AudioMapCopyCmd.c) */
    int AudioMap8To8CopyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16CopyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8CopyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16CopyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* get the map values in list form, from a 8-to-8 map variable */
/* un-documented commands */
    int AudioMap8To8ValuesCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* compose two maps (AudioMapComposeCmd.c) */
    int AudioMap8To88To8ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To88To16ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To88To8ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To88To16ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To1616To8ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To1616To16ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To1616To8ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To1616To16ComposeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* apply a map variable to an audio buffer (AudioMapApplyCmd.c) */
    int AudioMap8To8ApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To8ApplySomeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16ApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16ApplySomeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8ApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8ApplySomeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16ApplyCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16ApplySomeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* info slots (AudioMapInfoCmd.c) */
    int AudioMapGetSrcResCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMapGetDestResCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To8GetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16GetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8GetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16GetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* set slots (AudioMapSetCmd.c) */
    int AudioMap8To8SetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16SetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8SetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16SetValueCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* create a new map from a list (AudioMapInitCustomCmd.c) */
    int AudioMap8To8InitCustomCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16InitCustomCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8InitCustomCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16InitCustomCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* create an identity map (AudioMapInitBuiltinCmd.c) */
    int AudioMap8To8InitIdentityCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16InitIdentityCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16InitVolumeCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* create an inverse identity map (AudioMapInitBuiltinCmd.c) */
    int AudioMap8To8InitComplementCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To16InitComplementCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* create mapping between Big-endian and Little-Endian audio data (AudioMapInitBuiltinCmd.c) */
    int AudioMap16To16InitBigLittleSwapCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

/* create a-law or u-law manipulation map (AudioMapInitBuiltinCmd.c) */
    int AudioMap8To16InitULawToLinearCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap8To16InitALawToLinearCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8InitLinearToULawCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);
    int AudioMap16To8InitLinearToALawCmd(ClientData cd, Tcl_Interp * interp, int argc, char *argv[]);

#define AUDIO_MAP_PREFIX "dvmAudMap"
#define GetAudioMap(s) (!strncmp(s, AUDIO_MAP_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveAudioMap(s) (!strncmp(s, AUDIO_MAP_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutAudioMap(interp, buf) PutBuf(interp, AUDIO_MAP_PREFIX, buf)
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
