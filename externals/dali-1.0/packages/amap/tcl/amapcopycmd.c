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
 * audiomapcopycmd.c --
 *
 *      This file contains the tcl hook to the AudioMapCopy C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_copy <srcMap> <destMap>
 *
 * precond 
 *      <srcMap> <destMap> exists and is a valid mapping from 8-bit 
 *      audio to 8-bit audio
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     the content in <srcMap> is copied to <destMap>
 *
 *----------------------------------------------------------------------
 */

int
AudioMap8To8CopyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *srcMap, *destMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <srcMap> <destMap>", argv[0]);
    srcMap = GetAudioMap(argv[1]);
    ReturnErrorIf2(srcMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[1]);
    ReturnErrorIf2((srcMap->srcRes != 8) || (srcMap->destRes != 8),
            "%s: %s is of invalid mapping resolution", argv[0], argv[1]);
    destMap = GetAudioMap(argv[2]);
    ReturnErrorIf2(destMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[2]);
    ReturnErrorIf2((destMap->srcRes != 8) || (destMap->destRes != 8),
            "%s: %s is of invalid mapping resolution", argv[0], argv[2]);

    AudioMap8To8Copy(srcMap, destMap);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_copy <srcMap> <destMap>
 *
 * precond 
 *      <srcMap> <destMap> exists and is a valid mapping from 8-bit 
 *      audio to 16-bit audio
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     the content in <srcMap> is copied to <destMap> 
 *
 *----------------------------------------------------------------------
 */

int
AudioMap8To16CopyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *srcMap, *destMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <srcMap> <destMap>", argv[0]);
    srcMap = GetAudioMap(argv[1]);
    ReturnErrorIf2(srcMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[1]);
    ReturnErrorIf2((srcMap->srcRes != 8) || (srcMap->destRes != 16),
        "%s: %s is of invalid mapping resolution", argv[0], argv[1]);
    destMap = GetAudioMap(argv[2]);
    ReturnErrorIf2(destMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[2]);
    ReturnErrorIf2((destMap->srcRes != 8) || (destMap->destRes != 16),
        "%s: %s is of invalid mapping resolution", argv[0], argv[2]);

    AudioMap8To16Copy(srcMap, destMap);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_copy <srcMap> <destMap>
 *
 * precond 
 *      <srcMap> <destMap> exists and is a valid mapping from 16-bit 
 *      audio to 8-bit audio
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     the content in <srcMap> is copied to <destMap>
 *
 *----------------------------------------------------------------------
 */

int
AudioMap16To8CopyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *srcMap, *destMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <srcMap> <destMap>", argv[0]);
    srcMap = GetAudioMap(argv[1]);
    ReturnErrorIf2(srcMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[1]);
    ReturnErrorIf2((srcMap->srcRes != 16) || (srcMap->destRes != 8),
        "%s: %s is of invalid mapping resolution", argv[0], argv[1]);
    destMap = GetAudioMap(argv[2]);
    ReturnErrorIf2(destMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[2]);
    ReturnErrorIf2((destMap->srcRes != 16) || (destMap->destRes != 8),
        "%s: %s is of invalid mapping resolution", argv[0], argv[2]);

    AudioMap16To8Copy(srcMap, destMap);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_copy <srcMap> <destMap>
 *
 * precond 
 *      <srcMap> <destMap> exists and is a valid mapping from 16-bit 
 *      audio to 16-bit audio
 *      
 * return 
 *     the buffer number
 * 
 * side effect :
 *     the content in <srcMap> is copied to <destMap>
 *
 *----------------------------------------------------------------------
 */

int
AudioMap16To16CopyCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *srcMap, *destMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <srcMap> <destMap>", argv[0]);
    srcMap = GetAudioMap(argv[1]);
    ReturnErrorIf2(srcMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[1]);
    ReturnErrorIf2((srcMap->srcRes != 16) || (srcMap->destRes != 16),
        "%s: %s is of invalid mapping resolution", argv[0], argv[1]);
    destMap = GetAudioMap(argv[2]);
    ReturnErrorIf2(destMap == NULL,
        "%s: no such audiomap %s", argv[0], argv[2]);
    ReturnErrorIf2((destMap->srcRes != 16) || (destMap->destRes != 16),
        "%s: %s is of invalid mapping resolution", argv[0], argv[2]);

    AudioMap16To16Copy(srcMap, destMap);
    return TCL_OK;
}
