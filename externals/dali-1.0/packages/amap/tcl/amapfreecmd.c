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
 * audiomapfreecmd.c --
 *
 *      This file contains the tcl hook to the AudioMapFree C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_free <map>
 *
 * precond 
 *      <map> exists
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the memory allocated for <map> is freed.
 *
 *----------------------------------------------------------------------
 */
int
AudioMapFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioMap *map;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */
    ReturnErrorIf1(argc != 2,
                "wrong # args: should be %s <audiomap>", argv[0]);
    map = RemoveAudioMap(argv[1]);
    ReturnErrorIf2(map == NULL,
        "%s: no such image audiomap %s", argv[0], argv[1]);

    AudioMapFree(map);
    return TCL_OK;
}

