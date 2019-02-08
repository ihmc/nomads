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
 * mpegpckhdr.c
 *
 * Functions that manipulate MpegPckHdrs
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"

MpegPckHdr *
MpegPckHdrNew()
{
    MpegPckHdr *new;

    new = NEW(MpegPckHdr);
    return new;
}


void
MpegPckHdrFree(hdr)
    MpegPckHdr *hdr;
{
    FREE((char *) hdr);
}


int
MpegPckHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == PACK_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    return DVM_MPEG_NOT_FOUND;
}


int
MpegPckHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{

    int code;

    Bp_PeekInt(inbp, code);
    if (code != PACK_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_MoveBytes(inbp, outbp, 12);
    return 12;
}


int
MpegPckHdrSkip(inbp)
    BitParser *inbp;
{

    int code;

    Bp_PeekInt(inbp, code);
    if (code != PACK_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(inbp, 12);
    return 12;
}


int
MpegPckHdrParse(bp, hdr)
    BitParser *bp;
    MpegPckHdr *hdr;
{
    int scode;
    int code;

    Bp_PeekInt(bp, code);
    if (code != PACK_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(bp, 4);
    ReadTimeStamp(bp, &(hdr->sysClockRef));

    Bp_PeekInt(bp, scode);
    hdr->muxRate = ((scode & 0x7FFFFE00) >> 9) * 50;
    Bp_FlushBytes(bp, 3);

    return 12;
}


int
MpegPckHdrEncode(hdr, bp)
    MpegPckHdr *hdr;
    BitParser *bp;
{
    short scode;
    char ccode;
    int muxRate;

    Bp_PutInt(bp, PACK_START_CODE);
    WriteTimeStamp(0x20, hdr->sysClockRef, bp);

    muxRate = hdr->muxRate / 50;
    scode = 0x8000 | ((muxRate & 0x003FFF80) >> 7);
    Bp_PutShort(bp, scode);

    ccode = 0x01 | (muxRate & 0x0000007F);
    Bp_PutByte(bp, ccode);

    return 12;
}
