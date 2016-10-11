/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*-------------------------------------------------------------------------- 
 * jpeginit.c 
 *
 *      This file contains the code to make the Dali VM jpeg DLL for Tcl.
 *
 *-------------------------------------------------------------------------- 
 */

#include "tclDvmJpeg.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int,Tcldvmjpeg_Init) _ANSI_ARGS_((Tcl_Interp *interp));

static Commands jpegcmd[] =
{
    /*
     * Allocations
     */
    { "jpeg_hdr_new", JpegHdrNewCmd, NULL, NULL},
    { "jpeg_hdr_free", JpegHdrFreeCmd, NULL, NULL},
    { "jpeg_scan_hdr_new", JpegScanHdrNewCmd, NULL, NULL},
    { "jpeg_scan_hdr_free", JpegScanHdrFreeCmd, NULL, NULL},

    /*
     * Decoding
     */
    { "jpeg_hdr_parse", JpegHdrParseCmd, NULL, NULL, },
    { "jpeg_scan_hdr_parse", JpegScanHdrParseCmd, NULL, NULL,},
    { "jpeg_scan_parse", JpegScanParseCmd, NULL, NULL, },
    { "jpeg_scan_selective_parse", JpegScanSelectiveParseCmd, NULL, NULL, },
    { "jpeg_scan_inc_parse_start", JpegScanIncParseStartCmd, NULL, NULL, },
    { "jpeg_scan_inc_parse_end", JpegScanIncParseEndCmd, NULL, NULL, },
    { "jpeg_scan_inc_parse", JpegScanIncParseCmd, NULL, NULL, },

    /*
     * Encoding
     */
    { "jpeg_hdr_qt_encode", JpegHdrQtEncodeCmd, NULL, NULL,},
    { "jpeg_hdr_ht_encode", JpegHdrHtEncodeCmd, NULL, NULL,},
    { "jpeg_hdr_encode", JpegHdrEncodeCmd, NULL, NULL,},
    { "jpeg_scan_hdr_encode", JpegScanHdrEncodeCmd, NULL, NULL,},
    { "jpeg_scan_encode", JpegScanEncodeCmd, NULL, NULL,},
    { "jpeg_scan_encode_420", JpegScanEncode420Cmd, NULL, NULL,},
    { "jpeg_scan_encode_422", JpegScanEncode422Cmd, NULL, NULL,},
    { "jpeg_scan_inc_encode_420", JpegScanIncEncode420Cmd, NULL, NULL,},
    { "jpeg_scan_inc_encode_422", JpegScanIncEncode422Cmd, NULL, NULL,},
    { "jpeg_end_code_encode", JpegEndCodeEncodeCmd, NULL, NULL,},
    { "jpeg_start_code_encode", JpegStartCodeEncodeCmd, NULL, NULL,},

    /*
     * Header Query
     */
    { "jpeg_hdr_get_width", JpegHdrGetWidthCmd, NULL, NULL, },
    { "jpeg_hdr_get_height", JpegHdrGetHeightCmd, NULL, NULL, },
    { "jpeg_hdr_get_num_of_components", JpegHdrGetNumOfComponentsCmd, NULL, NULL, },
    { "jpeg_hdr_get_precision", JpegHdrGetPrecisionCmd, NULL, NULL, },
    { "jpeg_hdr_get_restart_interval", JpegHdrGetRestartIntervalCmd, NULL, NULL, },
    { "jpeg_hdr_get_max_block_width", JpegHdrGetMaxBlockWidthCmd, NULL, NULL, },
    { "jpeg_hdr_get_max_block_height", JpegHdrGetMaxBlockHeightCmd, NULL, NULL, },
    { "jpeg_hdr_get_component_id", JpegHdrGetComponentIdCmd, NULL, NULL, },
    { "jpeg_hdr_get_qt_id", JpegHdrGetQtIdCmd, NULL, NULL, },
    { "jpeg_hdr_get_qt", JpegHdrGetQtCmd, NULL, NULL, },
    { "jpeg_hdr_get_block_width", JpegHdrGetBlockWidthCmd, NULL, NULL, },
    { "jpeg_hdr_get_block_height", JpegHdrGetBlockHeightCmd, NULL, NULL, },

    /*
     * Scan Header Query
     */
    {"jpeg_scan_hdr_get_num_of_components", JpegScanHdrGetNumOfComponentsCmd, NULL, NULL,},
    {"jpeg_scan_hdr_get_dc_id", JpegScanHdrGetDcIdCmd, NULL, NULL,},
    {"jpeg_scan_hdr_get_ac_id", JpegScanHdrGetAcIdCmd, NULL, NULL,},
    {"jpeg_scan_hdr_get_scan_id", JpegScanHdrGetScanIdCmd, NULL, NULL,},

    /*
     * Header Initialization
     */
    {"jpeg_hdr_set_width", JpegHdrSetWidthCmd, NULL, NULL,},
    {"jpeg_hdr_set_height", JpegHdrSetHeightCmd, NULL, NULL,},
    {"jpeg_hdr_set_num_of_components", JpegHdrSetNumOfComponentsCmd, NULL, NULL,},
    {"jpeg_hdr_set_precision", JpegHdrSetPrecisionCmd, NULL, NULL,},
    {"jpeg_hdr_set_restart_interval", JpegHdrSetRestartIntervalCmd, NULL, NULL,},
    {"jpeg_hdr_set_block_width", JpegHdrSetBlockWidthCmd, NULL, NULL,},
    {"jpeg_hdr_set_block_height", JpegHdrSetBlockHeightCmd, NULL, NULL,},
    {"jpeg_hdr_set_max_block_width", JpegHdrSetMaxBlockWidthCmd, NULL, NULL,},
    {"jpeg_hdr_set_max_block_height", JpegHdrSetMaxBlockHeightCmd, NULL, NULL,},
    {"jpeg_hdr_std_qt_init", JpegHdrStdQtInitCmd, NULL, NULL,},
    {"jpeg_hdr_std_ht_init", JpegHdrStdHtInitCmd, NULL, NULL,},
    {"jpeg_hdr_set_qt", JpegHdrSetQtCmd, NULL, NULL,},
    {"jpeg_hdr_set_block_widths", JpegHdrSetBlockWidthsCmd, NULL, NULL,},
    {"jpeg_hdr_set_block_heights", JpegHdrSetBlockHeightsCmd, NULL, NULL,},
    {"jpeg_hdr_set_qt_id", JpegHdrSetQtIdCmd, NULL, NULL,},
    {"jpeg_hdr_set_qt_ids", JpegHdrSetQtIdsCmd, NULL, NULL,},
    {"jpeg_hdr_set_component_id", JpegHdrSetComponentIdCmd, NULL, NULL,},

    /*
     * Scan header initialization
     */
    {"jpeg_scan_hdr_set_num_of_components", JpegScanHdrSetNumOfComponentsCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_scan_id", JpegScanHdrSetScanIdCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_dc_id", JpegScanHdrSetDcIdCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_ac_id", JpegScanHdrSetAcIdCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_scan_ids", JpegScanHdrSetScanIdsCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_dc_ids", JpegScanHdrSetDcIdsCmd, NULL, NULL,},
    {"jpeg_scan_hdr_set_ac_ids", JpegScanHdrSetAcIdsCmd, NULL, NULL,},

    /*
     * Huffman table
     */
    {"jpeg_huff_table_new", JpegHuffTableNewCmd, NULL, NULL,},
    {"jpeg_huff_table_init", JpegHuffTableInitCmd, NULL, NULL,},
    {"jpeg_huff_table_free", JpegHuffTableFreeCmd, NULL, NULL,},

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
 * Tcldvmjpeg_Init --
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

EXPORT(int,Tcldvmjpeg_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading dvmbasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, jpegcmd, sizeof(jpegcmd));
    return Tcl_PkgProvide(interp, "DvmJpeg", "1.0");
}
