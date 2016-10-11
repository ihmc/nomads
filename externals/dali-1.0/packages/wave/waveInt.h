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
 * waveInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _WAVE_INT_
#define _WAVE_INT_

#include "dvmwave.h"
#include "bitparser.h"

#ifndef WIN32
#define WAVE_FORMAT_PCM                 (0x0001)
#endif //WIN32
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)

/*
 * macro to read 4 character from bitparser and store it at
 * a string of length 4
 */
#define Read4Char(bp, str4) {   \
        Bp_GetByte(bp, str4[0]);        \
        Bp_GetByte(bp, str4[1]);        \
        Bp_GetByte(bp, str4[2]);        \
        Bp_GetByte(bp, str4[3]);        \
}

/*
 * macro to write 4 character to bitparser from a string of
 * length 4
 */
#define Write4Char(bp, str4) {  \
        Bp_PutByte(bp, str4[0]);        \
        Bp_PutByte(bp, str4[1]);        \
        Bp_PutByte(bp, str4[2]);        \
        Bp_PutByte(bp, str4[3]);        \
}

#endif
