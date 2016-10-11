/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _TCL_DVM_JPEG_H_
#define _TCL_DVM_JPEG_H_

#include "tclDvmBasic.h"
#include "dvmjpeg.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for JPEG package.
 *
 * Jan 98 - Sugata
 *---------------------------------------------------------------------
 */


    int JpegHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetPrecisionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetNumOfComponentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetRestartIntervalCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetComponentIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetQtIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetQtCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetBlockWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetBlockHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetMaxBlockWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrGetMaxBlockHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


    int JpegScanHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrGetNumOfComponentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrGetScanIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrGetAcIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrGetDcIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


    int JpegScanParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanSelectiveParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanIncParseStartCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanIncParseEndCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanIncParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


    int JpegHdrSetQtCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetNumOfComponentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetPrecisionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetRestartIntervalCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetBlockWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetBlockHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetMaxBlockWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetMaxBlockHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetBlockWidthsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetBlockHeightsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetQtIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetQtIdsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrSetComponentIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrStdQtInitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrStdHtInitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int JpegScanHdrSetNumOfComponentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetScanIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetDcIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetAcIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetScanIdsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetDcIdsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrSetAcIdsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int JpegHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanEncode420Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanEncode422Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanIncEncode420Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegScanIncEncode422Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegEndCodeEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegStartCodeEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrHtEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHdrQtEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int JpegHuffTableNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHuffTableInitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int JpegHuffTableFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

#define JPEG_HDR_PREFIX "dvmJpgHdr"
#define GetJpegHdr(s) (!strncmp(s,JPEG_HDR_PREFIX, 9)?GetBuf(s):NULL)
#define FindJpegHdr(p)   FindBuf(p)
#define RemoveJpegHdr(s) (!strncmp(s,JPEG_HDR_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutJpegHdr(interp, buf) PutBuf(interp, JPEG_HDR_PREFIX, buf)

#define JPEG_SCANHDR_PREFIX "dvmJpgShd"
#define GetJpegScanHdr(s) (!strncmp(s,JPEG_SCANHDR_PREFIX, 9)?GetBuf(s):NULL)
#define FindJpegScanHdr(p)   FindBuf(p)
#define RemoveJpegScanHdr(s) (!strncmp(s,JPEG_SCANHDR_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutJpegScanHdr(interp, buf) PutBuf(interp,JPEG_SCANHDR_PREFIX, buf)

#define JPEG_HUFFTAB_PREFIX "dvmJpgHfT"
#define GetJpegHuffTable(s) (!strncmp(s,JPEG_HUFFTAB_PREFIX, 9)?GetBuf(s):NULL)
#define FindJpegHuffTable(p)   FindBuf(p)
#define RemoveJpegHuffTable(s) (!strncmp(s,JPEG_HUFFTAB_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutJpegHuffTable(interp, buf) PutBuf(interp,JPEG_HUFFTAB_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
