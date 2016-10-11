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
 * bytegeominit.c --
 *
 *      This file contains the code to make the Dali DV bytegeom DLL for Tcl.
 *
 */

#include "tclDvmByteGeom.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmbytegeom_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands bytegeomcmd[] =
{
    /* scaling */
    { "byte_shrink_4x4",                ByteShrink4x4Cmd, NULL, NULL, },
    { "byte_shrink_2x2",                ByteShrink2x2Cmd, NULL, NULL, },
    { "byte_shrink_1x2",                ByteShrink1x2Cmd, NULL, NULL, },
    { "byte_shrink_2x1",                ByteShrink2x1Cmd, NULL, NULL, },
    { "byte_expand_4x4",                ByteExpand4x4Cmd, NULL, NULL, },
    { "byte_expand_2x2",                ByteExpand2x2Cmd, NULL, NULL, },
    { "byte_expand_1x2",                ByteExpand1x2Cmd, NULL, NULL, },
    { "byte_expand_2x1",                ByteExpand2x1Cmd, NULL, NULL, },
    { "byte_scale_bilinear",            ByteScaleBilinearCmd, NULL, NULL, },
    
    /* rotation */
    { "byte_rotate",                    ByteRotateCmd, NULL, NULL, },
    { "byte_rotate_orig",               ByteRotateOrigCmd, NULL, NULL, },
    { "byte_rotate_90a",                ByteRotate90aCmd, NULL, NULL, },
    { "byte_rotate_90c",                ByteRotate90cCmd, NULL, NULL, },
    
    /* affine transformation */
    { "byte_affine",                    ByteAffineCmd, NULL, NULL, },
    { "byte_affine_rect_region",        ByteAffineRectRegionCmd, NULL, NULL, },

    /* general homogeneous transformation */
    { "byte_homo_compute_matrix",       ByteHomoComputeMatrixCmd, NULL, NULL, },
    { "byte_homo_invert_matrix",        ByteHomoInvertMatrixCmd, NULL, NULL, },
    { "byte_homo",                      ByteHomoCmd, NULL, NULL, },
    { "byte_homo_rect_region",          ByteHomoRectRegionCmd, NULL, NULL},
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
 * Dvmbytegeom_Init --
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

EXPORT(int, Tcldvmbytegeom_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, bytegeomcmd, sizeof(bytegeomcmd));
    return Tcl_PkgProvide(interp, "DvmByteGeom", "1.0");
}
