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
 * audiomapinfocmd.c --
 *
 *    This file contains the tcl hook to the AudioMapInfo C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_get_srcres <map>
 *
 * precond 
 *      none
 *      
 * return 
 *     the srcRes field - resolution of the domain audio
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMapGetSrcResCmd(clientData, interp, argc, argv)
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
        "wrong #args, should be %s <map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", AudioMapGetSrcRes(map));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_get_destres <map>
 *
 * precond 
 *      none
 *      
 * return 
 *     the destRes field - resolution of the image audio
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMapGetDestResCmd(clientData, interp, argc, argv)
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
        "wrong #args, should be %s <map>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", AudioMapGetDestRes(map));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_8to8_get_value <map> <i>
 *
 * precond
 *      0 <= <i> <= 255
 *
 * return
 *     the image value of <i> in the 8-bit to 8-bit mapping <map>
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8GetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
    int status, i;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <map> <i>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
            "%s: <i> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 8),
            "%s: invalid mapping resolution", argv[0]);

    sprintf(interp->result, "%d", AudioMap8To8GetValue(map, i));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_8to16_get_value <map> <i>
 *
 * precond
 *      0 <= <i> <= 255
 *
 * return
 *     the image value of <i> in the 8-bit to 16-bit mapping <map>
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16GetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
    int status, i;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <map> <i>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
            "%s: <i> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 16),
            "%s: invalid mapping resolution", argv[0]);

    sprintf(interp->result, "%d", AudioMap8To16GetValue(map, i));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_16to8_table <map> <i>
 *
 * precond
 *     -32768 <= <i> <= 32767
 *
 * return
 *     the image value of <i> in the 16-bit to 8-bit mapping <map>
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8GetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
    int status, i;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <map> <i>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
            "%s: <i> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 8),
            "%s: invalid mapping resolution", argv[0]);

    sprintf(interp->result, "%d", AudioMap16To8GetValue(map, i));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_16to16_table <map> <i>
 *
 * precond
 *     -32768 <= <i> <= 32767
 *
 * return
 *     the image value of <i> in the 16-bit to 16-bit mapping <map>
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16GetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
    int status, i;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong #args, should be %s <map> <i>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
            "%s: <i> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 16),
            "%s: invalid mapping resolution", argv[0]);

    sprintf(interp->result, "%d", AudioMap16To16GetValue(map, i));

    return TCL_OK;
}
