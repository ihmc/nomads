/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "dvmbasic.h"

int
BitStreamFileRead(bs, f, off)
    BitStream *bs;
    FILE *f;
    int off;
{
    int bytes;

    bytes = fread((bs->buffer + off), 1, bs->size - off, f);
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamFileReadSegment(bs, f, off, length)
    BitStream *bs;
    FILE *f;
    int off;
{
    int bytes;

    bytes = fread((bs->buffer + off), 1, length, f);
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamFileReadSegments(bs, f, off, length, skip, times)
    BitStream *bs;
    FILE *f;
    int off;
{
    int bytes;
    unsigned char *dest;

    dest = bs->buffer + off;

    bytes = 0;
    DO_N_TIMES(times,
        bytes += fread(dest, 1, length, f);
        fseek(f, skip, SEEK_CUR);
        dest += length;
        );
    bs->endDataPtr = bs->buffer + off + bytes;

    return bytes;
}

int
BitStreamFileWrite(bs, f, off)
    BitStream *bs;
    FILE *f;
{
    int total;

    total = fwrite(bs->buffer + off, 1, bs->endDataPtr - bs->buffer, f);
    return total;
}

int
BitStreamFileWriteSegment(bs, f, off, length)
    BitStream *bs;
    FILE *f;
    int off;
    int length;
{
    int total;

    total = fwrite(bs->buffer + off, 1, length, f);
    return total;
}

int
BitStreamFileWriteSegments(bs, f, off, length, skip, times)
    BitStream *bs;
    FILE *f;
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
        total += fwrite(src, 1, length, f);
        src += skip;
        );
    return total;
}


int
BitStreamFileFilterIn(bs, chan, off, index)
    BitStream *bs;
    FILE *chan;
    int off;
    BitStreamFilter *index;
{
    unsigned char *newEndPtr;
    int totalBytesRead;
    int bytesLeft, bytesToRead;
    int currEntry, currOffset;

    totalBytesRead = 0;
    bytesLeft = (bs->size - off);
    newEndPtr = bs->buffer + off;
    if (index->currLength == -1) {
        bytesToRead = index->table[0].length;
        currOffset = index->table[0].offset;
        currEntry = 0;
    } else {
        bytesToRead = index->currLength;
        currOffset = index->currOffset;
        currEntry = index->currEntry;
    }

    /*
     * Read in as many big blocks as we can
     */
    while (bytesToRead <= bytesLeft) {
        fseek(chan, currOffset, SEEK_SET);
        fread(newEndPtr, bytesToRead, 1, chan);
        totalBytesRead += bytesToRead;
        newEndPtr += bytesToRead;
        bytesLeft -= bytesToRead;
        currEntry++;
        if (currEntry == index->lastEntry) {
            bytesLeft = 0;
            break;
        }
        bytesToRead = index->table[currEntry].length;
        currOffset = index->table[currEntry].offset;
    }

    /*
     * Do a partial read if needed
     */
    if (bytesLeft != 0) {
        // if there are something else to read
        fseek(chan, currOffset, SEEK_SET);
        fread(newEndPtr, bytesLeft, 1, chan);
        totalBytesRead += bytesLeft;
        newEndPtr += bytesLeft;
    }
    /*
     * Set up for next read
     */
    index->currLength = index->table[currEntry].length - bytesLeft;
    index->currOffset = index->table[currEntry].offset + bytesLeft;
    index->currEntry = currEntry;
    bs->endDataPtr = newEndPtr;
    return totalBytesRead;
}
