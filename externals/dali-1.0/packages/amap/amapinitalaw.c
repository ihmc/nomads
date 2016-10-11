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
 * amapinitalaw.c
 *
 * Functions that form built-in a-law conversion AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"
#include "alawTable.h"

void 
AudioMap8To16InitALawToLinear(AudioMap * map)
{
    memcpy(map->table, aLawToLinearTable, 256 * sizeof(short));
}


void 
AudioMap16To8InitLinearToALaw(AudioMap * map)
{
    memcpy(map->table, linearToALawTable, 65536 * sizeof(unsigned char));
}
