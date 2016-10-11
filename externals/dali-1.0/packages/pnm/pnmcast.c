/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "dvmpnm.h"

ByteImage *
BitStreamCastToByte (bs, hdr, off)
    BitStream *bs;
    PnmHdr *hdr;
    int off;
{
    ByteImage *new;

    new = NEW(ByteImage);
    new->x = 0;
    new->y = 0;
    if (hdr->type == PGM_BIN) {
        new->width = PnmHdrGetWidth(hdr);
    } else if (hdr->type == PPM_BIN) {
        new->width = 3*PnmHdrGetWidth(hdr);
    } 
    new->height = PnmHdrGetHeight(hdr);
    new->parentWidth = new->width;
    new->isVirtual = 1;
    new->firstByte = bs->buffer + off;

    return new;
}

BitStream *
ByteCastToBitStream (byte)
    ByteImage *byte;
{
    BitStream *bs;

    bs = NEW(BitStream);
    bs->buffer = byte->firstByte;
    bs->size = byte->width*byte->height;
    bs->endDataPtr = bs->buffer + bs->size;
    bs->endBufPtr = bs->endDataPtr;
    bs->isVirtual = 1;

    return bs;
}


BitImage *
BitStreamCastToBit (bs, hdr, off)
    BitStream *bs;
    PnmHdr *hdr;
    int off;
{
    BitImage *new;
    int w;

    new = NEW(BitImage);
    new->x = 0;
    new->y = 0;
    new->unitWidth = PnmHdrGetWidth(hdr);
    new->height = PnmHdrGetHeight(hdr);
    new->firstBit = 0;
    new->lastBit = new->unitWidth & 0x07;

    w = new->unitWidth >> 3;
    new->byteWidth = w;

    if (new->lastBit != 0) {
        w++;
    }
    new->parentWidth = w;
    new->isVirtual = 1;
    new->firstByte = bs->buffer + off;

    return new;
}

BitStream *
BitCastToBitStream (bit)
    BitImage *bit;
{
    BitStream *bs;
    int w;

    w = bit->byteWidth;
    if (bit->lastBit != 0)
        w++;

    bs = NEW(BitStream);
    bs->buffer = bit->firstByte;
    bs->size = w*bit->height;
    bs->endDataPtr = bs->buffer + bs->size;
    bs->endBufPtr = bs->endDataPtr;
    bs->isVirtual = 1;

    return bs;
}
