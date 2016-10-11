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
 * tcl/avihdr.c
 *
 * Tcl interface to open/close/create manipulate avi files
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_file_open <filename>
 *
 * precond 
 *     filename exists and is a readable AVI file
 *      
 * return 
 *     None
 * 
 * side effect :
 *     The file <filename> is opened for reading -- state is stored in
 *     the avi header.  Use avi_file_close to close the file.
 *
 *----------------------------------------------------------------------
 */

int
AviFileOpenCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    char *filename;
    int status;

    /*
     * Check args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s filename", argv[0]);
    filename = argv[1];

    /*
     * Do the work, check return code
     */
    status = AviFileOpen (filename, &aviFile);
    ReturnErrorIf3 (status != 0,
        "%s: Couldn't open avi file %s -- %s",
        argv[0], filename, AviTranslateError(status));

    PutAviFile(interp, aviFile);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_file_close <aviFile>
 *
 * precond 
 *      <aviFile> exists and is an open AVI File
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the memory allocated for the avi header <aviFile> is freed
 *
 *----------------------------------------------------------------------
 */

int
AviFileCloseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <aviFile>", argv[0]);

    aviFile = RemoveAviFile(argv[1]);
    ReturnErrorIf2 (aviFile == NULL,
        "%s: no such avi file %s", argv[0], argv[1]);

    AviFileClose (aviFile);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_file_create <filename>
 *
 * precond 
 *     none
 *      
 * return 
 *     The hdr passed in.
 * 
 * side effect :
 *     The file <filename> is opened for writing. It is created if it
 *     does not exist. Use avi_file_close to close the file.
 *
 *----------------------------------------------------------------------
 */

int
AviFileCreateCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    char *filename;
        int status;

    /*
     * Check args
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s filename", argv[0]);
    filename = argv[1];

    status = AviFileCreate (filename, &aviFile);
    ReturnErrorIf3 (status != 0,
        "%s: Couldn't create avi file %s -- %s",
        argv[0], filename, AviTranslateError(status));
    PutAviFile(interp, aviFile);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_file_num_streams <aviFile>
 *     avi_file_length <aviFile>
 *
 * precond 
 *     AVI file <aviFile> exists.
 *
 * return 
 *     The corresponding field in the header
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
AviFileFieldCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviFile *aviFile;
    int field, value;

    /*
     * Check args, retrieve AVI header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s aviFile", argv[0]);

    aviFile = GetAviFile(argv[1]);
    ReturnErrorIf2 (aviFile == NULL,
        "%s: no such avi header %s", argv[0], argv[1]);

    field = (int)clientData;
    switch (field) {
    case AVIHDR_NUMSTREAMS:
        value = aviFile->numStreams;
        break;
    case AVIHDR_LENGTH:
        value = aviFile->length;
        break;
    default:
        sprintf(interp->result, "Internal error");
        return TCL_ERROR;
    }

    sprintf(interp->result, "%d", value);
    return TCL_OK;
}

