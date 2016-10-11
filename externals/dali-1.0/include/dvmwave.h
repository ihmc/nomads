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
 *----------------------------------------------------------------------
 *
 * dvmwave.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_WAVE_H_
#define _DVM_WAVE_H_

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#endif                          //WIN32

/*
 *--------------------------------------------------------------------
 *
 * This is the main C library interface header file for WAVE package.
 * It only contains a set of wave header commands for reading header
 * chunk of wave file
 *
 * Haye Chan Jan 98
 *---------------------------------------------------------------------
 */

    typedef struct WaveHdr {
        int format;             // format code of wave file, e.g. PCM

        int numOfChan;          // 1 = mono, 2 = stereo

        int samplesPerSec;      // sampling rate of audio in Hz

        int bytesPerSec;        // samplesPerSec * blockAlign

        int blockAlign;         // numOfChan * bitsPerSample / 8

        int bitsPerSample;      // resolution, 8-bit or 16-bit

        int dataLen;            // number of bytes of audio data

        unsigned char *extra;
        /*
         * extra - dynamically allocated space for extra information
         *         which is not anticipated in the primitive implementation.
         *         This field meant to enable future addition support of 
         *         other wave formats.
         */
        int extralen;           /* length of this data */
    } WaveHdr;


#define DVM_WAVE_OK                     (0)
#define DVM_WAVE_ERROR                  (1)
#define DVM_WAVE_ALREADY_OPEN           (2)
#define DVM_WAVE_NOT_OPEN               (3)

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM                 (0x0001)
#endif
#ifndef WAVE_FORMAT_ALAW
#define WAVE_FORMAT_ALAW                (0x0006)
#endif
#ifndef WAVE_FORMAT_MULAW
#define WAVE_FORMAT_MULAW               (0x0007)
#endif

#define WaveHdrGetFormat(hdr)        ((hdr)->format)
#define WaveHdrGetNumOfChannel(hdr)  ((hdr)->numOfChan)
#define WaveHdrGetSamplesPerSec(hdr) ((hdr)->samplesPerSec)
#define WaveHdrGetBytesPerSec(hdr)   ((hdr)->bytesPerSec)
#define WaveHdrGetBlockAlign(hdr)    ((hdr)->blockAlign)
#define WaveHdrGetBitsPerSample(hdr) ((hdr)->bitsPerSample)
#define WaveHdrGetDataLen(hdr)       ((hdr)->dataLen)

#define WaveHdrSetFormat(hdr, x) (hdr)->format = x
#define WaveHdrSetNumOfChannel(hdr, x) (hdr)->numOfChan = x
#define WaveHdrSetSamplesPerSec(hdr, x) (hdr)->samplesPerSec = x
#define WaveHdrSetBytesPerSec(hdr, x) (hdr)->bytesPerSec = x
#define WaveHdrSetBlockAlign(hdr, x) (hdr)->blockAlign = x
#define WaveHdrSetBitsPerSample(hdr, x) (hdr)->bitsPerSample = x
#define WaveHdrSetDataLen(hdr, x) (hdr)->dataLen = x

/* wavehdr.c */
    WaveHdr *WaveHdrNew();
    int WaveHdrParse(BitParser * bp, WaveHdr * hdr);
    void WaveHdrFree(WaveHdr * hdr);
    int WaveHdrEncode(WaveHdr * hdr, BitParser * bp);

#ifdef WIN32
/* waveout.c */
    int WaveOutOpen(WaveHdr * hdr);
    int WaveOutClose();
    int WaveAudioPrepPlay(Audio * audio);
    int WaveAudioPlay(int length);
    int WaveOutDone();
#endif                          //WIN32

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
