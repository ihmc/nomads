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
 * audiomapcomposecmd.c --
 *
 *      This file contains the tcl hook to the AudioMapCompose C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_8to8_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 8-bit to 8-bit mapping
 *      <map2> exists and is a 8-bit to 8-bit mapping
 *      <outmap> exists and is a 8-bit to 8-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To88To8ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 8) || (map1->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 8) || (map2->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 8) || (outMap->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap8To88To8Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_8to16_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 8-bit to 8-bit mapping
 *      <map2> exists and is a 8-bit to 16-bit mapping
 *      <outmap> exists and is a 8-bit to 16-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To88To16ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 8) || (map1->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 8) || (map2->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 8) || (outMap->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap8To88To16Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_8to8_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 16-bit to 8-bit mapping
 *      <map2> exists and is a 8-bit to 8-bit mapping
 *      <outmap> exists and is a 16-bit to 8-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To88To8ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 16) || (map1->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 8) || (map2->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 16) || (outMap->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap16To88To8Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_8to16_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 16-bit to 8-bit mapping
 *      <map2> exists and is a 8-bit to 16-bit mapping
 *      <outmap> exists and is a 16-bit to 16-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To88To16ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 16) || (map1->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 8) || (map2->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 16) || (outMap->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap16To88To16Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_16to8_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 8-bit to 16-bit mapping
 *      <map2> exists and is a 16-bit to 8-bit mapping
 *      <outmap> exists and is a 8-bit to 8-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To1616To8ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 8) || (map1->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 16) || (map2->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 8) || (outMap->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap8To1616To8Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_16to16_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 8-bit to 16-bit mapping
 *      <map2> exists and is a 16-bit to 16-bit mapping
 *      <outmap> exists and is a 8-bit to 16-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To1616To16ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 8) || (map1->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 16) || (map2->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 8) || (outMap->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap8To1616To16Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_16to8_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 16-bit to 16-bit mapping
 *      <map2> exists and is a 16-bit to 8-bit mapping
 *      <outmap> exists and is a 8-bit to 8-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To1616To8ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 16) || (map1->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 16) || (map2->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 16) || (outMap->destRes != 8),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap16To1616To8Compose(map1, map2, outMap);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_16to16_compose <map1> <map2> <outmap>
 *
 * precond 
 *      <map1> exists and is a 16-bit to 16-bit mapping
 *      <map2> exists and is a 16-bit to 16-bit mapping
 *      <outmap> exists and is a 16-bit to 16-bit mapping
 *      
 * return 
 *      none
 * 
 * side effect :
 *      <map1> is composed with <map2> and the result is stored into
 *         <outmap>
 *      Application of the composited <outmap> is equivalent to 
 *         applying <map1> and then <map2>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To1616To16ComposeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map1, *map2, *outMap;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map1> <map2> <outMap>", argv[0]);
    map1 = GetAudioMap(argv[1]);
    ReturnErrorIf2(map1 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    map2 = GetAudioMap(argv[2]);
    ReturnErrorIf2(map2 == NULL,
        "%s: no such image audiomap %s", argv[0], argv[2]);
    outMap = GetAudioMap(argv[3]);
    ReturnErrorIf2(outMap == NULL,
        "%s: no such image audiomap %s", argv[0], argv[3]);

    /*
     * check whether all the resolutions of maps are valid
     */
    ReturnErrorIf2((map1->srcRes != 16) || (map1->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[1]);
    ReturnErrorIf2((map2->srcRes != 16) || (map2->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[2]);
    ReturnErrorIf2((outMap->srcRes != 16) || (outMap->destRes != 16),
        "%s: %s has invalid map resolution", argv[0], argv[3]);

    AudioMap16To1616To16Compose(map1, map2, outMap);

    return TCL_OK;
}

