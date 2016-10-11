/*
 *----------------------------------------------------------------------
 *
 * main.c
 *
 * Wei Tsang Mar 97
 *
 * main function for initializing tcl. 
 *
 *----------------------------------------------------------------------
 */

#include "tcl.h"

int Tcldvmbasic_Init(Tcl_Interp * interp);
int Tcldvmpnm_Init(Tcl_Interp * interp);
int Tcldvmmpeg_Init(Tcl_Interp * interp);
int Tcldvmgif_Init(Tcl_Interp * interp);
int Tcldvmamap_Init(Tcl_Interp * interp);
int Tcldvmimap_Init(Tcl_Interp * interp);
int Tcldvmbytegeom_Init(Tcl_Interp * interp);
int Tcldvmwave_Init(Tcl_Interp * interp);
int Tcldvmvision_Init(Tcl_Interp * interp);
int Tcldvmkernel_Init(Tcl_Interp * interp);
int Tcldvmjpeg_Init(Tcl_Interp * interp);
int Tcldvmcolor_Init(Tcl_Interp * interp);

int 
Tcl_AppInit(interp)
    Tcl_Interp *interp;
{
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmbasic_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmpnm_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmmpeg_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmamap_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmimap_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmgif_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmbytegeom_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmkernel_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmvision_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmwave_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmjpeg_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tcldvmcolor_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
main(argc, argv)
    int argc;
    char *argv[];
{
    Tcl_Main(argc, argv, Tcl_AppInit);
    return (0);
}

