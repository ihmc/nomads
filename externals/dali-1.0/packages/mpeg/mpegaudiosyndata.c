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


MpegAudioGraData *
MpegAudioGraDataNew()
{
    MpegAudioGraData *data;

    data = NEW(MpegAudioGraData);
    memset(data, 0, sizeof(MpegAudioGraData));
    return data;
}


MpegAudioSynData *
MpegAudioSynDataNew()
{
    MpegAudioSynData *data;

    data = NEW(MpegAudioSynData);
    data->offset = 64;
    memset(data->syn, 0, sizeof(data->syn));
    return data;
}


void 
MpegAudioSynDataFree(data)
    MpegAudioSynData *data;
{
    FREE(data);
}


void 
MpegAudioGraDataFree(data)
    MpegAudioGraData *data;
{
    FREE(data);
}

void
Bp_RestoreAnyBits(bp, n)
    BitParser *bp;
    int n;
{
    // assume the bits is still in the buffer which is
    // the case for MpegAudioBufData

    int short_to_restore;
    int bit_to_restore;

    short_to_restore = n / 16;
    bp->offsetPtr -= short_to_restore;
    bp->currentBits = *(bp->offsetPtr - 2);

    bit_to_restore = n % 16;
    bp->bitCount = bit_to_restore;
}
