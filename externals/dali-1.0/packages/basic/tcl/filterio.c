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

void
BitStreamFilterChannelWrite(index, chan)
    BitStreamFilter *index;
    Tcl_Channel chan;
{
    Tcl_Write(chan, (char *) index, sizeof(BitStreamFilter));
    Tcl_Write(chan, (char *) index->table, index->lastEntry * sizeof(FilterEntry));
}

void
BitStreamFilterChannelRead(index, chan)
    BitStreamFilter *index;
    Tcl_Channel chan;
{
    if (index->table != NULL) {
        FREE((char *) index->table);
        index->table = NULL;
    }
    Tcl_Read(chan, (char *) index, sizeof(BitStreamFilter));
    index->table = (FilterEntry *) MALLOC(index->lastEntry * sizeof(FilterEntry));
    index->maxEntry = index->lastEntry;
    Tcl_Read(chan, (char *) index->table, index->lastEntry * sizeof(FilterEntry));
}
