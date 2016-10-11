/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * audiomapinitcustom.c
 *
 * C Functions that create new AudioMaps from arrays of values
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void 
AudioMap8To8InitCustom(unsigned char *values, AudioMap * map)
{
    memcpy(map->table, values, 256);
}

void 
AudioMap8To16InitCustom(short *values, AudioMap * map)
{
    memcpy(map->table, values, 256 * sizeof(short));
}

void 
AudioMap16To8InitCustom(unsigned char *values, AudioMap * map)
{
    memcpy(map->table, values, 65536);
}

void 
AudioMap16To16InitCustom(short *values, AudioMap * map)
{
    memcpy(map->table, values, 65536 * sizeof(short));
}
