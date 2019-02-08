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

static unsigned long mbAddrIncrTable[][2] =
{
    {0x0, 0},
    {0x1, 1},
    {0x3, 3},
    {0x2, 3},
    {0x3, 4},
    {0x2, 4},
    {0x3, 5},
    {0x2, 5},
    {0x7, 7},
    {0x6, 7},
    {0xb, 8},
    {0xa, 8},
    {0x9, 8},
    {0x8, 8},
    {0x7, 8},
    {0x6, 8},
    {0x17, 10},
    {0x16, 10},
    {0x15, 10},
    {0x14, 10},
    {0x13, 10},
    {0x12, 10},
    {0x23, 11},
    {0x22, 11},
    {0x21, 11},
    {0x20, 11},
    {0x1f, 11},
    {0x1e, 11},
    {0x1d, 11},
    {0x1c, 11},
    {0x1b, 11},
    {0x1a, 11},
    {0x19, 11},
    {0x18, 11}};

static unsigned long mbMotionVectorTable[][2] =
{
    {0x19, 11},
    {0x1b, 11},
    {0x1d, 11},
    {0x1f, 11},
    {0x21, 11},
    {0x23, 11},
    {0x13, 10},
    {0x15, 10},
    {0x17, 10},
    {0x7, 8},
    {0x9, 8},
    {0xb, 8},
    {0x7, 7},
    {0x3, 5},
    {0x3, 4},
    {0x3, 3},
    {0x1, 1},
    {0x2, 3},
    {0x2, 4},
    {0x2, 5},
    {0x6, 7},
    {0xa, 8},
    {0x8, 8},
    {0x6, 8},
    {0x16, 10},
    {0x14, 10},
    {0x12, 10},
    {0x22, 11},
    {0x20, 11},
    {0x1e, 11},
    {0x1c, 11},
    {0x1a, 11},
    {0x18, 11}};

static unsigned long mbPatTable[][2] =
{
    {0x0, 0},
    {0xb, 5},
    {0x9, 5},
    {0xd, 6},
    {0xd, 4},
    {0x17, 7},
    {0x13, 7},
    {0x1f, 8},
    {0xc, 4},
    {0x16, 7},
    {0x12, 7},
    {0x1e, 8},
    {0x13, 5},
    {0x1b, 8},
    {0x17, 8},
    {0x13, 8},
    {0xb, 4},
    {0x15, 7},
    {0x11, 7},
    {0x1d, 8},
    {0x11, 5},
    {0x19, 8},
    {0x15, 8},
    {0x11, 8},
    {0xf, 6},
    {0xf, 8},
    {0xd, 8},
    {0x3, 9},
    {0xf, 5},
    {0xb, 8},
    {0x7, 8},
    {0x7, 9},
    {0xa, 4},
    {0x14, 7},
    {0x10, 7},
    {0x1c, 8},
    {0xe, 6},
    {0xe, 8},
    {0xc, 8},
    {0x2, 9},
    {0x10, 5},
    {0x18, 8},
    {0x14, 8},
    {0x10, 8},
    {0xe, 5},
    {0xa, 8},
    {0x6, 8},
    {0x6, 9},
    {0x12, 5},
    {0x1a, 8},
    {0x16, 8},
    {0x12, 8},
    {0xd, 5},
    {0x9, 8},
    {0x5, 8},
    {0x5, 9},
    {0xc, 5},
    {0x8, 8},
    {0x4, 8},
    {0x4, 9},
    {0x7, 3},
    {0xa, 5},
    {0x8, 5},
    {0xc, 6}};

/*
 *Use tables for following:
 *int modulus = 1 << (4 + fCode);
 *int vec_range_neg = -(modulus >> 1);
 *int vec_range_pos = (modulus >> 1) - 1;
 */
static int modulusTable[] =
{0, 32, 64, 128, 256, 512, 1024, 2048};
static int vecNegTable[] =
{0, -16, -32, -64, -128, -256, -512, -1024};
static int vecPosTable[] =
{0, 15, 31, 63, 127, 255, 511, 1023};


static void
GenMotionCode(bp, vector)
    BitParser *bp;
    int vector;
{
    int code, num;

    if ((vector < -16) || (vector > 16)) {
        perror("Motion vector out of range.");
        exit(-1);
    }
    code = mbMotionVectorTable[vector + 16][0];
    num = mbMotionVectorTable[vector + 16][1];

    Bp_PutBits(bp, num, code);
}

/* VL-Coding for Macroblock Types */
static void
GenMBType(bp, frame_type, mb_quant, mb_intra, mb_pattern, motion_forw, motion_back)
    BitParser *bp;
    char frame_type;
    char mb_quant;              /* q_scale */
    char mb_intra, mb_pattern;
    char motion_forw, motion_back;
{
    int code;

    switch (frame_type) {
    case I_FRAME:
        if ((motion_forw != 0) || (motion_back != 0) || (mb_pattern != 0) || (mb_intra != 1)) {
            perror("Illegal parameters for macroblock type.");
            //exit(-1);
        }
        if (mb_quant) {
            Bp_PutBits(bp, 2, 0x1);
        } else {
            Bp_PutBits(bp, 1, 0x1);
        }
        break;

    case P_FRAME:
        code = 0;
        if (mb_quant) {
            code += 16;
        }
        if (motion_forw) {
            code += 8;
        }
        if (motion_back) {
            code += 4;
        }
        if (mb_pattern) {
            code += 2;
        }
        if (mb_intra) {
            code += 1;
        }
        switch (code) {
        case 1:
            Bp_PutBits(bp, 5, 0x3);
            break;
        case 2:
            Bp_PutBits(bp, 2, 0x1);
            break;
        case 8:
            Bp_PutBits(bp, 3, 0x1);
            break;
        case 10:
            Bp_PutBits(bp, 1, 0x1);
            break;
        case 17:
            Bp_PutBits(bp, 6, 0x1);
            break;
        case 18:
            Bp_PutBits(bp, 5, 0x1);
            break;
        case 26:
            Bp_PutBits(bp, 5, 0x2);
            break;
        default:
            perror("Illegal parameters for macroblock type.");
            //exit(-1);
            break;
        }
        break;

    case B_FRAME:
        code = 0;
        if (mb_quant) {
            code += 16;
        }
        if (motion_forw) {
            code += 8;
        }
        if (motion_back) {
            code += 4;
        }
        if (mb_pattern) {
            code += 2;
        }
        if (mb_intra) {
            code += 1;
        }
        switch (code) {
        case 12:
            Bp_PutBits(bp, 2, 0x2);
            break;
        case 14:
            Bp_PutBits(bp, 2, 0x3);
            break;
        case 4:
            Bp_PutBits(bp, 3, 0x2);
            break;
        case 6:
            Bp_PutBits(bp, 3, 0x3);
            break;
        case 8:
            Bp_PutBits(bp, 4, 0x2);
            break;
        case 10:
            Bp_PutBits(bp, 4, 0x3);
            break;
        case 1:
            Bp_PutBits(bp, 5, 0x3);
            break;
        case 30:
            Bp_PutBits(bp, 5, 0x2);
            break;
        case 26:
            Bp_PutBits(bp, 6, 0x3);
            break;
        case 22:
            Bp_PutBits(bp, 6, 0x2);
            break;
        case 17:
            Bp_PutBits(bp, 6, 0x1);
            break;
        default:
            code = 1;
            perror("Illegal parameters for macroblock type.");
            //exit(-1);
            break;
        }
        break;
    }
}

/* return ceiling(a/b) where a, b are ints, using temp value c */
#define INT_CEIL_DIV(a,b,c)  ((b*(c = a/b) < a) ? (c+1) : c)
#define INT_FLOOR_DIV(a,b,c) ((b*(c = a/b) > a) ? (c-1) : c)
#define ABS(x) ((x > 0) ? x : (-x))

#define DIVIDE_VECTOR(x, q, r, f) {\
    int tempQ, tempX;\
    \
    if ( x < vec_range_neg ) tempX = x + modulus;\
    else if ( x > vec_range_pos ) tempX = x - modulus;\
    else tempX = x;\
    if ( tempX >= 0 ) {\
        q = INT_CEIL_DIV(tempX, f, tempQ);\
        r = f - 1 + tempX - q*f;\
    } else {\
        q = INT_FLOOR_DIV(tempX, f, tempQ);\
        r = f - 1 - tempX + q*f;\
    }\
}\

void
MpegIMacroBlockHdrEncode(bp, pic_hdr, addr_incr, q_scale)
    BitParser *bp;
    MpegPicHdr *pic_hdr;
    int addr_incr;
    int q_scale;
{
    MpegMacroBlockHdrEncode(bp, pic_hdr, addr_incr, 0 /* qscale */ , 1, 0, 0, 0, 0, 0, 0, 0);
}


void
MpegMacroBlockHdrEncode(bp, pic_hdr, addr_incr, q_scale, mb_intra, mb_pattern,
    forw_f, back_f, offset_f_x, offset_f_y, offset_b_x, offset_b_y)
    BitParser *bp;
    MpegPicHdr *pic_hdr;
    int addr_incr;
    int q_scale;
    int mb_intra;
    int mb_pattern;
    char forw_f;
    char back_f;
    int offset_f_x;
    int offset_f_y;
    int offset_b_x;
    int offset_b_y;
{
    char frame_type = pic_hdr->type;
    char fCode = pic_hdr->forward_r_size + 1;
    int modulus = modulusTable[(int)fCode];
    int vec_range_neg = vecNegTable[(int)fCode];
    int vec_range_pos = vecPosTable[(int)fCode];
    int quotient, remainder;
    int code;
    int num;

    /* insert stuffings ???????? who put this here? */
    /* for (i = 0; i < hdr->numStuffing; i++)
       Bp_PutBits(bp, STUFFING_CODE, STUFFING_SIZE);
     */

    /* MB escape sequences if necessary. */
    while (addr_incr > 33) {
        Bp_PutBits(bp, 11, 0x008);
        addr_incr -= 33;
    }

    /* Generate address increment code. */
    code = mbAddrIncrTable[addr_incr][0];
    num = mbAddrIncrTable[addr_incr][1];
    Bp_PutBits(bp, num, code);

    /* Generate mb type code. */
    GenMBType(bp, frame_type, q_scale, mb_intra, mb_pattern, forw_f, back_f);

    /* MB quant. */
    if (q_scale) {
        Bp_PutBits(bp, 5, q_scale);
    }
    /* Forward predictive vector stuff. */
    if (forw_f) {
        char forw_r_size = pic_hdr->forward_r_size;

        DIVIDE_VECTOR(offset_f_x, quotient, remainder, forw_f);
        GenMotionCode(bp, quotient);

        if ((forw_f != 1) && (quotient != 0)) {
            Bp_PutBits(bp, forw_r_size, remainder);
        }
        DIVIDE_VECTOR(offset_f_y, quotient, remainder, forw_f);
        GenMotionCode(bp, quotient);
        if ((forw_f != 1) && (quotient != 0)) {
            Bp_PutBits(bp, forw_r_size, remainder);
        }
    }
    /* Back predicted vector stuff. */
    if (back_f) {
        char back_r_size = pic_hdr->backward_r_size;

        DIVIDE_VECTOR(offset_b_x, quotient, remainder, back_f);
        GenMotionCode(bp, quotient);
        if ((back_f != 1) && (quotient != 0)) {
            Bp_PutBits(bp, back_r_size, remainder);
        }
        DIVIDE_VECTOR(offset_b_y, quotient, remainder, back_f);
        GenMotionCode(bp, quotient);
        if ((back_f != 1) && (quotient != 0)) {
            Bp_PutBits(bp, back_r_size, remainder);
        }
    }
    /* MB pattern. */
    if (mb_pattern) {
        code = mbPatTable[mb_pattern][0];
        num = mbPatTable[mb_pattern][1];
        Bp_PutBits(bp, num, code);
    }
}
