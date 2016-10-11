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
 * visioninit.c --
 *
 *      This file contains the code to make the DVM vision DLL for Tcl.
 *
 */

#include "tclDvmVision.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmvision_Init) _ANSI_ARGS_((Tcl_Interp *interp));


static Commands visioncmd[] =
{
    { "byte_smooth", ByteSmoothCmd, NULL, NULL, },
    { "byte_edge_detect_sobel", ByteEdgeDetectSobelCmd, NULL, NULL, },
    { "byte_edge_detect_canny", ByteEdgeDetectCannyCmd, NULL, NULL, },
    { "byte_compute_threshold", ByteComputeThresholdCmd, NULL, NULL, },
    { "byte_make_from_threshold_8", ByteMakeFromThreshold8Cmd, NULL, NULL, },
    { "byte_smooth_gaussian", ByteSmoothGaussianCmd, NULL, NULL, },

    { "bit_adaptive_threshold_8", BitAdaptiveThreshold8Cmd, NULL, NULL, },
    { "bit_make_from_threshold_8", BitMakeFromThreshold8Cmd, NULL, NULL, },
    { "bit_erode_8", BitErode8Cmd, NULL, NULL, },
    { "bit_count_overlap", BitCountOverlapCmd, NULL, NULL, },
    { "bit_count_overlap_8", BitCountOverlap8Cmd, NULL, NULL, },
    { "bit_dilate_8", BitDilate8Cmd, NULL, NULL, },


    { "byte_find_bounding_box", ByteFindBoundingBoxCmd, NULL, NULL, },
    { "byte_count_edges", ByteCountEdgesCmd, NULL, NULL, },
    { "bit_compare", BitCompareCmd, NULL, NULL, },
    { "byte_find_outer_corners", ByteFindOuterCornersCmd, NULL, NULL, },
    { "bit_all_white", BitAllWhiteCmd, NULL, NULL, },
    { "bit_find_text_angle", BitFindTextAngleCmd, NULL, NULL, },
    { "bit_count_vert_scan", BitCountVertScanCmd, NULL, NULL, },
    { "byte_make_from_bit_8", ByteMakeFromBit8Cmd, NULL, NULL, },
    { "bit_find_centroid", BitFindCentroidCmd, NULL, NULL, },
    { "byte_find_background_intensity",ByteFindBackgroundIntensityCmd,NULL,NULL},
    { "byte_make_from_bit_intersect", ByteMakeFromBitIntersectCmd, NULL, NULL},

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
 * Rvmvision_Init --
 *
 *      This procedure initializes the rvm vision library.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmvision_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tcldvmbasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, visioncmd, sizeof(visioncmd));

    return Tcl_PkgProvide(interp, "DvmVision", "1.0");
}
