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
 * gifinit.c --
 *
 *      This file contains the code to make the RVM gif DLL for Tcl.
 *
 */

#include "tclDvmGif.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmgif_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands gifcmd[] =
{
    { "gif_seq_hdr_new", GifSeqHdrNewCmd, NULL, NULL, },
    { "gif_seq_hdr_parse", GifSeqHdrParseCmd, NULL, NULL, },
    { "gif_img_hdr_new", GifImgHdrNewCmd, NULL, NULL, },
    { "gif_img_hdr_parse", GifImgHdrParseCmd, NULL, NULL, },
    { "gif_img_interlaced_parse", GifImgInterlacedParseCmd, NULL, NULL, },
    { "gif_img_non_interlaced_parse", GifImgNonInterlacedParseCmd, NULL, NULL, },
    { "gif_img_skip", GifImgSkipCmd, NULL, NULL, },
    { "gif_img_find", GifImgFindCmd, NULL, NULL, },
    { "gif_seq_hdr_encode", GifSeqHdrEncodeCmd, NULL, NULL, },
    { "gif_img_hdr_encode", GifImgHdrEncodeCmd, NULL, NULL, },
    { "gif_img_encode", GifImgEncodeCmd, NULL, NULL, },
    { "gif_seq_trailer_encode", GifSeqTrailerEncodeCmd, NULL, NULL, },
    { "gif_seq_loop_encode", GifSeqLoopEncodeCmd, NULL, NULL, },
    { "gif_seq_hdr_free", GifSeqHdrFreeCmd, NULL, NULL, },
    { "gif_img_hdr_free", GifImgHdrFreeCmd, NULL, NULL, },

    { "gif_ct_parse", GifCtParseCmd, NULL, NULL, },
    { "gif_ct_encode", GifCtEncodeCmd, NULL, NULL, },

    { "gif_seq_hdr_get_width", GifSeqHdrGetWidthCmd, NULL, NULL, },
    { "gif_seq_hdr_get_height", GifSeqHdrGetHeightCmd, NULL, NULL, },
    { "gif_seq_hdr_get_ct_flag", GifSeqHdrGetCtFlagCmd, NULL, NULL, },
    { "gif_seq_hdr_get_ct_size", GifSeqHdrGetCtSizeCmd, NULL, NULL, },
    { "gif_seq_hdr_get_ct_sorted", GifSeqHdrGetCtSortedCmd, NULL, NULL, },
    { "gif_seq_hdr_get_resolution", GifSeqHdrGetResolutionCmd, NULL, NULL, },
    { "gif_seq_hdr_get_background_color", GifSeqHdrGetBackgroundColorCmd, NULL, NULL, },
    { "gif_seq_hdr_get_aspect_ratio", GifSeqHdrGetAspectRatioCmd, NULL, NULL, },
    { "gif_seq_hdr_get_version", GifSeqHdrGetVersionCmd, NULL, NULL, },

    { "gif_img_hdr_get_width", GifImgHdrGetWidthCmd, NULL, NULL, },
    { "gif_img_hdr_get_height", GifImgHdrGetHeightCmd, NULL, NULL, },
    { "gif_img_hdr_get_ct_flag", GifImgHdrGetCtFlagCmd, NULL, NULL, },
    { "gif_img_hdr_get_ct_size", GifImgHdrGetCtSizeCmd, NULL, NULL, },
    { "gif_img_hdr_get_left_position", GifImgHdrGetLeftPositionCmd, NULL, NULL, },
    { "gif_img_hdr_get_top_position", GifImgHdrGetTopPositionCmd, NULL, NULL, },
    { "gif_img_hdr_get_interlaced", GifImgHdrGetInterlacedCmd, NULL, NULL, },
    { "gif_img_hdr_get_graphic_control_flag", GifImgHdrGetGraphicControlFlagCmd, NULL, NULL, },
    { "gif_img_hdr_get_disposal_method", GifImgHdrGetDisposalMethodCmd, NULL, NULL, },
    { "gif_img_hdr_get_user_input_flag", GifImgHdrGetUserInputFlagCmd, NULL, NULL, },
    { "gif_img_hdr_get_transparent_color_flag", GifImgHdrGetTransparentColorFlagCmd, NULL, NULL, },
    { "gif_img_hdr_get_delay_time", GifImgHdrGetDelayTimeCmd, NULL, NULL, },
    { "gif_img_hdr_get_transparent_color_index", GifImgHdrGetTransparentColorIndexCmd, NULL, NULL, },

    { "gif_seq_hdr_set_width", GifSeqHdrSetWidthCmd, NULL, NULL, },
    { "gif_seq_hdr_set_height", GifSeqHdrSetHeightCmd, NULL, NULL, },
    { "gif_seq_hdr_set_ct_flag", GifSeqHdrSetCtFlagCmd, NULL, NULL, },
    { "gif_seq_hdr_set_ct_size", GifSeqHdrSetCtSizeCmd, NULL, NULL, },
    { "gif_seq_hdr_set_ct_sorted", GifSeqHdrSetCtSortedCmd, NULL, NULL, },
    { "gif_seq_hdr_set_resolution", GifSeqHdrSetResolutionCmd, NULL, NULL, },
    { "gif_seq_hdr_set_background_color", GifSeqHdrSetBackgroundColorCmd, NULL, NULL, },
    { "gif_seq_hdr_set_aspect_ratio", GifSeqHdrSetAspectRatioCmd, NULL, NULL, },
    { "gif_seq_hdr_set_version", GifSeqHdrSetVersionCmd, NULL, NULL, },

    { "gif_img_hdr_set_width", GifImgHdrSetWidthCmd, NULL, NULL, },
    { "gif_img_hdr_set_height", GifImgHdrSetHeightCmd, NULL, NULL, },
    { "gif_img_hdr_set_ct_flag", GifImgHdrSetCtFlagCmd, NULL, NULL, },
    { "gif_img_hdr_set_ct_size", GifImgHdrSetCtSizeCmd, NULL, NULL, },
    { "gif_img_hdr_set_left_position", GifImgHdrSetLeftPositionCmd, NULL, NULL, },
    { "gif_img_hdr_set_top_position", GifImgHdrSetTopPositionCmd, NULL, NULL, },
    { "gif_img_hdr_set_interlaced", GifImgHdrSetInterlacedCmd, NULL, NULL, },
    { "gif_img_hdr_set_graphic_control_flag", GifImgHdrSetGraphicControlFlagCmd, NULL, NULL, },
    { "gif_img_hdr_set_disposal_method", GifImgHdrSetDisposalMethodCmd, NULL, NULL, },
    { "gif_img_hdr_set_user_input_flag", GifImgHdrSetUserInputFlagCmd, NULL, NULL, },
    { "gif_img_hdr_set_transparent_color_flag", GifImgHdrSetTransparentColorFlagCmd, NULL, NULL, },
    { "gif_img_hdr_set_delay_time", GifImgHdrSetDelayTimeCmd, NULL, NULL, },
    { "gif_img_hdr_set_transparent_color_index", GifImgHdrSetTransparentColorIndexCmd, NULL, NULL, },

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
 * Dvmgif_Init --
 *
 *      This procedure initializes the dvm gif library.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int,Tcldvmgif_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tcldvmbasic package");
        return TCL_ERROR;
    }
    /*
    if (Tcl_PkgRequire(interp, "Rvmstreams", "1.0", 1) == NULL) {
    sprintf (interp->result, "Error loading rvmstreams package");
    return TCL_ERROR;
    }*/

    CreateCommands (interp, gifcmd, sizeof(gifcmd));

    return Tcl_PkgProvide(interp, "DvmGif", "1.0");
}
