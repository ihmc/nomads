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
 * mpegpic.c
 *
 * Functions that manipulate pictures.  Part of these codes are based
 * on the source code from Berkeley MPEG decoder.
 *
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *----------------------------------------------------------------------
 */

#include "mpegInt.h"
#include "mpegvector.h"
#include "videotable.h"

static quantizeTable[4] =
{ERROR, 1, 0, 0};

#define RUN_MASK    0xfc00
#define LEVEL_MASK  0x03f0
#define NUM_MASK    0x000f
#define RUN_SHIFT   10
#define LEVEL_SHIFT 4

#if defined(DEBUG_MB) || defined(DEBUG_MV)
static int debugOn = 0;
static int picCount = 0;
static int count = 0;

#endif

void
Next23BitsIsZero(bp, code)
    BitParser *bp;
    int *code;
{
    short _x;

    Bp_GetBits(bp, 8, _x);
    if (_x != 0) {
        *code = 0;
    } else {
        Bp_PeekBits(bp, 15, _x);
        if (_x != 0) {
            *code = 0;
        } else {
            *code = 1;
        }
    }
    Bp_RestoreBits(bp, 8);
}

/*
 * There isn't really any macroblock "header".  This macro just parse the
 * macroblock address and quantization scale, so that what's left are pure
 * scblock information.
 */
void
ParseIMacroBlockHdr(bp, prev_mb_addr, curr_mb_addr, quant_scale)
    BitParser *bp;
    int *prev_mb_addr;
    int *curr_mb_addr;
    int *quant_scale;
{
    int code, index, quantize;
    int mb_addr_incr = 0;

    do {
        Bp_PeekBits(bp, 11, index);
        code = mbai[index].value;
        Bp_FlushBits(bp, mbai[index].numOfBits);
        if (mbai[index].numOfBits == 0) {
            code = 1;           /* don't understand this but it fixs a bug */
        }
        if (code == MACRO_BLOCK_ESCAPE) {
            mb_addr_incr += 33;
        }
    } while (code == MACRO_BLOCK_ESCAPE || code == MACRO_BLOCK_STUFFING);
    mb_addr_incr += code;
    *curr_mb_addr = *prev_mb_addr + mb_addr_incr;
    *prev_mb_addr = *curr_mb_addr;
    Bp_PeekBits(bp, 2, code);
    quantize = quantizeTable[code];
    if (code)
        Bp_FlushBits(bp, 1 + quantize);
    if (quantize == 1) {
        Bp_GetBits(bp, 5, code);
        *quant_scale = code;
    }
}

#define PARSE_I_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, quant_scale)  \
{                                                                            \
    int code, index, quantize;                                               \
    int mb_addr_incr = 0;                                                    \
    do {                                                                     \
        Bp_PeekBits(bp, 11, index);                                             \
        code = mbai[index].value;                                            \
        Bp_FlushBits(bp, mbai[index].numOfBits);                                \
        if (mbai[index].numOfBits == 0) {                                    \
        }                                                                    \
        if (code == MACRO_BLOCK_ESCAPE) {                                    \
            mb_addr_incr += 33;                                              \
        }                                                                    \
    } while (code == MACRO_BLOCK_ESCAPE || code == MACRO_BLOCK_STUFFING);    \
    mb_addr_incr += code;                                                    \
    curr_mb_addr = prev_mb_addr + mb_addr_incr;                              \
    prev_mb_addr = curr_mb_addr;                                             \
    Bp_PeekBits(bp, 2, code);                                                   \
    quantize = quantizeTable[code];                                          \
    if (code)                                                                \
        Bp_FlushBits(bp, 1 + quantize);                                         \
    if (quantize) {                                                          \
        Bp_GetBits(bp, 5, code);                                                \
        quant_scale = code;                                                  \
    }                                                                        \
}

void
ParsePMacroBlockHdr(bp, prev_mb_addr, curr_mb_addr, i, quant_scale)
    BitParser *bp;
    int *prev_mb_addr;
    int *curr_mb_addr;
    int *i;
    int *quant_scale;
{
    int code;
    int mb_addr_incr = 0;

    do {
        Bp_PeekBits(bp, 11, *i);
        code = mbai[*i].value;
        Bp_FlushBits(bp, mbai[*i].numOfBits);
        if (mbai[*i].numOfBits == 0) {
        }
        if (code == MACRO_BLOCK_ESCAPE) {
            mb_addr_incr += 33;
        }
    } while (code == MACRO_BLOCK_ESCAPE || code == MACRO_BLOCK_STUFFING);
    mb_addr_incr += code;
    *curr_mb_addr = *prev_mb_addr + mb_addr_incr;
    *prev_mb_addr = *curr_mb_addr;
    Bp_PeekBits(bp, 6, code);
    *i = code;
    Bp_FlushBits(bp, mbp[code].num_of_bits);
    if (mbp[code].quantize) {
        Bp_GetBits(bp, 5, code);
        *quant_scale = code;
    }
}

#define PARSE_P_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, index, quant_scale) {\
    int code;                                                                \
    int mb_addr_incr = 0;                                                    \
    do {                                                                     \
        Bp_PeekBits(bp, 11, index);                                             \
        code = mbai[index].value;                                            \
        Bp_FlushBits(bp, mbai[index].numOfBits);                                \
        if (mbai[index].numOfBits == 0) {                                    \
        }                                                                    \
        if (code == MACRO_BLOCK_ESCAPE) {                                    \
            mb_addr_incr += 33;                                              \
        }                                                                    \
    } while (code == MACRO_BLOCK_ESCAPE || code == MACRO_BLOCK_STUFFING);    \
    mb_addr_incr += code;                                                    \
    curr_mb_addr = prev_mb_addr + mb_addr_incr;                              \
    prev_mb_addr = curr_mb_addr;                                             \
    Bp_PeekBits(bp, 6, code);                                                   \
    index = code;                                                            \
    Bp_FlushBits(bp, mbp[code].num_of_bits);                                    \
    if (mbp[code].quantize) {                                                \
        Bp_GetBits(bp, 5, code);                                                \
        quant_scale = code;                                                  \
    }                                                                        \
}

#define PARSE_B_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, index, quant_scale) {\
    int code;                                                                \
    int mb_addr_incr = 0;                                                    \
    do {                                                                     \
        Bp_PeekBits(bp, 11, index);                                             \
        code = mbai[index].value;                                            \
        Bp_FlushBits(bp, mbai[index].numOfBits);                                \
        if (mbai[index].numOfBits == 0) {                                    \
        }                                                                    \
        if (code == MACRO_BLOCK_ESCAPE) {                                    \
            mb_addr_incr += 33;                                              \
        }                                                                    \
    } while (code == MACRO_BLOCK_ESCAPE || code == MACRO_BLOCK_STUFFING);    \
    mb_addr_incr += code;                                                    \
    curr_mb_addr = prev_mb_addr + mb_addr_incr;                              \
    prev_mb_addr = curr_mb_addr;                                             \
    Bp_PeekBits(bp, 6, code);                                                   \
    index = code;                                                            \
    Bp_FlushBits(bp, mbb[code].num_of_bits);                                    \
    if (mbb[code].quantize) {                                                \
        Bp_GetBits(bp, 5, code);                                                \
        quant_scale = code;                                                  \
    }                                                                        \
}

#define SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v) {\
    curr_y1->skipMB = 1;\
    curr_y1++;\
    curr_y1->skipMB = 1;\
    curr_y1++;\
    curr_y2->skipMB = 1;\
    curr_y2++;\
    curr_y2->skipMB = 1;\
    curr_y2++;\
    curr_u->skipMB = 1;\
    curr_u++;\
    curr_v->skipMB = 1;\
    curr_v++;\
}

#define SKIP_ALL_BLOCKS(curr_y1, curr_y2, curr_u, curr_v) {\
    curr_y1->skipMB = 0;\
    curr_y1->skipBlock = 1;\
    curr_y1++;\
    curr_y1->skipMB = 0;\
    curr_y1->skipBlock = 1;\
    curr_y1++;\
    curr_y2->skipMB = 0;\
    curr_y2->skipBlock = 1;\
    curr_y2++;\
    curr_y2->skipMB = 0;\
    curr_y2->skipBlock = 1;\
    curr_y2++;\
    curr_u->skipMB = 0;\
    curr_u->skipBlock = 1;\
    curr_u++;\
    curr_v->skipMB = 0;\
    curr_v->skipBlock = 1;\
    curr_v++;\
}

#ifdef DEBUG_MV
#define SKIP_VECTOR(curr_vector) {\
    fprintf(stderr, "%d,%d\n", 0, 0);\
    curr_vector->exists = 0;\
    curr_vector->down = 0;\
    curr_vector->right = 0;\
    curr_vector++;\
}
#define SET_VECTOR(curr_vector, d, r) {\
    fprintf(stderr, "%d,%d\n", r, d);\
    (curr_vector)->down = d;\
    (curr_vector)->right = r;\
}
#else
#define SKIP_VECTOR(curr_vector) {\
    curr_vector->exists = 0;\
    curr_vector->down = 0;\
    curr_vector->right = 0;\
    curr_vector++;\
}
#define SET_VECTOR(curr_vector, d, r) {\
    (curr_vector)->down = d;\
    (curr_vector)->right = r;\
}
#endif


#define DecodeMotionVectors(bp, val)    \
{                                       \
  unsigned int index;                   \
  Bp_PeekBits(bp, 11, index);           \
  val = mvTable[index].value;           \
  Bp_FlushBits(bp, mvTable[index].numOfBits);   \
}

unsigned int bitMask[] =
{                               // note : diff from bitmask[]
    0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
    0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff,
    0x00ffffff, 0x007fffff, 0x003fffff, 0x001fffff,
    0x000fffff, 0x0007ffff, 0x0003ffff, 0x0001ffff,
    0x0000ffff, 0x00007fff, 0x00003fff, 0x00001fff,
    0x00000fff, 0x000007ff, 0x000003ff, 0x000001ff,
    0x000000ff, 0x0000007f, 0x0000003f, 0x0000001f,
    0x0000000f, 0x00000007, 0x00000003, 0x00000001
};


unsigned int rBitMask[] =
{
    0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
    0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
    0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
    0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
    0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
    0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
    0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
    0xf0000000, 0xe0000000, 0xc0000000, 0x80000000};

unsigned int bitTest[] =
{
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001};


static void
GetDctCoeff(bp, table, run, level)
    BitParser *bp;
    unsigned short *table;
    unsigned int *run;
    int *level;
{
    register unsigned int index, temp;
    register unsigned int value, numbits;

    Bp_PeekInt(bp, value);
    Bp_PeekBits(bp, 8, index);
    if (index <= 3) {
        switch (index) {
        case 0:
            Bp_PeekBits(bp, 16, index);
            value = dctCoeff[index & 255];
            break;
        case 1:
            Bp_PeekBits(bp, 12, index);
            value = dctCoeff1[index & 15];
            break;
        case 2:
            Bp_PeekBits(bp, 10, index);
            value = dctCoeff2[index & 3];
            break;
        case 3:
            Bp_PeekBits(bp, 10, index);
            value = dctCoeff3[index & 3];
            break;
        }
        *run = value >> RUN_SHIFT;
        // *run = (value & RUN_MASK) >> RUN_SHIFT;
        *level = (value & LEVEL_MASK) >> LEVEL_SHIFT;
        numbits = (value & NUM_MASK) + 1;
        Bp_FlushBits(bp, numbits);
        Bp_GetBits(bp, 1, value);
        if (value)
            *level = -*level;
    } else {
        value = table[index];
        *run = value >> RUN_SHIFT;
        // *run = (value & RUN_MASK) >> RUN_SHIFT;
        if (*run == EOB) {
            *level = EOB;
            return;
        }
        numbits = (value & NUM_MASK) + 1;
        Bp_FlushBits(bp, numbits);
        if (*run != ESCAPE) {
            *level = (value & LEVEL_MASK) >> LEVEL_SHIFT;
            Bp_GetBits(bp, 1, value);
            if (value)
                *level = -*level;
        } else {
            Bp_GetBits(bp, 14, temp);
            *run = temp >> 8;
            temp &= 0xff;
            if (temp == 0) {
                Bp_GetBits(bp, 8, (*level));
            } else if (temp != 128) {
                *level = ((int) (temp << 24)) >> 24;
            } else {
                Bp_GetBits(bp, 8, (*level));
                *level = *level - 256;
            }
        }
    }
}


static void
ParseIntraBlockY(bp, block, quantizer_scale, qTable, dcPredict)
    BitParser *bp;
    ScBlock *block;
    int quantizer_scale;
    char *qTable;
    int *dcPredict;
{
    int index, size, dcFirst;
    unsigned int i = 0;
    int coeff_count = 0, pos;
    short *curr_value;
    char *curr_index;
    int num_of_value;

    num_of_value = 0;
    Bp_PeekBits(bp, 5, index);
    if (index < 31) {
        size = dctl[index].value;
        Bp_FlushBits(bp, dctl[index].numOfBits);
    } else {
        Bp_PeekBits(bp, 9, index);
        index -= 0x1f0;
        size = dctl1[index].value;
        Bp_FlushBits(bp, dctl1[index].numOfBits);
    }
    if (size != 0) {
        Bp_GetBits(bp, size, dcFirst);
        if (!(dcFirst & bitTest[32 - size])) {
            dcFirst = rBitMask[size] | (dcFirst + 1);
        }
    } else {
        dcFirst = 0;
    }
    block->dc = (dcFirst << 3) + *dcPredict;
    *dcPredict = block->dc;
    curr_index = &(block->index[0]);
    curr_value = &(block->value[0]);

#ifdef DEBUG_MB
    if (debugOn)
        fprintf(stderr, "[%d]\n", block->dc);   // DEBUG
#endif

    pos = 0;
    /*
     * Parse the dct coefficient next for B, P, I frames.
     * assert : never reach here if it's a D frame.
     * (note : no dct coefficient first for intra block)
     */
    while (1) {
        unsigned int run;
        int level;
        int coeff;

        GetDctCoeff(bp, dctn, &run, &level);
        /*GetDctCoeffBug(bp, dctn, run, level); */
        if (run >= EOB) {
            break;
        }
        i += run + 1;
        pos = zzDirect[i];
        if (level < 0) {
            coeff = ((level << 1) * quantizer_scale * ((int) qTable[pos])) / 16;
            coeff += (1 - (coeff & 1));
        } else {
            coeff = ((level << 1) * quantizer_scale * ((int) (*(qTable + pos)))) >> 4;
            coeff -= (1 - (coeff & 1));
        }

        *curr_index++ = i;
        *curr_value++ = coeff;
#ifdef DEBUG_MB
        if (debugOn)
            fprintf(stderr, "%d %d %d\n", i, pos, coeff);
#endif
        coeff_count++;
    }
    block->numOfAC = coeff_count;
    Bp_FlushBits(bp, 2);        /* flush the end of block marker */
}


static void
ParseIntraBlockUV(bp, block, quantizer_scale, qTable, dcPredict)
    BitParser *bp;
    ScBlock *block;
    int quantizer_scale;
    char *qTable;
    int *dcPredict;
{
    int size, dcFirst, coeff;
    unsigned int i, index;
    char *curr_index;
    short *curr_value;
    int coeff_count = 0;
    unsigned int pos;

    /*
     * Cb and Cr plane
     */
    Bp_PeekBits(bp, 5, index);
    if (index < 31) {
        size = dctc[index].value;
        Bp_FlushBits(bp, dctc[index].numOfBits);
    } else {
        Bp_PeekBits(bp, 10, index);
        index -= 0x3e0;
        size = dctc1[index].value;
        Bp_FlushBits(bp, dctc1[index].numOfBits);
    }

    if (size != 0) {
        Bp_GetBits(bp, size, dcFirst);
        if (!(dcFirst & bitTest[32 - size])) {
            dcFirst = rBitMask[size] | (dcFirst + 1);
        }
    } else {
        dcFirst = 0;
    }
    block->dc = (dcFirst << 3) + *dcPredict;
#ifdef DEBUG_MB
    if (debugOn)
        fprintf(stderr, "[%d]\n", block->dc);
#endif

    *dcPredict = block->dc;
    curr_index = &(block->index[0]);
    curr_value = &(block->value[0]);
    /*
     * Parse the dct coefficient next for B, P, I frames.
     * assert : never reach here if it's a D frame.
     * (note : no dct coefficient first for intra block)
     */
    i = 0;
    pos = 0;
    while (1) {
        unsigned int run;
        int level;

        GetDctCoeff(bp, dctn, &run, &level);
        /*GetDctCoeffBug(bp, dctn, run, level); */

        if (run >= EOB) {
            break;
        }
        i += run + 1;
        pos = zzDirect[i];
        if (level < 0) {
            coeff = ((level << 1) * quantizer_scale * ((int) qTable[pos])) / 16;
            coeff += (1 - (coeff & 1));
        } else {
            coeff = ((level << 1) * quantizer_scale * ((int) (*(qTable + pos)))) >> 4;
            coeff -= (1 - (coeff & 1));
        }

        *curr_index++ = i;
        *curr_value++ = coeff;
#ifdef DEBUG_MB
        if (debugOn)
            fprintf(stderr, "%d %d %d\n", i, pos, coeff);       // DEBUG
#endif
        coeff_count++;
    }
    block->numOfAC = coeff_count;
    Bp_FlushBits(bp, 2);        /* flush the end of block marker */
}

static void
ParseNonIntraBlock(bp, block, quantizer_scale, qTable)
    BitParser *bp;
    ScBlock *block;
    int quantizer_scale;
    char *qTable;
{
    unsigned int pos, i;
    unsigned int run;
    int level;
    int coeff;
    int coeff_count = 0;
    char *curr_index;
    short *curr_value;

    GetDctCoeff(bp, dctf, &run, &level);
    /*GetDctCoeffBug(bp, dctf, run, level); */
    pos = zzDirect[run];
    if (level < 0) {
        coeff = (((level << 1) - 1) * quantizer_scale * ((int) qTable[pos])) / 16;
        coeff += (1 - (coeff & 1));
    } else {
        coeff = (((level << 1) + 1) * quantizer_scale * ((int) (*(qTable + pos)))) >> 4;
        coeff -= (1 - (coeff & 1));
    }

    curr_index = &(block->index[0]);
    curr_value = &(block->value[0]);
    if (pos) {
        block->dc = 0;
        *curr_index++ = run;
        *curr_value++ = coeff;
        coeff_count = 1;
    } else {
        block->dc = coeff;
    }

#ifdef DEBUG_MB
    if (debugOn)
        fprintf(stderr, "%d %d %d\n", run, pos, coeff);         // DEBUG
#endif

    i = run;

    /*
     * Parse the dct coefficient next for B, P, I frames.
     * assert : never reach here if it's a D frame.
     * (note : no dct coefficient first for intra block)
     */
    while (1) {
        /*GetDctCoeffBug(bp, dctn, run, level); */
        GetDctCoeff(bp, dctn, &run, &level);
        if (run >= EOB) {
            break;
        }
        i += run + 1;
        pos = zzDirect[i];
        if (level < 0) {
            coeff = (((level << 1) - 1) * quantizer_scale * ((int) qTable[pos])) / 16;
            coeff += (coeff & 1);
        } else {
            coeff = (((level << 1) + 1) * quantizer_scale * ((int) (*(qTable + pos)))) >> 4;
            coeff -= (coeff & 1);
        }

        *curr_index++ = i;
        *curr_value++ = coeff;
        coeff_count++;

#ifdef DEBUG_MB
        if (debugOn)
            fprintf(stderr, "%d %d %d\n", i, pos, coeff);       // DEBUG
#endif
    }
    block->numOfAC = coeff_count;
    Bp_FlushBits(bp, 2);        /* flush the end of block marker */
}

int
MpegPicIParse(bp, seq_hdr, pic_hdr, y, u, v)
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y, *u, *v;
{
    unsigned int code, off;
    register int quant_scale = 0;
    register int extra;
    register int slice_vertical_pos;
    register int i, j;
    ScBlock *curr_y1;
    ScBlock *curr_y2;
    ScBlock *curr_u;
    ScBlock *curr_v;
    int dct_dc_y_past;
    int dct_dc_u_past;
    int dct_dc_v_past;
    int curr_mb_addr;
    int prev_mb_addr = 0;
    int prev_intra_addr = 0;

    // Bp_UpdateBuffer(bp, seq_hdr->vbv_buffer_size, 0);
    Bp_ByteAlign(bp);

    curr_y1 = y->firstBlock;
    curr_y2 = y->firstBlock + y->parentWidth;
    curr_u = u->firstBlock;
    curr_v = v->firstBlock;
#if defined(DEBUG_MB) || defined(DEBUG_MV)
    if (picCount >= 5)
        debugOn = 1;
    fprintf(stderr, "PIC %d--\n", picCount++);
#endif
    for (i = 0; i < y->height / 2; i++) {
        for (j = 0; j < y->width / 2; j++) {
            /*
             * For each macroblock, we parse sc data into 4 y blocks 1 u block
             * and 1 v block.  after each macroblock we always check if we hit
             * the slice header, if yes, then parse off the slice header and
             * continue, if unfortunately we hit the sequence end code then
             * we return (sorry!)
             */
#if defined(DEBUG_MB) || defined(DEBUG_MV)
            if (debugOn)
                fprintf(stderr, "MB %d\n", count++);    // DEBUG
#endif
            Next23BitsIsZero(bp, &code);
            if (code == 1) {
                Bp_ByteAlign(bp);
                code = NextStartCode(bp, &off);
                if (code <= SLICE_MAX_START_CODE && code >= SLICE_MIN_START_CODE) {
                    /*
                     * Parse off slice hdr : first 5 bits is scale, followed by 1 bit
                     * to indicate extra info.
                     */
                    slice_vertical_pos = code & 0x000000FF;
                    Bp_GetBits(bp, 6, code);
                    extra = code & 0x00000001;
                    quant_scale = code >> 1;
                    if (extra)
                        Bp_FlushBits(bp, 8);
                    dct_dc_y_past = 1024;
                    dct_dc_u_past = 1024;
                    dct_dc_v_past = 1024;
                    prev_mb_addr = (slice_vertical_pos - 1) * seq_hdr->mb_width - 1;
                    prev_intra_addr = -2;
                } else if (code == SEQ_END_CODE) {
                    MY_ASSERT(1, "reach end of sequence before all mb are parsed");
                } else {
                    MY_ASSERT(1, "reach end of i-pic before all mb are parsed");
                }
            }
            /*
             * Now parse one macroblock :
             * - parse the header.
             * - parse 4 y blocks 1 u block and 1 v block.
             */
#ifdef DEBUG_MB
            ParseIMacroBlockHdr(bp, &prev_mb_addr, &curr_mb_addr, &quant_scale);
            // fprintf(stderr, "prev_mb_addr %d curr_mb_addr %d\n", prev_mb_addr, curr_mb_addr);*/
#else
            PARSE_I_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, quant_scale);
#endif
            if (curr_mb_addr - prev_intra_addr > 1) {
                /*
                 * Reset dc predictor for intracoded block
                 */
                dct_dc_y_past = 1024;
                dct_dc_u_past = 1024;
                dct_dc_v_past = 1024;
            }
            // prev_mb_addr = curr_mb_addr;
            prev_intra_addr = curr_mb_addr;

            ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);
            curr_y1++;
            ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);
            ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);
            curr_y2++;
            ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);
            ParseIntraBlockUV(bp, curr_u, quant_scale, seq_hdr->iqt, &dct_dc_u_past);
            ParseIntraBlockUV(bp, curr_v, quant_scale, seq_hdr->iqt, &dct_dc_v_past);

            curr_y1++;
            curr_y2++;
            curr_u++;
            curr_v++;
        }
        curr_y1 += 2 * y->parentWidth - y->width;
        curr_y2 += 2 * y->parentWidth - y->width;
        curr_u += u->parentWidth - u->width;
        curr_v += v->parentWidth - v->width;
    }

    if (code != SEQ_END_CODE && Bp_Underflow(bp)) {
        return 0;
    }
    Bp_ByteAlign(bp);
    Bp_PeekInt(bp, code);
    MY_ASSERT(
        code == PIC_START_CODE || code == GOP_START_CODE || code == SEQ_END_CODE,
        "next code after a pic is not a start code.\n"
        );
    return 1;
}

int
MpegPicPParse(bp, seq_hdr, pic_hdr, y, u, v, mv)
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y, *u, *v;
    VectorImage *mv;
{
    unsigned int code, data, off;
    register int quant_scale = 0;
    register int extra, index = 0, pattern = 0;
    register int slice_vertical_pos;
    register int i, j;
    Vector *curr_vector;
    ScBlock *curr_y1, *curr_y2, *curr_u, *curr_v;
    int right, down, prev_right = 0, prev_down = 0;
    int dct_dc_y_past, dct_dc_u_past, dct_dc_v_past;
    int curr_mb_addr = 0, prev_mb_addr = 0, skip_check_addr, prev_intra_addr = 0;
    int skip_count;
    int horiz, vert, horiz_r = 0, vert_r = 0;

    // Bp_UpdateBuffer(bp, seq_hdr->vbv_buffer_size, 0);
    Bp_ByteAlign(bp);

    curr_y1 = y->firstBlock;
    curr_y2 = curr_y1 + y->parentWidth;
    curr_u = u->firstBlock;
    curr_v = v->firstBlock;
    curr_vector = mv->firstVector;
    skip_count = 0;
    skip_check_addr = 0;

#if defined(DEBUG_MB) || defined(DEBUG_MV)
    if (picCount >= 5)
        debugOn = 1;
    fprintf(stderr, "PIC %d--\n", picCount++);
#endif
    /*fprintf(stderr, "P FRAME\n"); // DEBUG */
    for (i = 0; i < y->height / 2; i++) {
        for (j = 0; j < y->width / 2; j++) {

            /*
             * For each macroblock, we parse sc data into 4 y blocks 1 u block
             * and 1 v block.  after each macroblock we always check if we hit
             * the slice hdr, if yes, then parse off the slice hdr and
             * continue. if we hit the sequence end code then the last few
             * blocks are skipped.
             * P Frame might have skipped-macroblock.  I use skip_count to
             * indicate :
             *   skip_count == 0   : no skip block.  everything's fine.
             *   skip_count >  1   : this is a skip block. skip it.
             *   skip_count == 1   : last block is a skip block.  but this
             *                       block is not. however the "header" info
             *                       have been parse off. don't need to parse
             *                       again.
             */
            /*fprintf(stderr, "%d %d %d %x %x\n", i, j, bp->bitCount,
             *bp->currentBits, bp->bs->buf[0]);*/
            Next23BitsIsZero(bp, &code);
            if (code) {
                Bp_ByteAlign(bp);
                Bp_PeekInt(bp, code);
                if (code == SEQ_END_CODE) {
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                    if (debugOn)
                        fprintf(stderr, "MB %d\n", count++);    // DEBUG
#endif
                    while (i < y->height / 2) {
                        while (j < y->width / 2) {
                            SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                            SKIP_VECTOR(curr_vector);
                            j++;
                        }
                        i++;
                    }
                    continue;
                }
                code = NextStartCode(bp, &off);
                /*
                 * if we reach the beginning of slice, parse off slice hdr :
                 * first 5 bits is scale, followed by 1 bit to indicate extra info.
                 */
                if (code <= SLICE_MAX_START_CODE && code >= SLICE_MIN_START_CODE) {
                    slice_vertical_pos = code & 0x000000FF;
                    Bp_GetBits(bp, 6, code);
                    extra = code & 0x00000001;
                    quant_scale = code >> 1;
                    if (extra)
                        Bp_FlushBits(bp, 8);
                    dct_dc_y_past = 1024;
                    dct_dc_u_past = 1024;
                    dct_dc_v_past = 1024;
                    prev_down = 0;
                    prev_right = 0;
                    prev_mb_addr = (slice_vertical_pos - 1) * seq_hdr->mb_width - 1;
                } else {
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                    if (debugOn)
                        fprintf(stderr, "MB %d\n", count++);    // DEBUG
#endif
                    while (i < y->height / 2) {
                        while (j < y->width / 2) {
                            SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                            SKIP_VECTOR(curr_vector);
                            j++;
                        }
                        i++;
                    }
                    continue;
                }
            }
            /*
             * If this is a skipped macroblock, we skip !
             */
            if (skip_count > 1) {
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                if (debugOn)
                    fprintf(stderr, "MB %d\n", count++);        // DEBUG
#endif
                SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                SKIP_VECTOR(curr_vector);
                skip_count--;
                continue;
            }
            /*
             * Parse the macroblock header which contains address, index to
             * cbp table and quantscale. We don't do this if the header have
             * been read before when we try to read a skipped block.
             */
            if (skip_count == 0) {
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                if (debugOn)
                    fprintf(stderr, "MB %d\n", count++);        // DEBUG

                ParsePMacroBlockHdr(bp, &prev_mb_addr, &curr_mb_addr, &index, &quant_scale);
#else
                PARSE_P_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, index, quant_scale);
#endif
                /*fprintf(stderr, "curr_mb_addr : %d\n", curr_mb_addr); */
                if (curr_mb_addr - skip_check_addr > 1) {
                    /*
                     * This macroblock is skipped.  (This header actually
                     * belongs to someone else.)
                     */
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                    if (debugOn)
                        fprintf(stderr, "MB %d\n", count++);    // DEBUG
#endif
                    skip_count = curr_mb_addr - skip_check_addr - 1;
                    skip_check_addr = curr_mb_addr;
                    SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                    SKIP_VECTOR(curr_vector);
                    prev_down = 0;
                    prev_right = 0;
                    continue;
                } else {
                    skip_check_addr = curr_mb_addr;
                }
            } else {            // skip_count == 1
                /*
                 * We are done processing skipped-block. resume normal
                 * operation.
                 */
                skip_count = 0;
            }

            /*
             * If motion vector exists, read off the motion vector. else set
             * the vector to 0
             */
            if (mbp[index].macroblock_motion_forward) {
                DecodeMotionVectors(bp, horiz);
                if (pic_hdr->forward_f != 1 && horiz != 0) {
                    Bp_GetBits(bp, pic_hdr->forward_r_size, data);
                    horiz_r = data;
                }
                DecodeMotionVectors(bp, vert);
                if (pic_hdr->forward_f != 1 && vert != 0) {
                    Bp_GetBits(bp, pic_hdr->forward_r_size, data);
                    vert_r = data;
                }
                ComputeVector(
                    &right, &down, prev_right, prev_down,
                    pic_hdr->forward_f, pic_hdr->full_pel_forward_vector,
                    horiz, vert, horiz_r, vert_r);
                /*prev_right = right; */
                /*prev_down  = down; */
                SET_VECTOR(curr_vector, down, right);
                curr_vector++;
            } else {
                SKIP_VECTOR(curr_vector);
                prev_down = 0;
                prev_right = 0;
            }

            MY_ASSERT(
                mbp[index].macroblock_motion_backward == 0,
                "p-frame contains forward vector. unfair !"
                );

            if (mbp[index].pattern) {
                Bp_PeekBits(bp, 9, data);
                pattern = cbp[data].cbp;
                Bp_FlushBits(bp, cbp[data].num_of_bits);
            } else if (!mbp[index].intracoded) {
                /*
                 * If there is no pattern, and it is not intracoded, there is
                 * nothing we can do !
                 */
                SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                continue;
            }
            if (mbp[index].intracoded) {
                if (curr_mb_addr - prev_intra_addr > 1) {
                    /*
                     * Reset dc predictor for intracoded block
                     */
                    dct_dc_y_past = 1024;
                    dct_dc_u_past = 1024;
                    dct_dc_v_past = 1024;
                }
                prev_intra_addr = curr_mb_addr;

                curr_y1->skipMB = 0;
                curr_y1->skipBlock = 0;
                curr_y1->intracoded = 1;
                ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y1++;
                curr_y1->skipMB = 0;
                curr_y1->skipBlock = 0;
                curr_y1->intracoded = 1;
                ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y2->skipMB = 0;
                curr_y2->skipBlock = 0;
                curr_y2->intracoded = 1;
                ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y2++;
                curr_y2->skipMB = 0;
                curr_y2->skipBlock = 0;
                curr_y2->intracoded = 1;
                ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_u->skipMB = 0;
                curr_u->skipBlock = 0;
                curr_u->intracoded = 1;
                ParseIntraBlockUV(bp, curr_u, quant_scale, seq_hdr->iqt, &dct_dc_u_past);

                curr_v->skipMB = 0;
                curr_v->skipBlock = 0;
                curr_v->intracoded = 1;
                ParseIntraBlockUV(bp, curr_v, quant_scale, seq_hdr->iqt, &dct_dc_v_past);

                prev_right = 0;
                prev_down = 0;
            } else {
                curr_y1->skipMB = 0;
                if (pattern & 0x20) {
                    curr_y1->skipBlock = 0;
                    curr_y1->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y1, quant_scale, seq_hdr->niqt);
                } else
                    curr_y1->skipBlock = 1;
                curr_y1++;

                curr_y1->skipMB = 0;
                if (pattern & 0x10) {
                    curr_y1->skipBlock = 0;
                    curr_y1->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y1, quant_scale, seq_hdr->niqt);
                } else
                    curr_y1->skipBlock = 1;

                curr_y2->skipMB = 0;
                if (pattern & 0x08) {
                    curr_y2->skipBlock = 0;
                    curr_y2->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y2, quant_scale, seq_hdr->niqt);
                } else
                    curr_y2->skipBlock = 1;

                curr_y2++;
                curr_y2->skipMB = 0;
                if (pattern & 0x04) {
                    curr_y2->skipBlock = 0;
                    curr_y2->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y2, quant_scale, seq_hdr->niqt);
                } else
                    curr_y2->skipBlock = 1;

                curr_u->skipMB = 0;
                if (pattern & 0x02) {
                    curr_u->skipBlock = 0;
                    curr_u->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_u, quant_scale, seq_hdr->niqt);
                } else
                    curr_u->skipBlock = 1;

                curr_v->skipMB = 0;
                if (pattern & 0x01) {
                    curr_v->skipBlock = 0;
                    curr_v->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_v, quant_scale, seq_hdr->niqt);
                } else
                    curr_v->skipBlock = 1;
            }
            curr_y1++;
            curr_y2++;
            curr_u++;
            curr_v++;
        }
        curr_y1 += 2 * y->parentWidth - y->width;
        curr_y2 += 2 * y->parentWidth - y->width;
        curr_u += u->parentWidth - u->width;
        curr_v += v->parentWidth - v->width;
        curr_vector += mv->parentWidth - mv->width;
    }

    if (code != SEQ_END_CODE && Bp_Underflow(bp)) {
        return 0;
    }
    Bp_ByteAlign(bp);
    Bp_PeekInt(bp, code);
    MY_ASSERT(
        code == PIC_START_CODE || code == GOP_START_CODE || code == SEQ_END_CODE,
        "next code after a pic is not a start code.\n"
        );
    return 1;
}

int
MpegPicBParse(bp, seq_hdr, pic_hdr, y, u, v, fwd_mv, bwd_mv)
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y;
    ScImage *u;
    ScImage *v;
    VectorImage *fwd_mv;
    VectorImage *bwd_mv;
{
    unsigned int code, data, off;
    register int quant_scale = 0;
    register int extra, index = 0, pattern = 0;
    register int slice_vertical_pos;
    register int i, j;
    Vector *curr_fwd_vector;
    Vector *curr_bwd_vector;
    ScBlock *curr_y1, *curr_y2, *curr_u, *curr_v;
    int right, down, prev_fwd_right = 0, prev_fwd_down = 0;
    int prev_bwd_right = 0, prev_bwd_down = 0;
    int dct_dc_y_past, dct_dc_u_past, dct_dc_v_past;
    int curr_mb_addr = 0, prev_mb_addr = 0, skip_check_addr, prev_intra_addr = 0;
    int skip_count;
    int horiz, vert, horiz_r = 0, vert_r = 0;

    // Bp_UpdateBuffer(bp, seq_hdr->vbv_buffer_size, 0);
    Bp_ByteAlign(bp);

    curr_y1 = y->firstBlock;
    curr_y2 = curr_y1 + y->parentWidth;
    curr_u = u->firstBlock;
    curr_v = v->firstBlock;
    curr_bwd_vector = bwd_mv->firstVector;
    curr_fwd_vector = fwd_mv->firstVector;
    skip_count = 0;
    skip_check_addr = 0;

#if defined(DEBUG_MB) || defined(DEBUG_MV)
    if (picCount >= 5)
        debugOn = 1;
    fprintf(stderr, "PIC %d--\n", picCount++);
#endif
    /*fprintf(stderr, "B FRAME\n"); // DEBUG */
    for (i = 0; i < y->height / 2; i++) {
        for (j = 0; j < y->width / 2; j++) {

            /*
             * For each macroblock, we parse sc data into 4 y blocks 1 u block
             * and 1 v block.  after each macroblock we always check if we hit
             * the slice hdr, if yes, then parse off the slice hdr and
             * continue. if we hit the sequence end code then the last few
             * blocks are skipped.
             * P Frame might have skipped-macroblock.  I use skip_count to
             * indicate :
             *   skip_count == 0   : no skip block.  everything's fine.
             *   skip_count >  1   : this is a skip block. skip it.
             *   skip_count == 1   : last block is a skip block.  but this
             *                       block is not. however the "header" info
             *                       have been parse off. don't need to parse
             *                       again.
             */
            Next23BitsIsZero(bp, &code);
            if (code) {
                Bp_ByteAlign(bp);
                Bp_PeekInt(bp, code);
                if (code == SEQ_END_CODE) {
                    while (i < y->height / 2) {
                        while (j < y->width / 2) {
                            SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                            SKIP_VECTOR(curr_fwd_vector);
                            SKIP_VECTOR(curr_bwd_vector);
                            j++;
                        }
                        i++;
                    }
                    continue;
                }
                code = NextStartCode(bp, &off);

                /*
                 * if we reach the beginning of slice, parse off slice hdr :
                 * first 5 bits is scale, followed by 1 bit to indicate extra info.
                 */
                if (code <= SLICE_MAX_START_CODE && code >= SLICE_MIN_START_CODE) {
                    slice_vertical_pos = code & 0x000000FF;
                    Bp_GetBits(bp, 6, code);
                    extra = code & 0x00000001;
                    quant_scale = code >> 1;
                    if (extra)
                        Bp_FlushBits(bp, 8);
                    dct_dc_y_past = dct_dc_u_past = dct_dc_v_past = 1024;
                    prev_fwd_down = prev_bwd_down = prev_fwd_right = prev_bwd_right = 0;
                    prev_mb_addr = (slice_vertical_pos - 1) * seq_hdr->mb_width - 1;
                } else {
                    while (i < y->height / 2) {
                        while (j < y->width / 2) {
                            SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                            SKIP_VECTOR(curr_fwd_vector);
                            SKIP_VECTOR(curr_bwd_vector);
                            j++;
                        }
                        i++;
                    }
                    continue;
                }
            }
            /*
             * If this is a skipped macroblock, we skip !
             */
            if (skip_count > 1) {
                /*fprintf(stderr, "skip\n"); */
                SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                SKIP_VECTOR(curr_fwd_vector);
                SKIP_VECTOR(curr_bwd_vector);
                skip_count--;
                continue;
            }
            /*
             * Parse the macroblock header which contains address, index to
             * cbp table and quantscale. We don't do this if the header have
             * been read before when we try to read a skipped block.
             */
            if (skip_count == 0) {
#if defined(DEBUG_MB) || defined(DEBUG_MV)
                if (debugOn)
                    fprintf(stderr, "MB %d\n", count++);        // DEBUG
#endif
                PARSE_B_MACROBLOCK_HDR(bp, prev_mb_addr, curr_mb_addr, index, quant_scale);
                /*fprintf(stderr, "curr_mb_addr : %d\n", curr_mb_addr); */
                if (curr_mb_addr - skip_check_addr > 1) {
                    /*
                     * This macroblock is skipped.  (This header actually
                     * belongs to someone else.)
                     */
                    skip_count = curr_mb_addr - skip_check_addr - 1;
                    skip_check_addr = curr_mb_addr;
                    SKIP_MACROBLOCK(curr_y1, curr_y2, curr_u, curr_v);
                    SKIP_VECTOR(curr_fwd_vector);
                    SKIP_VECTOR(curr_bwd_vector);
                    continue;
                } else {
                    skip_check_addr = curr_mb_addr;
                }
            } else {            // skip_count == 1
                /*
                 * We are done processing skipped-block. resume normal
                 * operation.
                 */
                skip_count = 0;
            }

            /*
             * If forward motion vector exists, read off the forward motion
             * vector. else set the vector to previous vector
             */
            if (mbb[index].macroblock_motion_forward) {
                DecodeMotionVectors(bp, horiz);
                if (pic_hdr->forward_f != 1 && horiz != 0) {
                    Bp_GetBits(bp, pic_hdr->forward_r_size, data);
                    horiz_r = data;
                }
                DecodeMotionVectors(bp, vert);
                if (pic_hdr->forward_f != 1 && vert != 0) {
                    Bp_GetBits(bp, pic_hdr->forward_r_size, data);
                    vert_r = data;
                }
                ComputeVector(
                    &right, &down, prev_fwd_right, prev_fwd_down,
                    pic_hdr->forward_f, pic_hdr->full_pel_forward_vector,
                    horiz, vert, horiz_r, vert_r);
                /*prev_fwd_right = right; */
                /*prev_fwd_down  = down; */
                curr_fwd_vector->exists = 1;
            } else {
                down = prev_fwd_down;
                right = prev_fwd_right;
                curr_fwd_vector->exists = 0;
            }
            SET_VECTOR(curr_fwd_vector, down, right);
            curr_fwd_vector++;

            /*
             * If backward motion vector exists, read off the backward motion
             * vector. else set the vector to previous vector
             */
            if (mbb[index].macroblock_motion_backward) {
                DecodeMotionVectors(bp, horiz);
                if (pic_hdr->backward_f != 1 && horiz != 0) {
                    Bp_GetBits(bp, pic_hdr->backward_r_size, data);
                    horiz_r = data;
                }
                DecodeMotionVectors(bp, vert);
                if (pic_hdr->backward_f != 1 && vert != 0) {
                    Bp_GetBits(bp, pic_hdr->backward_r_size, data);
                    vert_r = data;
                }
                ComputeVector(
                    &right, &down, prev_bwd_right, prev_bwd_down,
                    pic_hdr->backward_f, pic_hdr->full_pel_backward_vector,
                    horiz, vert, horiz_r, vert_r);
                /*prev_bwd_right = right; */
                /*prev_bwd_down  = down; */
                curr_bwd_vector->exists = 1;
            } else {
                down = prev_bwd_down;   // ?

                right = prev_bwd_right;
                curr_bwd_vector->exists = 0;
            }
            SET_VECTOR(curr_bwd_vector, down, right);
            curr_bwd_vector++;

            if (mbb[index].pattern) {
                Bp_PeekBits(bp, 9, data);
                pattern = cbp[data].cbp;
                Bp_FlushBits(bp, cbp[data].num_of_bits);
            } else if (!mbb[index].intracoded) {
                /*
                 * If there is no pattern, and it is not intracoded, there is
                 * nothing we can do !
                 */
                SKIP_ALL_BLOCKS(curr_y1, curr_y2, curr_u, curr_v);
                continue;
            }
            if (mbb[index].intracoded) {
                if (curr_mb_addr - prev_intra_addr > 1) {
                    /*
                     * Reset dc predictor for intracoded block
                     */
                    dct_dc_y_past = 1024;
                    dct_dc_u_past = 1024;
                    dct_dc_v_past = 1024;
                }
                prev_intra_addr = curr_mb_addr;

                curr_y1->skipMB = 0;
                curr_y1->skipBlock = 0;
                curr_y1->intracoded = 1;
                ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y1++;
                curr_y1->skipMB = 0;
                curr_y1->skipBlock = 0;
                curr_y1->intracoded = 1;
                ParseIntraBlockY(bp, curr_y1, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y2->skipMB = 0;
                curr_y2->skipBlock = 0;
                curr_y2->intracoded = 1;
                ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_y2++;
                curr_y2->skipMB = 0;
                curr_y2->skipBlock = 0;
                curr_y2->intracoded = 1;
                ParseIntraBlockY(bp, curr_y2, quant_scale, seq_hdr->iqt, &dct_dc_y_past);

                curr_u->skipMB = 0;
                curr_u->skipBlock = 0;
                curr_u->intracoded = 1;
                ParseIntraBlockUV(bp, curr_u, quant_scale, seq_hdr->iqt, &dct_dc_u_past);

                curr_v->skipMB = 0;
                curr_v->skipBlock = 0;
                curr_v->intracoded = 1;
                ParseIntraBlockUV(bp, curr_v, quant_scale, seq_hdr->iqt, &dct_dc_v_past);

                prev_bwd_right = 0;
                prev_bwd_down = 0;
                prev_fwd_right = 0;
                prev_fwd_down = 0;

            } else {
                curr_y1->skipMB = 0;
                if (pattern & 0x20) {
                    curr_y1->skipBlock = 0;
                    curr_y1->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y1, quant_scale, seq_hdr->niqt);
                } else
                    curr_y1->skipBlock = 1;
                curr_y1++;

                curr_y1->skipMB = 0;
                if (pattern & 0x10) {
                    curr_y1->skipBlock = 0;
                    curr_y1->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y1, quant_scale, seq_hdr->niqt);
                } else
                    curr_y1->skipBlock = 1;

                curr_y2->skipMB = 0;
                if (pattern & 0x08) {
                    curr_y2->skipBlock = 0;
                    curr_y2->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y2, quant_scale, seq_hdr->niqt);
                } else
                    curr_y2->skipBlock = 1;
                curr_y2++;

                curr_y2->skipMB = 0;
                if (pattern & 0x04) {
                    curr_y2->skipBlock = 0;
                    curr_y2->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_y2, quant_scale, seq_hdr->niqt);
                } else
                    curr_y2->skipBlock = 1;

                curr_u->skipMB = 0;
                if (pattern & 0x02) {
                    curr_u->skipBlock = 0;
                    curr_u->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_u, quant_scale, seq_hdr->niqt);
                } else
                    curr_u->skipBlock = 1;

                curr_v->skipMB = 0;
                if (pattern & 0x01) {
                    curr_v->skipBlock = 0;
                    curr_v->intracoded = 0;
                    ParseNonIntraBlock(bp, curr_v, quant_scale, seq_hdr->niqt);
                } else
                    curr_v->skipBlock = 1;
            }
            curr_y1++;
            curr_y2++;
            curr_u++;
            curr_v++;
        }
        curr_y1 += 2 * y->parentWidth - y->width;
        curr_y2 += 2 * y->parentWidth - y->width;
        curr_u += u->parentWidth - u->width;
        curr_v += v->parentWidth - v->width;
        curr_fwd_vector += fwd_mv->parentWidth - fwd_mv->width;
        curr_bwd_vector += bwd_mv->parentWidth - bwd_mv->width;
    }

    if (code != SEQ_END_CODE && Bp_Underflow(bp)) {
        return 0;
    }
    Bp_ByteAlign(bp);
    Bp_PeekInt(bp, code);
    MY_ASSERT(
        code == PIC_START_CODE || code == GOP_START_CODE || code == SEQ_END_CODE,
        "next code after a pic is not a start code.\n"
        );
    return 1;
}


int
MpegPicDump(inbp, outbp)
    BitParser *inbp;
    BitParser *outbp;
{
    unsigned int total, code, off;

    total = 0;
    Bp_PeekInt(inbp, code);
    if (code > SLICE_MAX_START_CODE || code < SLICE_MIN_START_CODE)
        return -1;

    Bp_GetInt(inbp, code);
    Bp_PutInt(outbp, code);
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
MpegPicSkip(inbp)
    BitParser *inbp;
{
    unsigned int total, code, off;

    total = 0;
    Bp_PeekInt(inbp, code);
    if (code > SLICE_MAX_START_CODE || code < SLICE_MIN_START_CODE)
        return -1;

    Bp_GetInt(inbp, code);
    while ((code = NextStartCode(inbp, &off)) != SEQ_END_CODE) {
        total += off;
        if (code == GOP_START_CODE || code == PIC_START_CODE) {
            Bp_RestoreInt(inbp);
            return total - 4;
        }
    }
    Bp_RestoreInt(inbp);
    return total + off - 4;
}
