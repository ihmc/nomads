/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "pnmInt.h"


int 
PpmParse(bp, r, g, b)
    BitParser *bp;
    ByteImage *r;
    ByteImage *g;
    ByteImage *b;
{
    register unsigned char *currbs, *currr, *currg, *currb;
    register int i,w,h;
    register int rSkip, gSkip, bSkip;
    int bytes;

    w = min(r->width, g->width);
    w = min(b->width, w);
    h = min(r->height, g->height);
    h = min(b->height, h);

    rSkip = r->parentWidth - w;
    gSkip = g->parentWidth - w;
    bSkip = b->parentWidth - w;
 
    currbs = bp->offsetPtr;
    currr  = r->firstByte;
    currg  = g->firstByte;
    currb  = b->firstByte;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *currr++ = *currbs++;
            *currg++ = *currbs++;
            *currb++ = *currbs++;
        );
        currr += rSkip;
        currg += gSkip;
        currb += bSkip;
    }
    bytes = currbs - bp->offsetPtr;
    bp->offsetPtr = currbs;
    return bytes;
}


int 
PpmEncode(r, g, b, bp)
    ByteImage *r;
    ByteImage *g;
    ByteImage *b;
    BitParser *bp;
{
    register unsigned char *currbs, *currr, *currg, *currb;
    register int i,w,h;
    register int rSkip, gSkip, bSkip;

    w = min(r->width, g->width);
    w = min(b->width, w);
    h = min(r->height, g->height);
    h = min(b->height, h);

    rSkip = r->parentWidth - w;
    gSkip = g->parentWidth - w;
    bSkip = b->parentWidth - w;
 
    currbs = bp->offsetPtr;
    currr  = r->firstByte;
    currg  = g->firstByte;
    currb  = b->firstByte;

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *currbs++ = *currr++;
            *currbs++ = *currg++;
            *currbs++ = *currb++;
        );
        currr += rSkip;
        currg += gSkip;
        currb += bSkip;
    }
    bp->offsetPtr = currbs;
    bp->bs->endDataPtr = currbs;

    return 3*w*h;
}

int 
PgmParse(bp, byte)
    BitParser *bp;
    ByteImage *byte;
{
    register unsigned char *currbs, *curr;
    register int w,h;
    register int skip;
    int total;

    w = byte->width;
    h = byte->height;
    skip = byte->parentWidth;
    currbs = bp->offsetPtr;
    curr  = byte->firstByte;

    DO_N_TIMES(h,
        memcpy(curr, currbs, w);
        curr += skip;
        currbs += w;
    );
    total = currbs - bp->offsetPtr;
    bp->offsetPtr = currbs;

    return total;
}


int 
PgmEncode(byte, bp)
    ByteImage *byte;
    BitParser *bp;
{
    register unsigned char *currbs, *curr;
    register int w,h;
    register int skip;

    w = byte->width;
    h = byte->height;
    skip = byte->parentWidth;
    curr   = byte->firstByte;
    currbs = bp->offsetPtr;

    DO_N_TIMES(h,
        memcpy(currbs, curr, w);
        curr += skip;
        currbs += w;
    );
    bp->offsetPtr = currbs;
    bp->bs->endDataPtr = currbs;

    return w*h;
}


int 
PbmParse8(bp, bit)
    BitParser *bp;
    BitImage *bit;
{

    int w, h, skip, size;
    unsigned char *dest, *src;

    if (bit->firstBit != 0 || bit->lastBit != 0) {
        return DVM_PNM_NOT_BYTE_ALIGN;
    }

    /* calculate the number of bytes in each row in the pbm file. */
    w = bit->byteWidth;
    h = bit->height;
    size = w*h;
    skip = bit->parentWidth - w;

    dest = bit->firstByte;
    src  = bp->offsetPtr;

    if (bit->isVirtual) {
        DO_N_TIMES(h,
            memcpy(dest, src, w);
            dest += skip;
            src += w;
            );
    } else {
        memcpy (dest, src, size);
    }
    bp->offsetPtr += size;
    return size;
    
}


int 
PbmParse(bp, bit)
    BitParser *bp;
    BitImage *bit;
{
    int w, h, skip, size;
    unsigned char *src, *dest;

    if (bit->firstBit == 0 && bit->lastBit == 0) {
        return DVM_PNM_IS_BYTE_ALIGN;
    }

    if (bit->lastBit <= bit->firstBit || bit->firstBit==0 || bit->lastBit==0) {
        w = bit->byteWidth + 1;
    } else {
        w = bit->byteWidth + 2;
    } 
    h = bit->height;
    size = w*h;

    dest = bit->firstByte;
    src  = bp->offsetPtr;
    skip = bit->parentWidth;
    DO_N_TIMES(h,
        CopyRowSrcOnTheLeft(dest, bit->firstBit, bit->lastBit,
            src, 0, bit->unitWidth % 8, w);
        dest += skip;
        src += w;
        );
    bp->offsetPtr += size;
    return size;
}

int 
PbmEncode8(bit, bp)
    BitImage *bit;
    BitParser *bp;
{
    int w, h, skip;
    unsigned char *src, *dest;

    if (bit->firstBit != 0 || bit->lastBit != 0) {
        return DVM_PNM_NOT_BYTE_ALIGN;
    }
    src = bit->firstByte;
    dest  = bp->offsetPtr;

    w = bit->byteWidth;
    if (bit->lastBit != 0)
        w++;
    skip = bit->parentWidth; 
    h = bit->height;

    if (bit->isVirtual) {
        DO_N_TIMES(h,
            memcpy(dest, src, w);
            src += skip;
            dest += w;
            );
    } else {
        memcpy(dest, src, w*h);
    }

    bp->bs->endDataPtr += w*h;
    bp->offsetPtr = bp->bs->endDataPtr;

    return w*h;
}

int 
PbmEncode(bit, bp)
    BitImage *bit;
    BitParser *bp;
{
    int w, h, skip;
    unsigned char *src, *dest;

    if (bit->firstBit == 0 && bit->lastBit == 0) {
        return DVM_PNM_IS_BYTE_ALIGN;
    }

    src = bit->firstByte;
    dest  = bp->offsetPtr;

    if (bit->lastBit <= bit->firstBit || bit->firstBit == 0) {
        w = bit->byteWidth + 1;
    } else {
        w = bit->byteWidth + 2;
    }
    skip = bit->parentWidth; 
    h = bit->height;

    if (bit->firstBit == 0) {
        DO_N_TIMES(h,
            CopyRowEqual(dest, 0, bit->unitWidth % 8,
                src, bit->firstBit, bit->lastBit, w);
            src += skip;
            dest += w;
            );
    } else {
        DO_N_TIMES(h,
            CopyRowSrcOnTheRight(dest, 0, bit->unitWidth % 8,
                src, bit->firstBit, bit->lastBit, w);
            src += skip;
            dest += w;
            );
    }

    bp->bs->endDataPtr += w*h;
    bp->offsetPtr = bp->bs->endDataPtr;

    return w*h;
}
