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
 * gifct.c
 *
 * Steve Weiss January 98
 *
 * Parsing and encoding a gif color table
 *
 *----------------------------------------------------------------------
 */

#include "gifInt.h"

/* 
 * This table maps the number of entry in the color table to the number 
 * of bits requires to represent each entry.
 */
char theBppTable[257] = {
    1, 
    1, 1, 
    2, 2, 
    3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 
};

int
GifCtParse (bp, size, red, green, blue)
    BitParser* bp;
    int size;
    ImageMap *red, *green, *blue;    
{
    unsigned char *r, *g, *b;

    r = red->table;
    g = green->table;
    b = blue->table;

    if (size) {
        DO_N_TIMES(size,
            Bp_GetByte(bp, *r);
            Bp_GetByte(bp, *g);
            Bp_GetByte(bp, *b);
            r++;
            g++;
            b++;
        );
    }

    return DVM_GIF_OK;
}

void
GifCtEncode (size, red, green, blue, bp)
    ImageMap *red, *green, *blue;
    int size;
    BitParser *bp;
{
    unsigned char *r, *g, *b;

    r = red->table;
    g = green->table;
    b = blue->table;

    if (size) {
        DO_N_TIMES(size,
            Bp_PutByte( bp, *r );
            Bp_PutByte( bp, *g );
            Bp_PutByte( bp, *b );
            r++;
            g++;
            b++;
        );
    }
}
