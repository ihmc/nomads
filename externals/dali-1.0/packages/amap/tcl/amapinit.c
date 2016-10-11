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
 * audiomapinit.c --
 *
 *  This file contains the code to make the Dali DV audiomap DLL for Tcl.
 *
 */

#include "tclDvmAmap.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmamap_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands audiomapcmd[] =
{
    /* Map operations */

        { "audiomap_8to8_new",                   AudioMap8To8NewCmd, NULL, NULL, },
        { "audiomap_8to16_new",                  AudioMap8To16NewCmd, NULL, NULL, },
        { "audiomap_16to8_new",                  AudioMap16To8NewCmd, NULL, NULL, },
        { "audiomap_16to16_new",                 AudioMap16To16NewCmd, NULL, NULL, },

        { "audiomap_8to8_init_custom",           AudioMap8To8InitCustomCmd, NULL, NULL, },
        { "audiomap_8to16_init_custom",          AudioMap8To16InitCustomCmd, NULL, NULL, },
        { "audiomap_16to8_init_custom",          AudioMap16To8InitCustomCmd, NULL, NULL, },
        { "audiomap_16to16_init_custom",         AudioMap16To16InitCustomCmd, NULL, NULL, },

        { "audiomap_free",                       AudioMapFreeCmd, NULL, NULL, },
    
        { "audiomap_8to8_copy",                  AudioMap8To8CopyCmd, NULL, NULL, },
        { "audiomap_8to16_copy",                 AudioMap8To16CopyCmd, NULL, NULL, },
        { "audiomap_16to8_copy",                 AudioMap16To8CopyCmd, NULL, NULL, },
        { "audiomap_16to16_copy",                AudioMap16To16CopyCmd, NULL, NULL, },
    
        { "audiomap_8to8_apply",                 AudioMap8To8ApplyCmd, NULL, NULL, },
        { "audiomap_8to8_apply_some",            AudioMap8To8ApplySomeCmd, NULL, NULL, },
        { "audiomap_8to16_apply",                AudioMap8To16ApplyCmd, NULL, NULL, },
        { "audiomap_8to16_apply_some",           AudioMap8To16ApplySomeCmd, NULL, NULL, },
        { "audiomap_16to8_apply",                AudioMap16To8ApplyCmd, NULL, NULL, },
        { "audiomap_16to8_apply_some",           AudioMap16To8ApplySomeCmd, NULL, NULL, },
        { "audiomap_16to16_apply",               AudioMap16To16ApplyCmd, NULL, NULL, },
        { "audiomap_16to16_apply_some",          AudioMap16To16ApplySomeCmd, NULL, NULL, },
    
        { "audiomap_8to8_8to8_compose",          AudioMap8To88To8ComposeCmd, NULL, NULL, },
        { "audiomap_8to8_8to16_compose",         AudioMap8To88To16ComposeCmd, NULL, NULL, },
        { "audiomap_16to8_8to8_compose",         AudioMap16To88To8ComposeCmd, NULL, NULL, },
        { "audiomap_16to8_8to16_compose",        AudioMap16To88To16ComposeCmd, NULL, NULL, },
        { "audiomap_8to16_16to8_compose",        AudioMap8To1616To8ComposeCmd, NULL, NULL, },
        { "audiomap_8to16_16to16_compose",       AudioMap8To1616To16ComposeCmd, NULL, NULL, },
        { "audiomap_16to16_16to8_compose",       AudioMap16To1616To8ComposeCmd, NULL, NULL, },
        { "audiomap_16to16_16to16_compose",      AudioMap16To1616To16ComposeCmd, NULL, NULL, },
    
        /* info slots commands */
        { "audiomap_get_srcres",                 AudioMapGetSrcResCmd, NULL, NULL, },
        { "audiomap_get_destres",                AudioMapGetDestResCmd, NULL, NULL, },
        { "audiomap_8to8_get_value",             AudioMap8To8GetValueCmd, NULL, NULL, },
        { "audiomap_8to16_get_value",            AudioMap8To16GetValueCmd, NULL, NULL, },
        { "audiomap_16to8_get_value",            AudioMap16To8GetValueCmd, NULL, NULL, },
        { "audiomap_16to16_get_value",           AudioMap16To16GetValueCmd, NULL, NULL, },

        /* set slots commands */
        { "audiomap_8to8_set_value",             AudioMap8To8SetValueCmd, NULL, NULL, },
        { "audiomap_8to16_set_value",            AudioMap8To16SetValueCmd, NULL, NULL, },
        { "audiomap_16to8_set_value",            AudioMap16To8SetValueCmd, NULL, NULL, },
        { "audiomap_16to16_set_value",           AudioMap16To16SetValueCmd, NULL, NULL, },


        /* built-in mapping formation commands */
        { "audiomap_8to8_init_identity",         AudioMap8To8InitIdentityCmd, NULL, NULL, },
        { "audiomap_16to16_init_identity",       AudioMap16To16InitIdentityCmd, NULL, NULL, },
        { "audiomap_8to8_init_complement",       AudioMap8To8InitComplementCmd, NULL, NULL, },
        { "audiomap_16to16_init_complement",     AudioMap16To16InitComplementCmd, NULL, NULL, },
        { "audiomap_16to16_init_volume",         AudioMap16To16InitVolumeCmd, NULL, NULL, },
        { "audiomap_16to16_init_big_little_swap",AudioMap16To16InitBigLittleSwapCmd, NULL, NULL, },
        { "audiomap_8to16_init_ulaw_to_linear",  AudioMap8To16InitULawToLinearCmd, NULL, NULL, },
        { "audiomap_8to16_init_alaw_to_linear",  AudioMap8To16InitALawToLinearCmd, NULL, NULL, },
        { "audiomap_16to8_init_linear_to_ulaw",  AudioMap16To8InitLinearToULawCmd, NULL, NULL, },
        { "audiomap_16to8_init_linear_to_alaw",  AudioMap16To8InitLinearToALawCmd, NULL, NULL, },
};

/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *        This wrapper function is used by Windows to invoke the
 *        initialization code for the DLL.  If we are compiling
 *        with Visual C++, this routine will be renamed to DllMain.
 *        routine.
 *
 * Results:
 *        Returns TRUE;
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

#ifdef __WIN32__
BOOL APIENTRY
DllEntryPoint(hInst, reason, reserved)
    HINSTANCE hInst;                /* Library instance handle. */
    DWORD reason;                /* Reason this function is being called. */
    LPVOID reserved;                /* Not used. */
{
    return TRUE;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Rvmaudiomap_Init --
 *
 *        This procedure initializes the rvm command.
 *
 * Results:
 *        A standard Tcl result.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int, Tcldvmamap_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, audiomapcmd, sizeof(audiomapcmd));
    return Tcl_PkgProvide(interp, "DvmAmap", "1.0");
}
