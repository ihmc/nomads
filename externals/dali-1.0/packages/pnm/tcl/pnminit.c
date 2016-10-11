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
 * pnminit.c --
 *
 *      This file contains the code to make the Dali VM DLL for Tcl.
 *
 */

#include "tclDvmPnm.h"

EXPORT(int,Tcldvmpnm_Init) _ANSI_ARGS_((Tcl_Interp *interp));

EXTERN void InitHashTable _ANSI_ARGS_((Tcl_Interp *interp));

static Commands cmd[] =
{
    { "pnm_hdr_new", PnmHdrNewCmd, NULL, NULL, },
    { "pnm_hdr_free", PnmHdrFreeCmd, NULL, NULL, },
    { "pnm_hdr_parse", PnmHdrParseCmd, NULL, NULL, },
    { "pnm_hdr_encode", PnmHdrEncodeCmd, NULL, NULL, },
    { "pnm_hdr_copy", PnmHdrCopyCmd, NULL, NULL, },

    { "pnm_hdr_get_width", PnmHdrGetWidthCmd, NULL, NULL, },
    { "pnm_hdr_get_height", PnmHdrGetHeightCmd, NULL, NULL, },
    { "pnm_hdr_get_type", PnmHdrGetTypeCmd, NULL, NULL, },
    { "pnm_hdr_get_maxval", PnmHdrGetMaxValCmd, NULL, NULL, },

    { "pnm_hdr_set_width", PnmHdrSetWidthCmd, NULL, NULL, },
    { "pnm_hdr_set_height", PnmHdrSetHeightCmd, NULL, NULL, },
    { "pnm_hdr_set_type", PnmHdrSetTypeCmd, NULL, NULL, },
    { "pnm_hdr_set_maxval", PnmHdrSetMaxValCmd, NULL, NULL, },

    { "byte_cast_to_bitstream", ByteCastToBitStreamCmd, NULL, NULL, },
    { "bitstream_cast_to_byte", BitStreamCastToByteCmd, NULL, NULL, },
    { "bit_cast_to_bitstream", BitCastToBitStreamCmd, NULL, NULL, },
    { "bitstream_cast_to_bit", BitStreamCastToBitCmd, NULL, NULL, },

    { "ppm_parse",  PpmParseCmd, NULL, NULL, },
    { "ppm_encode", PpmEncodeCmd, NULL, NULL, },
    { "pgm_parse",  PgmParseCmd, NULL, NULL, },
    { "pgm_encode", PgmEncodeCmd, NULL, NULL, },
    { "pbm_parse_8",  PbmParse8Cmd, NULL, NULL, },
    { "pbm_parse",  PbmParseCmd, NULL, NULL, },
    { "pbm_encode_8", PbmEncode8Cmd, NULL, NULL, },
    { "pbm_encode", PbmEncodeCmd, NULL, NULL, },
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
 * DvmPnmioInit --
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

EXPORT(int,Tcldvmpnm_Init)(interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf (interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands (interp, cmd, sizeof(cmd));
    InitHashTable (interp);
    return Tcl_PkgProvide(interp, "DvmPnm", "1.0");
}
