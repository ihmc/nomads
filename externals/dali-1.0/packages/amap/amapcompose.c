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
 * audiomapcompose.c
 *
 * Functions that do composition on AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void 
AudioMap8To88To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    unsigned char *arrayPos;
    unsigned char *lookup, *newArrayPos;

    arrayPos = map1->table;
    lookup = map2->table;
    newArrayPos = outMap->table;
    DO_N_TIMES(256,
        *newArrayPos++ = lookup[Index8(*arrayPos++)];
        );
}

void 
AudioMap8To88To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    unsigned char *arrayPos;
    short *lookup, *newArrayPos;

    arrayPos = map1->table;
    lookup = (short *) map2->table;
    newArrayPos = (short *) outMap->table;
    DO_N_TIMES(256,
        *newArrayPos++ = lookup[Index8(*arrayPos++)];
        );
}

void 
AudioMap16To88To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    unsigned char *arrayPos;
    unsigned char *lookup, *newArrayPos;

    arrayPos = map1->table;
    lookup = map2->table;
    newArrayPos = outMap->table;
    DO_N_TIMES(65536,
        *newArrayPos++ = lookup[Index8(*arrayPos++)];
        );
}

void 
AudioMap16To88To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    unsigned char *arrayPos;
    short *lookup, *newArrayPos;

    arrayPos = map1->table;
    lookup = (short *) map2->table;
    newArrayPos = (short *) outMap->table;
    DO_N_TIMES(65536,
        *newArrayPos++ = lookup[Index8(*arrayPos++)];
        );
}

void 
AudioMap8To1616To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    short *arrayPos;
    unsigned char *lookup, *newArrayPos;

    arrayPos = (short *) map1->table;
    lookup = map2->table;
    newArrayPos = outMap->table;
    DO_N_TIMES(256,
        *newArrayPos++ = lookup[Index16(*arrayPos++)];
        );
}

void 
AudioMap8To1616To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    short *arrayPos;
    short *lookup, *newArrayPos;

    arrayPos = (short *) map1->table;
    lookup = (short *) map2->table;
    newArrayPos = (short *) outMap->table;
    DO_N_TIMES(256,
        *newArrayPos++ = lookup[Index16(*arrayPos++)];
        );
}

void 
AudioMap16To1616To8Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    short *arrayPos;
    unsigned char *lookup, *newArrayPos;

    arrayPos = (short *) map1->table;
    lookup = map2->table;
    newArrayPos = outMap->table;
    DO_N_TIMES(65536,
        *newArrayPos++ = lookup[Index16(*arrayPos++)];
        );
}

void 
AudioMap16To1616To16Compose(AudioMap * map1, AudioMap * map2, AudioMap * outMap)
{
    short *arrayPos;
    short *lookup, *newArrayPos;

    arrayPos = (short *) map1->table;
    lookup = (short *) map2->table;
    newArrayPos = (short *) outMap->table;
    DO_N_TIMES(65536,
        *newArrayPos++ = lookup[Index16(*arrayPos++)];
        );
}
