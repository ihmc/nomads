/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * imapinit.c --
 *
 *      This file contains the code to make the Dali DLL for Tcl.
 *
 *------------------------------------------------------------------------
 */

#include "tclDvmImap.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmimap_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands imagemapcmd[] =
{
    /* Map operations */

    { "imagemap_new", ImageMapNewCmd, NULL, NULL, },
    { "imagemap_free", ImageMapFreeCmd, NULL, NULL, },
    { "imagemap_copy", ImageMapCopyCmd, NULL, NULL, },
    { "imagemap_init", ImageMapInitCmd, NULL, NULL, },
    { "imagemap_init_identity", ImageMapInitIdentityCmd, NULL, NULL, },
    { "imagemap_init_inverse", ImageMapInitInverseCmd, NULL, NULL, },
    { "imagemap_init_histo_equal", ImageMapInitHistoEqualCmd, NULL, NULL, },
    { "imagemap_compose", ImageMapComposeCmd, NULL, NULL, },
    { "imagemap_apply", ImageMapApplyCmd, NULL, NULL, },
    { "imagemap_get_values", ImageMapGetValuesCmd, NULL, NULL, },
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
 * Tcldvmimagemap_Init --
 *
 *      This procedure initializes the dvm command.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmimap_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, imagemapcmd, sizeof(imagemapcmd));
    return Tcl_PkgProvide(interp, "DvmImap", "1.0");
}
