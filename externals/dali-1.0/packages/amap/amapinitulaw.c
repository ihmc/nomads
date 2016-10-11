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
 * amapinitulaw.c
 *
 * Functions that form built-in ulaw conversion AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"
#include "ulawTable.h"

void 
AudioMap16To8InitLinearToULaw(AudioMap * map)
{
    memcpy(map->table, linearToULawTable, 65536 * sizeof(unsigned char));
}

void 
AudioMap8To16InitULawToLinear(AudioMap * map)
{
    memcpy(map->table, uLawToLinearTable, 256 * sizeof(short));
}
