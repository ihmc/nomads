/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * mpegvideoindex.c
 *
 * Functions to work with an index of frame types and positions in
 * display order in order to grab and decode a given series of frames.
 *
 *----------------------------------------------------------------------
 */

#include "bitparser.h"
#include "stdio.h"
#include "dvmmpeg.h"

MpegVideoIndex *
MpegVideoIndexNew(size)
    int size;
{
    MpegVideoIndex *index;

    index = NEW(MpegVideoIndex);
    index->table = NEWARRAY(MpegVideoIndexElement, size);
    index->maxElements = size;
    index->numElements = 0;
    return index;
}


void
MpegVideoIndexFree(index)
    MpegVideoIndex *index;
{
    FREE((char *) index->table);
    FREE((char *) index);
}

/*
 * This function reads an MpegVideoIndex from a bitstream.
 * The data is stored into the parameter index.
 * index is assumed to be uninitialized (i.e., just
 * allocated using MpegVideoIndexNew(int size).
 */

void
MpegVideoIndexParse(bp, index)
    BitParser *bp;
    MpegVideoIndex *index;
{
    Bp_GetByteArray(bp, sizeof(int), &index->numElements);

    if (index->numElements > index->maxElements) {
        MpegVideoIndexResize(index, index->numElements);
    }
    index->table = NEWARRAY(MpegVideoIndexElement, (sizeof(MpegVideoIndexElement) * index->numElements));
    Bp_GetByteArray(bp, index->numElements * sizeof(MpegVideoIndexElement), index->table);
}


/*
 *Assume BitParser *bp is wrapped to an empty stream
 */

void
MpegVideoIndexEncode(index, bp)
    MpegVideoIndex *index;
    BitParser *bp;
{
    Bp_PutByteArray(bp, 4, &index->numElements);
    Bp_PutByteArray(bp, index->numElements * sizeof(MpegVideoIndexElement), index->table);
}

int
MpegVideoIndexFindRefs(in, out, frameNum)
    MpegVideoIndex *in;
    MpegVideoIndex *out;
    int frameNum;
{
    int type, past;
    int i = 0;

    type = MpegVideoIndexGetType(in, frameNum);
    if (type == I_FRAME) {
        return DVM_MPEG_OK;
    } else {
        while (type != I_FRAME) {
            past = MpegVideoIndexGetPast(in, frameNum);
            frameNum = frameNum + past;
            if (out->numElements == out->maxElements) {
                return DVM_MPEG_INDEX_FULL;
            }
            memcpy(&(out->table[i]), &(in->table[frameNum]), sizeof(MpegVideoIndexElement));
            out->numElements++;
            i++;
            type = MpegVideoIndexGetType(in, frameNum);
        }
    }
    return DVM_MPEG_OK;
}

int
MpegVideoIndexTableAdd(index, frameNum, bitOffset, ftype, flength, past, next)
    MpegVideoIndex *index;
    int frameNum;
    int bitOffset;
    char ftype;
    int flength;
    int past;

    int next;
{
    MpegVideoIndexElement *curr;

    curr = &(index->table[frameNum]);
    if (index->numElements == index->maxElements) {
        return DVM_MPEG_INDEX_FULL;
    }
    index->numElements++;
    /* index->maxElements; */
    curr->type = ftype;
    curr->length = flength;
    curr->offset = bitOffset;
    curr->pastOffset = past;
    curr->forOffset = next;

    if (ftype == I_FRAME) {
        curr->refCount = 0;
    } else {
        int skipToHere, ref;

        skipToHere = past + frameNum;
        ref = index->table[skipToHere].refCount;
        if (ftype == P_FRAME) {
            curr->refCount = ref + 1;
        } else {
            curr->refCount = ref + 1;
        }
    }
    return DVM_MPEG_OK;
}

void
MpegVideoIndexResize(index, newSize)
    MpegVideoIndex *index;
    unsigned int newSize;
{
    index->maxElements = newSize;
    index->table = REALLOC(index->table, sizeof(MpegVideoIndexElement) * newSize);
}

#ifdef DEBUG
void
MpegVideoIndexPrint(index)
    MpegVideoIndex *index;
{
    int i;

    for (i = 0; i < index->numElements; i++) {
        fprintf(stderr, "%3d (%d\t%d\t%d\t%d\t%d\t%d)\n", i,
            index->table[i].refCount, index->table[i].type,
            index->table[i].pastOffset, index->table[i].forOffset,
            index->table[i].offset, index->table[i].length);
    }
}
#endif
