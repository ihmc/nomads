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
 * audiomapcopy.c
 *
 * C Functions that copy AudioMaps to new memory location
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void 
AudioMap8To8Copy(AudioMap * srcMap, AudioMap * destMap)
{
    memcpy(destMap->table, srcMap->table, 256);
}

void 
AudioMap8To16Copy(AudioMap * srcMap, AudioMap * destMap)
{
    memcpy(destMap->table, srcMap->table, 256 * sizeof(short));
}

void 
AudioMap16To8Copy(AudioMap * srcMap, AudioMap * destMap)
{
    memcpy(destMap->table, srcMap->table, 65536);
}

void 
AudioMap16To16Copy(AudioMap * srcMap, AudioMap * destMap)
{
    memcpy(destMap->table, srcMap->table, 65536 * sizeof(short));
}
