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
 * basicInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _BASIC_INT_
#define _BASIC_INT_

#include "dvmbasic.h"

#define FIRST_BIT_ONE(byte) (byte & 0x80)
#define FIRST_BIT_ZERO(byte) (!(byte & 0x80))
#define ZERO_FIRST(n, b) (unsigned char)((b) & thePreMask[(n)])
#define ZERO_LAST(n, b)  (unsigned char)((b) & thePostMask[8-(n)])
#define KEEP_FIRST(n, b) (unsigned char)((b) & thePostMask[(n)])
#define KEEP_LAST(n, b)  (unsigned char)((b) & thePreMask[8-(n)])

#define COMBINE_2_BYTES_LF(n, byte1, byte2) \
    (KEEP_LAST((n), byte1) << (8-(n))) | (KEEP_FIRST(8-(n), byte2) >> (n))

#define COMBINE_2_BYTES_FL(n, byte1, byte2) \
    KEEP_FIRST((n), byte1) | ZERO_FIRST((n), byte2)

#define COMBINE_2_BYTES_ML(m, n, byte1, byte2) \
    ((byte1 & (thePostMask[m] >> (n))) << (n)) | ZERO_FIRST(m, byte2)

#define COMBINE_2_BYTES_FM(m, n, byte1, byte2) \
     ZERO_LAST(m, byte1) | ((unsigned char)(byte2 & (thePostMask[m] >> (n))) >> (8-(m)-(n)))

#define COMBINE_3_BYTES_FLF(m, n, byte1, byte2, byte3) \
    KEEP_FIRST(m, byte1) | (KEEP_LAST((n), byte2) << (8-(m)-(n))) \
    | (ZERO_LAST(m+(n), byte3) >> ((m)+(n)))

#define COMBINE_3_BYTES_LFL(m, n, byte1, byte2, byte3) \
    KEEP_LAST(m, byte1) | (KEEP_FIRST((n), byte2) >> (m)) \
    | ZERO_FIRST((m)+(n), byte3)

extern unsigned char thePreMask[8];
extern unsigned char thePostMask[8];

/*
 * BitImageScan API and constant
 */
BitImageScan *BitImageOpenScan(BitImage *, int, int);
int BitImageGetNextExtent(BitImageScan *, int *, int *, int *);
void BitImageCloseScan(BitImageScan *);

/*
 * Return code for BitImageGetNextExtent().
 * SCAN_NOT_DONE indicates the current extents output is valid.  
 * SCAN_DONE indicate the current extents is invalid and there are no more
 * extents to be found.
 */
#define SCAN_NOT_DONE 0
#define SCAN_DONE     1

/*
 * Utilities functions for bit manipulation
 */
void CopyRowEqual(unsigned char *, int, int, unsigned char *, int, int, int);
void CopyRowSrcOnTheLeft(unsigned char *, int, int, unsigned char *, int, int, int);
void CopyRowSrcOnTheRight(unsigned char *, int, int, unsigned char *, int, int, int);
unsigned char GetNextNBitsPutFront(int, unsigned char **, int *);
unsigned char GetNextNBitsPutEnd(int, unsigned char **, int *);
unsigned char GetNextByte(unsigned char **byte, int firstBits);

#endif
