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

BitStreamFilter *
BitStreamFilterNew(size)
    int size;
{

    BitStreamFilter *index;

    index = NEW(BitStreamFilter);
    index->maxEntry = size;
    index->lastEntry = 0;
    index->currEntry = 0;
    index->currOffset = 0;
    index->currLength = -1;
    if (size) {
        index->table = NEWARRAY(FilterEntry, size);
    } else {
        index->table = NULL;
    }
    return index;
}

void
BitStreamFilterFree(index)
    BitStreamFilter *index;
{
    FREE((char *) index->table);
    FREE((char *) index);
}

int
BitStreamFilterAdd(index, offset, length)
    BitStreamFilter *index;
    unsigned int offset;
    unsigned int length;
{
    int number;

    if (index->lastEntry == index->maxEntry) {
        return DVM_STREAMS_FILTER_FULL;
    }
    number = index->lastEntry;
    index->table[number].offset = offset;
    index->table[number].length = length;
    index->lastEntry++;

    return DVM_STREAMS_OK;
}


void
BitStreamFilterResize(index, newSize)
    BitStreamFilter *index;
    unsigned int newSize;
{
    index->maxEntry = newSize;
    index->table = REALLOC(index->table, sizeof(FilterEntry) * newSize);
}

void
BitStreamFilterStartScan(index)
    BitStreamFilter *index;
{
    index->currLength = -1;
    index->currEntry = 0;
    index->currOffset = 0;
}

void
BitStreamFilterWrite(file, index)
    FILE *file;
    BitStreamFilter *index;
{
    fwrite(index, sizeof(BitStreamFilter), 1, file);
    fwrite(index->table, sizeof(FilterEntry), index->lastEntry, file);
}

void
BitStreamFilterRead(file, index)
    FILE *file;
    BitStreamFilter *index;
{
    if (index->table != NULL) {
        FREE((char *) index->table);
        index->table = NULL;
    }
    fread(index, sizeof(BitStreamFilter), 1, file);
    index->maxEntry = index->lastEntry;
    index->table = (FilterEntry *) MALLOC(index->lastEntry * sizeof(FilterEntry));
    fread(index->table, sizeof(FilterEntry), index->lastEntry, file);
}
