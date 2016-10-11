/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * basicinit.c --
 *
 *        This file contains the code to make the Dali VM DLL for Tcl.
 *
 *------------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

/*
 * Declarations for functions defined in this file.
 */
EXPORT(int, Tcldvmbasic_Init) 
_ANSI_ARGS_((Tcl_Interp * interp));

/*
 * Declarations for externally defined functions called in this module
 */
    void InitHashTable _ANSI_ARGS_((Tcl_Interp * interp));

    static Commands basiccmd[] =
    {
    /*
     * Basic buffer operations.
     */
        {"byte_new", ByteNewCmd, NULL, NULL,},
        {"byte_free", ByteFreeCmd, NULL, NULL,},
        {"byte_reclip", ByteReclipCmd, NULL, NULL,},
        {"byte_clip", ByteClipCmd, NULL, NULL,},
        {"byte_copy", ByteCopyCmd, NULL, NULL,},
        {"byte_copy_mux_1", ByteCopyMuxCmd, (ClientData)1, NULL,},
        {"byte_copy_mux_2", ByteCopyMuxCmd, (ClientData)2, NULL,},
        {"byte_copy_mux_4", ByteCopyMuxCmd, (ClientData)4, NULL,},
        {"byte_set", ByteSetCmd, NULL, NULL,},
        {"byte_set_mux_1", ByteSetMuxCmd, (ClientData)1, NULL,},
        {"byte_set_mux_2", ByteSetMuxCmd, (ClientData)2, NULL,},
        {"byte_set_mux_4", ByteSetMuxCmd, (ClientData)4, NULL,},
        {"byte_extend", ByteExtendCmd, NULL, NULL,},
        {"byte_get_x", ByteGetXCmd, NULL, NULL,},
        {"byte_get_y", ByteGetYCmd, NULL, NULL,},
        {"byte_get_width", ByteGetWidthCmd, NULL, NULL,},
        {"byte_get_height", ByteGetHeightCmd, NULL, NULL,},
        {"byte_get_virtual", ByteGetVirtualCmd, NULL, NULL,},
        {"byte_copy_with_mask", ByteCopyWithMaskCmd, NULL, NULL,},
        {"byte_set_with_mask", ByteSetWithMaskCmd, NULL, NULL,},
        {"byte_to_sc", ByteToScCmd, NULL, NULL,},
        {"byte_to_sc_i", ByteToScICmd, NULL, NULL,},
        {"byte_y_to_sc_p", ByteYToScPCmd, NULL, NULL,},
        {"byte_uv_to_sc_p", ByteUVToScPCmd, NULL, NULL,},
        {"byte_y_to_sc_b", ByteYToScBCmd, NULL, NULL,},
        {"byte_uv_to_sc_b", ByteUVToScBCmd, NULL, NULL,},
        {"byte_add", ByteAddCmd, NULL, NULL,},
        {"byte_multiply", ByteMultiplyCmd, NULL, NULL,},

        {"bit_new", BitNewCmd, NULL, NULL,},
        {"bit_free", BitFreeCmd, NULL, NULL,},
        {"bit_clip", BitClipCmd, NULL, NULL,},
        {"bit_reclip", BitReclipCmd, NULL, NULL,},
        {"bit_copy_8", BitCopy8Cmd, NULL, NULL,},
        {"bit_copy", BitCopyCmd, NULL, NULL,},
        {"bit_set_8", BitSet8Cmd, NULL, NULL,},
        {"bit_set", BitSetCmd, NULL, NULL,},
        {"bit_get_x", BitGetXCmd, NULL, NULL,},
        {"bit_get_y", BitGetYCmd, NULL, NULL,},
        {"bit_get_width", BitGetWidthCmd, NULL, NULL,},
        {"bit_get_height", BitGetHeightCmd, NULL, NULL,},
        {"bit_get_virtual", BitGetVirtualCmd, NULL, NULL,},
        {"bit_get_size", BitGetSizeCmd, NULL, NULL,},
        {"bit_info", BitInfoCmd, NULL, NULL,},
        {"bit_is_aligned", BitIsAlignedCmd, NULL, NULL,},
        {"bit_is_left_aligned", BitIsLeftAlignedCmd, NULL, NULL,},

        {"bit_make_from_key", BitMakeFromKeyCmd, NULL, NULL,},
        {"bit_union_8", BitUnion8Cmd, NULL, NULL,},
        {"bit_union", BitUnionCmd, NULL, NULL,},
        {"bit_intersect_8", BitIntersect8Cmd, NULL, NULL,},
        {"bit_intersect", BitIntersectCmd, NULL, NULL,},

        {"sc_new", ScNewCmd, NULL, NULL,},
        {"sc_free", ScFreeCmd, NULL, NULL,},
        {"sc_clip", ScClipCmd, NULL, NULL,},
        {"sc_reclip", ScReclipCmd, NULL, NULL,},
        {"sc_copy", ScCopyCmd, NULL, NULL,},
        {"sc_copy_dc_ac", ScCopyDcAcCmd, NULL, NULL,},
        {"sc_i_to_byte", ScIToByteCmd, NULL, NULL,},
        {"sc_p_to_y", ScPToYCmd, NULL, NULL,},
        {"sc_p_to_uv", ScPToUVCmd, NULL, NULL,},
        {"sc_b_to_y", ScBToYCmd, NULL, NULL,},
        {"sc_b_to_uv", ScBToUVCmd, NULL, NULL,},
        {"sc_get_x", ScGetXCmd, NULL, NULL,},
        {"sc_get_y", ScGetYCmd, NULL, NULL,},
        {"sc_get_width", ScGetWidthCmd, NULL, NULL,},
        {"sc_get_height", ScGetHeightCmd, NULL, NULL,},
        {"sc_get_virtual", ScGetVirtualCmd, NULL, NULL,},
        {"sc_info", ScInfoCmd, NULL, NULL,},
        {"sc_quantize", ScQuantizeCmd, NULL, NULL,},
        {"sc_dequantize", ScDequantizeCmd, NULL, NULL,},
        {"sc_non_i_dequantize", ScNonIDequantizeCmd, NULL, NULL,},
        {"sc_add", ScAddCmd, NULL, NULL,},
        {"sc_multiply", ScMultiplyCmd, NULL, NULL,},

        {"vector_new", VectorNewCmd, NULL, NULL,},
        {"vector_free", VectorFreeCmd, NULL, NULL,},
        {"vector_clip", VectorClipCmd, NULL, NULL,},
        {"vector_reclip", VectorReclipCmd, NULL, NULL,},
        {"vector_copy", VectorCopyCmd, NULL, NULL,},
        {"vector_get_x", VectorGetXCmd, NULL, NULL,},
        {"vector_get_y", VectorGetYCmd, NULL, NULL,},
        {"vector_get_width", VectorGetWidthCmd, NULL, NULL,},
        {"vector_get_height", VectorGetHeightCmd, NULL, NULL,},
        {"vector_get_virtual", VectorGetVirtualCmd, NULL, NULL,},
        {"vector_info", VectorInfoCmd, NULL, NULL,},

        {"byte_16_new", Byte16NewCmd, NULL, NULL,},
        {"byte_16_free", Byte16FreeCmd, NULL, NULL,},
        {"byte_16_clip", Byte16ClipCmd, NULL, NULL,},
        {"byte_16_reclip", Byte16ReclipCmd, NULL, NULL,},
        {"byte_16_copy", Byte16CopyCmd, NULL, NULL,},
        {"byte_16_get_x", Byte16GetXCmd, NULL, NULL,},
        {"byte_16_get_y", Byte16GetYCmd, NULL, NULL,},
        {"byte_16_get_width", Byte16GetWidthCmd, NULL, NULL,},
        {"byte_16_get_height", Byte16GetHeightCmd, NULL, NULL,},
        {"byte_16_get_virtual", Byte16GetVirtualCmd, NULL, NULL,},
        {"byte_16_info", Byte16InfoCmd, NULL, NULL,},

        {"byte_32_new", Byte32NewCmd, NULL, NULL,},
        {"byte_32_free", Byte32FreeCmd, NULL, NULL,},
        {"byte_32_clip", Byte32ClipCmd, NULL, NULL,},
        {"byte_32_reclip", Byte32ReclipCmd, NULL, NULL,},
        {"byte_32_copy", Byte32CopyCmd, NULL, NULL,},
        {"byte_32_get_x", Byte32GetXCmd, NULL, NULL,},
        {"byte_32_get_y", Byte32GetYCmd, NULL, NULL,},
        {"byte_32_get_width", Byte32GetWidthCmd, NULL, NULL,},
        {"byte_32_get_height", Byte32GetHeightCmd, NULL, NULL,},
        {"byte_32_get_virtual", Byte32GetVirtualCmd, NULL, NULL,},
        {"byte_32_info", Byte32InfoCmd, NULL, NULL,},

        {"float_new", FloatNewCmd, NULL, NULL,},
        {"float_free", FloatFreeCmd, NULL, NULL,},
        {"float_clip", FloatClipCmd, NULL, NULL,},
        {"float_reclip", FloatReclipCmd, NULL, NULL,},
        {"float_copy", FloatCopyCmd, NULL, NULL,},
        {"float_get_x", FloatGetXCmd, NULL, NULL,},
        {"float_get_y", FloatGetYCmd, NULL, NULL,},
        {"float_get_width", FloatGetWidthCmd, NULL, NULL,},
        {"float_get_height", FloatGetHeightCmd, NULL, NULL,},
        {"float_get_virtual", FloatGetVirtualCmd, NULL, NULL,},
        {"float_info", FloatInfoCmd, NULL, NULL,},

        {"bitstream_new", BitStreamNewCmd, NULL, NULL,},
        {"bitstream_free", BitStreamFreeCmd, NULL, NULL,},
#ifdef HAVE_MMAP
        {"bitstream_mmap_read_new", BitStreamMmapReadNewCmd, NULL, NULL,},
        {"bitstream_mmap_read_free", BitStreamMmapReadFreeCmd, NULL, NULL,},
#endif
        {"bitstream_share_buffer", BitStreamShareBufferCmd, NULL, NULL,},
        {"bitstream_shift", BitStreamShiftCmd, NULL, NULL,},
        {"bitstream_resize", BitStreamResizeCmd, NULL, NULL,},
        {"bitstream_bytes_left", BitStreamBytesLeftCmd, NULL, NULL,},
        {"bitstream_dump", BitStreamDumpCmd, NULL, NULL,},
        {"bitstream_dump_segments", BitStreamDumpSegmentsCmd, NULL, NULL,},

        {"bitparser_new", BitParserNewCmd, NULL, NULL,},
        {"bitparser_free", BitParserFreeCmd, NULL, NULL,},
        {"bitparser_tell", BitParserTellCmd, NULL, NULL,},
        {"bitparser_seek", BitParserSeekCmd, NULL, NULL,},
        {"bitparser_wrap", BitParserWrapCmd, NULL, NULL,},
        {"bitparser_get_bitstream", BitParserGetBitStreamCmd, NULL, NULL,},

        {"bitstream_channel_read", BitStreamChannelReadCmd, NULL, NULL,},
        {"bitstream_channel_read_segment", BitStreamChannelReadSegmentCmd, NULL, NULL,},
        {"bitstream_channel_read_segments", BitStreamChannelReadSegmentsCmd, NULL, NULL,},
        {"bitstream_channel_filter_in", BitStreamChannelFilterInCmd, NULL, NULL,},
        {"bitstream_channel_write", BitStreamChannelWriteCmd, NULL, NULL,},
        {"bitstream_channel_write_segment", BitStreamChannelWriteSegmentCmd, NULL, NULL,},
        {"bitstream_channel_write_segments", BitStreamChannelWriteSegmentsCmd, NULL, NULL,},

        {"bitstream_filter_new", BitStreamFilterNewCmd, NULL, NULL,},
        {"bitstream_filter_free", BitStreamFilterFreeCmd, NULL, NULL,},
        {"bitstream_filter_add", BitStreamFilterAddCmd, NULL, NULL,},
        {"bitstream_filter_resize", BitStreamFilterResizeCmd, NULL, NULL,},
        {"bitstream_filter_read", BitStreamFilterChannelReadCmd, NULL, NULL,},
        {"bitstream_filter_write", BitStreamFilterChannelWriteCmd, NULL, NULL,},
        {"bitstream_filter_start_scan", BitStreamFilterStartScanCmd, NULL, NULL,},
        {"bitstream_dump_using_filter", BitStreamDumpUsingFilterCmd, NULL, NULL,},

        {"audio_8_new", Audio8NewCmd, NULL, NULL,},
        {"audio_16_new", Audio16NewCmd, NULL, NULL,},
        {"audio_8_clip", Audio8ClipCmd, NULL, NULL,},
        {"audio_16_clip", Audio16ClipCmd, NULL, NULL,},
        {"audio_8_reclip", Audio8ReclipCmd, NULL, NULL,},
        {"audio_16_reclip", Audio16ReclipCmd, NULL, NULL,},
        {"audio_8_copy", Audio8CopyCmd, NULL, NULL,},
        {"audio_8_copy_some", Audio8CopySomeCmd, NULL, NULL,},
        {"audio_16_copy", Audio16CopyCmd, NULL, NULL,},
        {"audio_16_copy_some", Audio16CopySomeCmd, NULL, NULL,},
        {"audio_8_set", Audio8SetCmd, NULL, NULL,},
        {"audio_8_set_some", Audio8SetSomeCmd, NULL, NULL,},
        {"audio_16_set_some", Audio16SetSomeCmd, NULL, NULL,},
        {"audio_8_split", Audio8SplitCmd, NULL, NULL,},
        {"audio_16_split", Audio16SplitCmd, NULL, NULL,},
        {"audio_8_merge", Audio8MergeCmd, NULL, NULL,},
        {"audio_16_merge", Audio16MergeCmd, NULL, NULL,},
        {"audio_16_resample_half", Audio16ResampleHalfCmd, NULL, NULL,},
        {"audio_16_resample_quarter", Audio16ResampleQuarterCmd, NULL, NULL,},
        {"audio_16_resample_linear", Audio16ResampleLinearCmd, NULL, NULL,},
        {"audio_16_resample_decimate", Audio16ResampleDecimateCmd, NULL, NULL,},
        {"audio_16_max_abs", Audio16MaxAbsCmd, NULL, NULL,},

    /*
     * audio data resolution dependent casting commands
     */
        {"bitstream_cast_to_audio_8", BitStreamCastToAudio8Cmd, NULL, NULL,},
        {"bitstream_cast_to_audio_16", BitStreamCastToAudio16Cmd, NULL, NULL,},
        {"audio_8_cast_to_bitstream", Audio8CastToBitStreamCmd, NULL, NULL,},
        {"audio_16_cast_to_bitstream", Audio16CastToBitStreamCmd, NULL, NULL,},

    /*
     * resolution independent commands
     */
        {"audio_free", AudioFreeCmd, NULL, NULL,},
        {"audio_get_start_offset", AudioGetStartOffsetCmd, NULL, NULL,},
        {"audio_get_num_of_samples", AudioGetNumOfSamplesCmd, NULL, NULL,},

    /*
     * helper commands for audio synchronization project
     */
        {"audio_8_chunk_abs_sum", Audio8ChunkAbsSumCmd, NULL, NULL,},
        {"audio_16_chunk_abs_sum", Audio16ChunkAbsSumCmd, NULL, NULL,},

    /*
       { "byte_addem", ByteAddemCmd, NULL, NULL, },
       { "byte_smooth", ByteSmoothCmd, NULL, NULL, },
       { "byte_edge_detect", ByteEdgeDetectCmd, NULL, NULL, },
     */
    };



/*
 *----------------------------------------------------------------------
 *
 * DllEntryPoint --
 *
 *        This wrapper function is used by Windows to invoke the
 *        initialization code for the DLL.  If we are compiling
 *        with Visual C++, this routine will be renamed to DllMain.
 *        routine.
 *
 * Results:
 *        Returns TRUE;
 *
 * Side effects:
 *        None.
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
 * Rvmbasic_Init --
 *
 *        This procedure initializes the rvm command.
 *
 * Results:
 *        A standard Tcl result.
 *
 * Side effects:
 *        None.
 *
 *----------------------------------------------------------------------
 */

EXPORT(int, Tcldvmbasic_Init) (interp)
    Tcl_Interp *interp;
{
    CreateCommands(interp, basiccmd, sizeof(basiccmd));
    InitHashTable(interp);
    InitMemory(interp);
    return Tcl_PkgProvide(interp, "DvmBasic", "1.0");
}
