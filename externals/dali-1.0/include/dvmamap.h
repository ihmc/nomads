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
 * dvmaudiomap.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_AUDIOMAP_H
#define _DVM_AUDIOMAP_H

#include "dvmbasic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 *--------------------------------------------------------------------
 *
 * This is the main C library interface header file for AUDIOMAP package.
 * It contains a set of commmands for AudioMaps manipulation
 * and some built-in mappings
 * These maps can be applied to Audio buffer
 *
 * Haye Chan Jan 98
 *---------------------------------------------------------------------
 */

    typedef struct AudioMap {
        unsigned char *table;   /* table of lookup values */
        int srcRes;             /* bits per Sample in source buffer */
        int destRes;            /* bits per Sample in destination buffer */
    } AudioMap;

#define AudioMapGetDestRes(am)        ((am)->destRes)
#define AudioMapGetSrcRes(am)         ((am)->srcRes)

/* new and free maps */
    AudioMap *AudioMap8To8New();
    AudioMap *AudioMap8To16New();
    AudioMap *AudioMap16To8New();
    AudioMap *AudioMap16To16New();
    void AudioMapFree(AudioMap * map);

/* copy commands */
    void AudioMap8To8Copy(AudioMap * srcMap, AudioMap * destMap);
    void AudioMap8To16Copy(AudioMap * srcMap, AudioMap * destMap);
    void AudioMap16To8Copy(AudioMap * srcMap, AudioMap * destMap);
    void AudioMap16To16Copy(AudioMap * srcMap, AudioMap * destMap);

/* table info commands */
    int AudioMap8To8GetValue(AudioMap * map, int i);
    int AudioMap8To16GetValue(AudioMap * map, int i);
    int AudioMap16To8GetValue(AudioMap * map, int i);
    int AudioMap16To16GetValue(AudioMap * map, int i);

/* set commands */
    void AudioMap8To8SetValue(AudioMap * map, int i, int value);
    void AudioMap8To16SetValue(AudioMap * map, int i, int value);
    void AudioMap16To8SetValue(AudioMap * map, int i, int value);
    void AudioMap16To16SetValue(AudioMap * map, int i, int value);

/* maps composition */
    void AudioMap8To88To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap8To88To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap16To88To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap16To88To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap8To1616To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap8To1616To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap16To1616To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);
    void AudioMap16To1616To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap);

/* apply c functions */
    void AudioMap8To8Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio);
    void AudioMap8To8ApplySome(AudioMap * map, Audio * srcAudio,
        int srcOffset, int srcStride,
        int destOffset, int destStride,
        Audio * destAudio);
    void AudioMap8To16Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio);
    void AudioMap8To16ApplySome(AudioMap * map, Audio * srcAudio,
        int srcOffset, int srcStride,
        int destOffset, int destStride,
        Audio * destAudio);
    void AudioMap16To8Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio);
    void AudioMap16To8ApplySome(AudioMap * map, Audio * srcAudio,
        int srcOffset, int srcStride,
        int destOffset, int destStride,
        Audio * destAudio);
    void AudioMap16To16Apply(AudioMap * map, Audio * srcAudio, Audio * destAudio);
    void AudioMap16To16ApplySome(AudioMap * map, Audio * srcAudio,
        int srcOffset, int srcStride,
        int destOffset, int destStride,
        Audio * destAudio);

/* create new maps from array of values */
    void AudioMap8To8InitCustom(unsigned char *values, AudioMap * map);
    void AudioMap8To16InitCustom(short *values, AudioMap * map);
    void AudioMap16To8InitCustom(unsigned char *values, AudioMap * map);
    void AudioMap16To16InitCustom(short *values, AudioMap * map);

/* forms built-in map */
    void AudioMap8To8InitIdentity(AudioMap * map);
    void AudioMap16To16InitIdentity(AudioMap * map);
    void AudioMap16To16InitVolume(AudioMap * map, int maxVal);
    void AudioMap8To8InitComplement(AudioMap * map);
    void AudioMap16To16InitComplement(AudioMap * map);
    void AudioMap16To16InitBigLittleSwap(AudioMap * map);
    void AudioMap8To16InitULawToLinear(AudioMap * map);
    void AudioMap8To16InitALawToLinear(AudioMap * map);
    void AudioMap16To8InitLinearToULaw(AudioMap * map);
    void AudioMap16To8InitLinearToALaw(AudioMap * map);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
