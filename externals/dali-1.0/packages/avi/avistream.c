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
 * avistream.c
 *
 * Functions that manipulate avi streams
 *
 *----------------------------------------------------------------------
 */

#include "aviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * AviStreamOpen 
 *
 *   Users call this function to prepare to read a stream from an AVI file
 *
 * Returns
 *     A avistream handle, to be used to read frames from the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use avi_stream_close to free this.
 *
 *----------------------------------------------------------------------
 */

int
AviStreamOpen(aviFile, streamnum, strPtr)
    AviFile *aviFile;
    int streamnum;
    AviStream **strPtr;
{
    AviStream *rv;
    AVISTREAMINFO si;
    int formatsize;
    char genFormat[1024];
    int status;

    /*
     * Check in streamnum is valid, allocate a new structure for
     * the return value.
     */
    if ((streamnum < 0) || (streamnum >= aviFile->numStreams)) {
        return DVM_AVI_BAD_STREAM_NUM;
    }
    rv = NEW(AviStream);

    /*
     * Call video for windows to get a handle to the stream and
     * information about the stream.
     */
    status = AVIFileGetStream(aviFile->aviHandle, &(rv->streamHandle), 0L,
        streamnum);
    if (status) {
        FREE(rv);
        return status;
    }
    memset((char *) &si, 0, sizeof(AVISTREAMINFO));
    status = AVIStreamInfo(rv->streamHandle, &si, sizeof(AVISTREAMINFO));
    if (status) {
        FREE(rv);
        return status;
    }
    /*
     * Check the stream type
     */
    if (si.fccType == streamtypeVIDEO) {        /* Video stream */
        LPBITMAPINFOHEADER lpbi;
        AviVideoStream *viddata;

        viddata = NEW(AviVideoStream);
        rv->data = (void *) viddata;
        rv->type = AVI_STREAM_VIDEO;
        rv->length = si.dwLength;

        if (si.dwScale) {
            viddata->fps = (short) (si.dwRate / si.dwScale);
        }
        viddata->gf = NULL;

        /* Read formatting info */
        formatsize = sizeof(genFormat);
        status = AVIStreamReadFormat(rv->streamHandle, si.dwStart, genFormat,
            &formatsize);
        if (status) {
            FREE(viddata);
            FREE(rv);
            return status;
        }
        lpbi = (LPBITMAPINFOHEADER) genFormat;
        if (lpbi->biBitCount != 24) {
            FREE(viddata);
            FREE(rv);
            return DVM_AVI_NOT_RGB;
        }
        viddata->width = (short) lpbi->biWidth;
        viddata->height = (short) lpbi->biHeight;
        viddata->gf = NULL;
        viddata->cs = NULL;
        viddata->fb = NULL;
    } else if (si.fccType == streamtypeAUDIO) {         /* AUDIO stream */
        rv->type = AVI_STREAM_AUDIO;
        rv->length = si.dwLength;
        /* TODO: Get WAVFORMAT info and fill in */
    } else {                    /* unsupported stream type */
        FREE(rv);
        return AVIERR_UNSUPPORTED;
    }

    /*
     * Fill in the values
     */
    rv->start = rv->sofar = (short) si.dwStart;
    rv->codec = si.fccHandler;
    *strPtr = rv;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * AviStreamClose
 *
 *    Free an AviStream allocated by AviStreamRead or AviStreamCreate
 *
 * Returns
 *     none
 * 
 * side effect :
 *     the memory allocated for the AVI stream is freed
 *
 *----------------------------------------------------------------------
 */

void
AviStreamClose(str)
    AviStream *str;
{
    AviVideoStream *viddata;
    AviAudioStream *auddata;

    if (str->type == AVI_STREAM_VIDEO) {
        viddata = (AviVideoStream *) (str->data);
        if (viddata->cs) {
            AVIStreamRelease(viddata->cs);
        }
        if (viddata->fb) {
            FREE(viddata->fb);
        }
    } else if (str->type == AVI_STREAM_AUDIO) {
        auddata = (AviAudioStream *) (str->data);
        if (auddata->cs) {
            AVIStreamRelease(auddata->cs);
        }
    }
    /*
     * Inform video for windows, and free the memory
     */
    AVIStreamRelease(str->streamHandle);
    if (str->data) {
        FREE(str->data);
    }
    FREE(str);
}

/*
 *----------------------------------------------------------------------
 *
 * AviStreamStartDecode
 *     Users call this function to begin the decoding process.
 *
 * return 
 *     0 if ok, error (decompressor not found) otherwise
 * 
 * side effect :
 *     If successful, memory is allocated for a a getframe object.
 *     Use AviStreamStopDecode to clean up
 *
 *----------------------------------------------------------------------
 */

int
AviStreamStartDecode(str)
    AviStream *str;
{
    AviVideoStream *viddata;

    /*
     * Init if it hasn't been done before (idempotence)
     */
    if (str->type == AVI_STREAM_VIDEO) {
        viddata = (AviVideoStream *) (str->data);
        if (viddata->gf == NULL) {
            viddata->gf = AVIStreamGetFrameOpen(str->streamHandle, NULL);
            if (viddata->gf == NULL) {
                /* decompressor not found */
                return AVIERR_NOCOMPRESSOR;
            }
        }
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * AviStreamStopDecode
 *
 *  Users call this function to stop the decoding process
 *
 * Returns
 *     none
 * 
 * side effect :
 *     memory allocated for the getframe is freed
 *
 *----------------------------------------------------------------------
 */

void
AviStreamStopDecode(str)
    AviStream *str;
{
    AviVideoStream *viddata;

    /*
     * free if it hasnt been done before (idempotence)
     */
    if (str->type == AVI_STREAM_VIDEO) {
        viddata = (AviVideoStream *) (str->data);
        if (viddata->gf) {
            return;
        }
        AVIStreamGetFrameClose(viddata->gf);
        viddata->gf = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AviVideoStreamCreate
 *
 *    Creates a new video stream with the specified parameters.
 *
 * Returns
 *     A avistream handle, to be used to write frames to the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use AviStreamFree to free this.
 *
 *----------------------------------------------------------------------
 */
int
AviVideoStreamCreate(aviFile, codec, w, h, fps, keyinterval, quality, bitrate, strPtr)
    AviFile *aviFile;
    int codec;                  /* FourCC name of the codec */
    int w, h, fps;
    int keyinterval;            /* Maximum period between video key frames. Default=30 */
    int quality;                /* Quality value passed to a video compressor. Default=85 */
    int bitrate;                /* Video compressor data rate.  Default=0 (unconstrained) */
    AviStream **strPtr;
{
    AviStream *rv;
    AVISTREAMINFO si;
    AVICOMPRESSOPTIONS copts;
    BITMAPINFOHEADER bmih;
    AviVideoStream *viddata;
    int status;

    /*
     * Create new stream header
     */
    rv = NEW(AviStream);
    if (rv == NULL) {
        return AVIERR_MEMORY;
    }
    viddata = NEW(AviVideoStream);
    if (viddata == NULL) {
        FREE(rv);
        return AVIERR_MEMORY;
    }
    viddata->gf = NULL;
    viddata->cs = NULL;
    viddata->fb = NULL;
    viddata->width = w;
    viddata->height = h;
    viddata->fps = fps;
    viddata->keyinterval = keyinterval;


    /*
     * (XXX) Todo: pass this data in
     */
    viddata->fb = NEWARRAY(unsigned char, w * h * 3);
    if (viddata->fb == NULL) {
        FREE(viddata);
        FREE(rv);
        return AVIERR_MEMORY;
    }
    /*
     * initialize the structures we just allocated
     */
    rv->type = AVI_STREAM_VIDEO;
    rv->length = 0;
    rv->start = 0;
    rv->sofar = 0;
    rv->codec = codec;
    rv->data = (void *) viddata;

    /*
     * Prepare the STREAMINFO fields and call video for windows
     */
    memset(&si, 0, sizeof(si));
    si.fccType = streamtypeVIDEO;
    si.fccHandler = 0;
    si.dwScale = 1;
    si.dwRate = fps;
    si.dwSuggestedBufferSize = w * h * 3;
    SetRect(&si.rcFrame, 0, 0, w, h);
    status = AVIFileCreateStream(aviFile->aviHandle, &(rv->streamHandle), &si);
    if (status != AVIERR_OK) {
        FREE(rv->data);
        FREE(rv);
        return status;
    }
    /*
     * Set up the compress options and the cstream
     */
    memset(&copts, 0, sizeof(copts));
    copts.fccType = streamtypeVIDEO;
    copts.fccHandler = codec;
    copts.dwKeyFrameEvery = keyinterval;
    copts.dwQuality = quality;
    copts.dwBytesPerSecond = bitrate;
    copts.dwFlags = 0;
    if (bitrate && keyinterval) {
        copts.dwFlags = AVICOMPRESSF_DATARATE | AVICOMPRESSF_KEYFRAMES;
    } else if (bitrate == 0) {
        copts.dwFlags = AVICOMPRESSF_DATARATE;
    } else if (keyinterval) {
        copts.dwFlags = AVICOMPRESSF_KEYFRAMES;
    }
    status = AVIMakeCompressedStream(&(viddata->cs), rv->streamHandle,
        &copts, NULL);
    if (status != AVIERR_OK) {
        AVIStreamRelease(rv->streamHandle);
        FREE(rv->data);
        FREE(rv);
        return status;
    }
    /*
     * Finally, set the format and alloc framebufer memory
     */
    memset(&bmih, 0, sizeof(bmih));
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = w;
    bmih.biHeight = h;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = BI_RGB;
    status = AVIStreamSetFormat(viddata->cs, 0, &bmih, sizeof(BITMAPINFOHEADER));
    if (status != AVIERR_OK) {
        AVIStreamRelease(rv->streamHandle);
        AVIStreamRelease(viddata->cs);
        FREE(rv->data);
        FREE(rv);
        return status;
    }
    aviFile->numStreams++;
    *strPtr = rv;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * AviAudioStreamCreate
 *
 *    Creates a new audio stream with the specified parameters.
 *
 * Returns
 *     A avistream handle, to be used to write audio to the
 *     specified stream
 * 
 * side effect :
 *     memory is allocated for a new avistream object.
 *     Use AviStreamFree to free this.
 *
 *----------------------------------------------------------------------
 */
int
AviAudioStreamCreate(aviFile, strPtr, nc, bps, rate)
    AviFile *aviFile;
    AviStream **strPtr;
    short nc;
    short bps;
    int rate;
{
    AviStream *ra;
    AVISTREAMINFO si;
    LPWAVEFORMATEX infmt;
    AviAudioStream *auddata;
    int status;

    /*
     * Create new stream header
     */
    ra = NEW(AviStream);
    if (ra == NULL) {
        return AVIERR_MEMORY;
    }
    auddata = NEW(AviAudioStream);
    if (auddata == NULL) {
        FREE(ra);
        return AVIERR_MEMORY;
    }

    /*
     * Fill in the source format
     */
    auddata->channels = nc;
    auddata->bps = bps;
    auddata->rate = rate;

    infmt = (LPWAVEFORMATEX) & (auddata->afmt);
    memset((char *) infmt, 0, sizeof(WAVEFORMATEX));
    infmt->wFormatTag = WAVE_FORMAT_PCM;
    infmt->nChannels = nc;
    infmt->nSamplesPerSec = rate;
    infmt->nAvgBytesPerSec = rate * nc * bps / 8;
    infmt->nBlockAlign = (nc * bps) / 8;
    infmt->wBitsPerSample = nc * bps;
    infmt->cbSize = 0;

    /*
     * Now that the audio format is prepared,
     * initialize the stream struct
     */
    ra->type = AVI_STREAM_AUDIO;
    ra->length = 0;
    ra->start = 0;
    ra->sofar = 0;
    ra->codec = 0;
    ra->data = (void *) auddata;

    /*
     * Prepare the STREAMINFO fields and call video for windows
     */
    memset(&si, 0, sizeof(si));
    si.fccType = streamtypeAUDIO;
    si.fccHandler = 0;
    si.dwScale = infmt->nBlockAlign;
    si.dwRate = infmt->nSamplesPerSec*
		infmt->nChannels*
		infmt->nBlockAlign;
    si.dwQuality=(DWORD)-1;
    si.dwSampleSize=infmt->nBlockAlign;
    status = AVIFileCreateStream(aviFile->aviHandle, &(ra->streamHandle), &si);
    if (status != AVIERR_OK) {
        FREE(ra->data);
        FREE(ra);
        return status;
    }

    /*
     * Apply the audio format
     * to the stream.
     */
    status = AVIStreamSetFormat(ra->streamHandle, 0, (LPVOID)infmt,
				sizeof(WAVEFORMATEX));
    if (status != AVIERR_OK) {
        AVIStreamRelease(ra->streamHandle);
        FREE(ra->data);
        FREE(ra);
        return status;
    }

    aviFile->numStreams++;
    *strPtr = ra;
    return 0;
}
