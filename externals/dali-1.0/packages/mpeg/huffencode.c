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
 */

#include "mpegInt.h"
#include "tables.h"

static int lengths[256] =
{
    0, 1, 2, 2, 3, 3, 3, 3,     /* 0 - 7 */
    4, 4, 4, 4, 4, 4, 4, 4,     /* 8 - 15 */
    5, 5, 5, 5, 5, 5, 5, 5,     /* 16 - 31 */
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6,     /* 32 - 63 */
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7,     /* 64 - 127 */
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8
};

// this def needs to gathered together
#define ABS(x) (x > 0) ? x : -(x)
/*===========================================================================*
 *
 * EncodeYDC
 *
 *      Encode the DC portion of a DCT of a luminance block
 *
 * RETURNS:     result appended to bp
 *
 * SIDE EFFECTS:    updates pred_term
 *
 *===========================================================================*/
void
EncodeYDC(bp, dc_term, pred_term)
    BitParser *bp;
    short dc_term;
    short *pred_term;
{
    /* see Table B.5a -- MPEG-I doc */
    static int codes[9] =
    {0x4, 0x0, 0x1, 0x5, 0x6, 0xe, 0x1e, 0x3e, 0x7e};
    static int codeLengths[9] =
    {3, 2, 2, 3, 3, 4, 5, 6, 7};
    int ydiff, ydiff_abs;
    int length;

    ydiff = (dc_term - (*pred_term));
    if (ydiff > 255) {
        ydiff = 255;
    } else if (ydiff < -255) {
        ydiff = -255;
    }
    ydiff_abs = ABS(ydiff);
    length = lengths[ydiff_abs];
    Bp_PutBits(bp, codeLengths[length], codes[length]);
    if (length != 0) {
        if (ydiff > 0) {
            Bp_PutBits(bp, length, ydiff_abs);
        } else {
            Bp_PutBits(bp, length, ~ydiff_abs);
        }
    }
    (*pred_term) += ydiff;
}


/*===========================================================================*
 *
 * EncodeCDC
 *
 *      Encode the DC portion of a DCT of a chrominance block
 *
 * RETURNS:     result appended to bb
 *
 * SIDE EFFECTS:    updates pred_term
 *
 *===========================================================================*/
void
EncodeCDC(bp, dc_term, pred_term)
    BitParser *bp;
    short dc_term;
    short *pred_term;
{
    /* see Table B.5b -- MPEG-I doc */
    static int codes[9] =
    {0x0, 0x1, 0x2, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe};
    static int codeLengths[9] =
    {2, 2, 2, 3, 4, 5, 6, 7, 8};
    int cdiff, cdiff_abs;
    int length;

    cdiff = (dc_term - (*pred_term));
    if (cdiff > 255) {
        cdiff = 255;
    } else if (cdiff < -255) {
        cdiff = -255;
    }
    cdiff_abs = ABS(cdiff);
    length = lengths[cdiff_abs];
    Bp_PutBits(bp, codeLengths[length], codes[length]);
    if (length != 0) {
        if (cdiff > 0) {
            Bp_PutBits(bp, length, cdiff_abs);
        } else {
            Bp_PutBits(bp, length, ~cdiff_abs);
        }
    }
    (*pred_term) += cdiff;
}

#define ESCAPE_CODE_ENCODE(bp, run, cur, acur) {\
        Bp_PutBits(bp, 6, 0x1);         /* ESCAPE */\
        Bp_PutBits(bp, 6, run);\
        \
        /* this shouldn't happen, but the other choice is to bomb bp and dump core... */\
    if (cur < -255) {\
            cur = -255;\
        } else if (cur > 255) {\
                cur = 255;\
        }\
        if (acur < 128) {\
                Bp_PutBits(bp, 8, cur);\
        } else {\
                if (cur < 0) {\
                        Bp_PutBits(bp, 16, (0x8001 + cur + 255));\
                } else {\
                        Bp_PutBits(bp, 16, cur);\
                }\
        }\
}\

void
IBlockHuffEncode(bp, scBlock)
    BitParser *bp;
    ScBlock *scBlock;
{
    register int i;
    register int run = 0;
    short cur, acur;            /* level */
    register int code;
    register int nbits;

    for (i = 0; i < scBlock->numOfAC; i++) {
        cur = scBlock->value[i];
        acur = ABS(cur);
        if (i > 0) {
            run = scBlock->index[i] - scBlock->index[i - 1] - 1;
        } else {
            run = scBlock->index[i] - 1;
        }

        if ((run < HUFF_MAXRUN) && (acur < huff_maxlevel[run])) {
            /* encode using the Huffman tables      */
            code = (huff_table[run])[acur];
            nbits = (huff_bits[run])[acur];

            if (cur < 0) {
                code |= 1;      /* the sign bit */
            }
            if (nbits > 15) {
                Bp_PutBits(bp, 15, (code >> (nbits - 15)));
                Bp_PutBits(bp, (nbits - 15), (code & bitmask[nbits - 15]));
            } else {
                Bp_PutBits(bp, nbits, code);
            }
        } else {
            ESCAPE_CODE_ENCODE(bp, run, cur, acur);
        }
    }
    Bp_PutBits(bp, 2, 0x2);     /* end of block marker */
}

void
NonIBlockHuffEncode(bp, in)
    BitParser *bp;
    ScBlock *in;
{
    register int i;
    register int run;
    register int nbits;
    register int code;
    short cur;
    short acur;                 /* level */
    int first_is_zero = 1;      /* first coeff */

    cur = in->dc;
    if (cur) {
        first_is_zero = 0;
        acur = ABS(cur);
        if (acur < huff_maxlevel[0]) {
            if (acur == 1) {
                code = 0x2;
                nbits = 2;
            } else {
                code = (huff_table[0])[acur];
                nbits = (huff_bits[0])[acur];
            }
            if (cur < 0) {
                code |= 1;      /* the sign bit */
            }
            if (nbits > 15) {
                Bp_PutBits(bp, 15, (code >> (nbits - 15)));
                Bp_PutBits(bp, (nbits - 15), (code & bitmask[nbits - 15]));
            } else {
                Bp_PutBits(bp, nbits, code);
            }
        } else {
            ESCAPE_CODE_ENCODE(bp, 0, cur, acur);
        }
    }
    for (i = 0; i < in->numOfAC; i++) {
        cur = in->value[i];
        acur = ABS(cur);
        if (i > 0) {
            run = in->index[i] - in->index[i - 1] - 1;
        } else {
            run = in->index[i] - 1 + first_is_zero;
        }

        if ((run < HUFF_MAXRUN) && (acur < huff_maxlevel[run])) {
            /* encode using the Huffman tables      */
            code = (huff_table[run])[acur];
            nbits = (huff_bits[run])[acur];
            assert(nbits);
            if (cur < 0) {
                code |= 1;      /* the sign bit */
            }
            if (nbits > 15) {
                Bp_PutBits(bp, 15, (code >> (nbits - 15)));
                Bp_PutBits(bp, (nbits - 15), (code & bitmask[nbits - 15]));
            } else {
                Bp_PutBits(bp, nbits, code);
            }

        } else {
            ESCAPE_CODE_ENCODE(bp, run, cur, acur);
        }
    }                           /* for */
    Bp_PutBits(bp, 2, 0x2);     /* end of block marker */
}
