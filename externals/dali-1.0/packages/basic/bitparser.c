/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "basicInt.h"

int bitmask[] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff
};

BitParser *
BitParserNew()
{
    BitParser *bp;

    bp = NEW(BitParser);
    bp->bs = NULL;
    bp->currentBits = 0;
    bp->bitCount = 0;

    return bp;
}

void
BitParserFree(BitParser * bp)
{
    FREE((char *) bp);
}

int
BitParserTell(BitParser * bp)
{
    int tell;

    tell = (bp->offsetPtr - bp->bs->buffer);
    if (tell > bp->bs->size)
        tell = bp->bs->size;

    return tell;
}

void
BitParserSeek(BitParser * bp, int off)
{
    bp->offsetPtr = bp->bs->buffer + off;
    bp->bitCount = 0;
}

void
BitParserWrap(BitParser * bp, BitStream * bs)
{
    bp->bs = bs;
    bp->offsetPtr = bs->buffer;
    bp->bitCount = 0;
}
