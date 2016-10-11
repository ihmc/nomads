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
 * audiomapsetcmd.c --
 *
 *  This file contains the tcl hook to the AudioMapSet C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_8to8_set_value <map> <i> <value>
 *
 * precond
 *     0 <= <i> <= 255
 *     0 <= <value> <= 255
 *
 * return
 *     none
 *
 * side effect :
 *     set the image value of <i> in the mapping <map> to <value>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8SetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
        int status, i, value;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <i> <value>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <i> must be an integer", argv[0]);
    status = Tcl_GetInt(interp, argv[3], &value);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <value> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To8SetValue(map, i, value);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_8to16_set_table <map> <i> <value>
 *
 * precond
 *     0 <= <i> <= 255
 *     -32768 <= <value> <= 32767
 *
 * return
 *     none
 *
 * side effect :
 *     set the image value of <i> in the mapping <map> to <value>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16SetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
        int status, i, value;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <i> <value>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <i> must be an integer", argv[0]);
    status = Tcl_GetInt(interp, argv[3], &value);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <value> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 8) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap8To16SetValue(map, i, value);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_16to8_set_table <map> <i> <value>
 *
 * precond
 *     -32768 <= <i> <= 32767
 *     0 <= <value> <= 255
 *
 * return
 *     none
 *
 * side effect :
 *     set the image value of <i> in the mapping <map> to <value>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8SetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
        int status, i, value;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <i> <value>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <i> must be an integer", argv[0]);
    status = Tcl_GetInt(interp, argv[3], &value);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <value> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 8),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To8SetValue(map, i, value);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage
 *     audiomap_16to16_set_table <map> <i> <value>
 *
 * precond
 *     -32768 <= <i> <= 32767
 *     -32768 <= <value> <= 32767
 *
 * return
 *     none
 *
 * side effect :
 *     set the image value of <i> in the mapping <map> to <value>
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16SetValueCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;
        int status, i, value;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 4,
        "wrong #args, should be %s <map> <i> <value>", argv[0]);
    map = GetAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &i);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <i> must be an integer", argv[0]);
    status = Tcl_GetInt(interp, argv[3], &value);
    ReturnErrorIf1(status != TCL_OK,
        "%s: <value> must be an integer", argv[0]);
    ReturnErrorIf1((map->srcRes != 16) || (map->destRes != 16),
        "%s: invalid mapping resolution", argv[0]);

    AudioMap16To16SetValue(map, i, value);

    return TCL_OK;
}
