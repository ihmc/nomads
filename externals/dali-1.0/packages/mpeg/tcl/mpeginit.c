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
 * mpeginit.c --
 *
 *      This file contains the code to make the RVM mpeg DLL for Tcl.
 *
 */

#include "tclDvmMpeg.h"

/*
 * Declarations for functions defined in this file.
 */

EXPORT(int, Tcldvmmpeg_Init) 
_ANSI_ARGS_((Tcl_Interp * interp));

    static Commands mpegcmd[] =
    {
        {"mpeg_get_curr_start_code", MpegGetCurrStartCodeCmd, NULL, NULL,},
        {"mpeg_any_hdr_find", MpegAnyHdrFindCmd, NULL, NULL,},
        {"mpeg_seq_end_code_encode", MpegSeqEndCodeEncodeCmd, NULL, NULL,},

    /* MPEG Sequence Header */

        {"mpeg_seq_hdr_new", MpegSeqHdrNewCmd, NULL, NULL,},
        {"mpeg_seq_hdr_free", MpegSeqHdrFreeCmd, NULL, NULL,},

        {"mpeg_seq_hdr_find", MpegSeqHdrFindCmd, NULL, NULL,},
        {"mpeg_seq_hdr_dump", MpegSeqHdrDumpCmd, NULL, NULL,},
        {"mpeg_seq_hdr_skip", MpegSeqHdrSkipCmd, NULL, NULL,},
        {"mpeg_seq_hdr_parse", MpegSeqHdrParseCmd, NULL, NULL,},
        {"mpeg_seq_hdr_encode", MpegSeqHdrEncodeCmd, NULL, NULL,},
        {"mpeg_seq_ender", MpegSeqEnderCmd, NULL, NULL,},

        {"mpeg_seq_hdr_get_width", MpegSeqHdrGetWidthCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_height", MpegSeqHdrGetHeightCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_pic_rate", MpegSeqHdrGetPicRateCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_bit_rate", MpegSeqHdrGetBitRateCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_aspect_ratio", MpegSeqHdrGetAspectRatioCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_buffer_size", MpegSeqHdrGetBufferSizeCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_iqt", MpegSeqHdrGetIQTCmd, NULL, NULL,},
        {"mpeg_seq_hdr_get_niqt", MpegSeqHdrGetNIQTCmd, NULL, NULL,},
    /*{ "mpeg_seq_hdr_info",  MpegSeqHdrInfoCmd, NULL, NULL, }, */
    /*{ "mpeg_seq_hdr_init",  MpegSeqHdrInitCmd, NULL, NULL, }, */
    /* temporary, to be deleted */
        {"mpeg_seq_hdr_set", MpegSeqHdrSetCmd, NULL, NULL,},

        {"mpeg_seq_hdr_set_width", MpegSeqHdrSetWidthCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_height", MpegSeqHdrSetHeightCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_pic_rate", MpegSeqHdrSetPicRateCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_bit_rate", MpegSeqHdrSetBitRateCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_aspect_ratio", MpegSeqHdrSetAspectRatioCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_buffer_size", MpegSeqHdrSetBufferSizeCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_constrained", MpegSeqHdrSetConstrainedCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_iqt", MpegSeqHdrSetIQTCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_niqt", MpegSeqHdrSetNIQTCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_default_iqt", MpegSeqHdrSetDefaultIQTCmd, NULL, NULL,},
        {"mpeg_seq_hdr_set_default_niqt", MpegSeqHdrSetDefaultNIQTCmd, NULL, NULL,},

    /* MPEG GOP Header */

        {"mpeg_gop_hdr_new", MpegGopHdrNewCmd, NULL, NULL,},
        {"mpeg_gop_hdr_free", MpegGopHdrFreeCmd, NULL, NULL,},

        {"mpeg_gop_hdr_find", MpegGopHdrFindCmd, NULL, NULL,},
        {"mpeg_gop_hdr_dump", MpegGopHdrDumpCmd, NULL, NULL,},
        {"mpeg_gop_hdr_skip", MpegGopHdrSkipCmd, NULL, NULL,},
        {"mpeg_gop_hdr_parse", MpegGopHdrParseCmd, NULL, NULL,},
        {"mpeg_gop_hdr_encode", MpegGopHdrEncodeCmd, NULL, NULL,},

        {"mpeg_gop_hdr_get_broken_link", MpegGopHdrGetBrokenLinkCmd, NULL, NULL,},
        {"mpeg_gop_hdr_get_closed_gop", MpegGopHdrGetClosedGopCmd, NULL, NULL,},

    /*{ "mpeg_gop_hdr_info",  MpegGopHdrGetInfoCmd, NULL, NULL, }, */
    /*{ "mpeg_gop_hdr_init",  MpegGopHdrGetInitCmd, NULL, NULL, }, */
    /* temporary, to be deleted */
        {"mpeg_gop_hdr_set", MpegGopHdrSetCmd, NULL, NULL,},

        {"mpeg_gop_hdr_set_drop_frame_flag", MpegGopHdrSetDropFrameFlagCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_hours", MpegGopHdrSetHoursCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_minutes", MpegGopHdrSetMinutesCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_seconds", MpegGopHdrSetSecondsCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_pictures", MpegGopHdrSetPicturesCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_closed_gop", MpegGopHdrSetClosedGopCmd, NULL, NULL,},
        {"mpeg_gop_hdr_set_broken_link", MpegGopHdrSetBrokenLinkCmd, NULL, NULL,},

    /* MPEG Picture Header */

        {"mpeg_pic_hdr_new", MpegPicHdrNewCmd, NULL, NULL},
        {"mpeg_pic_hdr_free", MpegPicHdrFreeCmd, NULL, NULL},

        {"mpeg_pic_hdr_find", MpegPicHdrFindCmd, NULL, NULL},
        {"mpeg_pic_hdr_dump", MpegPicHdrDumpCmd, NULL, NULL},
        {"mpeg_pic_hdr_skip", MpegPicHdrSkipCmd, NULL, NULL},
        {"mpeg_pic_hdr_parse", MpegPicHdrParseCmd, NULL, NULL},
        {"mpeg_pic_hdr_encode", MpegPicHdrEncodeCmd, NULL, NULL},

        {"mpeg_pic_hdr_get_type", MpegPicHdrGetTypeCmd, NULL, NULL},
        {"mpeg_pic_hdr_get_temporal_ref", MpegPicHdrGetTemporalRefCmd, NULL, NULL},

        {"mpeg_pic_hdr_set_temporal_ref", MpegPicHdrSetTemporalRefCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_type", MpegPicHdrSetTypeCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_vbv_delay", MpegPicHdrSetVBVDelayCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_full_pel_forward", MpegPicHdrSetFullPelForwardCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_forward_f_code", MpegPicHdrSetForwardFCodeCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_full_pel_backward", MpegPicHdrSetFullPelBackwardCmd, NULL, NULL,},
        {"mpeg_pic_hdr_set_backward_f_code", MpegPicHdrSetBackwardFCodeCmd, NULL, NULL,},

    /*{ "mpeg_pic_hdr_info", MpegPicHdrInfoCmd, NULL, NULL }, */
    /*{ "mpeg_pic_hdr_init", MpegPicHdrInitCmd, NULL, NULL }, */
        {"mpeg_pic_hdr_set", MpegPicHdrSetCmd, NULL, NULL},

    /* MPEG Picture */

        {"mpeg_pic_dump", MpegPicDumpCmd, NULL, NULL},
        {"mpeg_pic_skip", MpegPicSkipCmd, NULL, NULL},
        {"mpeg_pic_i_parse", MpegPicIParseCmd, NULL, NULL,},
        {"mpeg_pic_p_parse", MpegPicPParseCmd, NULL, NULL,},
        {"mpeg_pic_b_parse", MpegPicBParseCmd, NULL, NULL,},
        {"mpeg_pic_i_encode", MpegPicIEncodeCmd, NULL, NULL,},
        {"mpeg_pic_p_encode", MpegPicPEncodeCmd, NULL, NULL,},
        {"mpeg_pic_b_encode", MpegPicBEncodeCmd, NULL, NULL,},

        {"byte_p_motion_vec_search", BytePMotionVecSearchCmd, NULL, NULL,},
        {"byte_b_motion_vec_search", ByteBMotionVecSearchCmd, NULL, NULL,},
        {"byte_compute_intermediates", ByteComputeIntermediatesCmd, NULL, NULL,},

    /* MPEG Audio Header */

        {"mpeg_audio_hdr_new", MpegAudioHdrNewCmd, NULL, NULL,},
        {"mpeg_audio_hdr_free", MpegAudioHdrFreeCmd, NULL, NULL,},

        {"mpeg_audio_hdr_find", MpegAudioHdrFindCmd, NULL, NULL,},
        {"mpeg_audio_hdr_dump", MpegAudioHdrDumpCmd, NULL, NULL,},
        {"mpeg_audio_hdr_skip", MpegAudioHdrSkipCmd, NULL, NULL,},
        {"mpeg_audio_hdr_parse", MpegAudioHdrParseCmd, NULL, NULL,},
        {"mpeg_audio_hdr_encode", MpegAudioHdrEncodeCmd, NULL, NULL,},

        {"mpeg_audio_hdr_get_mode", MpegAudioHdrGetModeCmd, NULL, NULL,},
        {"mpeg_audio_hdr_get_layer", MpegAudioHdrGetLayerCmd, NULL, NULL,},
        {"mpeg_audio_hdr_get_bit_rate", MpegAudioHdrGetBitRateCmd, NULL, NULL,},
        {"mpeg_audio_hdr_get_sampling_rate", MpegAudioHdrGetSamplingRateCmd, NULL, NULL,},
    /*{ "mpeg_audio_hdr_info", MpegAudioHdrInfoCmd, NULL, NULL, }, */
    /*{ "mpeg_audio_hdr_init", MpegAudioHdrInitCmd, NULL, NULL, }, */

    /* MPEG Audio samples */

        {"mpeg_audio_l1_new", MpegAudioL1NewCmd, NULL, NULL,},
        {"mpeg_audio_l1_free", MpegAudioL1FreeCmd, NULL, NULL,},
        {"mpeg_audio_l1_mono_parse", MpegAudioL1MonoParseCmd, NULL, NULL,},
        {"mpeg_audio_l1_mono_encode", MpegAudioL1MonoEncodeCmd, NULL, NULL,},
        {"mpeg_audio_l1_stereo_parse", MpegAudioL1StereoParseCmd, NULL, NULL,},
        {"mpeg_audio_l1_stereo_encode", MpegAudioL1StereoEncodeCmd, NULL, NULL,},
        {"mpeg_audio_l1_to_audio", MpegAudioL1ToAudioCmd, NULL, NULL,},

        {"mpeg_audio_l2_new", MpegAudioL2NewCmd, NULL, NULL,},
        {"mpeg_audio_l2_free", MpegAudioL2FreeCmd, NULL, NULL,},
        {"mpeg_audio_l2_mono_parse", MpegAudioL2MonoParseCmd, NULL, NULL,},
        {"mpeg_audio_l2_mono_encode", MpegAudioL2MonoEncodeCmd, NULL, NULL,},
        {"mpeg_audio_l2_stereo_parse", MpegAudioL2StereoParseCmd, NULL, NULL,},
        {"mpeg_audio_l2_stereo_encode", MpegAudioL2StereoEncodeCmd, NULL, NULL,},
        {"mpeg_audio_l2_to_audio", MpegAudioL2ToAudioCmd, NULL, NULL,},
        {"mpeg_audio_l2_scale_factor_sum", MpegAudioL2ScaleFactorSumCmd, NULL, NULL,},

        {"mpeg_audio_l3_new", MpegAudioL3NewCmd, NULL, NULL,},
        {"mpeg_audio_l3_free", MpegAudioL3FreeCmd, NULL, NULL,},
        {"mpeg_audio_l3_parse", MpegAudioL3ParseCmd, NULL, NULL,},
        {"mpeg_audio_l3_mono_to_audio", MpegAudioL3MonoToAudioCmd, NULL, NULL,},
        {"mpeg_audio_l3_stereo_to_audio", MpegAudioL3StereoToAudioCmd, NULL, NULL,},

        {"mpeg_audio_gra_data_free", MpegAudioGraDataFreeCmd, NULL, NULL,},
        {"mpeg_audio_syn_data_free", MpegAudioSynDataFreeCmd, NULL, NULL,},
        {"mpeg_audio_gra_data_new", MpegAudioGraDataNewCmd, NULL, NULL,},
        {"mpeg_audio_syn_data_new", MpegAudioSynDataNewCmd, NULL, NULL,},

        {"mpeg_pkt_hdr_new", MpegPktHdrNewCmd, NULL, NULL},
        {"mpeg_pkt_hdr_free", MpegPktHdrFreeCmd, NULL, NULL},
        {"mpeg_pkt_hdr_find", MpegPktHdrFindCmd, NULL, NULL},
        {"mpeg_pkt_hdr_dump", MpegPktHdrDumpCmd, NULL, NULL},
        {"mpeg_pkt_hdr_skip", MpegPktHdrSkipCmd, NULL, NULL},
        {"mpeg_pkt_hdr_parse", MpegPktHdrParseCmd, NULL, NULL},
        {"mpeg_pkt_hdr_encode", MpegPktHdrEncodeCmd, NULL, NULL},
        {"mpeg_pkt_hdr_get_length", MpegPktHdrGetLengthCmd, NULL, NULL},
        {"mpeg_pkt_hdr_get_stream_id", MpegPktHdrGetStreamIdCmd, NULL, NULL},
        {"mpeg_pkt_hdr_get_buffer_size", MpegPktHdrGetBufferSizeCmd, NULL, NULL},
        {"mpeg_pkt_hdr_get_pts", MpegPktHdrGetPtsCmd, NULL, NULL},
        {"mpeg_pkt_hdr_get_dts", MpegPktHdrGetDtsCmd, NULL, NULL},
    /*{ "mpeg_pkt_hdr_info",MpegPktHdrInfoCmd, NULL, NULL }, */
    /*{ "mpeg_pkt_hdr_init",MpegPktHdrInfoCmd, NULL, NULL }, */

        {"mpeg_pkt_hdr_set_length", MpegPktHdrSetLengthCmd, NULL, NULL},
        {"mpeg_pkt_hdr_set_stream_id", MpegPktHdrSetStreamIdCmd, NULL, NULL},
        {"mpeg_pkt_hdr_set_buffer_size", MpegPktHdrSetBufferSizeCmd, NULL, NULL},
        {"mpeg_pkt_hdr_set_pts", MpegPktHdrSetPtsCmd, NULL, NULL},
        {"mpeg_pkt_hdr_set_dts", MpegPktHdrSetDtsCmd, NULL, NULL},

        {"mpeg_pck_hdr_new", MpegPckHdrNewCmd, NULL, NULL},
        {"mpeg_pck_hdr_free", MpegPckHdrFreeCmd, NULL, NULL},
        {"mpeg_pck_hdr_find", MpegPckHdrFindCmd, NULL, NULL},
        {"mpeg_pck_hdr_dump", MpegPckHdrDumpCmd, NULL, NULL},
        {"mpeg_pck_hdr_skip", MpegPckHdrSkipCmd, NULL, NULL},
        {"mpeg_pck_hdr_parse", MpegPckHdrParseCmd, NULL, NULL},
        {"mpeg_pck_hdr_encode", MpegPckHdrEncodeCmd, NULL, NULL},
        {"mpeg_pck_hdr_get_mux_rate", MpegPckHdrGetMuxRateCmd, NULL, NULL},
        {"mpeg_pck_hdr_get_sys_clock_ref", MpegPckHdrGetSysClockRefCmd, NULL, NULL},
        {"mpeg_pck_hdr_set_mux_rate", MpegPckHdrSetMuxRateCmd, NULL, NULL},
        {"mpeg_pck_hdr_set_sys_clock_ref", MpegPckHdrSetSysClockRefCmd, NULL, NULL},
    /*{ "mpeg_pck_hdr_info",MpegPckHdrInfoCmd, NULL, NULL }, */
    /*{ "mpeg_pck_hdr_init",MpegPckHdrInitCmd, NULL, NULL }, */

        {"mpeg_sys_hdr_new", MpegSysHdrNewCmd, NULL, NULL},
        {"mpeg_sys_hdr_free", MpegSysHdrFreeCmd, NULL, NULL},
        {"mpeg_sys_hdr_find", MpegSysHdrFindCmd, NULL, NULL},
        {"mpeg_sys_hdr_dump", MpegSysHdrDumpCmd, NULL, NULL},
        {"mpeg_sys_hdr_skip", MpegSysHdrSkipCmd, NULL, NULL},
        {"mpeg_sys_hdr_parse", MpegSysHdrParseCmd, NULL, NULL},
        {"mpeg_sys_hdr_encode", MpegSysHdrEncodeCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_rate_bound", MpegSysHdrGetRateBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_buffer_size", MpegSysHdrGetBufferSizeCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_video_bound", MpegSysHdrGetAudioBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_audio_bound", MpegSysHdrGetVideoBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_fixed_flag", MpegSysHdrGetFixedFlagCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_csps_flag", MpegSysHdrGetCspsFlagCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_audio_lock", MpegSysHdrGetAudioLockCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_video_lock", MpegSysHdrGetVideoLockCmd, NULL, NULL},
        {"mpeg_sys_hdr_get_num_of_stream_info", MpegSysHdrGetNumOfStreamInfoCmd, NULL, NULL},
    /*{ "mpeg_sys_hdr_info",MpegSysHdrInfoCmd, NULL, NULL }, */

        {"mpeg_sys_hdr_set_rate_bound", MpegSysHdrSetRateBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_buffer_size", MpegSysHdrSetBufferSizeCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_video_bound", MpegSysHdrSetAudioBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_audio_bound", MpegSysHdrSetVideoBoundCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_fixed_flag", MpegSysHdrSetFixedFlagCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_csps_flag", MpegSysHdrSetCspsFlagCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_audio_lock", MpegSysHdrSetAudioLockCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_video_lock", MpegSysHdrSetVideoLockCmd, NULL, NULL},
        {"mpeg_sys_hdr_set_num_of_stream_info", MpegSysHdrSetNumOfStreamInfoCmd, NULL, NULL},
    /*{ "mpeg_sys_hdr_init",MpegSysHdrInitCmd, NULL, NULL }, */

        {"mpeg_sys_toc_new", MpegSysTocNewCmd, NULL, NULL,},
        {"mpeg_sys_toc_free", MpegSysTocFreeCmd, NULL, NULL,},
        {"mpeg_sys_toc_add", MpegSysTocAddCmd, NULL, NULL,},
        {"mpeg_sys_toc_get_filter", MpegSysTocGetFilterCmd, NULL, NULL,},
        {"mpeg_sys_toc_get_offset", MpegSysTocGetOffsetCmd, NULL, NULL,},
        {"mpeg_sys_toc_list_filters", MpegSysTocListFiltersCmd, NULL, NULL,},
        {"mpeg_sys_toc_read", MpegSysTocReadCmd, NULL, NULL,},
        {"mpeg_sys_toc_write", MpegSysTocWriteCmd, NULL, NULL,},

    /* MPEG Video Index */
        {"mpeg_video_index_new", MpegVideoIndexNewCmd, NULL, NULL,},
        {"mpeg_video_index_free", MpegVideoIndexFreeCmd, NULL, NULL,},
        {"mpeg_video_index_parse", MpegVideoIndexParseCmd, NULL, NULL,},
        {"mpeg_video_index_encode", MpegVideoIndexEncodeCmd, NULL, NULL,},
        {"mpeg_video_index_numrefs", MpegVideoIndexNumRefsCmd, NULL, NULL,},
        {"mpeg_video_index_findrefs", MpegVideoIndexFindRefsCmd, NULL, NULL,},
        {"mpeg_video_index_get_type", MpegVideoIndexGetTypeCmd, NULL, NULL,},
        {"mpeg_video_index_get_next", MpegVideoIndexGetNextCmd, NULL, NULL,},
        {"mpeg_video_index_get_past", MpegVideoIndexGetPastCmd, NULL, NULL,},
        {"mpeg_video_index_get_offset", MpegVideoIndexGetOffsetCmd, NULL, NULL,},
        {"mpeg_video_index_get_length", MpegVideoIndexGetLengthCmd, NULL, NULL,},
        {"mpeg_video_index_table_add", MpegVideoIndexTableAddCmd, NULL, NULL,},
        {"mpeg_video_index_resize", MpegVideoIndexResizeCmd, NULL, NULL,},
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
 * Rvmmpeg_Init --
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

EXPORT(int, Tcldvmmpeg_Init) (interp)
    Tcl_Interp *interp;
{
    if (Tcl_PkgRequire(interp, "DvmBasic", "1.0", 1) == NULL) {
        sprintf(interp->result, "Error loading tclDvmBasic package");
        return TCL_ERROR;
    }
    CreateCommands(interp, mpegcmd, sizeof(mpegcmd));
    return Tcl_PkgProvide(interp, "DvmMpeg", "1.0");
}
