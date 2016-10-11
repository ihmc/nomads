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
 * displayinit.c --
 *
 *      This file contains the code to make the RVM display DLL for Tcl.
 *
 */

#include "tk.h"
#include "tclDvmDisplay.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmdisplay_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands displaycmd[] =
{
    /*
     * RGB / Gray to photo conversions
     */
    { "byte_to_photo", ByteToPhotoCmd, NULL, NULL, },
    { "rgb_to_photo", RgbToPhotoCmd, NULL, NULL, },
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
 * Tcldvmdisplay_Init --
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

EXPORT(int,Tcldvmdisplay_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading dvmbasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, displaycmd, sizeof(displaycmd));
    return Tcl_PkgProvide(interp, "DvmDisplay", "1.0");
}
