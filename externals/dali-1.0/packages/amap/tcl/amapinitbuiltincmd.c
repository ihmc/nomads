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
 * audiomapinitbuiltincmd.c --
 *
 *        This file contains the tcl hook to the AudioMapInitBuiltin C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_init_identity <target map>
 *
 * precond 
 *      none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 8-bit to 8-bit map <target map> to a identity map
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8InitIdentityCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap8To8InitIdentity(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_init_volume <target map> <max value>
 *
 * precond 
 *      none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 16-bit to 16-bit map <target map> to a map
 *     that adjusts the volume so that samples in the range -maxVal..maxVal
 *     are mapped to the range -32767..32767
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16InitIdentityCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap16To16InitIdentity(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_init_complement <target map>
 *
 * precond 
 *      none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 8-bit to 8-bit map <target map> to a complement 
 *           map
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8InitComplementCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap8To8InitComplement(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_init_complement <target map>
 *
 * precond 
 *      none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 16-bit to 16-bit map <target map> to a complement 
 *           map
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16InitComplementCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap16To16InitComplement(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_init_identity <target map>
 *
 * precond 
 *      none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 16-bit to 16-bit map <target map> to a identity map
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16InitVolumeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
    int maxVal;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <target map> <max val>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);
    if (Tcl_GetInt(interp, argv[2], &maxVal) != TCL_OK) {
        return TCL_ERROR;
    }

    AudioMap16To16InitVolume(map, maxVal);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_init_big_little_swap <target map>
 *
 * precond 
 *     none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize the 16-bit to 16-bit audiomap <target map> which maps 
 *        between big-endian and little-endian audio data
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16InitBigLittleSwapCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /* 
     * Check and parse args 
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap16To16InitBigLittleSwap(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_init_ulaw_to_linear <target map>
 *
 * precond 
 *     none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize a 8-bit to 16-bit audiomap <target map> which maps 
 *        u-law audio to linear PCM
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16InitULawToLinearCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args 
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap8To16InitULawToLinear(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_init_alaw_to_linear <target map>
 *
 * precond 
 *     none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize an 8-bit to 16-bit audiomap <target map> which maps 
 *        a-law audio to linear PCM
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16InitALawToLinearCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args 
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap8To16InitALawToLinear(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_init_linear_to_ulaw <target map>
 *
 * precond 
 *     none
 *      
 * return 
 *     none
 * 
 * side effect :
 *     initialize a 16-bit to 8-bit audiomap <target map> which maps 
 *        linear PCM audio to u-law audio
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8InitLinearToULawCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args 
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap16To8InitLinearToULaw(map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_init_linear_to_alaw <target map>
 *
 * precond 
 *     none
 *      
 * return 
 *     none
 * 
 * side effect :
 *      initialize a 16-bit to 8-bit audiomap <target map> which maps 
 *        linear PCM audio to a-law audio
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8InitLinearToALawCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args 
     */
    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <target map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[1]);

    AudioMap16To8InitLinearToALaw(map);

    return TCL_OK;
}
