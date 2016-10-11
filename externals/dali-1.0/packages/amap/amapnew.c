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
 * audiomapnew.c
 *
 * C Functions that create new AudioMaps from arrays of values
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

AudioMap *
AudioMap8To8New()
{
    AudioMap *newMap;

    newMap = NEW(AudioMap);
    newMap->srcRes = 8;
    newMap->destRes = 8;
    newMap->table = NEWARRAY(unsigned char, 256);

    return newMap;
}

AudioMap *
AudioMap8To16New()
{
    AudioMap *newMap;

    newMap = NEW(AudioMap);
    newMap->srcRes = 8;
    newMap->destRes = 16;
    newMap->table = (unsigned char *) NEWARRAY(short, 256);

    return newMap;
}

AudioMap *
AudioMap16To8New()
{
    AudioMap *newMap;

    newMap = NEW(AudioMap);
    newMap->srcRes = 16;
    newMap->destRes = 8;
    newMap->table = NEWARRAY(unsigned char, 65536);

    return newMap;
}

AudioMap *
AudioMap16To16New()
{
    AudioMap *newMap;

    newMap = NEW(AudioMap);
    newMap->srcRes = 16;
    newMap->destRes = 16;
    newMap->table = (unsigned char *) NEWARRAY(short, 65536);

    return newMap;
}
