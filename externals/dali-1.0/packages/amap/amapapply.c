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
 * audiomapapply.c
 *
 * C Functions that apply AudioMaps to Audio
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void 
AudioMap8To8Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio)
{
    unsigned char *lookup, *currDest;
    unsigned char *currSrc;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = srcAudio->firstSample;
    currDest = destAudio->firstSample;
    lookup = map->table;

    DO_N_TIMES(srcAudio->length,
        *currDest++ = lookup[Index8(*currSrc++)];
        );
}

void 
AudioMap8To8ApplySome(AudioMap * map, Audio * srcAudio,
    int srcOffset, int srcStride,
    int destOffset, int destStride, Audio * destAudio)
{
    unsigned char *lookup, *currDest;
    unsigned char *currSrc;
    int size;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = srcAudio->firstSample + srcOffset;
    currDest = destAudio->firstSample + destOffset;
    lookup = map->table;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (srcAudio->length - srcOffset + srcStride - 1) / srcStride;

    DO_N_TIMES(size,
        *currDest = lookup[Index8(*currSrc)];
        currSrc += srcStride;
        currDest += destStride;
        );
}

void 
AudioMap8To16Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio)
{
    short *lookup, *currDest;
    unsigned char *currSrc;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = srcAudio->firstSample;
    currDest = (short *) destAudio->firstSample;
    lookup = (short *) map->table;

    DO_N_TIMES(srcAudio->length,
        *currDest++ = lookup[Index8(*currSrc++)];
        );
}

void 
AudioMap8To16ApplySome(AudioMap * map, Audio * srcAudio,
    int srcOffset, int srcStride,
    int destOffset, int destStride, Audio * destAudio)
{
    short *lookup, *currDest;
    unsigned char *currSrc;
    int size;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = srcAudio->firstSample + srcOffset;
    currDest = (short *) destAudio->firstSample;
    currDest += destOffset;
    lookup = (short *) map->table;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (srcAudio->length - srcOffset + srcStride - 1) / srcStride;

    DO_N_TIMES(size,
        *currDest = lookup[Index8(*currSrc)];
        currSrc += srcStride;
        currDest += destStride;
        );
}

void 
AudioMap16To8Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio)
{
    unsigned char *lookup, *currDest;
    short *currSrc;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = (short *) srcAudio->firstSample;
    currDest = destAudio->firstSample;
    lookup = map->table;

    DO_N_TIMES(srcAudio->length,
        *currDest++ = lookup[Index16(*currSrc++)];
        );
}

void 
AudioMap16To8ApplySome(AudioMap * map, Audio * srcAudio,
    int srcOffset, int srcStride,
    int destOffset, int destStride, Audio * destAudio)
{
    unsigned char *lookup, *currDest;
    short *currSrc;
    int size;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = (short *) srcAudio->firstSample;
    currSrc += srcOffset;
    currDest = destAudio->firstSample + destOffset;
    lookup = map->table;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (srcAudio->length - srcOffset + srcStride - 1) / srcStride;

    DO_N_TIMES(size,
        *currDest = lookup[Index16(*currSrc)];
        currSrc += srcStride;
        currDest += destStride;
        );
}

void 
AudioMap16To16Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio)
{
    short *lookup, *currDest;
    short *currSrc;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = (short *) srcAudio->firstSample;
    currDest = (short *) destAudio->firstSample;
    lookup = (short *) map->table;

    DO_N_TIMES(srcAudio->length,
        *currDest++ = lookup[Index16(*currSrc++)];
        );
}

void 
AudioMap16To16ApplySome(AudioMap * map, Audio * srcAudio,
    int srcOffset, int srcStride,
    int destOffset, int destStride, Audio * destAudio)
{
    short *lookup, *currDest;
    short *currSrc;
    int size;

    /* 
     * apply the audio map to each sample in srcAudio 
     * and store the result in destAudio
     */
    currSrc = (short *) srcAudio->firstSample;
    currSrc += srcOffset;
    currDest = (short *) destAudio->firstSample;
    currDest += destOffset;
    lookup = (short *) map->table;
    /*
     * size = ceil(1.0*(length - offset) / stride)
     */
    size = (srcAudio->length - srcOffset + srcStride - 1) / srcStride;

    DO_N_TIMES(size,
        *currDest = lookup[Index16(*currSrc)];
        currSrc += srcStride;
        currDest += destStride;
        );
}
