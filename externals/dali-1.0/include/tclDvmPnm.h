/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef TCL_DVM_PNM_H
#define TCL_DVM_PNM_H

#include "tclDvmBasic.h"
#include "dvmpnm.h"
#ifdef __cplusplus
extern "C" {
#endif

    int PnmHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int PnmHdrGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrGetTypeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrGetMaxValCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int PnmHdrSetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrSetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrSetTypeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int PnmHdrSetMaxValCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int ByteCastToBitStreamCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int BitStreamCastToByteCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int BitCastToBitStreamCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int BitStreamCastToBitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int PpmParseCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PpmEncodeCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PgmParseCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PgmEncodeCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PbmParseCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PbmParse8Cmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PbmEncodeCmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));
    int PbmEncode8Cmd _ANSI_ARGS_((ClientData, Tcl_Interp *, int, char *argv[]));

#define PNM_HDR_PREFIX "dvmPnmHdr"
#define GetPnmHdr(s) (!strncmp(s, PNM_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemovePnmHdr(s) (!strncmp(s, PNM_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutPnmHdr(interp, buf) PutBuf(interp, PNM_HDR_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
