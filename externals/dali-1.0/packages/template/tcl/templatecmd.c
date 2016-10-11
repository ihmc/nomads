/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmTemplate.h"

int 
TemplateSampleCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int status, size;

        ReturnErrorIf1 (argc != 2,
                "wrong # args: should be %s bits", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &size);
    ReturnErrorIf (status != TCL_OK);

    TemplateSample(size);
    return TCL_OK;
}
