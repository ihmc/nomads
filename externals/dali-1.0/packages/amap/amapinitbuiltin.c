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
 * audiomapinitbuiltin.c
 *
 * Functions that form built-in AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void
AudioMap8To8InitIdentity(AudioMap * map)
{
    unsigned char *arrayPos;
    int i;

    arrayPos = map->table;
    i = 0;
    DO_N_TIMES(256,
        *arrayPos++ = i++;
        );
}

void
AudioMap16To16InitIdentity(AudioMap * map)
{
    short *arrayPos;
    int i;

    arrayPos = (short *) map->table;
    i = -32768;
    DO_N_TIMES(65536,
        *arrayPos++ = i++;
        );
}

void
AudioMap8To8InitComplement(AudioMap * map)
{
    unsigned char *arrayPos;
    int i;

    arrayPos = map->table;
    i = 255;
    DO_N_TIMES(256,
        *arrayPos++ = i--;
        );
}

void
AudioMap16To16InitComplement(AudioMap * map)
{
    short *arrayPos;
    int i;

    arrayPos = (short *) map->table;
    i = 32767;
    DO_N_TIMES(65536,
        *arrayPos++ = i--;
        );
}

void
AudioMap16To16InitVolume(AudioMap * map, int maxVal)
{
    short *arrayPos;
    int x, i;

    arrayPos = (short *) map->table;
    for (i = 0; i < (32768 - maxVal); i++) {
        *arrayPos++ = -32768;
    }
    for (x = -maxVal; x <= maxVal; x++) {
        *arrayPos++ = (x * 32767) / maxVal;
    }
    for (i = arrayPos - (short *) map->table; i < 65536; i++) {
        *arrayPos++ = 32767;
    }
}

void
AudioMap16To16InitBigLittleSwap(AudioMap * map)
{
    short *arrayPos;
    int i;

    arrayPos = (short *) map->table;
    i = -32768;
    DO_N_TIMES(65536,
    /* 
     * i = 0xABCD
     * ((i >> 8) & 0xff) ==> (0xFFAB & 0xff) ==> 0xAB   if sign-extended
     *                or ==> (0x00AB & 0xff) ==> 0xAB   if zero-extended
     * ((i & 0xff) << 8) ==> (0xCD << 8) ==> 0xCD00
     * therefore, *arrayPos becomes 0xCDAB
     */
        *arrayPos++ = ((i >> 8) & 0xff) | ((i & 0xff) << 8);
        i++;
        );
}
