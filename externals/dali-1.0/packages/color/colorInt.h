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
 * colorInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _COLOR_INT_
#define _COLOR_INT_

#include "dvmcolor.h"

#if 0
#define HASH(size, i, r, g, b, h) {\
    long __h = ((((long)table33023[r] +\
                (long)table30013[g] +  \
                (long)table27011[b]) & 0x7fffffff));\
    (h) = (__h % size + i*(1 + __h % (size - 1))) % size;\
}
#endif

#define PACK(r, g, b) ((int)0 | (r << 16) | (g << 8) | b)
#define HASH(size, i, __h, h) (h) = ((__h % size) + i*(1 +(__h % (size - 1)))) % size

#endif
