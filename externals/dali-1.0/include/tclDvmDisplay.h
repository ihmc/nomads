/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef TCL_DVM_DISPLAY_H
#define TCL_DVM_DISPLAY_H

#include "tclDvmBasic.h"
#ifdef __cplusplus
extern "C" {
#endif

    int RgbToPhotoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ByteToPhotoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
