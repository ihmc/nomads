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
 * mpegpichdr.c
 *
 * Functions that manipulate picture header
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"


MpegPicHdr *
MpegPicHdrNew()
{
    MpegPicHdr *hdr;

    hdr = NEW(MpegPicHdr);
    return hdr;
}


void
MpegPicHdrFree(hdr)
    MpegPicHdr *hdr;
{
    FREE(hdr);
}


int
MpegPicHdrParse(bp, pic_hdr)
    BitParser *bp;
    MpegPicHdr *pic_hdr;
{
    unsigned int offset;
    int startCode;
    short code;
    int total;

    Bp_GetInt(bp, startCode);
    if (startCode != PIC_START_CODE) {
        Bp_RestoreInt(bp);
        return DVM_MPEG_INVALID_START_CODE;
    }
    /*
     * Bp_GetBits(bp, 10, pic_hdr->temporal_reference);
     * Bp_GetBits(bp, 3, pic_hdr->type);
     */
    Bp_GetBits(bp, 13, code);
    pic_hdr->temporal_reference = (code & 0x1FF8) >> 3;
    pic_hdr->type = (code & 0x0007);

    Bp_GetBits(bp, 16, pic_hdr->vbv_delay);
    total = 8;

    if (pic_hdr->type == B_FRAME || pic_hdr->type == P_FRAME) {
        Bp_GetBits(bp, 1, pic_hdr->full_pel_forward_vector);
        Bp_GetBits(bp, 3, code);
        pic_hdr->forward_r_size = code - 1;
        pic_hdr->forward_f = (1 << pic_hdr->forward_r_size);
        pic_hdr->full_pel_backward_vector = 0;
        total++;
    }
    if (pic_hdr->type == B_FRAME) {
        Bp_GetBits(bp, 1, pic_hdr->full_pel_backward_vector);
        Bp_GetBits(bp, 3, code);
        pic_hdr->backward_r_size = code - 1;
        pic_hdr->backward_f = (1 << pic_hdr->backward_r_size);
    }
    Bp_ByteAlign(bp);
    while ((code = NextStartCode(bp, &offset)) != SEQ_END_CODE) {
        total += offset;
        if (code >= SLICE_MIN_START_CODE && code <= SLICE_MAX_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }

    Bp_RestoreInt(bp);
    return total + offset - 4;
}

int
MpegPicHdrEncode(pic_hdr, bp)
    MpegPicHdr *pic_hdr;
    BitParser *bp;
{
    short code;
    int len;

    Bp_PutInt(bp, PIC_START_CODE);
    Bp_PutBits(bp, 10, pic_hdr->temporal_reference);
    Bp_PutBits(bp, 3, pic_hdr->type);
    Bp_PutBits(bp, 16, pic_hdr->vbv_delay);
    len = 8;

    if (pic_hdr->type == B_FRAME || pic_hdr->type == P_FRAME) {
        Bp_PutBits(bp, 1, pic_hdr->full_pel_forward_vector);
        code = pic_hdr->forward_r_size + 1;
        Bp_PutBits(bp, 3, code);
        len++;
    }
    if (pic_hdr->type == B_FRAME) {
        Bp_PutBits(bp, 1, pic_hdr->full_pel_backward_vector);
        code = pic_hdr->backward_r_size + 1;
        Bp_PutBits(bp, 3, code);
    }
    Bp_OutByteAlign(bp);
    return len;
}


int
MpegPicHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;

    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == PIC_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return -1;
}


int
MpegPicHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{
    unsigned int total, code, off;

    Bp_PeekInt(inbp, code);
    if (code != PIC_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_MoveBytes(inbp, outbp, 8);

    total = 8;
    while ((code = DumpUntilNextStartCode(inbp, outbp, &off)) != SEQ_END_CODE) {
        total += off;
        if (code <= SLICE_MAX_START_CODE && code >= SLICE_MIN_START_CODE) {
            Bp_RestoreInt(inbp);
            Bp_UnputInt(outbp);
            return total - 4;
        }
    }
    Bp_RestoreInt(inbp);
    Bp_UnputInt(outbp);
    return total + off - 4;
}


int
MpegPicHdrSkip(bp)
    BitParser *bp;
{
    unsigned int code, total, off;

    Bp_GetInt(bp, code);
    if (code != PIC_START_CODE) {
        Bp_RestoreInt(bp);
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_FlushBytes(bp, 4);
    total = 8;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code >= SLICE_MIN_START_CODE && code <= SLICE_MAX_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + off - 4;
}

int
MpegAnyHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;

    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == PIC_START_CODE
            || code == GOP_START_CODE
            || code == SEQ_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + off - 4;
}


/* See Table 2-D.7 on page D-31 for valid values */
void
MpegPicHdrSetForwardFCode(hdr, forwardFCode)
    MpegPicHdr *hdr;
    int forwardFCode;
{
    hdr->forward_r_size = forwardFCode - 1;
    hdr->forward_f = 1 << hdr->forward_r_size;
}

/* See Table 2-D.7 on page D-31 for valid values */
void
MpegPicHdrSetBackwardFCode(hdr, backwardFCode)
    MpegPicHdr *hdr;
    int backwardFCode;
{
    hdr->backward_r_size = backwardFCode - 1;
    hdr->backward_f = 1 << hdr->backward_r_size;
}


/* Temporary */
void
MpegPicHdrSet(pic_hdr, tempRef, type, vbvDelay, fullPelForVec, forwardFCode, fullPelBackVec, backwardFCode)
    MpegPicHdr *pic_hdr;
    short tempRef;
    char type;
    unsigned short vbvDelay;
    char fullPelForVec, forwardFCode;
    char fullPelBackVec, backwardFCode;
{
    pic_hdr->temporal_reference = tempRef;
    pic_hdr->type = type;
    //pic_hdr->vbv_delay = vbvDelay;
    pic_hdr->vbv_delay = 0xffff;        /* see page 36 (section 2.4.3.4) */
    if (type == P_FRAME || type == B_FRAME) {
        pic_hdr->full_pel_forward_vector = fullPelForVec;
        pic_hdr->forward_r_size = forwardFCode - 1;
        pic_hdr->forward_f = 1 << pic_hdr->forward_r_size;
        if (type == B_FRAME) {
            pic_hdr->full_pel_backward_vector = fullPelBackVec;
            pic_hdr->backward_r_size = backwardFCode - 1;
            pic_hdr->backward_f = 1 << pic_hdr->backward_r_size;
        }
    }
}
