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
/* Documentation updated 10/9/98 by bsmith */

BitStream *
BitStreamNew(size)
    int size;
{
    BitStream *bs;

    bs = NEW(BitStream);
    if (size) {
        bs->buffer = NEWARRAY(unsigned char, size);
    } else {
        bs->buffer = NULL;
    }
    bs->size = size;
    bs->endDataPtr = bs->buffer;
    bs->endBufPtr = bs->buffer + size;
    bs->isVirtual = 0;

    return bs;
}

#ifdef HAVE_MMAP
BitStream *
BitStreamMmapReadNew(char *fname)
{
    BitStream *bs;
    int size = 0;

    bs = NEW(BitStream);
    bs->buffer = Dvm_Mmap(0, &size, DVM_MMAP_READ,
        DVM_MMAP_MAP_PRIVATE, fname, 0);
    if (bs->buffer == NULL) {
        FREE(bs);
        return NULL;
    }
    bs->size = size;
    bs->endDataPtr = bs->buffer + size;
    bs->endBufPtr = bs->buffer + size;
    bs->isVirtual = 0;

    return bs;
}

void
BitStreamMmapReadFree(BitStream * bs)
{
    if (bs->buffer != NULL) {
        Dvm_Munmap(bs->buffer, bs->size);
    }
    FREE((char *) bs);
}

#endif /* HAVE_MMAP */

void
BitStreamResize(bs, size)
    BitStream *bs;
    int size;
{
    int len;

    len = bs->endDataPtr - bs->buffer;
    if (bs->buffer != NULL) {
        bs->buffer = REALLOC(bs->buffer, sizeof(char) * size);
    } else {
        bs->buffer = NEWARRAY(unsigned char, size);
    }
    bs->size = size;
    bs->endDataPtr = bs->buffer + len;
    bs->endBufPtr = bs->buffer + size;
}

int
BitStreamBytesLeft(bs, off)
    BitStream *bs;
    int off;
{
    int left;

    left = (bs->endDataPtr - bs->buffer) - off;
    if (left < 0)
        left = 0;
    return left;
}

void
BitStreamFree(bs)
    BitStream *bs;
{
    if (!bs->isVirtual)
        if (bs->buffer != NULL) {
            FREE((char *) bs->buffer);
        }
    FREE((char *) bs);
}

void
BitStreamFreeBuffer(bs)
    BitStream *bs;
{
    if (!bs->isVirtual)
        FREE((char *) bs->buffer);
}

void
BitStreamShareBuffer(bs1, bs2)
    BitStream *bs1;
    BitStream *bs2;
{
    bs2->buffer = bs1->buffer;
    bs2->size = bs1->size;
    bs2->endDataPtr = bs1->endDataPtr;
    bs2->endBufPtr = bs1->endBufPtr;
}

void
BitStreamShift(bs, off)
    BitStream *bs;
    int off;
{
    int toCopy;

    toCopy = (bs->endDataPtr - bs->buffer) - off;
    if (toCopy > 0) {
        memmove(bs->buffer, bs->buffer + off, toCopy);
        bs->endDataPtr = bs->buffer + toCopy;
    }
}

void
BitStreamDump(inbs, inoff, outbs, outoff, length)
    BitStream *inbs;
    int inoff;
    BitStream *outbs;
    int outoff;
    int length;
{
    memcpy(outbs->buffer + outoff, inbs->buffer + inoff, length);
    outbs->endDataPtr = outbs->buffer + outoff + length;
}


void
BitStreamDumpSegments(inbs, inoff, outbs, outoff, length, skip, times)
    BitStream *inbs;
    int inoff;
    BitStream *outbs;
    int outoff;
    int length;
    int skip;
    int times;
{

    unsigned char *currSrc, *currDest;

    currDest = outbs->buffer + outoff;
    currSrc = inbs->buffer + inoff;

    DO_N_TIMES(times,
        memcpy(currDest, currSrc, length);
        currSrc += length + skip;
        currDest += length;
        );
}

int
BitStreamDumpUsingFilter(src, srcOff, dest, destOff, length, index)
    BitStream *src;             /* Holds source data */
    int srcOff;                 /* Offset of first byte in src w.r.t.

                                 * the beginning of the file */
    BitStream *dest;            /* Destination buffer */
    int destOff;                /* Data will be copied into

                                 * dest->buffer+destOff */
    int length;                 /* The number of bytes to copy. Use

                                 * length = -1 to fill up dest */
    BitStreamFilter *index;     /* The index to use */
{
    unsigned char *newEndPtr;
    int totalBytesCopied;
    int bytesLeft, segmentLength;
    int currEntry, currOffset;

    /*
     * If we've reached the end of the index, return 0
     */
    if (index->currEntry == index->lastEntry) {
        return 0;
    }
    totalBytesCopied = 0;
    bytesLeft = dest->size - destOff;
    if (length > 0) {
        bytesLeft = min(length, bytesLeft);
    }
    newEndPtr = dest->buffer + destOff;

    /*
     * We might have left off in the middle of a read
     * last time.  If so, reinitialize segmentLength, which
     * is the number of bytes to copy each time through
     * the loop below
     */
    if (index->currLength == -1) {
        segmentLength = index->table[0].length;
        currOffset = index->table[0].offset - srcOff;
        currEntry = 0;
    } else {
        segmentLength = index->currLength;
        currOffset = index->currOffset - srcOff;
        currEntry = index->currEntry;
    }

    /*
     * Copy all the whole chunks we can
     */
    while (segmentLength <= bytesLeft) {
        memcpy(newEndPtr, src->buffer + currOffset, segmentLength);
        totalBytesCopied += segmentLength;
        newEndPtr += segmentLength;
        bytesLeft -= segmentLength;
        currEntry++;
        if (currEntry == index->lastEntry) {
            index->currEntry = currEntry;
            return totalBytesCopied;
        }
        segmentLength = index->table[currEntry].length;
        currOffset = index->table[currEntry].offset - srcOff;
    }

    /*
     * If we didn't end up on a whole chunk boundary,
     * do a partial copy
     */
    if (bytesLeft != 0) {
        memcpy(newEndPtr, src->buffer + currOffset, bytesLeft);
        totalBytesCopied += bytesLeft;
        newEndPtr += bytesLeft;
    }
    /*
     * Store data for next read
     */
    index->currLength = index->table[currEntry].length - bytesLeft;
    index->currOffset = index->table[currEntry].offset + bytesLeft;
    index->currEntry = currEntry;
    dest->endDataPtr = newEndPtr;
    return totalBytesCopied;
}
