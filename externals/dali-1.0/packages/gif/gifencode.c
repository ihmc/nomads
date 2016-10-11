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
 * gifencode.c
 * Routines to encode gifs
 * Steve Weiss
 */

/* Derived from code which carries the following copyright */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1993, David Koblas (koblas@netcom.com)                  | */
/* |                                                                   | */
/* | Permission to use, copy, modify, and to distribute this software  | */
/* | and its documentation for any purpose is hereby granted without   | */
/* | fee, provided that the above copyright notice appear in all       | */
/* | copies and that both that copyright notice and this permission    | */
/* | notice appear in supporting documentation.  There is no           | */
/* | representations about the suitability of this software for        | */
/* | any purpose.  this software is provided "as is" without express   | */
/* | or implied warranty.                                              | */
/* |                                                                   | */
/* +-------------------------------------------------------------------+ */

/* ppmtogif.c - read a portable pixmap and produce a GIF file
 * 
 * Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>.A
 * Lempel-Zim compression based on "compress".
 * 
 * Copyright (C) 1989 by Jef Poskanzer.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 * 
 * The Graphics Interchange Format(c) is the Copyright property of
 * CompuServe Incorporated.  GIF(sm) is a Service Mark property of
 * CompuServe Incorporated.
 */

#include "gifInt.h"

#define MAXCOLORS 256

/*
 * Number of characters so far in this 'packet'
 */
static int a_count;

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long count_int;
typedef unsigned short count_short;

#else
typedef long count_int;

#endif

static int GIFNextPixel (ByteImage * srcBuf);
static void output (BitParser * bp, code_int code);
static void cl_block (BitParser * bp);
static void cl_hash (count_int hsize);
static void char_out (BitParser * bp, int c);
static void flush_char (BitParser * bp);

#define GetAPixel(x, y, srcBuf) (srcBuf->firstByte[y*(srcBuf->parentWidth)+x])



/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background,
 *            BitsPerPixel, Red, Green, Blue, GetPixel )
 *
 *****************************************************************************/

#define TRUE 1
#define FALSE 0


static int Width, Height;
static int curx, cury;
static long CountDown;
static int Pass;
static int Interlace;


/*
 * Return the next pixel from the image
 */
static int
GIFNextPixel (srcBuf)
    ByteImage *srcBuf;
{
    int r;

    if (CountDown == 0)
        return EOF;

    --CountDown;

    r = GetAPixel (curx, cury, srcBuf);

    /*
     * Bump the current X position
     */
    ++curx;

    /*
     * If we are at the end of a scan line, set curx back to the beginning
     * If we are interlaced, bump the cury to the appropriate spot,
     * otherwise, just increment it.
     */
    if (curx == Width) {
        curx = 0;

        if (!Interlace)
            ++cury;
        else {
            switch (Pass) {

            case 0:
                cury += 8;
                if (cury >= Height) {
                    ++Pass;
                    cury = 4;
                }
                break;

            case 1:
                cury += 8;
                if (cury >= Height) {
                    ++Pass;
                    cury = 2;
                }
                break;

            case 2:
                cury += 4;
                if (cury >= Height) {
                    ++Pass;
                    cury = 1;
                }
                break;

            case 3:
                cury += 2;
                break;
            }
        }
    }
    return r;
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define BITS    12

#define HSIZE  5003             /* 80% occupancy */

#ifdef NO_UCHAR
typedef char char_type;

#else /*NO_UCHAR */
typedef unsigned char char_type;

#endif /*NO_UCHAR */

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */
#include <ctype.h>

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

static int n_bits;              /* number of bits/code */
static int maxbits;             /* user settable max # bits/code */
static code_int maxcode;        /* maximum code, given n_bits */
static code_int maxmaxcode;     /* should NEVER generate this code */

#define MAXCODE(n_bits)                (((code_int) 1 << (n_bits)) - 1)

static count_int htab[HSIZE];
static unsigned short codetab[HSIZE];

#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static code_int hsize;          /* for dynamic table sizing */

static unsigned long cur_accum;
static int cur_bits;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) CodeTabOf(i)
#define tab_suffixof(i)        ((char_type*)(htab))[i]
#define de_stack               ((char_type*)&tab_suffixof((code_int)1<<BITS))

static code_int free_ent;       /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg;

static int offset;
static long int in_count;       /* length of input */
static long int out_count;      /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int g_init_bits;
static int ClearCode;
static int EOFCode;

void
Compress (init_bits, bp, srcBuf, imgHdr)
    int init_bits;
    BitParser *bp;
    ByteImage *srcBuf;
    GifImgHdr *imgHdr;
{
    register long fcode;
    register code_int i /* = 0 */ ;
    register int c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register int hshift;

    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    g_init_bits = init_bits;
    curx = cury = 0;

    Width = imgHdr->width;
    Height = imgHdr->height;
    Interlace = imgHdr->interlaced;

    /*
     * Calculate number of bits we are expecting
     */
    CountDown = (long) Width *(long) Height;

    /*
     * Indicate which pass we are on (if interlace)
     */
    Pass = 0;

    /*
     * Set up the necessary values
     */
    offset = 0;
    out_count = 0;
    clear_flg = 0;
    in_count = 1;
    maxbits = BITS;
    maxmaxcode = 1 << BITS;
    maxcode = MAXCODE (n_bits = g_init_bits);
    hsize = HSIZE;
    cur_accum = 0;
    cur_bits = 0;

    ClearCode = (1 << (init_bits - 1));
    EOFCode = ClearCode + 1;
    free_ent = ClearCode + 2;

    a_count = 0;

    ent = GIFNextPixel (srcBuf);

    hshift = 0;
    for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
        ++hshift;
    hshift = 8 - hshift;        /* set hash code range bound */

    hsize_reg = hsize;
    cl_hash ((count_int) hsize_reg);    /* clear hash table */

    output (bp, (code_int) ClearCode);

    while ((c = GIFNextPixel (srcBuf)) != EOF) {

        ++in_count;

        fcode = (long) (((long) c << maxbits) + ent);
        i = (((code_int) c << hshift) ^ ent);   /* xor hashing */

        if (HashTabOf (i) == fcode) {
            ent = CodeTabOf (i);
            continue;
        } else if ((long) HashTabOf (i) < 0)    /* empty slot */
            goto nomatch;
        disp = hsize_reg - i;   /* secondary hash (after G. Knott) */
        if (i == 0)
            disp = 1;
      probe:
        if ((i -= disp) < 0)
            i += hsize_reg;

        if (HashTabOf (i) == fcode) {
            ent = CodeTabOf (i);
            continue;
        }
        if ((long) HashTabOf (i) > 0)
            goto probe;
      nomatch:
        output (bp, (code_int) ent);
        ++out_count;
        ent = c;

        if (free_ent < maxmaxcode) {
            CodeTabOf (i) = free_ent++;         /* code -> hashtable */
            HashTabOf (i) = fcode;
        } else
            cl_block (bp);
    }
    /*
     * Put out the final code.
     */
    output (bp, (code_int) ent);
    ++out_count;
    output (bp, (code_int) EOFCode);
}



/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static unsigned long masks[] =
{0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
 0x001F, 0x003F, 0x007F, 0x00FF,
 0x01FF, 0x03FF, 0x07FF, 0x0FFF,
 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

static void
output (bp, code)
    BitParser *bp;
    code_int code;
{
    cur_accum &= masks[cur_bits];

    if (cur_bits > 0)
        cur_accum |= ((long) code << cur_bits);
    else
        cur_accum = code;

    cur_bits += n_bits;

    while (cur_bits >= 8) {
        char_out (bp, (unsigned int) (cur_accum & 0xff));
        cur_accum >>= 8;
        cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
    if (free_ent > maxcode || clear_flg) {

        if (clear_flg) {

            maxcode = MAXCODE (n_bits = g_init_bits);
            clear_flg = 0;

        } else {

            ++n_bits;
            if (n_bits == maxbits)
                maxcode = maxmaxcode;
            else
                maxcode = MAXCODE (n_bits);
        }
    }
    if (code == EOFCode) {
        /*
         * At EOF, write the rest of the buffer.
         */
        while (cur_bits > 0) {
            char_out (bp, (unsigned int) (cur_accum & 0xff));
            cur_accum >>= 8;
            cur_bits -= 8;
        }

        flush_char (bp);

        /*Bp_FlushByte( bp ); */
    }
}

/*
 * Clear out the hash table
 */
static void
cl_block (BitParser * bp)
{                               /* table clear for block compress */

    cl_hash ((count_int) hsize);
    free_ent = ClearCode + 2;
    clear_flg = 1;

    output (bp, (code_int) ClearCode);
}

static void
cl_hash (hsize)                 /* reset code table */
    register count_int hsize;
{

    register count_int *htab_p = htab + hsize;

    register long i;
    register long m1 = -1;

    i = hsize - 16;
    do {                        /* might use Sys V memset(3) here */
        *(htab_p - 16) = m1;
        *(htab_p - 15) = m1;
        *(htab_p - 14) = m1;
        *(htab_p - 13) = m1;
        *(htab_p - 12) = m1;
        *(htab_p - 11) = m1;
        *(htab_p - 10) = m1;
        *(htab_p - 9) = m1;
        *(htab_p - 8) = m1;
        *(htab_p - 7) = m1;
        *(htab_p - 6) = m1;
        *(htab_p - 5) = m1;
        *(htab_p - 4) = m1;
        *(htab_p - 3) = m1;
        *(htab_p - 2) = m1;
        *(htab_p - 1) = m1;
        htab_p -= 16;
    } while ((i -= 16) >= 0);

    for (i += 16; i > 0; --i)
        *--htab_p = m1;
}

/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/



/*
 * Define the storage for the packet accumulator
 */
static char accum[256];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void
char_out (bp, c)
    BitParser *bp;
    int c;
{
    accum[a_count++] = c;
    if (a_count >= 254) {
        Bp_PutByte (bp, a_count);
        Bp_PutByteArray (bp, a_count, accum);
        a_count = 0;
    }
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void
flush_char (BitParser * bp)
{
    if (a_count > 0) {
        Bp_PutByte (bp, a_count);
        Bp_PutByteArray (bp, a_count, accum);
        a_count = 0;
    }
}

/* The End */
