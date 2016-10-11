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
 * audiomapapplycmd.c --
 *
 *        This file contains the tcl hook to the AudioMapApply C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_apply <map> <srcaudio> <destaudio>
 *
 * precond 
 *      <map> exists and is a 8-bit to 8-bit mapping
 *      <srcaudio> exists and is 8-bit audio
 *      <destaudio> exists and is 8-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8ApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <srcaudio> <destaudio>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    destAudio = GetAudio(argv[3]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To8Apply(map, srcAudio, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_apply_some <map> <srcaudio>
 *                              <src offset>  <src stride>
 *                              <dest offset> <dest stride> <destaudio>
 *
 * precond 
 *      <map> exists and is a 8-bit to 8-bit mapping
 *      <srcaudio> exists and is 8-bit audio
 *      <destaudio> exists and is 8-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>. The channel in effect can be specified by param
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8ApplySomeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;
    int srcOffset, srcStride, destOffset, destStride;
    int status;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 8,
        "wrong #args, should be %s map srcaudio src offset src stride dest offset dest stride destaudio", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);
    destAudio = GetAudio(argv[7]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[7]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To8ApplySome(map, srcAudio,
        srcOffset, srcStride, destOffset, destStride, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_apply <map> <srcaudio> <destaudio>
 *
 * precond 
 *      <map> exists and is a 8-bit to 16-bit mapping
 *      <srcaudio> exists and is 8-bit audio
 *      <destaudio> exists and is 16-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16ApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <srcaudio> <destaudio>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    destAudio = GetAudio(argv[3]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To16Apply(map, srcAudio, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_apply_some <map> <srcaudio>
 *                               <src offset>  <src stride>
 *                               <dest offset> <dest stride> <destaudio>
 *
 * precond 
 *      <map> exists and is a 8-bit to 16-bit mapping
 *      <srcaudio> exists and is 8-bit audio
 *      <destaudio> exists and is 16-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>. The channel in effect can be specified by param
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16ApplySomeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;
    int srcOffset, srcStride, destOffset, destStride;
    int status;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 8,
        "wrong #args, should be %s map srcaudio src offset src stride dest offset dest stride destaudio", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);
    destAudio = GetAudio(argv[7]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[7]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To16ApplySome(map, srcAudio,
        srcOffset, srcStride, destOffset, destStride, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_apply <map> <srcaudio> <destaudio>
 *
 * precond 
 *      <map> exists and is a 16-bit to 8-bit mapping
 *      <srcaudio> exists and is 16-bit audio
 *      <destaudio> exists and is 8-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8ApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <srcaudio> <destaudio>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    destAudio = GetAudio(argv[3]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To8Apply(map, srcAudio, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_apply_some <map> <srcaudio>
 *                               <src offset>  <src stride>
 *                               <dest offset> <dest stride> <destaudio>
 *
 * precond 
 *      <map> exists and is a 16-bit to 8-bit mapping
 *      <srcaudio> exists and is 16-bit audio
 *      <destaudio> exists and is 8-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>. The channel in effect can be specified by param
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8ApplySomeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;
    int srcOffset, srcStride, destOffset, destStride;
    int status;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 8,
        "wrong #args, should be %s map srcaudio src offset src stride dest offset dest stride destaudio", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);
    destAudio = GetAudio(argv[7]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[7]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To8ApplySome(map, srcAudio,
        srcOffset, srcStride, destOffset, destStride, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_apply <map> <srcaudio> <destaudio>
 *
 * precond 
 *      <map> exists and is a 16-bit to 16-bit mapping
 *      <srcaudio> exists and is 16-bit audio
 *      <destaudio> exists and is 16-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16ApplyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <srcaudio> <destaudio>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    destAudio = GetAudio(argv[3]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To16Apply(map, srcAudio, destAudio);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_apply_some <map> <srcaudio>
 *                                <src offset>  <src stride>
 *                                <dest offset> <dest stride> <destaudio>
 *
 * precond 
 *      <map> exists and is a 16-bit to 16-bit mapping
 *      <srcaudio> exists and is 16-bit audio
 *      <destaudio> exists and is 16-bit audio
 *      
 * return 
 *     none
 * 
 * side effect :
 *     <map> is applied to the <srcaudio> and the result is stored
 *      in <destaudio>. The channel in effect can be specified by param
 *     offset = 0, stride = 2 means taking the left channel in stereo
 *     offset = 1, stride = 2 means taking the right channel in stereo
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16ApplySomeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    Audio *srcAudio, *destAudio;
        int srcOffset, srcStride, destOffset, destStride;
        int status;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 8,
        "wrong #args, should be %s map srcaudio src offset src stride dest offset dest stride destaudio", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    srcAudio = GetAudio(argv[2]);
    ReturnErrorIf2(srcAudio == NULL,
        "%s: no such audio %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &srcOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &srcStride);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf(status != TCL_OK);
    destAudio = GetAudio(argv[7]);
    ReturnErrorIf2(destAudio == NULL,
        "%s: no such audio %s", argv[0], argv[7]);

    /* 
     * check to whether all arguments have valid contents
     */
    ReturnErrorIf1(srcAudio->length != destAudio->length,
        "%s: source and destination audio must have the same length",argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To16ApplySome(map, srcAudio,
        srcOffset, srcStride, destOffset, destStride, destAudio);

    return TCL_OK;
}
