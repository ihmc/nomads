/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "amapInt.h"

void 
AudioMap8To8SetValue(AudioMap * map, int i, int value)
{
    *(((unsigned char *) map->table) + Index8(i)) = value;
}

void 
AudioMap8To16SetValue(AudioMap * map, int i, int value)
{
    *(((short *) map->table) + Index8(i)) = value;
}

void 
AudioMap16To8SetValue(AudioMap * map, int i, int value)
{
    *(((unsigned char *) map->table) + Index16(i)) = value;
}

void 
AudioMap16To16SetValue(AudioMap * map, int i, int value)
{
    *(((short *) map->table) + Index16(i)) = value;
}
