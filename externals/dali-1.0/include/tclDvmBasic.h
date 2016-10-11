/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * tclDvmBasic.h
 * 
 * contains core and essential stuff for the whole DVM. includes the
 * hash table functions and definitions
 *
 *------------------------------------------------------------------
 */

#ifndef TCL_DVM_BASIC_H
#define TCL_DVM_BASIC_H

#include "tcl.h"
#include <stdlib.h>
#include "macro.h"
#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

int ByteNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteCopyMuxCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteSetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteSetMuxCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteExtendCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteCopyWithMaskCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteSetWithMaskCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteGetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteGetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteGetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteToScCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteToScICmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteYToScPCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteUVToScPCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteYToScBCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteUVToScBCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteAddCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ByteMultiplyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int BitNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitSetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitSet8Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitCopy8Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitGetSizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitIsAlignedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitIsLeftAlignedCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitMakeFromKeyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitUnionCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitUnion8Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitIntersectCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitIntersect8Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int ScNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScGetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScGetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScGetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScCopyDcAcCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScIToByteCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScPToYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScPToUVCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScBToYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScBToUVCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScQuantizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScDequantizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScNonIDequantizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScAddCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int ScMultiplyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int VectorNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorGetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorGetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorGetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int VectorCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int Byte16NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16ClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16ReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16FreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16GetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16GetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16GetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16GetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16GetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16InfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte16CopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int Byte32NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32ClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32ReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32FreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32GetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32GetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32GetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32GetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32GetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32InfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Byte32CopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int FloatNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatGetXCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatGetYCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatGetWidthCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatGetHeightCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatGetVirtualCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatInfoCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int FloatCopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int BitStreamNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamMmapReadNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamMmapReadFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamResizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamShareBufferCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamShiftCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamBytesLeftCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamDumpCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamDumpSegmentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int BitParserNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserSeekCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserTellCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserResetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserWrapCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitParserGetBitStreamCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int BitStreamChannelReadCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelReadSegmentCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelReadSegmentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelFilterInCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelWriteCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelWriteSegmentCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamChannelWriteSegmentsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int BitStreamFilterNewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterAddCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterResizeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterChannelReadCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterChannelWriteCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamFilterStartScanCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamDumpUsingFilterCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

int Audio8NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16NewCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ClipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ReclipCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8CopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8CopySomeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16CopyCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16CopySomeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8SetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8SetSomeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16SetSomeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8SplitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16SplitCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8MergeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16MergeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/* casting commands */
int BitStreamCastToAudio8Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int BitStreamCastToAudio16Cmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8CastToBitStreamCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16CastToBitStreamCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
* audio resolution independent commands
*/
int AudioFreeCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int AudioGetStartOffsetCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int AudioGetNumOfSamplesCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
* commands for multiple video synchronization project
*/
int Audio8ChunkAbsSumCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ChunkAbsSumCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ResampleHalfCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ResampleHalfCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ResampleQuarterCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ResampleQuarterCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ResampleLinearCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ResampleLinearCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8ResampleDecimateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16ResampleDecimateCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio8MaxAbsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));
int Audio16MaxAbsCmd _ANSI_ARGS_((ClientData cd, Tcl_Interp * interp, int argc, char *argv[]));

/*
 * All prefixes MUST be 9 characters long
 */
#define BYTE_PREFIX   "dvmByte__"
#define BYTE_16_PREFIX   "dvmByte16"
#define BYTE_32_PREFIX   "dvmByte32"
#define FLOAT_PREFIX   "dvmFloat_"
#define BIT_PREFIX    "dvmBit___"
#define SC_PREFIX     "dvmSC____"
#define VECTOR_PREFIX "dvmVector"
#define BIT_STREAM_PREFIX   "dvmStream"
#define BIT_PARSER_PREFIX       "dvmParser"
#define BIT_STREAM_FILTER_PREFIX "dvmFilter"
#define AUDIO_PREFIX  "dvmAudio_"

#define GetByteImage(s)    (!strncmp(s, BYTE_PREFIX, 9)?GetBuf(s):NULL)
#define FindByteImage(p)    FindBuf(p)
#define RemoveByteImage(s) (!strncmp(s, BYTE_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutByteImage(interp, buf) PutBuf(interp, BYTE_PREFIX, buf)

#define GetByte16Image(s)    (!strncmp(s, BYTE_16_PREFIX, 9)?GetBuf(s):NULL)
#define FindByte16Image(p)    FindBuf(p)
#define RemoveByte16Image(s) (!strncmp(s, BYTE_16_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutByte16Image(interp, buf) PutBuf(interp, BYTE_16_PREFIX, buf)

#define GetByte32Image(s)    (!strncmp(s, BYTE_32_PREFIX, 9)?GetBuf(s):NULL)
#define FindByte32Image(p)    FindBuf(p)
#define RemoveByte32Image(s) (!strncmp(s, BYTE_32_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutByte32Image(interp, buf) PutBuf(interp, BYTE_32_PREFIX, buf)

#define GetFloatImage(s)    (!strncmp(s, FLOAT_PREFIX, 9)?GetBuf(s):NULL)
#define FindFloatImage(p)    FindBuf(p)
#define RemoveFloatImage(s) (!strncmp(s, FLOAT_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutFloatImage(interp, buf) PutBuf(interp, FLOAT_PREFIX, buf)

#define GetBitImage(s)    (!strncmp(s, BIT_PREFIX, 9)?GetBuf(s):NULL)
#define FindBitImage(p)   FindBuf(p)
#define RemoveBitImage(s) (!strncmp(s, BIT_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutBitImage(interp, buf) PutBuf(interp, BIT_PREFIX, buf)

#define GetScImage(s)    (!strncmp(s, SC_PREFIX, 9)?GetBuf(s):NULL)
#define FindScImage(p)   FindBuf(p)
#define RemoveScImage(s) (!strncmp(s, SC_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutScImage(interp, buf) PutBuf(interp, SC_PREFIX, buf)

#define GetVectorImage(s)    (!strncmp(s, VECTOR_PREFIX, 9)?GetBuf(s):NULL)
#define FindVectorImage(p)   FindBuf(p)
#define RemoveVectorImage(s) (!strncmp(s, VECTOR_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutVectorImage(interp, buf) PutBuf(interp, VECTOR_PREFIX, buf)

#define GetBitStream(s)    (!strncmp(s, BIT_STREAM_PREFIX, 9)?GetBuf(s):NULL)
#define FindBitStream(p)   FindBuf(p)
#define RemoveBitStream(s) (!strncmp(s, BIT_STREAM_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutBitStream(interp, buf) PutBuf(interp, BIT_STREAM_PREFIX, buf)

#define GetBitParser(s)    (!strncmp(s, BIT_PARSER_PREFIX, 9)?GetBuf(s):NULL)
#define FindBitParser(p)   FindBuf(p)
#define RemoveBitParser(s) (!strncmp(s, BIT_PARSER_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutBitParser(interp, buf) PutBuf(interp, BIT_PARSER_PREFIX, buf)

#define GetBitStreamFilter(s)    (!strncmp(s, BIT_STREAM_FILTER_PREFIX, 9)?GetBuf(s):NULL)
#define FindBitStreamFilter(p)   FindBuf(p)
#define RemoveBitStreamFilter(s) (!strncmp(s, BIT_STREAM_FILTER_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutBitStreamFilter(interp, buf) PutBuf(interp, BIT_STREAM_FILTER_PREFIX, buf)

#define GetAudio(s)     (!strncmp(s, AUDIO_PREFIX, 9)?GetBuf(s):NULL)
#define FindAudio(p)   FindBuf(p)
#define RemoveAudio(s)  (!strncmp(s, AUDIO_PREFIX, 9)?RemoveBuf(s):NULL)
#define PutAudio(interp, buf) PutBuf(interp, AUDIO_PREFIX, buf)

void *GetBuf(char *str);
char *FindBuf(void *buf);
void *RemoveBuf(char *str);
int PutBuf(Tcl_Interp * interp, char *prefix, void *buf);
void *GetBufByNum(int id);
void *RemoveBufByNum(int id);

int BitStreamChannelRead(BitStream *, Tcl_Channel, int);
int BitStreamChannelReadSegment(BitStream *, Tcl_Channel, int, int);
int BitStreamChannelReadSegments(BitStream *, Tcl_Channel, int, int, int, int);
int BitStreamChannelFilterIn(BitStream *, Tcl_Channel, int, BitStreamFilter *);
void BitStreamFilterChannelRead(BitStreamFilter *, Tcl_Channel);
void BitStreamFilterChannelWrite(BitStreamFilter *, Tcl_Channel);
int BitStreamChannelWrite(BitStream *, Tcl_Channel, int);
int BitStreamChannelWriteSegment(BitStream *, Tcl_Channel, int, int);
int BitStreamChannelWriteSegments(BitStream *, Tcl_Channel, int, int, int, int);

void InitMemory(Tcl_Interp * interp);

/*
 * begin include init.h by Jose
 */
#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

/*
 * VC++ has an alternate entry point called DllMain, so we need to rename
 * our entry point.
 */

#if defined(_MSC_VER)
#define EXPORT(a,b) __declspec(dllexport) a b
#define DllEntryPoint DllMain
#else
#if defined(__BORLANDC__)
#define EXPORT(a,b) a _export b
#else
#define EXPORT(a,b) a b
#endif
#endif
#else
#define EXPORT(a,b) a b
#endif
/* 
 * end include from init.h 
 */

typedef struct _Commands {
    char *name;             /* name of the vm instruction */
    Tcl_CmdProc *proc;      /* function corresponds to the instruction */
    ClientData cd;          /* arguments to the function */
    Tcl_CmdDeleteProc *delProc;     /* what to do when we delete this command */
} Commands;

void CreateCommands(Tcl_Interp * interp, Commands cmd[], int cmdsize);
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
