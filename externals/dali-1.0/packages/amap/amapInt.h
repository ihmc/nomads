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
 * amapInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _AUDIOMAP_INT_
#define _AUDIOMAP_INT_

#include "dvmamap.h"
#include "dvmbasic.h"

/* macros to tranform mapping values to table indices */
#define Index8(v) (v)
#define Index16(v) ((v)+32768)

#endif
