/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */

#include "mpegInt.h"

// check data types for all the unsigned long's
void
MpegSliceHdrEncode(bp, verticalPos, qScale, extraInfo, extraInfoSize)
    BitParser *bp;
    int verticalPos;
    int qScale;
    unsigned char *extraInfo;
    unsigned long extraInfoSize;
{
    int i;

    Bp_OutByteAlign(bp);

    /* Write slice start code. */
    Bp_PutInt(bp, ((1 << 8) + verticalPos));    /* replace 1<<8 with SLICE_BASE_CODE */

    /* Quant. scale. */
    Bp_PutBits(bp, 5, qScale);

    /* Extra bit slice info. */
    if (extraInfo != NULL) {
        for (i = 0; (unsigned) i < extraInfoSize; i++) {
            Bp_PutBits(bp, 1, 0x01);
            Bp_PutBits(bp, 8, extraInfo[i]);
        }
    }
    /* extra bit slice */
    Bp_PutBits(bp, 1, 0x00);
}
