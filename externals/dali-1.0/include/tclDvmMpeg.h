/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _TCL_DVM_MPEG_H_
#define _TCL_DVM_MPEG_H_

#include "tclDvmBasic.h"
#include "dvmmpeg.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main Tcl interface header file for MPEG package.
 * It includes MPEG Video, Audio and System stream.
 *
 * Wei Tsang Dec 97
 * Dan Rabinovitz July 98
 *---------------------------------------------------------------------
 */

    int MpegAnyHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * Sequence Header 
 */
    int MpegSeqHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSeqHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSeqHdrGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetNIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetBitRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetPicRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetAspectRatioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrGetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSeqHdrSetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetNIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetDefaultIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetDefaultNIQTCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetBitRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetPicRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetAspectRatioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSeqHdrSetConstrainedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSeqEnderCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
/* temporary */
    int MpegSeqHdrSetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * GOP Header 
 */
    int MpegGopHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegGopHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegGopHdrGetBrokenLinkCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrGetTimeCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrGetClosedGopCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrGetStartCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegGopHdrSetDropFrameFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetHoursCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetMinutesCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetSecondsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetPicturesCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetClosedGopCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGopHdrSetBrokenLinkCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
/* temporary */
    int MpegGopHdrSetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * Picture Header
 */
    int MpegPicHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPicHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPicHdrGetTypeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrGetTemporalRefCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrGetStartCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPicHdrSetTemporalRefCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetTypeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetVBVDelayCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetFullPelForwardCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetForwardFCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetFullPelBackwardCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicHdrSetBackwardFCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
/* temporary */
    int MpegPicHdrSetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * Picture
 */
    int MpegPicDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicIParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicPParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicBParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPicIEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicPEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPicBEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * Motion Search
 */
    int BytePMotionVecSearchCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ByteBMotionVecSearchCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int ByteComputeIntermediatesCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * Audio Synthesis and Granule Data
 */
    int MpegAudioSynDataNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioGraDataNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioSynDataFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioGraDataFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/*
 * Audio Header
 */
    int MpegAudioHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegAudioHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegAudioHdrGetModeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrGetLayerCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrGetBitRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrGetSamplingRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioHdrGetStartCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* 
 * Audio Layer 1
 */
    int MpegAudioL1NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL1FreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegAudioL1MonoParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL1MonoEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL1StereoParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL1StereoEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL1ToAudioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/* 
 * Audio Layer 2
 */
    int MpegAudioL2NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2FreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegAudioL2MonoParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2MonoEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2StereoParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2StereoEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2ToAudioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL2ScaleFactorSumCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/* 
 * Audio Layer 3
 */
    int MpegAudioL3NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL3FreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegAudioL3ParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL3StereoToAudioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegAudioL3MonoToAudioCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


/* 
 * Packet Header
 */

    int MpegPktHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPktHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPktHdrGetLengthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrGetStreamIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrGetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrGetPtsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrGetDtsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPktHdrSetLengthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrSetStreamIdCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrSetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrSetPtsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPktHdrSetDtsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPckHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegPckHdrGetSysClockRefCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrGetMuxRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrSetSysClockRefCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegPckHdrSetMuxRateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSysHdrNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrFindCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSkipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSysHdrGetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetRateBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetAudioBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetVideoBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetFixedFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetCspsFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetAudioLockCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetVideoLockCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrGetNumOfStreamInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSysHdrSetBufferSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetRateBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetAudioBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetVideoBoundCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetFixedFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetCspsFlagCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetAudioLockCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetVideoLockCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysHdrSetNumOfStreamInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSysTocNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocAddCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocGetFilterCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocGetOffsetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocListFiltersCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocReadCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegSysTocWriteCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegSeqEndCodeEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegGetCurrStartCodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

    int MpegVideoIndexNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexParseCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexEncodeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexGetOffsetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexGetTypeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexGetPastCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexGetNextCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexGetLengthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexNumRefsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexFindRefsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexTableAddCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
    int MpegVideoIndexResizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));


#define MPEG_VIDEO_INDEX_PREFIX "dvmMpgVI_"
#define FindMpegVideoIndex(s)           (!strncmp(s, MPEG_VIDEO_INDEX_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegVideoIndex(s)            (!strncmp(s, MPEG_VIDEO_INDEX_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegVideoIndex(s)         (!strncmp(s, MPEG_VIDEO_INDEX_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegVideoIndex(interp, buf)  PutBuf(interp, MPEG_VIDEO_INDEX_PREFIX, buf)


#define MPEG_HDR_PREFIX "dvmMpgSHd"
#define FindMpegSeqHdr(s)          (!strncmp(s, MPEG_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegSeqHdr(s)           (!strncmp(s, MPEG_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegSeqHdr(s)        (!strncmp(s, MPEG_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegSeqHdr(interp, buf) PutBuf(interp, MPEG_HDR_PREFIX, buf)

#define MPEG_GOP_HDR_PREFIX "dvmMpgGHd"
#define FindMpegGopHdr(s)          (!strncmp(s, MPEG_GOP_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegGopHdr(s)           (!strncmp(s, MPEG_GOP_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegGopHdr(s)        (!strncmp(s, MPEG_GOP_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegGopHdr(interp, buf) PutBuf(interp, MPEG_GOP_HDR_PREFIX, buf)

#define MPEG_PIC_HDR_PREFIX "dvmMpgPHd"
#define FindMpegPicHdr(s)          (!strncmp(s, MPEG_PIC_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegPicHdr(s)           (!strncmp(s, MPEG_PIC_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegPicHdr(s)        (!strncmp(s, MPEG_PIC_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegPicHdr(interp, buf) PutBuf(interp, MPEG_PIC_HDR_PREFIX, buf)

#define MPEG_PIC_PREFIX "dvmMpgPic"
#define FindMpegPic(s)          (!strncmp(s, MPEG_PIC_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegPic(s)           (!strncmp(s, MPEG_PIC_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegPic(s)        (!strncmp(s, MPEG_PIC_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegPic(interp, buf) PutBuf(interp, MPEG_PIC_PREFIX, buf)

#define MPEG_AUDIO_HDR_PREFIX "dvmMpgAHd"
#define FindMpegAudioHdr(s)          (!strncmp(s, MPEG_AUDIO_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegAudioHdr(s)           (!strncmp(s, MPEG_AUDIO_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioHdr(s)        (!strncmp(s, MPEG_AUDIO_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegAudioHdr(interp, buf) PutBuf(interp, MPEG_AUDIO_HDR_PREFIX, buf)

#define MPEG_AUDIO_L1_PREFIX "dvmMpgAL1"
#define FindMpegAudioL1(s)          (!strncmp(s, MPEG_AUDIO_L1_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegAudioL1(s)           (!strncmp(s, MPEG_AUDIO_L1_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioL1(s)        (!strncmp(s, MPEG_AUDIO_L1_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegAudioL1(interp, buf) PutBuf(interp, MPEG_AUDIO_L1_PREFIX, buf)

#define MPEG_AUDIO_L2_PREFIX "dvmMpgAL2"
#define FindMpegAudioL2(s)          (!strncmp(s, MPEG_AUDIO_L2_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegAudioL2(s)           (!strncmp(s, MPEG_AUDIO_L2_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioL2(s)        (!strncmp(s, MPEG_AUDIO_L2_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegAudioL2(interp, buf) PutBuf(interp, MPEG_AUDIO_L2_PREFIX, buf)

#define MPEG_AUDIO_L3_PREFIX "dvmMpgAL3"
#define FindMpegAudioL3(s)          (!strncmp(s, MPEG_AUDIO_L3_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegAudioL3(s)           (!strncmp(s, MPEG_AUDIO_L3_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioL3(s)        (!strncmp(s, MPEG_AUDIO_L3_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegAudioL3(interp, buf) PutBuf(interp, MPEG_AUDIO_L3_PREFIX, buf)

#define MPEG_AUDIO_SYN_DATA_PREFIX "dvmMpgSyn"
#define FindMpegAudioSynData(s)          (!strncmp(s, MPEG_AUDIO_SYN_DATA_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegAudioSynData(s)           (!strncmp(s, MPEG_AUDIO_SYN_DATA_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioSynData(s)        (!strncmp(s, MPEG_AUDIO_SYN_DATA_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegAudioSynData(interp, buf) PutBuf(interp, MPEG_AUDIO_SYN_DATA_PREFIX, buf)

#define MPEG_AUDIO_GRA_DATA_PREFIX "dvmMpgGra"
#define FindMpegAudioGraData(s)         (!strncmp(s, MPEG_AUDIO_GRA_DATA_PREFIX, 9)?  FindBuf(s) : NULL)
#define GetMpegAudioGraData(s)           (!strncmp(s, MPEG_AUDIO_GRA_DATA_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegAudioGraData(s)       (!strncmp(s, MPEG_AUDIO_GRA_DATA_PREFIX, 9)?  RemoveBuf(s) : NULL)
#define PutMpegAudioGraData(interp, buf) PutBuf(interp, MPEG_AUDIO_GRA_DATA_PREFIX, buf)

#define MPEG_PKT_HDR_PREFIX "dvmMpgPtH"
#define FindMpegPktHdr(s)          (!strncmp(s, MPEG_PKT_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegPktHdr(s)           (!strncmp(s, MPEG_PKT_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegPktHdr(s)        (!strncmp(s, MPEG_PKT_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegPktHdr(interp, buf) PutBuf(interp, MPEG_PKT_HDR_PREFIX, buf)

#define MPEG_PCK_HDR_PREFIX "dvmMpgPcH"
#define FindMpegPckHdr(s)          (!strncmp(s, MPEG_PCK_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegPckHdr(s)           (!strncmp(s, MPEG_PCK_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegPckHdr(s)        (!strncmp(s, MPEG_PCK_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegPckHdr(interp, buf) PutBuf(interp, MPEG_PCK_HDR_PREFIX, buf)

#define MPEG_SYS_HDR_PREFIX "dvmMpgSsH"
#define FindMpegSysHdr(s)          (!strncmp(s, MPEG_SYS_HDR_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegSysHdr(s)           (!strncmp(s, MPEG_SYS_HDR_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegSysHdr(s)        (!strncmp(s, MPEG_SYS_HDR_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegSysHdr(interp, buf) PutBuf(interp, MPEG_SYS_HDR_PREFIX, buf)

#define MPEG_SYS_TOC_PREFIX "dvmMpgToc"
#define FindMpegSysToc(s)          (!strncmp(s, MPEG_SYS_TOC_PREFIX, 9)? FindBuf(s) : NULL)
#define GetMpegSysToc(s)           (!strncmp(s, MPEG_SYS_TOC_PREFIX, 9)? GetBuf(s) : NULL)
#define RemoveMpegSysToc(s)        (!strncmp(s, MPEG_SYS_TOC_PREFIX, 9)? RemoveBuf(s) : NULL)
#define PutMpegSysToc(interp, buf) PutBuf(interp, MPEG_SYS_TOC_PREFIX, buf)

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
