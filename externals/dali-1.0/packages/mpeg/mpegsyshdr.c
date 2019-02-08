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
 * Functions that manipulate MpegSysHdrs
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"

MpegSysHdr *
MpegSysHdrNew()
{
    MpegSysHdr *new;

    new = NEW(MpegSysHdr);
    new->numOfStreamInfo = 0;
    return new;
}


void
MpegSysHdrFree(hdr)
    MpegSysHdr *hdr;
{
    FREE((char *) hdr);
}


int
MpegSysHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == SYS_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    return DVM_MPEG_NOT_FOUND;
}


int
MpegSysHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{
    short length;
    int code;

    Bp_PeekInt(inbp, code);
    if (code != SYS_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_MoveBytes(inbp, outbp, 4);
    Bp_GetShort(inbp, length);
    Bp_PutShort(outbp, length);
    Bp_MoveBytes(inbp, outbp, length);

    return length + 6;
}


int
MpegSysHdrSkip(bp)
    BitParser *bp;
{
    short length;
    int code;

    Bp_PeekInt(bp, code);
    if (code != SYS_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(bp, 4);
    Bp_GetShort(bp, length);
    Bp_FlushBytes(bp, length);

    return length + 6;
}


int
MpegSysHdrParse(bp, hdr)
    BitParser *bp;
    MpegSysHdr *hdr;
{
    char ccode;
    short length, scode;
    int i, code, total;

    Bp_PeekInt(bp, code);
    if (code != SYS_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(bp, 4);
    Bp_GetShort(bp, length);
    total = length + 6;

    Bp_PeekInt(bp, code);
    hdr->rateBound = ((code & 0x7FFFFE00) >> 9) * 50;

    Bp_GetByte(bp, ccode);
    hdr->audioBound = ccode >> 2;
    hdr->fixedFlag = (ccode & 0x03) >> 1;
    hdr->cspsFlag = ccode & 0x01;

    Bp_GetByte(bp, ccode);
    hdr->audioLock = ccode >> 7;
    hdr->videoLock = (ccode & 0x40) >> 6;
    hdr->videoBound = ccode & 0x1F;

    Bp_FlushByte(bp);           /* marker 1111 1111 */

    length -= 5;
    i = 0;
    while (length > 0) {
        Bp_GetByte(bp, ccode);
        Bp_GetShort(bp, scode);
        length -= 3;
        hdr->streamId[i] = ccode;
        if (scode & 0x2000)
            hdr->bufferSize[i] = (scode & 0x01FFF) << 10;
        else
            hdr->bufferSize[i] = (scode & 0x01FFF) << 7;

        i++;
    }
    hdr->numOfStreamInfo = i;
    return total;
}


int
MpegSysHdrEncode(hdr, bp)
    MpegSysHdr *hdr;
    BitParser *bp;
{
    int code, length, i;
    short scode;

    Bp_PutInt(bp, SYS_START_CODE);
    length = 6 + (hdr->numOfStreamInfo * 3);
    Bp_PutInt(bp, length);      // need to calc this fella.

    code = (0x80000100 | ((hdr->rateBound / 50) << 9));
    code |= (hdr->audioBound << 2) | (hdr->fixedFlag << 1) | (hdr->cspsFlag);
    Bp_PutInt(bp, code);

    scode = (hdr->audioLock << 15) | (hdr->videoLock << 14);
    scode |= (hdr->videoBound << 8) | 0xFF;
    Bp_PutShort(bp, scode);

    for (i = 0; i < hdr->numOfStreamInfo; i++) {
        Bp_PutByte(bp, hdr->streamId[i]);
        if ((hdr->bufferSize[i] << 7) < 0xFFFFFF) {
            scode = hdr->bufferSize[i] << 7;
        } else {
            scode = hdr->bufferSize[i] << 10;
        }
        Bp_PutShort(bp, scode);
    }

    return length + 6;
}


void
MpegSysHdrSetBufferSize(hdr, id, bufSize)
    MpegSysHdr *hdr;
    int id;
    int bufSize;
{
    hdr->streamId[hdr->numOfStreamInfo] = id;
    hdr->bufferSize[hdr->numOfStreamInfo] = bufSize;
    hdr->numOfStreamInfo++;
}


int
MpegSysHdrGetBufferSize(hdr, id)
    MpegSysHdr *hdr;
    int id;
{
    int i;

    for (i = 0; i < hdr->numOfStreamInfo; i++) {
        if (hdr->streamId[i] == DVM_STREAM_ID_STD_AUDIO &&
            id >= 0 && id < 32) {
            return hdr->bufferSize[i];
        } else if (hdr->streamId[i] == DVM_STREAM_ID_STD_VIDEO &&
            id >= 32) {
            return hdr->bufferSize[i];
        } else if (id == hdr->streamId[i]) {
            return hdr->bufferSize[i];
        }
    }

    return DVM_MPEG_NOT_FOUND;
}
