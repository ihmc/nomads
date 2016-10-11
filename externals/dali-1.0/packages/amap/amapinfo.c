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
 * audiomapinfo.c
 *
 * C Functions that extract information from AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"


int 
AudioMap8To8GetValue(AudioMap * map, int i)
{
    return *(((unsigned char *) map->table) + Index8(i));
}

int 
AudioMap8To16GetValue(AudioMap * map, int i)
{
    return *(((short *) map->table) + Index8(i));
}

int 
AudioMap16To8GetValue(AudioMap * map, int i)
{
    return *(((unsigned char *) map->table) + Index16(i));
}

int 
AudioMap16To16GetValue(AudioMap * map, int i)
{
    return *(((short *) map->table) + Index16(i));
}
