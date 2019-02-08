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
 * mpegseqhdr.c
 *
 * Functions that manipulate MpegSeqHdrs
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"
#include "tables.h"

#define RESERVE -1

const double
  pel_aspect_ratio_table[16] =
{
    RESERVE, 1.000, 0.6735, 0.7031, 0.7615, 0.8055, 0.8437, 0.8935, 0.9375,
    0.9815, 1.0255, 1.0695, 1.1250, 1.1575, 1.2015, RESERVE};

const double
  picture_rate_table[16] =
{
    RESERVE, 23.976, 24, 25, 29.97, 30, 50, 59.94, 60,
    RESERVE, RESERVE, RESERVE, RESERVE, RESERVE, RESERVE, RESERVE};


MpegSeqHdr *
MpegSeqHdrNew()
{
    MpegSeqHdr *hdr;

    hdr = NEW(MpegSeqHdr);
    return hdr;
}


void
MpegSeqHdrFree(hdr)
    MpegSeqHdr *hdr;
{
    FREE((char *) hdr);
}


int
MpegSeqHdrFind(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == SEQ_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    return DVM_MPEG_NOT_FOUND;
}


int
MpegSeqHdrDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{
    unsigned int code, off, total;

    Bp_PeekInt(inbp, code);
    if (code != SEQ_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    /*
     * Move the first 12 bytes over. (width, height, bitrate etc)
     */
    Bp_MoveBytes(inbp, outbp, 12);

    total = 12;
    while ((code = DumpUntilNextStartCode(inbp, outbp, &off)) != SEQ_END_CODE) {
        total += off;
        if (code == GOP_START_CODE || code == PIC_START_CODE) {
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
MpegSeqHdrSkip(bp)
    BitParser *bp;
{
    unsigned int code, off, total;

    Bp_PeekInt(bp, code);
    if (code != SEQ_START_CODE) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    total = 0;
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == GOP_START_CODE || code == PIC_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + off - 4;
}

int
MpegSeqHdrParse(bp, hdr)
    BitParser *bp;
    MpegSeqHdr *hdr;
{
    unsigned int data, code, off;
    int i, total;
    unsigned char r1, r2;
    static unsigned char defaultIQT[64] =
    {
        8, 16, 19, 22, 26, 27, 29, 34,
        16, 16, 22, 24, 27, 29, 34, 37,
        19, 22, 26, 27, 29, 34, 34, 38,
        22, 22, 26, 27, 29, 34, 37, 40,
        22, 26, 27, 29, 32, 35, 40, 48,
        26, 27, 29, 32, 35, 40, 48, 58,
        26, 27, 29, 34, 38, 46, 56, 69,
        27, 29, 35, 38, 46, 56, 69, 83};


    /* Flush off sequence start code. */

    Bp_GetInt(bp, data);
    if (data != SEQ_START_CODE) {
        Bp_RestoreInt(bp);
        return DVM_MPEG_INVALID_START_CODE;
    }
    /* Get horizontal size of image space. */

    Bp_GetBits(bp, 12, data);
    hdr->width = data;

    /* Get vertical size of image space. */

    Bp_GetBits(bp, 12, data);
    hdr->height = data;

    /* Calculate macroblock width and height of image space. */

    hdr->mb_width = (hdr->width + 15) / 16;
    hdr->mb_height = (hdr->height + 15) / 16;

    /* Parse of pel_aspect_ratio ratio code. */

    Bp_GetBits(bp, 4, data);
    hdr->pel_aspect_ratio = data;

    /* Parse off picture rate code. */

    Bp_GetBits(bp, 4, data);
    hdr->picture_rate = data;

    /* Parse off bit rate. */

    Bp_GetBits(bp, 8, r1);
    Bp_GetBits(bp, 8, r2);
    Bp_GetBits(bp, 2, data);
    data = (r1 << 10) | (r2 << 2) | data;
    hdr->bitrate = data;

    /* Flush marker bit. */

    Bp_FlushBits(bp, 1);

    /*
     * Parse off vbv buffer size. In some bitstreams, this is set to zero.
     * Since our decoder uses it, set it to a reasonable value (64K)
     */

    Bp_GetBits(bp, 10, data);
    if (data == 0) {
        data = 32;
    }
    hdr->vbv_buffer_size = data * 2048;

    /* Parse off contrained parameter flag. */

    Bp_GetBits(bp, 1, data);
    if (data) {
        hdr->constrained = 1;
    } else
        hdr->constrained = 0;

    /*
     * Total number of bytes so far
     */
    total = 12;

    /*
     * If intra_quant_matrix_flag set, parse off intra quant matrix values.
     */
    Bp_GetBits(bp, 1, data);
    if (data) {
        for (i = 0; i < 64; i++) {
            Bp_GetBits(bp, 8, data);
            hdr->iqt[i] = (unsigned char) data;
        }
        total += 64;
        hdr->default_qt = 1;
    } else {
        for (i = 0; i < 64; i++) {
            hdr->iqt[i] = defaultIQT[i];
        }
        hdr->default_qt = 0;
    }

    /*
     * If non intra quant matrix flag set, parse off non intra quant matrix
     * values.
     */

    Bp_GetBits(bp, 1, data);
    if (data) {
        for (i = 0; i < 64; i++) {
            Bp_GetBits(bp, 8, data);
            hdr->niqt[i] = (unsigned char) data;
        }
        total += 64;
        if (hdr->default_qt == 1)
            hdr->default_qt = 3;
        else
            hdr->default_qt = 2;
    } else {
        for (i = 0; i < 64; i++) {
            hdr->niqt[i] = 16;
        }
    }
    /* Go to gop start code. */

    Bp_ByteAlign(bp);
    while (((code = NextStartCode(bp, &off)) != SEQ_END_CODE) &&
        (code != 0)) {
        total += off;
        if (code == GOP_START_CODE || code == PIC_START_CODE) {
            Bp_RestoreInt(bp);
            return total - 4;
        }
    }
    Bp_RestoreInt(bp);
    return total + off - 4;
}


int
MpegSeqHdrEncode(hdr, bp)
    MpegSeqHdr *hdr;
    BitParser *bp;
{
    unsigned int data;
    int i, total;

    Bp_PutInt(bp, SEQ_START_CODE);
    Bp_PutBits(bp, 12, hdr->width);
    Bp_PutBits(bp, 12, hdr->height);
    Bp_PutBits(bp, 4, hdr->pel_aspect_ratio);
    Bp_PutBits(bp, 4, hdr->picture_rate);
    data = (hdr->bitrate & 0x003fffc) >> 2;
    Bp_PutBits(bp, 16, data);
    data = (hdr->bitrate & 0x0000003);
    Bp_PutBits(bp, 2, data);
    Bp_PutBits(bp, 1, 1);       /* marker bit */
    data = (hdr->vbv_buffer_size) >> 11;
    Bp_PutBits(bp, 10, data);
    Bp_PutBits(bp, 1, hdr->constrained);

    total = 12;

    if (hdr->default_qt & 0x01) {
        Bp_PutBits(bp, 1, 1);
        for (i = 0; i < 64; i++) {
            Bp_PutBits(bp, 8, hdr->iqt[i]);
        }
        total += 64;
    } else {
        Bp_PutBits(bp, 1, 0);
    }

    if (hdr->default_qt & 0x10) {
        Bp_PutBits(bp, 1, 1);
        for (i = 0; i < 64; i++) {
            Bp_PutBits(bp, 8, hdr->niqt[i]);
        }
        total += 64;
    } else {
        Bp_PutBits(bp, 1, 0);
    }

    Bp_OutByteAlign(bp);
    return total;
}


double
MpegSeqHdrGetAspectRatio(hdr)
    MpegSeqHdr *hdr;
{
    return pel_aspect_ratio_table[(int) hdr->pel_aspect_ratio];
}


double
MpegSeqHdrGetPicRate(hdr)
    MpegSeqHdr *hdr;
{
    return picture_rate_table[(int) hdr->picture_rate];
}


void
MpegSeqHdrSetAspectRatio(hdr, aspectRatio)
    MpegSeqHdr *hdr;
    double aspectRatio;
{
    int i;

    for (i = 1; i < 14; i++) {
        if (pel_aspect_ratio_table[i] == aspectRatio) {
            hdr->pel_aspect_ratio = i;
            return;
        }
    }
}

void
MpegSeqHdrSetPicRate(hdr, picRate)
    MpegSeqHdr *hdr;
    double picRate;
{
    int i;

    for (i = 1; i < 9; i++) {
        if (picture_rate_table[i] == picRate) {
            hdr->picture_rate = i;
            return;
        }
    }
}


void
MpegSeqHdrSetIQT(hdr, qTable)
    MpegSeqHdr *hdr;
    int *qTable;
{
    int i;

    for (i = 0; i < 64; i++) {
        hdr->iqt[i] = qTable[i];
    }

    /* could just add 1 to hdr->default_qt here but it may not be initialized */
    if (hdr->default_qt == 2) {
        hdr->default_qt = 3;
    } else {
        hdr->default_qt = 1;
    }
}

void
MpegSeqHdrSetNIQT(hdr, qTable)
    MpegSeqHdr *hdr;
    int *qTable;
{
    int i;

    for (i = 0; i < 64; i++) {
        hdr->niqt[i] = qTable[i];
    }

    /* could just add 2 to hdr->default_qt here but it may not be initialized */
    if (hdr->default_qt == 1) {
        hdr->default_qt = 3;
    } else {
        hdr->default_qt = 2;
    }
}

void
MpegSeqHdrSetDefaultIQT(hdr)
    MpegSeqHdr *hdr;
{
    int i;

    for (i = 0; i < 64; i++) {
        hdr->iqt[i] = default_intra_quantizer_table[i];
    }

    if (hdr->default_qt == 3) {
        hdr->default_qt = 2;
    } else if (hdr->default_qt != 2) {
        hdr->default_qt = 0;
    }
}

void
MpegSeqHdrSetDefaultNIQT(hdr)
    MpegSeqHdr *hdr;
{
    int i;

    for (i = 0; i < 64; i++) {
        hdr->niqt[i] = 16;
    }

    if (hdr->default_qt == 3) {
        hdr->default_qt = 1;
    } else if (hdr->default_qt != 1) {
        hdr->default_qt = 0;
    }
}


/* this goes at the end of the sequence (not the sequence header) */
void
MpegSeqEnder(bp)
    BitParser *bp;
{
    Bp_ByteAlign(bp);
    Bp_PutInt(bp, SEQ_END_CODE);
}


/* TEMPORARY */
void
MpegSeqHdrSet(seqHdr, w, h, pelAspectRatio, picRate, bitRate, vbvBufferSize, constrained, load_iqt, load_niqt, iqt, niqt)
    MpegSeqHdr *seqHdr;
    short w, h;
    char pelAspectRatio, picRate;
    int bitRate, vbvBufferSize;
    char constrained, load_iqt, load_niqt;
    unsigned char *iqt, *niqt;
{
    int i;

    seqHdr->width = w;
    seqHdr->height = h;
    seqHdr->mb_width = (w + 15) / 16;
    seqHdr->mb_height = (h + 15) / 16;
    seqHdr->pel_aspect_ratio = pelAspectRatio;
    seqHdr->picture_rate = picRate;
    seqHdr->bitrate = bitRate;  /* should not be zero */
    seqHdr->vbv_buffer_size = vbvBufferSize;
    seqHdr->constrained = constrained;

    seqHdr->default_qt = 0;
    if (load_iqt) {
        for (i = 0; i < 64; i++) {
            seqHdr->iqt[i] = iqt[i];
        }
        seqHdr->default_qt = 1;
    }
    if (load_niqt) {
        for (i = 0; i < 64; i++) {
            seqHdr->niqt[i] = niqt[i];
        }
        seqHdr->default_qt += 2;
    }
}
