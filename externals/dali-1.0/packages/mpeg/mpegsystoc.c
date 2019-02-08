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



MpegSysToc *
MpegSysTocNew()
{
    MpegSysToc *toc;
    StreamInfo **info;

    toc = NEW(MpegSysToc);
    toc->numOfVideoStreams = 0;
    toc->numOfAudioStreams = 0;
    info = toc->streamInfo;
    DO_N_TIMES(48,
        *info++ = NULL;
        );

    return toc;
}

void
MpegSysTocAdd(BitParser * bp, MpegSysToc * toc, int bsOff)
{

#define CHUNK 1024

    int id, code, total, pos;
    unsigned int off;
    MpegPktHdr hdr;
    StreamInfo *info;
    int status;

    total = 0;
    code = NextStartCode(bp, &off);

    while (code != ISO_11172_END_CODE && (code != 0)) {
        total += off;

        if (code == SYS_START_CODE) {
            // do nothing
        } else if (code == PACK_START_CODE) {
            // do nothing
        } else if ((code & 0xFF) <= 0xEF && (code & 0xFF) >= 0xC0) {
            /*
             * We are at the start of a packet.  Check to see if the
             * whole packet is in the buffer.  If not, return.  The
             * caller will have to shift in more data.  They can tell
             * how much using BitParserTell()
             */
            Bp_RestoreInt(bp);
            pos = BitParserTell(bp);
            off = MpegPktHdrParse(bp, &hdr);
            if ((int) Bp_DataRemain(bp) < (int) (hdr.packetLength - off + 6)) {
                BitParserSeek(bp, pos);
                return;
            }
            id = (code & 0xFF) - 192;   // map to 0 - 48

            info = toc->streamInfo[id];
            if (info == NULL) {
                if (id >= 32) {
                    toc->numOfVideoStreams++;
                } else {
                    toc->numOfAudioStreams++;
                }
                info = toc->streamInfo[id] = NEW(StreamInfo);
                info->index = BitStreamFilterNew(CHUNK);
                info->time = NEWARRAY(float, CHUNK);

                info->max = CHUNK;
                info->numOfPacket = 0;
            }
            total += off - 4;

            status = BitStreamFilterAdd(info->index, total + bsOff,
                hdr.packetLength - off + 6);
            if (status == DVM_STREAMS_FILTER_FULL) {
                BitStreamFilterResize(info->index, info->index->maxEntry * 2);
                BitStreamFilterAdd(info->index, total + bsOff,
                    hdr.packetLength - off + 6);
            }
            if (info->numOfPacket == info->max) {
                info->max += CHUNK;
                info->time = REALLOC(info->time, sizeof(int) * info->max);
            }
            info->time[info->numOfPacket] = (float) hdr.pts;
            info->numOfPacket++;
            Bp_FlushBytes(bp, hdr.packetLength - off + 6);
            total += hdr.packetLength - off + 6;
        }
        code = NextStartCode(bp, &off);
    }
}

BitStreamFilter *
MpegSysTocGetFilter(toc, id)
    MpegSysToc *toc;
    int id;
{
    if (toc->streamInfo[id] == NULL)
        return NULL;
    return toc->streamInfo[id]->index;
}

int
MpegSysTocGetOffset(toc, id, time)
    MpegSysToc *toc;
    int id;
    double time;
{
    int currEntry = 0;
    StreamInfo *info = toc->streamInfo[id];

    if (info == NULL)
        return -1;

    while (currEntry < info->numOfPacket && info->time[currEntry] < time) {
        currEntry++;
    }
    if (currEntry == info->numOfPacket || currEntry == 0)
        return -1;
    else {
        currEntry--;
        return info->index->table[currEntry].offset;
    }
}


void
MpegSysTocFree(toc)
    MpegSysToc *toc;
{
    StreamInfo **info;

    info = toc->streamInfo;
    DO_N_TIMES(48,
        if (*info != NULL) {
            BitStreamFilterFree((*info)->index);
            FREE((*info)->time);
            FREE(*info);
        }
        info++;
        );
    FREE(toc);
}

int
MpegSysTocWrite(toc, fileName)
    MpegSysToc *toc;
    char *fileName;
{
    int i;
    FILE *file;

    file = fopen(fileName, "wb");
    if (file == NULL) {
        return 0;
    }
    fwrite(toc, sizeof(MpegSysToc), 1, file);
    for (i = 0; i < 48; i++) {
        if (toc->streamInfo[i] != NULL) {
            fwrite(toc->streamInfo[i], sizeof(StreamInfo), 1, file);
            BitStreamFilterWrite(file, toc->streamInfo[i]->index);
            fwrite(toc->streamInfo[i]->time, sizeof(float),
                toc->streamInfo[i]->numOfPacket, file);
        }
    }
    fclose(file);
    return 1;
}

int
MpegSysTocRead(toc, fileName)
    MpegSysToc *toc;
    char *fileName;
{
    int i, n;
    FILE *file;

    file = fopen(fileName, "rb");
    if (file == NULL) {
        return 0;
    }
    /*
     * Free up any lingering memory in the TOC passed in
     */
    for (i = 0; i < 48; i++) {
        if (toc->streamInfo[i] != NULL) {
            BitStreamFilterFree(toc->streamInfo[i]->index);
            FREE(toc->streamInfo[i]->time);
            FREE(toc->streamInfo[i]);
            toc->streamInfo[i] = NULL;
        }
    }

    fread(toc, sizeof(MpegSysToc), 1, file);
    for (i = 0; i < 48; i++) {
        if (toc->streamInfo[i] != NULL) {
            /*
             * Overwrite bogus pointer and fill it in.
             */
            toc->streamInfo[i] = NEW(StreamInfo);
            fread(toc->streamInfo[i], sizeof(StreamInfo), 1, file);

            /*
             * Allocate and fill in the index
             */
            n = toc->streamInfo[i]->numOfPacket;
            toc->streamInfo[i]->max = n;
            toc->streamInfo[i]->index = BitStreamFilterNew(0);
            BitStreamFilterRead(file, toc->streamInfo[i]->index);

            toc->streamInfo[i]->time = NEWARRAY(float, n);
            fread(toc->streamInfo[i]->time, sizeof(float), n, file);
        }
    }
    fclose(file);
    return 1;
}
