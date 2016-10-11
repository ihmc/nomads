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
 * colorinit.c --
 *
 *      This file contains the code to make the Dali VM DLL for Tcl.
 *
 */

#include "tclDvmColor.h"

EXPORT(int,Tcldvmcolor_Init) _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void InitHashTable _ANSI_ARGS_((Tcl_Interp *interp));

static Commands cmd[] =
{
    { "color_hash_table_new", ColorHashTableNewCmd, NULL, NULL, },
    { "color_hash_table_clear", ColorHashTableClearCmd, NULL, NULL, },
    { "color_hash_table_free", ColorHashTableFreeCmd, NULL, NULL, },
    { "color_hash_table_get_size", ColorHashTableGetSizeCmd, NULL, NULL, },
    { "color_hash_table_get_num_of_entry", ColorHashTableGetNumOfEntryCmd, NULL, NULL, },

    { "rgb_to_256", RgbTo256Cmd, NULL, NULL, },
    { "rgb_quant_with_hash_table", RgbQuantWithHashTableCmd, NULL, NULL, },
    { "rgb_quant_with_vp_tree", RgbQuantWithVpTreeCmd, NULL, NULL, },

    { "rgb_to_y", RgbToYCmd, NULL, NULL, },
    { "rgb_to_yuv_420", RgbToYuv420Cmd, NULL, NULL, },
    { "rgb_to_yuv_411", RgbToYuv411Cmd, NULL, NULL, },
    { "rgb_to_yuv_422", RgbToYuv422Cmd, NULL, NULL, },
    { "rgb_to_yuv_444", RgbToYuv444Cmd, NULL, NULL, },
    { "yuv_to_rgb_444", YuvToRgb444Cmd, NULL, NULL, },
    { "yuv_to_rgb_411", YuvToRgb411Cmd, NULL, NULL, },
    { "yuv_to_rgb_422", YuvToRgb422Cmd, NULL, NULL, },
    { "yuv_to_rgb_420", YuvToRgb420Cmd, NULL, NULL, },

    { "vp_tree_new",  VpTreeNewCmd, NULL, NULL, },
    { "vp_tree_init", VpTreeInitCmd, NULL, NULL, },
    { "vp_tree_free", VpTreeFreeCmd, NULL, NULL, },
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
 * DvmFooioInit --
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

EXPORT(int,Tcldvmcolor_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, cmd, sizeof(cmd));
    InitHashTable (interp);
    return Tcl_PkgProvide(interp, "DvmColor", "1.0");
}
