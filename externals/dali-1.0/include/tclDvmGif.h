/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _TCL_DVM_GIF_H_
#define _TCL_DVM_GIF_H_

#include "tclDvmBasic.h"
#include "tclDvmImap.h"
#include "dvmgif.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for GIF package.
 *
 * Steve Weiss January 1998
 *---------------------------------------------------------------------
 */

/* sequence manipulation */
    int GifSeqNumImgsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqLoopEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqTrailerEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/* color table manipulation */
    int GifCtParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifCtEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/* image manipulation */
    int GifImgHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgNonInterlacedParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgInterlacedParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* functions for getting member variables of gif sequence headers */
    int GifSeqHdrGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetCtFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetCtSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetCtSortedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetResolutionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetBackgroundColorCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetAspectRatioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrGetVersionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* functions for getting member variables of gif image headers */
    int GifImgHdrGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetCtFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetCtSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetLeftPositionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetTopPositionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetInterlacedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetGraphicControlFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetDisposalMethodCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetUserInputFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetTransparentColorFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetDelayTimeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrGetTransparentColorIndexCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* functions for setting member variables of gif sequence headers */
    int GifSeqHdrSetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetCtFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetCtSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetCtSortedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetResolutionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetBackgroundColorCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetAspectRatioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifSeqHdrSetVersionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* functions for setting member variables of gif image headers */
    int GifImgHdrSetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetCtFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetCtSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetLeftPositionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetTopPositionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetInterlacedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetGraphicControlFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetDisposalMethodCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetUserInputFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetTransparentColorFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetDelayTimeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int GifImgHdrSetTransparentColorIndexCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

#define GIF_SEQ_HDR_PREFIX "dvmGifSeq"
#define GetGifSeqHdr(s)           (!strncmp(s, GIF_SEQ_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveGifSeqHdr(s)        (!strncmp(s, GIF_SEQ_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutGifSeqHdr(interp, buf) PutBuf(interp, GIF_SEQ_HDR_PREFIX, buf)

#define GIF_IMG_HDR_PREFIX "dvmGifImg"
#define GetGifImgHdr(s)           (!strncmp(s, GIF_IMG_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveGifImgHdr(s)        (!strncmp(s, GIF_IMG_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutGifImgHdr(interp, buf) PutBuf(interp, GIF_IMG_HDR_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
