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
 * Core routines
 */

#include "mpegInt.h"


/*
 *--------------------------------------------------------------
 *
 * NextStartCode
 *
 * Return the next start code in bitstream bp. Record the
 * byte offset to the beginning of the code in offsetPtr
 *
 * Results:
 *    The code value, or 0 if no code is found.
 *
 * Side effects:
 *    The bitstream is parsed.
 *
 *--------------------------------------------------------------
 */

int
NextStartCode(bp, offsetPtr)
    BitParser *bp;
    unsigned int *offsetPtr;
{
    unsigned char val;
    unsigned int marker;
    int idx = 0;
    int remain = Bp_DataRemain(bp);

    if (remain < 4) {
        return SEQ_END_CODE;
    }
    Bp_GetByte(bp, marker);
    Bp_GetByte(bp, val);
    marker = (marker << 8) | val;
    Bp_GetByte(bp, val);
    marker = (marker << 8) | val;
    idx = 3;

    while (!Bp_Underflow(bp)) {
        Bp_GetByte(bp, val);
        marker = (marker << 8) | val;
        idx++;
        if ((marker & 0xFFFFFF00) == 0x00000100) {
            *offsetPtr = idx;
            return marker;
        }
    }
    return 0;
}


/*
 *--------------------------------------------------------------
 *
 * DumpUntilNextStartCode
 *
 * Return the next start code in bitstream bp. Record the
 * byte offset to the beginning of the code in offsetPtr
 *
 * Results:
 *    The code value or 0 if input bitstream underflow.
 *
 * Side effects:
 *    The bitstream is parsed.
 *
 *--------------------------------------------------------------
 */

int
DumpUntilNextStartCode(inbp, outbp, offsetPtr)
    BitParser *inbp;
    BitParser *outbp;
    unsigned int *offsetPtr;
{
    unsigned char val;
    unsigned int marker;
    int idx = 0;

    if (Bp_DataRemain(inbp) < 4) {
        return 0;
    }
    Bp_GetByte(inbp, marker);
    Bp_PutByte(outbp, marker);
    Bp_GetByte(inbp, val);
    Bp_PutByte(outbp, val);
    marker = (marker << 8) | val;
    Bp_GetByte(inbp, val);
    Bp_PutByte(outbp, val);
    marker = (marker << 8) | val;
    idx = 3;

    while (!Bp_Underflow(inbp)) {
        Bp_GetByte(inbp, val);
        Bp_PutByte(outbp, val);
        marker = (marker << 8) | val;
        idx++;
        if ((marker & 0xFFFFFF00) == 0x00000100) {
            *offsetPtr = idx;
            return marker;
        }
    }
    return 0;
}


void
MpegSeqEndCodeEncode(bp)
    BitParser *bp;
{
    Bp_PutInt(bp, SEQ_END_CODE);
}

int
MpegGetCurrStartCode(bp)
    BitParser *bp;
{
    int code;

    Bp_PeekInt(bp, code);
    return code;
}
