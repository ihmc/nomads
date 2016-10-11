/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmBasic.h"

int
BitStreamChannelRead(bs, chan, off)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
{
    int bytes;

    bytes = Tcl_Read(chan, (char *) (bs->buffer + off), bs->size - off);
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamChannelReadSegment(bs, chan, off, length)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
    int length;
{
    int bytes;

    bytes = Tcl_Read(chan, (char *) (bs->buffer + off), length);
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamChannelReadSegments(bs, chan, off, length, skip, times)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
    int length;
{
    int bytes;
    unsigned char *dest;

    dest = bs->buffer + off;
    bytes = 0;
    DO_N_TIMES(times,
        bytes += Tcl_Read(chan, (char *) dest, length);
        Tcl_Seek(chan, skip, SEEK_CUR);
        dest += length;
        );
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamChannelWrite(bs, chan, off)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
{
    return Tcl_Write(chan, (char *) (bs->buffer + off), bs->endDataPtr - bs->buffer);
}


int
BitStreamChannelWriteSegment(bs, chan, off, length)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
{
    return Tcl_Write(chan, (char *) (bs->buffer + off), length);
}


int
BitStreamChannelWriteSegments(bs, chan, off, length, skip, times)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
    int length;
    int skip;
    int times;
{
    int total;
    unsigned char *src;

    total = 0;
    src = bs->buffer + off;
    DO_N_TIMES(times,
        total = Tcl_Write(chan, (char *) src, length);
        src += skip;
        );
    return total;
}


int
BitStreamChannelFilterIn(bs, chan, off, index)
    BitStream *bs;
    Tcl_Channel chan;
    int off;
    BitStreamFilter *index;
{
    unsigned char *newEndPtr;
    int totalBytesRead;
    int bytesLeft, bytesToRead;
    int currEntry, currOffset;

    if (index->currEntry == index->lastEntry) {
        return 0;
    }
    totalBytesRead = 0;
    bytesLeft = (bs->size - off);
    if (index->currLength == -1) {
        bytesToRead = index->table[0].length;
        currOffset = index->table[0].offset;
        currEntry = 0;
    } else {
        bytesToRead = index->currLength;
        currOffset = index->currOffset;
        currEntry = index->currEntry;
    }
    newEndPtr = bs->buffer + off;
    while (bytesToRead <= bytesLeft) {
        Tcl_Seek(chan, currOffset, SEEK_SET);
        Tcl_Read(chan, (char *) newEndPtr, bytesToRead);
        totalBytesRead += bytesToRead;
        newEndPtr += bytesToRead;
        bytesLeft -= bytesToRead;
        currEntry++;
        if (currEntry == index->lastEntry) {
            index->currEntry = currEntry;
            return totalBytesRead;
        }
        bytesToRead = index->table[currEntry].length;
        currOffset = index->table[currEntry].offset;
    }
    if (bytesLeft != 0) {
        // if there are something else to read
        index->currLength = bytesToRead - bytesLeft;    // prepare for next read

        Tcl_Seek(chan, currOffset, SEEK_SET);
        Tcl_Read(chan, (char *) newEndPtr, bytesLeft);
        index->currOffset = Tcl_Tell(chan);
        index->currEntry = currEntry;
        totalBytesRead += bytesLeft;
        newEndPtr += bytesLeft;
    } else {
        currEntry++;
        index->currLength = index->table[currEntry].length;
        index->currOffset = index->table[currEntry].offset;
        index->currEntry = currEntry;
    }
    bs->endDataPtr = newEndPtr;
    return totalBytesRead;
}
