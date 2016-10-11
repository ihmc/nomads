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
 * aviaudio.c
 *
 * Functions that deal with avi audio
 *
 *----------------------------------------------------------------------
 */

#include "aviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * AviAudioFrameWrite
 *
 *   Users call this function to write audio data to an AVI stream
 *
 * Returns
 *   0 for sucess, error code otherwise (see dvmavi.h for error codes) 
 * 
 * side effect :
 *   The data is written at the next position in the stream
 *
 *----------------------------------------------------------------------
 */

int
AviAudioFrameWrite(str, audio, sampwritten)
    AviStream *str;
    Audio *audio;
    int *sampwritten;
{
    AviAudioStream *auddata;
    int status, bps, datasize, channels;
    unsigned char *data;

    /*
     * Make sure it's an audio stream
     */
    if (str->type != AVI_STREAM_AUDIO) {
        return DVM_AVI_NOT_AUDIO;
    }
    auddata = (AviAudioStream *) (str->data);
    bps = auddata->bps;
    channels = auddata->channels;
    data = audio->firstSample;
    datasize = (channels * bps * audio->length) / 8;

    status = AVIStreamWrite(str->streamHandle, str->sofar, audio->length,
        data, datasize, 0, sampwritten, NULL);

    if (!status) {
        str->sofar += *sampwritten;
        str->length += *sampwritten;
    }
    return status;
}
