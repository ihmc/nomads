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
 * templateinit.c --
 *
 *      This file contains the code to make the Dali VM DLL for Tcl.
 *
 */

#include "tclDvmTemplate.h"

EXPORT(int,Tcldvmtemplate_Init) _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void InitHashTable _ANSI_ARGS_((Tcl_Interp *interp));

static Commands cmd[] =
{
    { "template_sample", TemplateSampleCmd, NULL, NULL, },
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
 * Tcldvmtemplate_Init --
 *
 *      This procedure initializes the Dali template command.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmtemplate_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, cmd, sizeof(cmd));
    InitHashTable (interp);
    return Tcl_PkgProvide(interp, "DvmTemplate", "1.0");
}
