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
 * audiomapfree.c
 *
 * C Functions that free memory for AudioMaps
 *
 *----------------------------------------------------------------------
 */

#include "amapInt.h"

void 
AudioMapFree(AudioMap * map)
{
    FREE(map->table);
    FREE(map);
}
