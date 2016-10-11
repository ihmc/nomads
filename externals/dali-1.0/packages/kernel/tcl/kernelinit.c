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
 * kernelinit.c --
 *
 *      This file contains the code to make the RVM kernel DLL for Tcl.
 *
 */

#include "tclDvmKernel.h"

/*
 * Declarations for functions defined in this file.
 */
EXPORT(int,Tcldvmkernel_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands kernelcmd[] =
{
    /* convolution kernel operations*/
    { "kernel_new", KernelNewCmd, NULL, NULL, },
    { "kernel_free", KernelFreeCmd, NULL, NULL, },
    { "kernel_set_values", KernelSetValuesCmd, NULL, NULL, },
    { "kernel_set_div_factor", KernelSetDivFactorCmd, NULL, NULL, },
    { "kernel_set_offset", KernelSetOffsetCmd, NULL, NULL, },
    { "kernel_get_height", KernelGetHeightCmd, NULL, NULL, },
    { "kernel_get_width", KernelGetWidthCmd, NULL, NULL, },
    { "kernel_get_values", KernelGetValuesCmd, NULL, NULL, },
    { "kernel_get_div_factor", KernelGetDivFactorCmd, NULL, NULL, },
    { "kernel_get_offset", KernelGetOffsetCmd, NULL, NULL, },
    { "kernel_apply", KernelApplyCmd, NULL, NULL, },
    { "kernel_compose", KernelComposeCmd, NULL, NULL, },
};

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *      This wrapper function is used by Windows to invoke the
 *      initialization code for the DLL.  If we are compiling
 *      with Visual C++, this routine will be renamed to DllMain.
 *      routine.
 *
 * Results:
 *      Returns TRUE;
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;            /* Library instance handle. */
    DWORD reason;               /* Reason this function is being called. */
    LPVOID reserved;            /* Not used. */
{
    return TRUE;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Tcldvmkernel_Init --
 *
 *      This procedure initializes the rvm command.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmkernel_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading dvmbasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, kernelcmd, sizeof(kernelcmd));
    return Tcl_PkgProvide(interp, "DvmKernel", "1.0");
}
