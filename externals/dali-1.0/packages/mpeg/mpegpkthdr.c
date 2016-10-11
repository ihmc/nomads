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
 * mpegpkthdr.c
 *
 * Functions that manipulate MpegPktHdrs
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"

MpegPktHdr *
MpegPktHdrNew()
{
    MpegPktHdr *new;

    new = NEW(MpegPktHdr);
    return new;
}


void
MpegPktHdrFree(hdr)
    MpegPktHdr *hdr;
{
    FREE((char *) hdr);
}


int
MpegPktHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code <= PACKET_MAX_START_CODE && code >= PACKET_MIN_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    return DVM_MPEG_NOT_FOUND;
}


int
MpegPktHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{

    int code, off;

    Bp_PeekInt(inbp, code);
    if (code > PACKET_MAX_START_CODE || code < PACKET_MIN_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_MoveBytes(inbp, outbp, 6);
    off = 6;

    /*
     * If this straem is private stream 2, go home.
     */
    if ((code & 0x000000FF) == DVM_STREAM_ID_PRIVATE_2) {
        return off;
    }
    /*
     * Remove stuffing bits
     */
    Bp_PeekByte(inbp, code);
    while (code == 0xFF) {
        Bp_MoveByte(inbp, outbp);
        off++;
        Bp_PeekByte(inbp, code);
    }

    if ((code & 0xC0) == 0x40) {        // next two bits is 01
        // read STD values

        Bp_MoveShort(inbp, outbp);
        off += 2;
    }
    Bp_PeekByte(inbp, code);
    if ((code & 0xF0) == 0x20) {
        Bp_MoveBytes(inbp, outbp, 5);
        off += 5;
    } else if ((code & 0xF0) == 0x30) {
        Bp_MoveBytes(inbp, outbp, 10);
        off += 10;
    } else {
        Bp_MoveByte(inbp, outbp);
        off += 1;
    }
    return off;
}


int
MpegPktHdrSkip(inbp)
    BitParser *inbp;
{

    int code, off;

    Bp_PeekInt(inbp, code);
    if (code > PACKET_MAX_START_CODE || code < PACKET_MIN_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(inbp, 6);
    off = 6;

    /*
     * If this straem is private stream 2, go home.
     */
    if ((code & 0x000000FF) == DVM_STREAM_ID_PRIVATE_2) {
        return off;
    }
    /*
     * Remove stuffing bits
     */
    Bp_PeekByte(inbp, code);
    while (code == 0xFF) {
        Bp_FlushByte(inbp);
        off++;
        Bp_PeekByte(inbp, code);
    }

    if ((code & 0xC0) == 0x40) {        // next two bits is 01
        // read STD values

        Bp_FlushShort(inbp);
        off += 2;
    }
    Bp_PeekByte(inbp, code);
    if ((code & 0xF0) == 0x20) {
        Bp_FlushBytes(inbp, 5);
        off += 5;
    } else if ((code & 0xF0) == 0x30) {
        Bp_FlushBytes(inbp, 10);
        off += 10;
    } else {
        Bp_FlushByte(inbp);
        off += 1;
    }
    return off;
}


int
MpegPktHdrParse(bp, hdr)
    BitParser *bp;
    MpegPktHdr *hdr;
{
    int code, off;
    int bufScale;

    Bp_PeekInt(bp, code);
    if (code > PACKET_MAX_START_CODE || code < PACKET_MIN_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    hdr->streamId = code & 0x000000FF;

    Bp_FlushInt(bp);

    Bp_GetShort(bp, hdr->packetLength);
    off = 6;

    /*
     * If this straem is private stream 2, go home.
     */
    if ((code & 0x000000FF) == DVM_STREAM_ID_PRIVATE_2) {
        return off;
    }
    /*
     * Remove stuffing bits
     */
    Bp_PeekByte(bp, code);
    while (code == 0xFF) {
        Bp_GetByte(bp, code);
        off++;
        Bp_PeekByte(bp, code);
    }

    if ((code & 0xC0) == 0x40) {        // next two bits is 01
        // read STD values

        Bp_GetShort(bp, code);
        bufScale = (code & 0x3000) >> 13;
        if (bufScale)
            hdr->bufferSize = (code & 0x1FFF) << 10;
        else
            hdr->bufferSize = (code & 0x1FFF) << 7;
        /*
           Bp_FlushBits(bp ,2);
           Bp_GetBits(bp, 1, packet_header->std_buffer_scale);
           Bp_GetBits(bp, 13, packet_header->std_buffer_size);
         */
        off += 2;
    }
    Bp_PeekByte(bp, code);
    if ((code & 0xF0) == 0x20) {
        ReadTimeStamp(bp, &(hdr->pts));
        off += 5;
        hdr->dts = -1;
    } else if ((code & 0xF0) == 0x30) {
        ReadTimeStamp(bp, &(hdr->pts));
        ReadTimeStamp(bp, &(hdr->dts));
        off += 10;
    } else {
        Bp_FlushByte(bp);
        off += 1;
        hdr->pts = -1;
        hdr->dts = -1;
    }
    return off;
}


int
MpegPktHdrEncode(hdr, stuff, bp)
    MpegPktHdr *hdr;
    int stuff;
    BitParser *bp;
{
    int code;
    short scode;
    int bufScale, bufSize;

    code = 0x00000100 | hdr->streamId;
    Bp_PutInt(bp, code);
    Bp_PutShort(bp, hdr->packetLength);

    if (hdr->streamId == DVM_STREAM_ID_PRIVATE_2)
        return 6;

    if (hdr->streamId < 224) {
        /* audio */
        bufScale = 0;
    } else {
        /* video */
        bufScale = 1;
    }

    DO_N_TIMES(stuff,
        Bp_PutByte(bp, 0xFF);
        );

    if (hdr->bufferSize != -1) {
        if (bufScale) {
            bufSize = hdr->bufferSize << 10;
        } else {
            bufSize = hdr->bufferSize << 7;
        }
        scode = 0x4000 | (bufScale << 13) | bufSize;
        Bp_PutShort(bp, scode);
    }
    if (hdr->dts == -1 && hdr->pts == -1) {
        Bp_PutByte(bp, 0x0F);
    } else if (hdr->dts == -1) {
        WriteTimeStamp(0x20, hdr->pts, bp);
    } else {
        WriteTimeStamp(0x30, hdr->pts, bp);
        WriteTimeStamp(0x10, hdr->dts, bp);
    }

    return 0;
}


void
ReadTimeStamp(bp, time)
    BitParser *bp;
    double *time;
{
    unsigned char hiBit;
    unsigned int lowInt;
    unsigned char code;

    Bp_GetByte(bp, code);
    hiBit = (unsigned char) ((unsigned long) code >> 3) & 0x01;
    lowInt = (((unsigned long) code >> 1) & 0x03) << 30;
    Bp_GetByte(bp, code);
    lowInt |= (unsigned long) code << 22;
    Bp_GetByte(bp, code);
    lowInt |= ((unsigned long) code >> 1) << 15;
    Bp_GetByte(bp, code);
    lowInt |= (unsigned long) code << 7;
    Bp_GetByte(bp, code);
    lowInt |= ((unsigned long) code) >> 1;

#define FLOAT_0x10000 (double)((unsigned long)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ 90000L

    if (hiBit != 0 && hiBit != 1) {
        *time = 0.0;
    } else {
        *time
            = (double) hiBit *FLOAT_0x10000 * FLOAT_0x10000 + (double) lowInt;

        *time /= (double) STD_SYSTEM_CLOCK_FREQ;
    }
}

void
WriteTimeStamp(marker, time, bp)
    char marker;                /* 4 bit marker follow by 2 0s */
    double time;
    BitParser *bp;
{
    unsigned int lowInt;
    unsigned char hiBit;
    unsigned char ccode;
    int code;

    time *= STD_SYSTEM_CLOCK_FREQ;
    if (time > FLOAT_0x10000 * FLOAT_0x10000) {
        hiBit = 1;
        time -= FLOAT_0x10000 * FLOAT_0x10000;
    } else {
        hiBit = 0;
    }

    lowInt = (int) time;

    ccode = marker |
        (hiBit << 3) |
        ((lowInt & 0xE0000000) >> 29) |
        0x01;

    code = 0x00010001 |
        ((lowInt & 0x3FFF8000) >> 14) |
        ((lowInt & 0x00007FFF) << 1);

    Bp_PutByte(bp, ccode);
    Bp_PutInt(bp, code);
}
