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
 * waveout.c
 * defines functions that play wave audio
 * header
 *     Haye Chan   May 1998
 *
 *----------------------------------------------
 */

#include "waveInt.h"
static HWAVEOUT SThWaveOut;
static WAVEHDR STWaveHdr;
static WAVEHDR* STpWaveHdr = &STWaveHdr;

/* 
 * open wave output device for format specified by <hdr>
 * returns DVM_WAVE_OK if all is well,
 *         DVM_WAVE_ALREADY_OPEN if device is already open
 *         DVM_WAVE_ERROR if device not opened successfully
 */
int WaveOutOpen(WaveHdr* hdr)
{
    WAVEFORMATEX format;
    MMRESULT result;

    /*
     * Check if device is already in use
     */
    if (SThWaveOut != NULL) {
        return DVM_WAVE_ALREADY_OPEN;
    }

    /*
     * Copy format into windows standard format
     */
    format.wFormatTag = hdr->format;
    format.nChannels = hdr->numOfChan;
    format.nSamplesPerSec = hdr->samplesPerSec;
    format.nBlockAlign = hdr->blockAlign;
    format.wBitsPerSample = hdr->bitsPerSample;
    format.nAvgBytesPerSec = hdr->bytesPerSec;
    format.cbSize = 0;              // no additional format chunk

    /*
     * open device with auto-Mapper, no callback
     */
    sndPlaySound(NULL, 0);      // stop any currently playing sound
    result = waveOutOpen(&SThWaveOut, WAVE_MAPPER, &format, 0L, 0L, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        SThWaveOut = NULL;
        return DVM_WAVE_ERROR;
    }

    return DVM_WAVE_OK;
}

/*
 * Close a previously open wave audio device
 *
 */
int WaveOutClose()
{
    if (SThWaveOut == NULL) {
        return DVM_WAVE_NOT_OPEN;
    }

    if (STpWaveHdr->dwFlags == WHDR_DONE) {
        return DVM_WAVE_ERROR;
    }
    waveOutUnprepareHeader(SThWaveOut, STpWaveHdr, sizeof(WAVEHDR));
    waveOutClose(SThWaveOut);

    SThWaveOut = NULL;
    return DVM_WAVE_OK;
}

/*
 * prepare the header and audio block for playing
 * should be called only after WaveOutOpen is called
 */
int WaveAudioPrepPlay(Audio* audio)
{
    MMRESULT result;

    if (SThWaveOut == NULL) {
        return DVM_WAVE_NOT_OPEN;
    }
    waveOutUnprepareHeader(SThWaveOut, STpWaveHdr, sizeof(WAVEHDR));

    STpWaveHdr->lpData = audio->firstSample;
    STpWaveHdr->dwBufferLength = audio->length;
    STpWaveHdr->dwFlags = 0L;   // has to be intialized to 0
    STpWaveHdr->dwLoops = 0L;   // no loops

    /*
     * prepare waveHdr
     */
    result = waveOutPrepareHeader(SThWaveOut, STpWaveHdr, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR) {
        return DVM_WAVE_OK;
    } else {
        return DVM_WAVE_ERROR;
    }
}

/*
 * Send the data to the audio device
 */
int WaveAudioPlay(int length)
{
    MMRESULT result;

    if (SThWaveOut == NULL) {
        return DVM_WAVE_NOT_OPEN;
    }
    STpWaveHdr->dwBufferLength = length;
    result = waveOutWrite(SThWaveOut, STpWaveHdr, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR) {
        return DVM_WAVE_OK;
    } else {
        return DVM_WAVE_ERROR;
    }
}

/*
 * Return true (non-zero) if the wave audio played by previous call
 * to WaveAudioPlay() is done, false otherwise
 */
int WaveOutDone()
{
    if (SThWaveOut == NULL) {
        return 1;
    }
    return ((STpWaveHdr->dwFlags & WHDR_DONE) == 1);
}
