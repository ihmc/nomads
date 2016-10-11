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
 * mpegaudiohdr.c
 *
 * Functions that manipulate Mpeg Audio Header
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"

extern float bit_rate_table[16][2][3];
extern float sampling_rate_table[4][2];


MpegAudioHdr *
MpegAudioHdrNew()
{
    MpegAudioHdr *header;

    header = NEW(MpegAudioHdr);
    return header;
}


void
MpegAudioHdrFree(header)
    MpegAudioHdr *header;
{
    FREE(header);
}


int
MpegAudioHdrFind(bp)
    BitParser *bp;
{
    unsigned char code;
    int state;
    int len;

    len = 0;
    state = 0;

    /*
     * Scan the bitstream for the syncword 1111 1111 1111
     */

    while (1) {
        Bp_GetByte(bp, code);
        len++;
        switch (state) {
        case 0:
            if (code == 0xFF)
                state = 1;
            break;
        case 1:
            if ((code & 0xF0) == 0xF0) {
                Bp_RestoreShort(bp);
                return len - 2;
            } else
                state = 0;
            break;
        default:;
        }
        if (Bp_Underflow(bp)) {
            return -1;
        }
    }
}




int 
MpegAudioHdrDump(inbp, outbp)
    BitParser *inbp, *outbp;
{
    unsigned int code;
    unsigned int protection;

    Bp_GetInt(inbp, code);
    if ((code & 0xfff00000) != 0xfff00000) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    Bp_PutInt(outbp, code);
    protection = (code & 0x00010000) >> 16;
    if (!protection) {
        Bp_MoveShort(inbp, outbp);
        return 6;
    }
    return 4;
}


int 
MpegAudioHdrSkip(bp)
    BitParser *bp;
{
    unsigned int code;
    unsigned int protection;

    Bp_GetInt(bp, code);
    if ((code & 0xfff00000) != 0xfff00000) {
        return DVM_MPEG_INVALID_START_CODE;
    }
    protection = (code & 0x00010000) >> 16;
    if (!protection) {
        Bp_GetShort(bp, code);
        return 6;
    }
    return 4;
}


int
MpegAudioHdrParse(bp, header)
    BitParser *bp;
    MpegAudioHdr *header;
{
    unsigned int header_code;

    Bp_GetInt(bp, header_code);

    if ((header_code & 0xfff00000) != 0xfff00000) {
        // invalid synword
        return DVM_MPEG_INVALID_START_CODE;
    }
    header->id = (header_code & 0x00080000) >> 19;
    switch (header_code & 0x00060000) {
    case 0x00060000:
        header->layer = 1;
        break;
    case 0x00040000:
        header->layer = 2;
        break;
    case 0x00020000:
        header->layer = 3;
        break;
    }
    header->protection_bit = (header_code & 0x00010000) >> 16;  // 1

    header->bit_rate_index = (header_code & 0x0000f000) >> 12;  // 4

    header->sampling_rate_index = (header_code & 0x00000c00) >> 10;     // 2

    header->padding_bit = (header_code & 0x00000200) >> 9;      // 1

    header->extension = (header_code & 0x00000100) >> 8;        // 1

    header->mode = (header_code & 0x000000c0) >> 6;     // 2

    header->mode_extension = (header_code & 0x00000030) >> 4;   // 2

    header->copyright = (header_code & 0x00000008) >> 3;        // 1

    header->original_or_copy = (header_code & 0x00000004) >> 2;         // 1

    header->emphasis_index = (header_code & 0x00000003);        // 2

    header->bit_rate = bit_rate_table[(int) header->bit_rate_index][(int) header->id][(int) header->layer - 1];
    header->sampling_rate = sampling_rate_table[(int) header->sampling_rate_index][(int) header->id];

    if (!header->protection_bit) {
        Bp_GetShort(bp, header->error_check);
        return 6;
    }
    return 4;
}


int
MpegAudioHdrEncode(header, bp)
    MpegAudioHdr *header;
    BitParser *bp;
{
    unsigned int code;

    /*
     * Initialize the first 12 bits to the sync word.
     */

    code = 0xfff00000;
    if (header->id) {
        code |= 0x00080000;
    }
    switch (header->layer) {
    case 1:
        code |= 0x00060000;
    case 2:
        code |= 0x00040000;
    case 3:
        code |= 0x00020000;
    }

    if (header->protection_bit) {
        code |= 0x00010000;
    }
    code |= ((header->bit_rate_index) << 12) & 0x0000f000;
    code |= ((header->sampling_rate_index) << 10) & 0x00000c00;
    code |= ((header->padding_bit) << 9) & 0x00000200;
    code |= ((header->extension) << 8) & 0x00000100;
    code |= ((header->mode) << 6) & 0x000000c0;
    code |= ((header->mode_extension) << 4) & 0x00000030;
    code |= ((header->copyright) << 3) & 0x00000008;
    code |= ((header->original_or_copy) << 2) & 0x00000004;
    code |= ((header->emphasis_index)) & 0x00000003;

    Bp_PutInt(bp, code);
    if (!header->protection_bit) {
        Bp_PutShort(bp, header->error_check);
        return 6;
    }
    return 4;
}
