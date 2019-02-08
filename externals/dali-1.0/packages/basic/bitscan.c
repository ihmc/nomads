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
 * bitscan.c
 *
 * Wei Tsang May 97
 *
 * Defines functions that returns a scan line extents from bitmap-region.
 * 3 functions are defined :
 *  OpenScan()
 *  GetNextExtent()
 *  CloseScan()
 *
 *  -  OpenScan() takes in two buffer as input :- one byte buffer, which
 *     contained the data to be process, and one bit buffer, which acts
 *     as a bitmap-region.  It returns a pointer to a BitImageScan
 *     structure.  This structure records the state of a scan and need
 *     to be passed around to other scan related functions.
 *
 *  -  GetNextExtent() takes in a BitImageScan pointer, and output an
 *     extent.  An extent is a pair of coordinates (x1, y) and (x2, y)
 *     in the same row, such that in the bitmap-region, all bits between
 *     (x1, y) and (x2-1, y) is 1, and bits at (x1-1, y) and (x2, y) has
 *     value 0.   GetNextExtent() output an extent in the form of
 *     (x1, x2 - x1, y)  GetNextExtent() returns SCAN_NOT_DONE if it founds
 *     an extent and return it.  SCAN_DONE is returned if it cannot
 *     found anymore extent in the bitmap-region.  If SCAN_DONE is
 *     returned, the output pointers are invalid.
 *
 * -   CloseScan() free the BitImageScan structure.  After CloseScan()
 *     is called on a BitImageScan structure, the structure cannot be
 *     used any more.
 *
 * Here is a typical usage of the bit scan functions :
 *
 * scan = BitImageOpenScan(bit);
 * while (BitImageGetNextExtent(scan, &x1, &len, &y) != SCAN_DONE) {
 *     curr = calc_start(x1, byte);;
 *     DO_N_TIMES(len,
 *         // do something with curr
 *         curr++;
 *     }
 * }
 * BitImageCloseScan(scan);
 *
 *
 * TODO : Make GetNextExtent faster by comparing 32-bits / 8-bits at a
 *        time.
 *      : Make CloseScan into a macro.
 *
 * Yongjian Xiang: July 97
 *
 *	1.	Modified the function _BitImageGetNextExtent_ .
 *		In the previous implementation, if _BitImageGetNextExtent_
 *		is called when the current byte is the last partial byte on a row of the
 *		BitImage that is scaned, a bug occures. Consider the situation:
 *			... | 000{00111 | 11111111 | 11100}000 |...
 *											^
 *		where { } indicates the horizontal bound of a isVirtual bit buffer.
 *		The first call scans upto position ^.
 *		The second call would step out of _lastbit_,
 *		because it does not check that the current byte is already the last byte.
 *		on the row.
 *
 *	2.	Added a new data type:  _ExtentArray_,
 *		Added a function to get the bounding box of a bit image.
 *
 *----------------------------------------------------------------------
 */

#include "basicInt.h"

static int _firstTime;

#define TEST_ONE_AND_JUMP {\
    if (FIRST_BIT_ONE(regionByte)) {\
	goto found1;\
    } else {\
	currBit = (currBit == 7) ? 0 : currBit + 1;\
	x++;\
	regionByte <<= 1;\
    }\
}

#define TEST_ZERO_AND_JUMP {\
    if (FIRST_BIT_ZERO(regionByte)) {\
	goto found0;\
    } else {\
	currBit = (currBit == 7) ? 0 : currBit + 1;\
	x++;\
	regionByte <<= 1;\
    }\
}


BitImageScan*
BitImageOpenScan(bit, width, height)
    BitImage *bit;
    int width;      /* height and width of the ByteImage being process */
    int height;
{
    BitImageScan *scan;

    if (!bit) {
	scan = NEW(BitImageScan);
	scan->region = NULL;
	scan->x = width;
	scan->y = height;
	_firstTime = TRUE;
	return scan;
    }

    /*
     * The bitmask must be smaller or equal to the area to be scanned.
     */

    /*
    if (bit->unitWidth > width || bit->height > height) {
	return NULL;
    }
    */


    scan = NEW(BitImageScan);
    scan->currByte = bit->firstByte;
    scan->currBit  = bit->firstBit;
    scan->region = bit;
    // begin 16-1-98 addition
    if (width < bit->unitWidth) {
	scan->unitWidth = width;
	width = width - 8 + bit->firstBit;
	scan->byteWidth = width >> 3;
	if (bit->firstBit == 0)
	    scan->byteWidth++;
	scan->lastBit = width & 0x07;
    } else {
	scan->unitWidth = bit->unitWidth;
	scan->byteWidth = bit->byteWidth;
	scan->lastBit   = bit->lastBit;
    }
    scan->height = min(height, bit->height);
    // end 16-1-98 addition
    if (bit->firstBit != 0)
	scan->regionSkip = bit->parentWidth - scan->byteWidth - 1;
    else
	scan->regionSkip = bit->parentWidth - scan->byteWidth;
    scan->x = 0;
    scan->y = 0;

    return scan;
}

int
BitImageGetNextExtent(s, startx, len, outy)
    BitImageScan *s;
    int *startx;
    int *len;
    int *outy;
{

    register unsigned char *currByte;
    register int  currBit;
    register unsigned char regionByte;
    register int x, y, unitsLeft, bytesLeft, firstBit;
    int unitWidth, byteWidth, h, lastBit, tail;
    register BitImageScan *scan = s;

    if (scan->region == NULL) {
	if (_firstTime) {
	    *startx = 0;
	    *len = scan->x;
	    *outy = 0;
	    _firstTime = FALSE;
	    return SCAN_NOT_DONE;
	}
	else if (++(*outy) != scan->y) {
	    return SCAN_NOT_DONE;
	}
	else
	    return SCAN_DONE;
    }

    byteWidth = scan->byteWidth;
    unitWidth = scan->unitWidth;
    h = scan->height;
    x = scan->x;
    y = scan->y;
    firstBit = scan->region->firstBit;
    lastBit = scan->lastBit;
    currByte = scan->currByte;
    currBit = scan->currBit;

    while ( y < h )  {
	regionByte = *currByte;
	regionByte <<= currBit;
	if ( unitWidth - x > lastBit )	{
	    /*
	     * We are not in the last partial byte yet.
	     * Process the first byte.  Need to do this differently since
	     * we may not be interested in the whole byte.
	     */
	    if (currBit != 0) {
		switch (currBit) {
		case 1 : TEST_ONE_AND_JUMP;
		case 2 : TEST_ONE_AND_JUMP;
		case 3 : TEST_ONE_AND_JUMP;
		case 4 : TEST_ONE_AND_JUMP;
		case 5 : TEST_ONE_AND_JUMP;
		case 6 : TEST_ONE_AND_JUMP;
		case 7 : TEST_ONE_AND_JUMP;
		}
		currByte++;
	    }
	}

	bytesLeft =  (unitWidth - x - lastBit)/8; /* number of full internal bytes */
	if( bytesLeft > 0)	 {
	    DO_N_TIMES(bytesLeft,
	    {
		if (*currByte) {
		/*
		 * Some bit within this byte is 1. Find it and jump to
		 * the end.
		 */
		    regionByte = *currByte;
		    while (1)
			TEST_ONE_AND_JUMP;
		} else {
		    x += 8;
		    currByte++;
		}
	    }
	    );
	}

	/* There are several possibilities that you reach here
	 *
	 * case 1:  The starting bytes to scan at the time this function is
	 *          called is already the last bytes on the current row of the bit images.
	 *
	 * case 2;  All bits before the beginning of the last partial bytes
	 *          have been scanned, no 1 found.
	 *
	 * In both cases, we can scan the last sevaral bits of the current row.
	 */

	tail = lastBit - currBit;
	if (lastBit > currBit) {
	    regionByte = (*currByte) << (8 - tail);
	    DO_N_TIMES(tail,
		TEST_ONE_AND_JUMP);
	}

	/* No 1 found on the current row, continue on the next row.   */
	x = 0;
	y = y + 1;
	currBit  = firstBit;
	currByte += scan->regionSkip;
    }
    return SCAN_DONE;			/* The last row has been scanned */

found1:

    *startx = x;
    *outy = y;

    /*  need to find next 0 on the current row */

    if (unitWidth - x > lastBit) {
    /*
     * * We are not in the last partial byte yet.
     * * Process the first byte.  Need to do this differently since
     * * we may not be interested in the whole byte.
     */
	if (currBit != 0) {
	    switch (currBit) {
	    case 1: TEST_ZERO_AND_JUMP;
	    case 2: TEST_ZERO_AND_JUMP;
	    case 3: TEST_ZERO_AND_JUMP;
	    case 4: TEST_ZERO_AND_JUMP;
	    case 5: TEST_ZERO_AND_JUMP;
	    case 6: TEST_ZERO_AND_JUMP;
	    case 7: TEST_ZERO_AND_JUMP;
	    }
	    currByte++;
	}
    }

    unitsLeft = unitWidth - x ;
    bytesLeft =  (unitsLeft - lastBit)/8;	/* number of full internal bytes left */
    if( bytesLeft > 0)	 {
	DO_N_TIMES(bytesLeft,
	{
	    if (*currByte != 0xff) {
	    /*
	     * Some bit within this byte is 0. Find it and jump to
	     * the end.
	     */
		regionByte = *currByte;
		while (1)
		    TEST_ZERO_AND_JUMP;
	    } else {
		x += 8;
		currByte++;
	    }
	}
	);
    }

    tail = lastBit - currBit;
    if (lastBit > currBit) {
	regionByte = (*currByte) << (8 - tail);
	DO_N_TIMES(tail,
	    TEST_ZERO_AND_JUMP);
    }

    /*
     * No 0 found on the current row, move scan
     * to the next line. The function returns
     */
    *len = unitWidth - *startx ;
    scan->x = 0;
    scan->y = y+1;
    scan->currBit  = scan->region->firstBit;
    scan->currByte = currByte + scan->regionSkip;
    return SCAN_NOT_DONE;

found0:
    *len = x - *startx;
    scan->x = x;
    scan->y = y;
    scan->currBit = currBit;
    scan->currByte = currByte;
    return SCAN_NOT_DONE;
}


void
BitImageCloseScan(scan)
    BitImageScan *scan;
{
    if (scan) {
	FREE((char*)scan);
    }
}
