/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*
 *----------------------------------------------
 *
 * wavehdr.c
 * defines functions that manipulate WAV files
 * header
 *     Haye Chan   Nov 1997
 *
 *----------------------------------------------
 */

#include "waveInt.h"

/*
 * C subroutine called by WaveHdrNewCmd in wavecmd.c
 */
WaveHdr *WaveHdrNew()
{
    WaveHdr *hdr;

    hdr = NEW(WaveHdr);

    hdr->format = 1;
    hdr->numOfChan = 1;
    hdr->samplesPerSec = 11025;
    hdr->bytesPerSec = 11025;
    hdr->blockAlign = 1;
    hdr->bitsPerSample = 8;
    hdr->dataLen = 0;
    hdr->extra = NULL;
    hdr->extralen = 0;

    return hdr;
}
  
/*
 * C subroutine called by WaveHdrReadCmd in wavecmd.c
 */
int WaveHdrParse(bp, hdr)
    BitParser *bp;
    WaveHdr *hdr;
{
    int ckLen;
    char hdrStr[4];
    unsigned char *bpStart;

    bpStart = bp->offsetPtr;

    Read4Char(bp, hdrStr);
    if (strncmp(hdrStr, "RIFF", 4) != 0) {
        return 0;
    }
    
    Bp_FlushInt(bp);
    Read4Char(bp, hdrStr);
    if (strncmp(hdrStr, "WAVE", 4) != 0) {
        return 0;
    }

    /*
     * read format chunk
     * offset = 12 now.
     */
    Read4Char(bp, hdrStr);
    if (strncmp(hdrStr, "fmt ", 4) != 0) {
        return 0;
    }
    Bp_GetLittleInt(bp, ckLen);
    Bp_GetLittleShort(bp, (hdr->format));
    Bp_GetLittleShort(bp, (hdr->numOfChan));
    Bp_GetLittleInt(bp, (hdr->samplesPerSec));
    Bp_GetLittleInt(bp, (hdr->bytesPerSec));
    Bp_GetLittleShort(bp, (hdr->blockAlign));


    if ((hdr->format == WAVE_FORMAT_PCM) ||
        (hdr->format == WAVE_FORMAT_ALAW) ||
        (hdr->format == WAVE_FORMAT_MULAW)) {
        /*
         * PCM, a-law, u-law format
         */
        Bp_GetLittleShort(bp, (hdr->bitsPerSample));
        Bp_FlushBytes(bp, ckLen - 16);
    } else {
        /*
         * other format
         */
        Bp_FlushBytes(bp, ckLen - 14);
    }

    Read4Char(bp, hdrStr);
    /*
     * read other chunks
     * --------------------------------------------
     * read fact chunk
     */
    if (strncmp(hdrStr, "fact", 4) == 0) {
        Bp_GetLittleInt(bp, ckLen);
        Bp_FlushBytes(bp, ckLen);
        Read4Char(bp, hdrStr);
    }
     
    /*
     * read cue chunk
     */
    if (strncmp(hdrStr, "cue ", 4) == 0) {
        Bp_GetLittleInt(bp, ckLen);
        Bp_FlushBytes(bp, ckLen);
        Read4Char(bp, hdrStr);
    }

    /*
     * read playlist chunk
     */
    if (strncmp(hdrStr, "plst", 4) == 0) {
        Bp_GetLittleInt(bp, ckLen);
        Bp_FlushBytes(bp, ckLen);
        Read4Char(bp, hdrStr);
    }

    /*
     * read assoc-data chunk
     */
    if (strncmp(hdrStr, "adtl", 4) == 0) {
        Bp_GetLittleInt(bp, ckLen);
        Bp_FlushBytes(bp, ckLen);
        Read4Char(bp, hdrStr);
    }
    
    /*
     * read data chunk
     */
    if (strncmp(hdrStr, "data", 4) == 0) {
        Bp_GetLittleInt(bp, (hdr->dataLen));
    }

    return (bp->offsetPtr - bpStart);
}

/*
 * C subroutine called by WaveHdrFreeCmd in wavecmd.c
 */
void WaveHdrFree (hdr)
    WaveHdr *hdr;
{
    free(hdr->extra);
    free(hdr);
}

/*
 * C subroutine called by WaveHdrWriteCmd in wavecmd.c
 */
int WaveHdrEncode(hdr, bp)
    WaveHdr *hdr;
    BitParser *bp;
{
    char hdrStr[4];
    unsigned char *bpStart;
    int padding = 0;

    if (hdr->extralen) {
        padding = hdr->extralen + 2;
    }

    bpStart = bp->offsetPtr;

    /*
     * preparing header buffer
     */
    strncpy(hdrStr, "RIFF", 4);
    Write4Char(bp, hdrStr);
    Bp_PutLittleInt(bp, hdr->dataLen+36+padding);
    strncpy(hdrStr, "WAVE", 4);
    Write4Char(bp, hdrStr);
    strncpy(hdrStr, "fmt ", 4);
    Write4Char(bp, hdrStr);
    Bp_PutLittleInt(bp, 16+padding);
    Bp_PutLittleShort(bp, hdr->format);
    Bp_PutLittleShort(bp, hdr->numOfChan);
    Bp_PutLittleInt(bp, hdr->samplesPerSec);
    Bp_PutLittleInt(bp, hdr->bytesPerSec);
    Bp_PutLittleShort(bp, hdr->blockAlign);
    Bp_PutLittleShort(bp, hdr->bitsPerSample);
    if (hdr->extralen) {
        unsigned char *d;
        d = hdr->extra;
        Bp_PutLittleShort(bp, (short)(hdr->extralen));
        DO_N_TIMES(hdr->extralen, Bp_PutByte(bp,*d++));
    }
    strncpy(hdrStr, "data", 4);
    Write4Char(bp, hdrStr);
    Bp_PutLittleInt(bp, hdr->dataLen);

    return (bp->offsetPtr - bpStart);
}
