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
 *----------------------------------------------------------------------
 *
 * mpeggophdr.c
 *
 * Functions that manipulate MpegGopLists and GOPs
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"

MpegGopHdr *
MpegGopHdrNew()
{
    MpegGopHdr *new;

    new = NEW(MpegGopHdr);
    return new;
}


int
MpegGopHdrParse(bp, gh)
    BitParser *bp;
    MpegGopHdr *gh;
{
    unsigned int offset, code;
    int total;
    int startCode;

    Bp_GetInt(bp, startCode);
    if (startCode != GOP_START_CODE) {
        Bp_RestoreInt(bp);
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_GetBits(bp, 1, gh->drop_frame_flag);
    Bp_GetBits(bp, 5, gh->time_code_hours);
    Bp_GetBits(bp, 6, gh->time_code_minutes);
    Bp_FlushBits(bp, 1);        // flush marker bits

    Bp_GetBits(bp, 6, gh->time_code_seconds);
    Bp_GetBits(bp, 6, gh->time_code_pictures);
    Bp_GetBits(bp, 1, gh->closed_gop);
    Bp_GetBits(bp, 1, gh->broken_link);
    Bp_ByteAlign(bp);

    total = 8;

    while (((code = NextStartCode(bp, &offset)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += offset;
        if (code == PIC_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + offset - 4;
}


int
MpegGopHdrEncode(gh, bp)
    MpegGopHdr *gh;
    BitParser *bp;
{
    Bp_PutInt(bp, GOP_START_CODE);
    Bp_PutBits(bp, 1, gh->drop_frame_flag);
    Bp_PutBits(bp, 5, gh->time_code_hours);
    Bp_PutBits(bp, 6, gh->time_code_minutes);
    Bp_PutBits(bp, 1, 1);
    Bp_PutBits(bp, 6, gh->time_code_seconds);
    Bp_PutBits(bp, 6, gh->time_code_pictures);
    Bp_PutBits(bp, 1, gh->closed_gop);
    Bp_PutBits(bp, 1, gh->broken_link);
    Bp_OutByteAlign(bp);

    return 8;
}


int
MpegGopHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == GOP_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    return -1;
}


int
MpegGopHdrSkip(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    Bp_GetInt(bp, code);
    if (code != GOP_START_CODE) {
        Bp_RestoreInt(bp);
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushInt(bp);            /* skip time code, close gop and broken link */

    total = 8;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == PIC_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + off - 4;
}


int
MpegGopHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{
    unsigned int code, total, off;

    Bp_PeekInt(inbp, code);
    if (code != GOP_START_CODE)
        return DVM_MPEG_INVALID_START_CODE;

    Bp_MoveBytes(inbp, outbp, 8);
    total = 8;
    while (((code = DumpUntilNextStartCode(inbp, outbp, &off)) != SEQ_END_CODE) &&
        !Bp_Underflow(inbp)) {
        total += off;
        if (code == PIC_START_CODE) {
            Bp_RestoreInt(inbp);
            Bp_UnputInt(outbp);
            return total - 4;
        }
    }
    Bp_RestoreInt(inbp);
    Bp_UnputInt(outbp);
    return total + off - 4;
}


void
MpegGopHdrFree(hdr)
    MpegGopHdr *hdr;
{
    FREE(hdr);
}

void
MpegGopHdrSet(gopHdr, dropFrame, hours, minutes, seconds, pictures, closed, broken)
    MpegGopHdr *gopHdr;
    char dropFrame;
    char hours, minutes, seconds, pictures;
    char closed, broken;
{
    gopHdr->drop_frame_flag = dropFrame;
    gopHdr->time_code_hours = hours;
    gopHdr->time_code_minutes = minutes;
    gopHdr->time_code_seconds = seconds;
    gopHdr->time_code_pictures = pictures;
    gopHdr->closed_gop = closed;
    gopHdr->broken_link = broken;
}
