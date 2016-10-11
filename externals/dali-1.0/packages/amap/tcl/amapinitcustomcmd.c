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
 * audiomapinitcustomcmd.c --
 *
 *      This file contains the tcl hook to the AudioMapInitCustom C routines
 *
 */

#include "tclDvmAmap.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to8_init_custom <values> <target map>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     <target map> is set to have a table with the values in the list
 *         <values>
 *
 * comment
 *     the 0th element from the list is the lookup value for 0
 *     the last element from the list is the lookup value for 255
 *     (i)th element --> transformation of (i)
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To8InitCustomCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    int listArgc, listValue, i;
    char **listArgv,**ArgvPos;
    unsigned char *values, *currValue;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <values> <target map>", argv[0]);
    ReturnErrorIf1(
        Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK,
        "%s: 1st argument must be of type list", argv[0]);
    ReturnErrorIf1(listArgc != 256,
        "%s: list should have exactly 256 elements", argv[0]);
    map = GetAudioMap(argv[2]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[2]);

    /*
     * check whether map is of the correct resolution
     */
    ReturnErrorIf2(map->srcRes != 8 || map->destRes != 8,
        "%s: audio map %s of wrong resolution", argv[0], argv[2]);


    /*
     * extracting the list of values to array
     */
    values = currValue = NEWARRAY (unsigned char, 256);
    ArgvPos = listArgv;
    for (i=0;i<256;i++) {
        Tcl_GetInt(interp, *ArgvPos++, &listValue);
        /*
         * check for illegitimate values
         */
        if ((listValue < 0) || (listValue > 255)) {
            sprintf (interp->result, "List values must be between 0 and 255");
            ckfree(values);
            return TCL_ERROR;
        }
        *currValue++ = listValue;
    }

    AudioMap8To8InitCustom(values, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_8to16_init_custom <values>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     <target map> is set to have a table with the values in the list
 *         <values>
 *
 * comment
 *     the 0th element from the list is the lookup value for 0
 *     the last element from the list is the lookup value for 255
 *     (i)th element --> transformation of (i)
 *
 *----------------------------------------------------------------------
 */
int
AudioMap8To16InitCustomCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    int listArgc, listValue, i;
    char **listArgv,**ArgvPos;
    short *values, *currValue;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <values> <target map>", argv[0]);
    ReturnErrorIf1(
        Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK,
        "%s: 1st argument must be of type list", argv[0]);
    ReturnErrorIf1(listArgc != 256,
        "%s: list should have exactly 256 elements", argv[0]);
    map = GetAudioMap(argv[2]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[2]);

    /*
     * extracting the list of values to array
     */
    values = currValue = NEWARRAY (short, 256);
    ArgvPos = listArgv;
    for (i=0;i<256;i++) {
        Tcl_GetInt(interp, *ArgvPos++, &listValue);
        /*
         * check for illegitimate values
         */
        if ((listValue < -32768) || (listValue > 32767)) {
            sprintf (interp->result, "List values must be between -32768 and 32767");
            FREE(values);
            return TCL_ERROR;
        }
        *currValue++ = listValue;
    }

    AudioMap8To16InitCustom(values, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to8_init_custom <values> <target map>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     <target map> is set to have a table with the values in the list
 *         <values>
 *
 * comment
 *     the 0th element from the list is the lookup value for -32768
 *     the last element from the list is the lookup value for 32767
 *     (i)th element --> transformation of (i-32768)
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To8InitCustomCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    int listArgc, listValue, i;
    char **listArgv,**ArgvPos;
    unsigned char *values, *currValue;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <values> <target map>", argv[0]);
    ReturnErrorIf1(
        Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK,
        "%s: 1st argument must be of type list", argv[0]);
    ReturnErrorIf1(listArgc != 65536,
        "%s: list should have exactly 65536 elements", argv[0]);
    map = GetAudioMap(argv[2]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[2]);

    /*
     * extracting the list of values to array
     */
    values = currValue = NEWARRAY (unsigned char, 65536);
    ArgvPos = listArgv;
    for (i=0;i<65536;i++) {
        Tcl_GetInt(interp, *ArgvPos++, &listValue);
        /*
         * check for illegitimate values
         */
        if ((listValue < 0) || (listValue > 255)) {
            sprintf (interp->result, "List values must be between 0 and 255");
            FREE(values);
            return TCL_ERROR;
        }
        *currValue++ = listValue;
    }

    AudioMap16To8InitCustom(values, map);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audiomap_16to16_init_custom <values> <target map>
 *
 * precond 
 *     none
 *
 * return 
 *     none
 * 
 * side effect :
 *     <target map> is set to have a table with the values in the list
 *         <values>
 *
 * comment
 *     the 0th element from the list is the lookup value for -32768
 *     the last element from the list is the lookup value for 32767
 *     (i)th element --> transformation of (i-32768)
 *
 *----------------------------------------------------------------------
 */
int
AudioMap16To16InitCustomCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];

{
    AudioMap *map;
    int listArgc, listValue, i;
    char **listArgv,**ArgvPos;
    short *values, *currValue;

    /* 
     * Check and parse args
     */
    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <values> <target map>", argv[0]);
    ReturnErrorIf1(
        Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK,
        "%s: 1st argument must be of type list", argv[0]);
    ReturnErrorIf1(listArgc != 65536,
        "%s: list should have exactly 65536 elements", argv[0]);
    map = GetAudioMap(argv[2]);
    ReturnErrorIf2(map == NULL,
        "%s: no such audio map %s", argv[0], argv[2]);

    /*
     * extracting the list of values to array
     */
    values = currValue = NEWARRAY (short, 65536);
    ArgvPos = listArgv;
    for (i=0;i<65536;i++) {
        Tcl_GetInt(interp, *ArgvPos++, &listValue);
        /*
         * check for illegitimate values
         */
        if ((listValue < -32768) || (listValue > 32767)) {
            sprintf (interp->result, "List values must be between -32768 and 32767");
            FREE(values);
            return TCL_ERROR;
        }
        *currValue++ = listValue;
    }

    AudioMap16To16InitCustom(values, map);

    return TCL_OK;
}
