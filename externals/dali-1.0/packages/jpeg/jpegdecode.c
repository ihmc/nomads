/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*---------------------------------------------------------------------------
 * jpegdecode.c
 *
 * This file contains routines for parsing a JPEG image.
 *
 * Parts of this file are based on code under the following
 * copyrights.  Include these copyrights if you do anything
 * with this code
 *
 * Copyright (C) 1997-1998 Cornell University
 * Sugata Mukhopadhyay sugata@cs.cornell.edu
 *
 *--------------------------------------------------------------------------
 */

/*
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 */

/*
 *
 * Andrew Swan (aswan@cs.berkeley.edu)
 * Department of Computer Science,
 * University of California, Berkeley
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
 */

#include "jpegInt.h"

static int extendedvals[2048] =
{
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -1, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -3, -2, 2, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -7, -6, -5, -4, 4, 5, 6, 7,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -15, -14, -13, -12, -11, -10, -9, -8,
    8, 9, 10, 11, 12, 13, 14, 15,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -31, -30, -29, -28, -27, -26, -25, -24,
    -23, -22, -21, -20, -19, -18, -17, -16,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -63, -62, -61, -60, -59, -58, -57, -56,
    -55, -54, -53, -52, -51, -50, -49, -48,
    -47, -46, -45, -44, -43, -42, -41, -40,
    -39, -38, -37, -36, -35, -34, -33, -32,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    -127, -126, -125, -124, -123, -122, -121, -120,
    -119, -118, -117, -116, -115, -114, -113, -112,
    -111, -110, -109, -108, -107, -106, -105, -104,
    -103, -102, -101, -100, -99, -98, -97, -96,
    -95, -94, -93, -92, -91, -90, -89, -88,
    -87, -86, -85, -84, -83, -82, -81, -80,
    -79, -78, -77, -76, -75, -74, -73, -72,
    -71, -70, -69, -68, -67, -66, -65, -64,
    64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87,
    88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};
#if 1
#define JPEG_EXTEND(v, t, res)                                      \
{                                                                   \
  if (t < EXT_CONST) {                                              \
      res = extendedvals[(t << EXT_CONST) | v];                     \
  } else {                                                          \
      res = ((v) < (1 << (t-1) ) ? (v) + (((-1) << t) + 1) : (v));  \
  }                                                                 \
}
#else
#define JPEG_EXTEND(v, t, res) \
  res = ((v) < (1 << (t-1) ) ? (v) + (((-1) << t) + 1) : (v))
#endif

/*
 *--------------------------------------------------------------
 *
 * CreateHufftab
 *
 *    Convert a list of bits and values for a Huffman
 *    table in to a table that can be used in decoding.
 *
 * Results:
 *    The table itself is returned.  It should be freed
 *    with ckfree()
 *
 * Side effects:
 *    Memory is allocated for the table
 *
 *--------------------------------------------------------------
 */
static unsigned short *
CreateHufftab(bits, vals)
    unsigned char *bits;
    unsigned char *vals;
{
    int symbols, sizes[256], codes[256], code, i, j;
    unsigned short *table;

    /*
     * Allocate some memory
     */
    table = (unsigned short *) MALLOC(65536 * sizeof(short));
    if (table == NULL) {
        return NULL;
    }
    
    memset(table, 0, 65536 * sizeof(short) );

    /* 
     * Count the total number of symbols
     * and generate the codes
     */
    symbols = 0;
    code = 0;
    for (i=0; i<16; i++) {
        for (j=0; j<(int)(bits[i]); j++) {
            codes[symbols] = code++;
            sizes[symbols] = i + 1;
            symbols++;
        }
        code <<= 1;
    }
    
    /*
     * Generate the values in the final table
     */
    for (i=0; i<symbols; i++) {
        int val, index, bits;
        
        val = (sizes[i] << 8) | vals[i];
        bits = 16 - sizes[i];
        index = codes[i] << bits;
        
        for (j=0; j< 1<<bits; j++) {
            table[index | j] = val ;
        }
    }

    return table;
}

/*
 * ZigZag tables.
 */
static char zz[] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18,
    11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49,
    56, 57, 50, 43, 36, 29, 22, 15, 23, 30,
    37, 44, 51, 58, 59, 52, 45, 38, 31, 39,
    46, 53, 60, 61, 54, 47, 55, 62, 63 
};

/*
 * Standard bitmask table for bitparser
 * Conflict with bitparser.obj ?
int bitmask[] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
};
 */


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     ParseQuantizationTable --
 *
 *     parse the DQT structure from the bitstream.
 *
 * return 
 *     none
 * 
 * side effect :
 *     File pointer is advanced to the next marker.
 *
 *----------------------------------------------------------------------
 */

static void
ParseQuantizationTable (bp, hdr)
    BitParser *bp;
    JpegHdr *hdr;
{
    int i, p, id, length;
    JpegQt *qt;
    int value;

    /*
     * Get the length field
     */
    Bp_GetShort(bp, length);

    /*
     * There may be more than one table defined in this
     * marker.  Parse all of them
     */
    length -= 2;
    while (length) {
        /*
         * Get the precision and id
         */
        Bp_GetByte(bp, value);
        p = (value & 0xF0) >> 4;
        id = (value & 0x0F);
        qt = &hdr->qt[id];
        if (p) {
            qt->precision = 16;
            length -= 129;
            for (i = 0; i < 64; i++) {
                Bp_GetShort(bp, value);
                qt->v[(int)zz[i]] = value;
            }
        } else {
            qt->precision = 8;
            length -= 65;
            for (i = 0; i < 64; i++) {
                Bp_GetByte(bp, value);
                qt->v[(int)zz[i]] = value;
            }
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     ParseHuffmanTable --
 *
 *     parse the DHT structure from the bitstream.
 *
 * return 
 *     none
 * 
 * side effect :
 *     File pointer is advanced to the next marker.
 *
 *----------------------------------------------------------------------
 */

static void
ParseHuffmanTable (bp, hdr)
    BitParser *bp;
    JpegHdr *hdr;
{
    int i, class, id, length, numVals;
    unsigned char *bits, *vals;
    short value;

    /*
     * Get the length field
     */
    Bp_GetShort(bp, length);

    /*
     * There may be more than one table defined in this
     * marker.  Parse all of them
     */
    length -= 2;

    while (length) {
        /*
         * Get the class (DC or AC table) and id (0-3)
         */
        Bp_GetByte(bp, value);
        class = (value & 0xF0) >> 4;
        id = (value & 0x0F);

        /*
         * Get pointer to bits table
         */
        if (class == 0) {
            bits = hdr->ht[id].dcBits;
        } else {
            bits = hdr->ht[id].acBits;
        }

        /*
         * Parse the bits table.  This is a table L[i] containing the
         * number of Huffman codes of length i.  The sum of L[i]
         * is the number of value codes following.
         */
        numVals = 0;
        for (i = 0; i < 16; i++) {
            Bp_GetByte(bp, bits[i]);
            numVals += bits[i];
        }

        /*
         * Allocate and get a pointer to vals table
         */
        if (class == 0) {
            vals = hdr->ht[id].dcVals;
        } else {
            vals = hdr->ht[id].acVals;
        }

        /*
         * Parse the vals table
         */
        for (i = 0; i < numVals; i++) {
            Bp_GetByte(bp, vals[i]);
        }
        length -= 17 + numVals;
    }
}


int
JpegHdrParse (bp, hdr)
    BitParser *bp;
    JpegHdr *hdr;
{
    int marker, value, i, w, h;
    int foundSOF;
    int startPos;

     foundSOF = 0;
     startPos = BitParserTell(bp);

    /*
     * Parse the tables, up to SOF0 marker.  Tables (quantization,
     * Huffman) are stored in the hdr structure.
     * If it's not baseline sequential DCT, return an error
     */
    Bp_PeekByte(bp, marker);
    while (marker == 0xff) {
        Bp_FlushByte(bp);
        Bp_GetByte(bp, marker);

        switch (marker) {
        case SOI:
            break;

        case APP0:
        case APP15:
        case COM:
            /*
             * Ignore these codes
             */
            Bp_PeekShort(bp, marker);
            Bp_FlushBytes(bp, marker);
            break;

        case DRI:
            Bp_FlushByte(bp);
            Bp_GetShort(bp, value);
            hdr->restartInterval = value;
            break;

        case DQT: 
            ParseQuantizationTable (bp, hdr);
            break;

        case DHT:
            ParseHuffmanTable (bp, hdr);
            break;

        case SOF0:
            /*
             * Flush length field
             */
            foundSOF = 1;
            Bp_FlushShort(bp);
            
            /*
             * Get sample precision 
             */
            Bp_GetByte(bp, value);
            hdr->precision = value;
            
            /*
             * Get w and h 
             */
            Bp_GetShort(bp, h);
            Bp_GetShort(bp, w);
            hdr->width = w;
            hdr->height = h;
            
            /*
             * Get number of components 
             */
            Bp_GetByte(bp, value);
            hdr->numComps = value;

            /*
             * Get decimation and quantization table id for each component
             */
            for (i=0; i<hdr->numComps; i++) {
                /*
                 * Get component ID number 
                 */
                Bp_GetByte(bp, value);
                hdr->compId[i] = value;
                
                /*
                 * Get decimation
                 */
                Bp_GetByte(bp, value);
                hdr->blockWidth[i] = (value & 0xF0) >> 4;
                if (hdr->blockWidth[i] > hdr->maxBlockWidth) {
                    hdr->maxBlockWidth = hdr->blockWidth[i];
                }
                hdr->blockHeight[i] = value & 0x0F;
                if (hdr->blockHeight[i] > hdr->maxBlockHeight) {
                    hdr->maxBlockHeight = hdr->blockHeight[i];
                }

                /*
                 * Get quantization table id
                 */
                Bp_GetByte(bp, value);
                hdr->qtId[i] = value;
            }
            return (BitParserTell(bp) - startPos);

        case SOF1:
        case SOF2:
        case SOF3:
        case SOF5:
        case SOF6:
        case SOF7:
        case SOF9:
        case SOF10:
        case SOF11:
        case SOF13:
        case SOF14:
        case SOF15:
            return DVM_JPEG_INVALID_MARKER;
        case DAC:
            return DVM_JPEG_AC_UNSUPPORTED;
        
        default:
            /* 
             * Not SOF or SOI.  Must not be a JPEG file (or file pointer
             * is placed wrong).  In either case, it's an error.
             */
            return DVM_JPEG_INVALID_MARKER;
        }
        Bp_PeekByte(bp, marker);
    }
    
    if (!foundSOF) {
        return DVM_JPEG_INVALID_MARKER;
    }
    return (BitParserTell(bp) - startPos);
}

int
JpegScanHdrParse (bp, hdr, scanHdr)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
{
    int marker, value, i, j;
    int startPos;

    startPos = BitParserTell(bp);

    Bp_PeekByte(bp, marker);
    while (marker == 0xff) {
        Bp_FlushByte(bp);
        Bp_GetByte(bp, marker);

        switch (marker) {

        case APP0:
        case APP15:
        case COM:
            /*
             * Ignore these codes
             */
            Bp_PeekShort(bp, marker);
            Bp_FlushBytes(bp, marker);
            break;

        case DRI:
            Bp_FlushByte(bp);
            Bp_GetShort(bp, value);
            hdr->restartInterval = value;
            break;

        case DQT: 
            ParseQuantizationTable (bp, hdr);
            break;

        case SOS:
            /*
             * Flush length field
             */
            Bp_FlushShort(bp);
            
            /*
             * Get number of scans
             */
            Bp_GetByte(bp, value);
            scanHdr->numComps = value;
            
            /*
             * Get per scan specs
             */
            for (i=0; i<scanHdr->numComps; i++) {
                /*
                 * Get component ID number and make it the ordinal  
                 */
                Bp_GetByte(bp, value);
                for (j=0; j<hdr->numComps; j++) {
                    if (hdr->compId[j] == value) {
                        scanHdr->scanid[i] = j;
                        break;
                    }
                }
                
                /*
                 * Get dc and ac table ids
                 */
                Bp_GetByte(bp, value);
                scanHdr->dcid[i] = (value & 0xF0) >> 4;
                scanHdr->acid[i] = (value & 0x0F);
            }

            /*
             * Flush the spectral selection and successive approximation info
             */
            Bp_FlushBytes (bp, 3);

            return (BitParserTell(bp) - startPos);

        case DHT:
            ParseHuffmanTable (bp, hdr);
            break;

        case DAC:
            return DVM_JPEG_AC_UNSUPPORTED;
        
        default:
            /* 
             * Not SOS or marker from section B.2.4.  Must not be a
             * JPEG file (or file pointer is placed wrong).  In either
             * case, it's an error.
             */
            return DVM_JPEG_INVALID_MARKER;
        }
        Bp_PeekByte(bp, marker);
    }
    
    /* SOS Must not have been found */
    return DVM_JPEG_INVALID_MARKER;
}


int
JpegScanParse (bp, hdr, scanHdr, scans, num)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    ScImage *scans[];
    int num;
{
    int w, h, wFull, hFull;  /* width, height */
    int id,  numPieces;
    CompInfo compInfo[4], *ci;
    int mcu, maxmcu;
    MCU mcuInfo[10];
    int x, y, i, j, k, maxbw, maxbh, mcuWidth, mcuHeight;

    ScBlock *block;
    unsigned short *dcht;
    unsigned short *acht;

    int piece, code, value;
    unsigned char difflen;
    short diff;
    unsigned char mixed, run, coefflen, coeffVal;
    int mcux, mcuy;
    int numComps, numAC;
    int errcode, startPos;
    unsigned short *qval;


    startPos = BitParserTell(bp);
    ci = NULL; /* make compiler happy */
    numComps = scanHdr->numComps;

    /* Check if we have the right number of sc bufs */
    if (numComps != num) {
        return DVM_JPEG_WRONG_COMPONENTS;
    }

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in compInfo[]
     */
    numPieces = 0;
    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        id = scanHdr->dcid[i];
        ci->dcht = CreateHufftab(hdr->ht[id].dcBits, hdr->ht[id].dcVals);
        id = scanHdr->acid[i];
        ci->acht = CreateHufftab(hdr->ht[id].acBits, hdr->ht[id].acVals);
        ci->predictor = 0;
        id = scanHdr->scanid[i];
        ci->blockPtr = scans[i]->firstBlock;
        ci->bw = hdr->blockWidth[id];
        ci->bh = hdr->blockHeight[id];
        /* For dequantized storage */
        id = hdr->qtId[id];
        ci->qvals = (unsigned short *)(hdr->qt[id]).v;
        /* End for dequantized storage */
        for (j=0; j<ci->bw * ci->bh; j++) {
            mcuInfo[numPieces].x = j % ci->bw;
            mcuInfo[numPieces].y = j / ci->bw;
            mcuInfo[numPieces++].compNum = i;
        }
    }
    if (numComps == 1) {
        ci->bw = ci->bh = 1;
        maxbw = maxbh = 1;
        numPieces = 1;
    } else {
        maxbw = hdr->maxBlockWidth;
        maxbh = hdr->maxBlockHeight;
    }

    /*
     * Compute the parameters for each MCU:
     *  (w,h) give the dimension of the image in pixels.
     *  (mcuWidth,mcuHeight) give the dimensions of an MCU (in pixels)
     *  (wFull,hFull) give the dimensions of the image in MCUs
     *  maxmcu is the number of MCUs in this scan
     */
    mcuWidth = maxbw * 8;
    mcuHeight = maxbh * 8;
    w = hdr->width;
    h = hdr->height;

    wFull = (int)ceil((double)w/mcuWidth);
    hFull = (int)ceil((double)h/mcuHeight);
    maxmcu = wFull*hFull;  

    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        ci->w = wFull*ci->bw;
        ci->h = hFull*ci->bh;
        if (scans[i]->width != ci->w) {
            errcode = DVM_JPEG_INVALID_WIDTH;
            goto err;
        }
        if (scans[i]->height != ci->h) {
            errcode = DVM_JPEG_INVALID_HEIGHT;
            goto err;
        }
        ci->numBlocks = ci->w * ci->h;
        ci->scanpad = scans[i]->parentWidth;
    }

    /*
     * Main decode loop
     */
    for (mcu = 0; mcu < maxmcu; mcu++) {
        /*
         * Compute x,y offset of first block in MCU within its
         * component's block array
         */
        mcux = (mcu % wFull);
        mcuy = (mcu / wFull);

        /*
         * Decode all the MCUs
         */
        for (piece = 0; piece < numPieces; piece++) {
            /*
             * Calculate everything specific to which plane we
             * are decoding to avoid conditionals in the main loop
             */

            id = mcuInfo[piece].compNum;
            ci = &compInfo[id];
            dcht = ci->dcht;
            acht = ci->acht;
            qval = ci->qvals;
            x = mcux*ci->bw + mcuInfo[piece].x;
            y = mcuy*ci->bh + mcuInfo[piece].y;
            block = ci->blockPtr + x + y*ci->scanpad;

            /*
             * Get the DC value
             */
            Bp_PeekBits(bp, 16, code);
            value = dcht[code];
            Bp_FlushBits(bp, value >> 8);
            difflen = value & 0xff;
            if (difflen) {
                Bp_GetBits(bp, difflen, diff);
                JPEG_EXTEND(diff, difflen, diff);
                ci->predictor += diff;
            }
            block->dc = ci->predictor*qval[0] + NORMAL_OFFSET;

            /*
             * Get the AC values
             */
            numAC = 0;
            for (k=1; k<64; ) {
                Bp_PeekBits(bp, 16, code);
                value = acht[code];
                Bp_FlushBits(bp, value >> 8);
                mixed = value & 0xff;
                run = mixed >> 4;
                coefflen = mixed & 0xf;
                if (coefflen == 0) {
                    if (run == 15) {
                        k += 16;
                    } else {
                        break;
                    }
                } else {
                    k += run;
                    Bp_GetBits(bp, coefflen, coeffVal);
                    block->index[numAC] = k;
                    JPEG_EXTEND(coeffVal, coefflen, block->value[numAC]);
                    block->value[numAC] *= qval[zz[k]];
                    numAC++;
                    k++;
                }
            }
            block->numOfAC = numAC;
        }
    }

    Bp_ByteAlign(bp);
    errcode = BitParserTell(bp) - startPos;
    /*
     * Free up temporary variables
     */
 err:
    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        FREE(ci->dcht);
        FREE(ci->acht);
    }

    return errcode;
}

int
JpegScanSelectiveParse (bp, hdr, scanHdr, scans, num)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    ScImage *scans[];
    int num;
{
    int w, h, wFull, hFull;  /* width, height */
    int id,  numPieces;
    CompInfo compInfo[4], *ci;
    int mcu, maxmcu;
    MCU mcuInfo[10];
    int i, j;
    int maxbw, maxbh, mcuWidth, mcuHeight;

    ScBlock *block;
    unsigned short *dcht;
    unsigned short *acht;

    int piece;
    int code, value;
    unsigned char difflen;
    short diff;
    int k, startPos;
    unsigned char mixed, run, coefflen, coeffVal;
    int mcux, mcuy, x, y;
    int numComps, numAC;
    int errcode=0;
    unsigned short *qval;


    startPos = BitParserTell(bp);
    ci = NULL; /* make compiler happy */
    numComps = scanHdr->numComps;

    /* Check if we have the right number of sc bufs */
    if (numComps != num) {
        return DVM_JPEG_WRONG_COMPONENTS;
    }

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in compInfo[]
     */
    numPieces = 0;
    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        if (scans[i] == NULL) {
            ci->ignored = 1;
        } else {
            ci->ignored = 0;
            ci->predictor = 0;
            ci->blockPtr = scans[i]->firstBlock;
        }
        id = scanHdr->dcid[i];
        ci->dcht = CreateHufftab(hdr->ht[id].dcBits, hdr->ht[id].dcVals);
        id = scanHdr->acid[i];
        ci->acht = CreateHufftab(hdr->ht[id].acBits, hdr->ht[id].acVals);
        id = scanHdr->scanid[i];
        ci->bw = hdr->blockWidth[id];
        ci->bh = hdr->blockHeight[id];
        /* For dequantized storage */
        id = hdr->qtId[id];
        ci->qvals = (unsigned short *)(hdr->qt[id]).v;
        /* End for dequantized storage */
        for (j=0; j<ci->bw * ci->bh; j++) {
            mcuInfo[numPieces].x = j % ci->bw;
            mcuInfo[numPieces].y = j / ci->bw;
            mcuInfo[numPieces++].compNum = i;
        }
    }
    if (numComps == 1) {
        ci->bw = ci->bh = 1;
        maxbw = maxbh = 1;
        numPieces = 1;
    } else {
        maxbw = hdr->maxBlockWidth;
        maxbh = hdr->maxBlockHeight;
    }

    /*
     * Compute the parameters for each MCU:
     *  (w,h) give the dimension of the image in pixels.
     *  (mcuWidth,mcuHeight) give the dimensions of an MCU (in pixels)
     *  (wFull,hFull) give the dimensions of the image in MCUs
     *  maxmcu is the number of MCUs in this scan
     */
    mcuWidth = maxbw * 8;
    mcuHeight = maxbh * 8;
    w = hdr->width;
    h = hdr->height;

    wFull = (int)ceil((double)w/mcuWidth);
    hFull = (int)ceil((double)h/mcuHeight);
    maxmcu = wFull*hFull;  

    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        ci->w = wFull*ci->bw;
        ci->h = hFull*ci->bh;
        ci->numBlocks = ci->w * ci->h;
        if (compInfo[i].ignored == 0) {
            if (scans[i]->width != ci->w) {
                errcode = DVM_JPEG_INVALID_WIDTH;
                goto err;
            }
            if (scans[i]->height != ci->h) {
                errcode = DVM_JPEG_INVALID_HEIGHT;
                goto err;
            }
            ci->scanpad = scans[i]->parentWidth;
        }
    }

    /*
     * Main decode loop
     */
    for (mcu = 0; mcu < maxmcu; mcu++) {
        /*
         * Compute x,y offset of first block in MCU within its
         * component's block array
         */
        mcux = (mcu % wFull);
        mcuy = (mcu / wFull);

        /*
         * Decode all the MCUs
         */
        for (piece = 0; piece < numPieces; piece++) {
            id = mcuInfo[piece].compNum;
            ci = &compInfo[id];
            if (!ci->ignored) {
                /*
                 * Calculate everything specific to which plane we
                 * are decoding to avoid conditionals in the main loop
                 */

                dcht = ci->dcht;
                acht = ci->acht;
                qval = ci->qvals;
                x = mcux*ci->bw + mcuInfo[piece].x;
                y = mcuy*ci->bh + mcuInfo[piece].y;
                block = ci->blockPtr + x + y*ci->scanpad;
                
                /*
                 * Get the DC value
                 */
                Bp_PeekBits(bp, 16, code);
                value = dcht[code];
                Bp_FlushBits(bp, value >> 8);
                difflen = value & 0xff;
                if (difflen) {
                    Bp_GetBits(bp, difflen, diff);
                    JPEG_EXTEND(diff, difflen, diff);
                    ci->predictor += diff;
                }
                block->dc = ci->predictor*qval[0] + NORMAL_OFFSET;
                
                /*
                 * Get the AC values
                 */
                numAC = 0;
                for (k=1; k<64; ) {
                    Bp_PeekBits(bp, 16, code);
                    value = acht[code];
                    Bp_FlushBits(bp, value >> 8);
                    mixed = value & 0xff;
                    run = mixed >> 4;
                    coefflen = mixed & 0xf;
                    if (coefflen == 0) {
                        if (run == 15) {
                            k += 16;
                        } else {
                            break;
                        }
                    } else {
                        k += run;
                        Bp_GetBits(bp, coefflen, coeffVal);
                        block->index[numAC] = k;
                        JPEG_EXTEND(coeffVal, coefflen, block->value[numAC]);
                        block->value[numAC] *= qval[zz[k]];
                        numAC++;
                        k++;
                    }
                }
                block->numOfAC = numAC;
            } else {
                /*
                 * Were gonna ignore this plane
                 */

                dcht = ci->dcht;
                acht = ci->acht;
                qval = ci->qvals;
                x = mcux*ci->bw + mcuInfo[piece].x;
                y = mcuy*ci->bh + mcuInfo[piece].y;
                
                /*
                 * Get the DC value
                 */
                Bp_PeekBits(bp, 16, code);
                value = dcht[code];
                Bp_FlushBits(bp, value >> 8);
                difflen = value & 0xff;
                if (difflen) {
                    Bp_FlushBits(bp, difflen);
                }
                
                /*
                 * Get the AC values
                 */
                numAC = 0;
                for (k=1; k<64; ) {
                    Bp_PeekBits(bp, 16, code);
                    value = acht[code];
                    Bp_FlushBits(bp, value >> 8);
                    mixed = value & 0xff;
                    run = mixed >> 4;
                    coefflen = mixed & 0xf;
                    if (coefflen == 0) {
                        if (run == 15) {
                            k += 16;
                        } else {
                            break;
                        }
                    } else {
                        k += run;
                        Bp_FlushBits(bp, coefflen);
                        k++;
                    }
                }
            }
        }
    }

    errcode = BitParserTell(bp) - startPos;
        
    /*
     * Free up temporary variables
     */
 err:
    for (i=0; i<numComps; i++) {
        ci = &compInfo[i];
        FREE(ci->dcht);
        FREE(ci->acht);
    }

    return errcode;
    }

/*
 *----------------------------------------------------------------------
 *
 * JPEG Scan indcremental parse related
 *
 *----------------------------------------------------------------------
 */

static MCU incMcuInfo[10];
static CompInfo incCompInfo[4];

struct GlobInfo {
    int numComps;            /* The number of components */
    int numPieces;           /* Blocks per MCU */
    int curMcu;              /* Where we are */
    int maxMcu;
    int wFull;               /* Width and height */
    int hFull;
} globInfo;

int
JpegScanIncParseStart (bp, hdr, scanHdr, scans, num)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    ScImage *scans[];
    int num;
{
    int w, h;  /* width, height */
    int id,  numPieces;
    CompInfo *ci;
    int i, j;
    int maxbw, maxbh, mcuWidth, mcuHeight;
    int errcode = 0;
    int numComps;



    ci = NULL; /* make compiler happy */
    numComps = scanHdr->numComps;

    /* Check if we have the right number of sc bufs */
    if (numComps != num) {
        return DVM_JPEG_WRONG_COMPONENTS;
    }

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in compInfo[]
     */
    numPieces = 0;
    for (i=0; i<numComps; i++) {
        ci = &incCompInfo[i];
        id = scanHdr->dcid[i];
        ci->dcht = CreateHufftab(hdr->ht[id].dcBits, hdr->ht[id].dcVals);
        id = scanHdr->acid[i];
        ci->acht = CreateHufftab(hdr->ht[id].acBits, hdr->ht[id].acVals);
        ci->predictor = 0;
        id = scanHdr->scanid[i];
        ci->blockPtr = scans[i]->firstBlock;
        ci->bw = hdr->blockWidth[id];
        ci->bh = hdr->blockHeight[id];
        /* For dequantized storage */
        id = hdr->qtId[id];
        ci->qvals = (unsigned short *)(hdr->qt[id]).v;
        /* End for dequantized storage */
        for (j=0; j<ci->bw * ci->bh; j++) {
            incMcuInfo[numPieces].x = j % ci->bw;
            incMcuInfo[numPieces].y = j / ci->bw;
            incMcuInfo[numPieces++].compNum = i;
        }
    }

    if (numComps == 1) {
        ci->bw = ci->bh = 1;
        maxbw = maxbh = 1;
        numPieces = 1;
    } else {
        maxbw = hdr->maxBlockWidth;
        maxbh = hdr->maxBlockHeight;
    }
    globInfo.numPieces = numPieces;

    /*
     * Compute the parameters for each MCU:
     *  (w,h) give the dimension of the image in pixels.
     *  (mcuWidth,mcuHeight) give the dimensions of an MCU (in pixels)
     *  (wFull,hFull) give the dimensions of the image in MCUs
     *  maxmcu is the number of MCUs in this scan
     */
    mcuWidth = maxbw * 8;
    mcuHeight = maxbh * 8;
    w = hdr->width;
    h = hdr->height;

    globInfo.wFull = (int)ceil((double)w/mcuWidth);
    globInfo.hFull = (int)ceil((double)h/mcuHeight);
    globInfo.maxMcu = globInfo.wFull*globInfo.hFull;  
    globInfo.curMcu = 0;

    for (i=0; i<numComps; i++) {
        ci = &incCompInfo[i];
        ci->w = globInfo.wFull*ci->bw;
        ci->h = globInfo.hFull*ci->bh;
        if (scans[i]->width != ci->w) {
            errcode = DVM_JPEG_INVALID_WIDTH;
            goto err;
        }
        if (scans[i]->height != ci->h) {
            errcode = DVM_JPEG_INVALID_HEIGHT;
            goto err;
        }
        ci->numBlocks = ci->w * ci->h;
        ci->scanpad = scans[i]->parentWidth;
    }

    globInfo.numComps = numComps;
    return globInfo.maxMcu;

    /* Free stuff on error */
 err:
    for (i=0; i<numComps; i++) {
        ci = &incCompInfo[i];
        FREE(ci->dcht);
        FREE(ci->acht);
    }

    return errcode;
}

int
JpegScanIncParseEnd (bp, hdr, scanHdr, scans, num)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    ScImage *scans[];
    int num;
{
    int i;
    CompInfo *ci;

    /* Free alloced stuff */

    for (i=0; i<globInfo.numComps; i++) {
        ci = &incCompInfo[i];
        FREE(ci->dcht);
        FREE(ci->acht);
    }

    return 0;
}

int
JpegScanIncParse (bp, hdr, scanHdr, scans, num, howmany)
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    ScImage *scans[];
    int num;
    int howmany;
{
    int mcu, lastmcu, mcux, mcuy;
    int piece;
    int x, y, id;
    ScBlock *block;
    CompInfo *ci;
    unsigned short *dcht;
    unsigned short *acht;
    unsigned short *qval;

    int code, value;
    unsigned char difflen;
    short diff;
    int k, numAC;
    unsigned char mixed, run, coefflen, coeffVal;

    lastmcu = globInfo.curMcu + howmany;
    if (lastmcu > globInfo.maxMcu) {
        lastmcu = globInfo.maxMcu;
    }

    /*
     * Main decode loop
     */
    for (mcu = globInfo.curMcu; mcu < lastmcu; mcu++) {
        /*
         * Compute x,y offset of first block in MCU within its
         * component's block array
         */
        mcux = (mcu % globInfo.wFull);
        mcuy = (mcu / globInfo.wFull);

        /*
         * Decode all the MCUs
         */
        for (piece = 0; piece < globInfo.numPieces; piece++) {
            /*
             * Calculate everything specific to which plane we
             * are decoding to avoid conditionals in the main loop
             */

            id = incMcuInfo[piece].compNum;
            ci = &incCompInfo[id];
            dcht = ci->dcht;
            acht = ci->acht;
            qval = ci->qvals;
            x = mcux*ci->bw + incMcuInfo[piece].x;
            y = mcuy*ci->bh + incMcuInfo[piece].y;
            block = ci->blockPtr + x + y*ci->scanpad;

            /*
             * Get the DC value
             */
            Bp_PeekBits(bp, 16, code);
            value = dcht[code];
            Bp_FlushBits(bp, value >> 8);
            difflen = value & 0xff;
            if (difflen) {
                Bp_GetBits(bp, difflen, diff);
                JPEG_EXTEND(diff, difflen, diff);
                ci->predictor += diff;
            }
            block->dc = ci->predictor*qval[0] + NORMAL_OFFSET;

            /*
             * Get the AC values
             */
            numAC = 0;
            for (k=1; k<64; ) {
                Bp_PeekBits(bp, 16, code);
                value = acht[code];
                Bp_FlushBits(bp, value >> 8);
                mixed = value & 0xff;
                run = mixed >> 4;
                coefflen = mixed & 0xf;
                if (coefflen == 0) {
                    if (run == 15) {
                        k += 16;
                    } else {
                        break;
                    }
                } else {
                    k += run;
                    Bp_GetBits(bp, coefflen, coeffVal);
                    block->index[numAC] = k;
                    JPEG_EXTEND(coeffVal, coefflen, block->value[numAC]);
                    block->value[numAC] *= qval[zz[k]];
                    numAC++;
                    k++;
                }
            }
            block->numOfAC = numAC;
        }
    }
    globInfo.curMcu = lastmcu;
    return lastmcu;
}

int
JpegHdrSetQt(hdr, id, precision, table)
    JpegHdr *hdr;
    int id;
    int precision;
    short *table;
{
    JpegQt *qt;

    if ((id < 0) || (id > 3)) {
        return DVM_JPEG_INVALID_ID;
    }
    if ((precision != 8) && (precision != 16)) {
        return DVM_JPEG_INVALID_PRECISION;
    }
    qt = &hdr->qt[id];
    qt->precision = precision;
    memcpy((char *)(&(qt->v[0])), (char *)table, sizeof(short)*64);
    return DVM_JPEG_OK;
}

