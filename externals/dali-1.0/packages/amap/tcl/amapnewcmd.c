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
 * audiomapnewcmd.c --
 *
 *      This file contains the tcl hook to the AudioMapNew C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_new
 *
 * precond 
 *     none
 *
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8NewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    map = AudioMap8To8New();
    PutAudioMap(interp, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_new
 *
 * precond 
 *     none
 *
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16NewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    map = AudioMap8To16New();
    PutAudioMap(interp, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_new
 *
 * precond 
 *     none
 *
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8NewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    map = AudioMap16To8New();
    PutAudioMap(interp, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_new
 *
 * precond 
 *     none
 *
 * return 
 *     the buffer number
 * 
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16NewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    map = AudioMap16To16New();
    PutAudioMap(interp, map);

    return TCL_OK;
}
