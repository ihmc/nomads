/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Modified version of colorquant.c 
 * 
 * Quantize an RGB images
 * 
 * Steve Weiss, Wei Tsang
 *------------------------------------------------------------------------
 */


/* ppmquant.c - quantize the colors in a pixmap down to a specified number
**
** Copyright (C) 1989, 1991 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "dvmcolor.h"
#include <time.h>

#ifdef assert
#undef assert
#define assert(c) { }
#endif

extern unsigned char theCropTable[];
extern unsigned short theSquareTable[];

#define sqr(x) theSquareTable[x + 255]
#define swap(x, y) { int _temp; _temp = x; x = y; y = _temp; }

#define MAXCOLORS 32767
#define CROP(n) (unsigned char)theCropTable[(n) + 2048]

typedef struct box* box_vector;
struct box
    {
    int ind;
    int colors;
    int sum;
    };

#define P _ANSI_ARGS_
static void MedianCut (ColorHashTable* , int, ImageMap *, ImageMap *, ImageMap *);
static int sumcompare (const void* b1, const void* b2 );
static void ColorHistogramSortGreen(ColorHashEntry*, int size);
static void ColorHistogramSortRed(ColorHashEntry*, int size);
static void ColorHistogramSortBlue(ColorHashEntry*, int size);
static int ComputeHashTable( ByteImage *rBuf, ByteImage *gBuf, ByteImage *bBuf, ColorHashTable *table);
#undef P

void
RgbTo256(rBuf, gBuf, bBuf, table, rMap, gMap, bMap)
    ByteImage *rBuf, *gBuf, *bBuf;
    ColorHashTable *table;
    ImageMap *rMap, *gMap, *bMap;
{
    int w, h;
    int row;
    register int rSkip, gSkip, bSkip;
    unsigned char maxval = 255, newmaxval, origmaxval;
    int status;
    unsigned char precompute[256];

    register unsigned char *currR, *currG, *currB;

    w = rBuf->width;
    h = rBuf->height;
    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;

    /*
     * Step 1: attempt to make a histogram of the colors, unclustered.
     * If at first we don't succeed, lower maxval to increase color
     * coherence and try again.  This will eventually terminate, with
     * maxval at worst 15, since 32^3 is approximately MAXCOLORS.
     */
    origmaxval = maxval;
    for (;;) {
        ColorHashTableClear(table);
        status = ComputeHashTable(rBuf, gBuf, bBuf, table);
        if (status != DVM_COLOR_HASH_TABLE_FULL)
            break;
        /* there are more than 32768 colors, try to reduce it */
        newmaxval = maxval >> 1;

        for (row = 0; row < 256; row++) {
            precompute[row] = (row+1)*(maxval >> 1)/maxval;
        }
        currR = rBuf->firstByte;
        currG = gBuf->firstByte;
        currB = bBuf->firstByte;
        for (row = 0; row < h; row++) {
            DO_N_TIMES (w,
                *currR = precompute[*currR];
                *currG = precompute[*currG];
                *currB = precompute[*currB];
                /*
                *currR = ((*currR + 1)*(maxval >> 1))/maxval;
                *currG = ((*currG + 1)*(maxval >> 1))/maxval;
                *currB = ((*currB + 1)*(maxval >> 1))/maxval;
                */
                currR++;
                currG++;
                currB++;
                );
            currR += rSkip;
            currG += gSkip;
            currB += bSkip;

        }
        maxval = newmaxval;
    }

    /* scale all the colors back */
    /* precompute the table */

    for (row = 0; row < 256; row++) {
        precompute[row] = (row * origmaxval + (maxval >> 1))/maxval;
    }
    currR = rBuf->firstByte;
    currG = gBuf->firstByte;
    currB = bBuf->firstByte;
    for (row = 0; row < h; row++) {
        DO_N_TIMES(w,
            *currR = precompute[*currR];
            *currG = precompute[*currG];
            *currB = precompute[*currB];

            currR++;
            currG++;
            currB++;
            );
        currR += rSkip;
        currG += gSkip;
        currB += bSkip;
    }

    maxval = origmaxval;

    ColorHashTableClear(table);
    status = ComputeHashTable(rBuf, gBuf, bBuf, table);

    /*
     * ** Step 2: apply median-cut to histogram, making the new colormap.
     */
    ColorHashTablePackSelf(table);
    MedianCut (table, w*h, rMap, gMap, bMap);
}


void
RgbQuantWithVpTree(rBuf, gBuf, bBuf, tree, table, rMap, gMap, bMap, outBuf)
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
    VpNode *tree;
    ColorHashTable *table;
    ImageMap *rMap;
    ImageMap *gMap;
    ImageMap *bMap;
    ByteImage *outBuf;
{
    int rSkip, gSkip, bSkip;
    unsigned char *currR, *currB, *currG, *currOut;
    int row, col, w, h, ind, value;
    int usehash, status;

    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;
    w = rBuf->width;
    h = rBuf->height;

    usehash = 1;

    currR = rBuf->firstByte;
    currG = gBuf->firstByte;
    currB = bBuf->firstByte;
    currOut = outBuf->firstByte;

    for (row = 0; row < h; ++row) {
        col = 0;
        do {
            status = ColorHashTableFind(table, *currR, *currG, *currB, &value, &ind);
            if (status == DVM_COLOR_NOT_FOUND) {
                /* No; search colormap for closest match. */
                ind = VpTreeFind(tree, rMap, gMap, bMap, *currR, *currG, *currB);
                if (usehash) {
                    if (ColorHashTableAdd (table, *currR, *currG, *currB, ind) == DVM_COLOR_HASH_TABLE_FULL) {
                        usehash = 0;
                    }
                }
                // unmatch++;
            } else {
                // ind = chl->ch.value;
                // match++;
            }
            *currOut = ind;

            ++col;

            currR++;
            currG++;
            currB++;
            currOut++;
        } while (col != w);

        currR += rSkip;
        currG += gSkip;
        currB += bSkip;
        currOut += outBuf->parentWidth - outBuf->width;
    }

    return;
}
void
RgbQuantWithHashTable(rBuf, gBuf, bBuf, table, rMap, gMap, bMap, outBuf)
    ByteImage *rBuf;
    ByteImage *gBuf;
    ByteImage *bBuf;
    ColorHashTable *table;
    ImageMap *rMap;
    ImageMap *gMap;
    ImageMap *bMap;
    ByteImage *outBuf;
{
    int rSkip, gSkip, bSkip;
    unsigned char *currR, *currB, *currG, *currOut;
    int row, col, w, h, ind, value;
    int usehash, status;

    rSkip = rBuf->parentWidth - rBuf->width;
    gSkip = gBuf->parentWidth - gBuf->width;
    bSkip = bBuf->parentWidth - bBuf->width;
    w = rBuf->width;
    h = rBuf->height;

    usehash = 1;

    currR = rBuf->firstByte;
    currG = gBuf->firstByte;
    currB = bBuf->firstByte;
    currOut = outBuf->firstByte;

    for (row = 0; row < h; ++row) {
        col = 0;
        do {
            status = ColorHashTableFind(table, *currR, *currG, *currB, &value, &ind);
            if (status == DVM_COLOR_NOT_FOUND) {
                /* No; search colormap for closest match. */
                register int i;
                register long dist, newdist;

                dist = sqr ((*currR - rMap->table[0])) + 
                    sqr ((*currG - gMap->table[0])) + 
                    sqr ((*currB - bMap->table[0]));
                ind = 0;
                for (i = 1; i < 256; i++) {
                    newdist = 
                        sqr ((*currR - rMap->table[i])) + 
                        sqr ((*currG - gMap->table[i])) + 
                        sqr ((*currB - bMap->table[i]));
                    if (newdist < dist) {
                        ind = i;
                        dist = newdist;
                    }
                }
                if (usehash) {
                    if (ColorHashTableAdd (table, *currR, *currG, *currB, ind) == DVM_COLOR_HASH_TABLE_FULL) {
                        usehash = 0;
                    }
                }
                // unmatch++;
            } else {
                // ind = chl->ch.value;
                // match++;
            }
            *currOut = ind;

            ++col;

            currR++;
            currG++;
            currB++;
            currOut++;
        } while (col != w);

        currR += rSkip;
        currG += gSkip;
        currB += bSkip;
        currOut += outBuf->parentWidth - outBuf->width;
    }

    return;
}

/*-------------------------------------------------------------------------
 * 
 * Median Cut
 *
 * The median-cut colormap generator based on Paul Heckbert's paper 
 * "Color Image Quantization for Frame Buffer * Display", SIGGRAPH '82 
 *
 *-------------------------------------------------------------------------
 */

void
MedianCut (table, sum, rMap, gMap, bMap)
    ColorHashTable *table;
    int sum;
    ImageMap *rMap;
    ImageMap *gMap;
    ImageMap *bMap;
{
    struct box bv[256];
    register int bi, i;
    int boxes, colors;
    float rl, gl, bl;
    ColorHashEntry *curr;

    memset(rMap->table, 256, 0);
    memset(gMap->table, 256, 0);
    memset(bMap->table, 256, 0);

    colors = table->numOfEntry;

    /*
     * Set up the initial box.
     */
    bv[0].ind = 0;
    bv[0].colors = colors;
    bv[0].sum = sum;
    boxes = 1;

    /*
     * Main loop: split boxes until we have enough.
     */
    while ( boxes < 256 ) {
        register int index, clrs;
        int sm;
        register int minr, maxr, ming, maxg, minb, maxb;
        int halfsum, lowersum;

        /*
         * Find the first splittable box.
         */
        for ( bi = 0; bi < boxes; ++bi )
            if ( bv[bi].colors >= 2 )
                break;

        if ( bi == boxes )
            break;      /* ran out of colors! */

        index = bv[bi].ind;
        clrs = bv[bi].colors;
        sm = bv[bi].sum;

        /*
         * Go through the box finding the minimum and maximum of each
         * component - the boundaries of the box.
         */
        curr = &(table->table[index]);
        minr = maxr = curr->color.unpack.r;
        ming = maxg = curr->color.unpack.g;
        minb = maxb = curr->color.unpack.b;

        curr++;
        DO_N_TIMES(clrs - 1,

            if (curr->color.unpack.r < minr)
                minr = curr->color.unpack.r;
            else if (curr->color.unpack.r > maxr)
                maxr = curr->color.unpack.r;
            if (curr->color.unpack.g < ming)
                ming = curr->color.unpack.g;
            else if (curr->color.unpack.g > maxg)
                maxg = curr->color.unpack.g;
            if (curr->color.unpack.b < minb)
                minb = curr->color.unpack.b;
            else if (curr->color.unpack.b > maxb)
                maxb = curr->color.unpack.b;
            curr++;
        );

        /*
         * Find the largest dimension, and sort by that component.  I have
         * included two methods for determining the "largest" dimension;
         * first by simply comparing the range in RGB space, and second
         * by transforming into luminosities before the comparison.  You
         * can switch which method is used by switching the commenting on
         * the LARGE_ defines at the beginning of this source file.
         */

        rl = (float)0.299*(maxr - minr);
        gl = (float)0.587*(maxg - ming); 
        bl = (float)0.114*(maxb - minb); 

        if ( rl >= gl && rl >= bl )
            ColorHistogramSortRed (&(table->table[index]), clrs);
        else if ( gl >= bl )
            ColorHistogramSortGreen (&(table->table[index]), clrs);
        else
            ColorHistogramSortBlue (&(table->table[index]), clrs);
        
        /*
         * Now find the median based on the counts, so that about half the
         * pixels (not colors, pixels) are in each subdivision.
         */
        lowersum = table->table[index].value;
        halfsum = sm >> 1;
        for (i = 1; i < clrs - 1; i++) {
            if (lowersum >= halfsum)
                break;
            lowersum += table->table[index + i].value;
        }

        /*
        ** Split the box, and sort to bring the biggest boxes to the top.
        */
        bv[bi].colors = i;
        bv[bi].sum = lowersum;
        bv[boxes].ind = index + i;
        bv[boxes].colors = clrs - i;
        bv[boxes].sum = sm - lowersum;
        ++boxes;
        qsort( (char*) bv, boxes, sizeof(struct box), sumcompare );
    }

    /*
     * Ok, we've got enough boxes.  Now choose a representative color for
     * each box.  There are a number of possible ways to make this choice.
     * One would be to choose the center of the box; this ignores any structure
     * within the boxes.  Another method would be to average all the colors in
     * the box - this is the method specified in Heckbert's paper.  A third
     * method is to average all the pixels in the box.  You can switch which
     * method is used by switching the commenting on the REP_ defines at
     * the beginning of this source file.
     */
    for (bi = 0; bi < boxes; bi++) {
        register int index = bv[bi].ind;
        register int clrs = bv[bi].colors;
        register long r = 0, g = 0, b = 0, sum = 0;

        for (i = 0; i < clrs; i++) {
            r += table->table[index + i].color.unpack.r * table->table[index + i].value;
            g += table->table[index + i].color.unpack.g * table->table[index + i].value;
            b += table->table[index + i].color.unpack.b * table->table[index + i].value;
            sum += table->table[index + i].value;
        }
        rMap->table[bi] = CROP(r / sum);
        gMap->table[bi] = CROP(g / sum);
        bMap->table[bi] = CROP(b / sum);
    }

}

static int
sumcompare( b1, b2 )
    const void *b1, *b2;
{
    return ((box_vector)b2)->sum - ((box_vector)b1)->sum;
}


static void 
ColorHistogramSortRed(ch, size)
    ColorHashEntry *ch;
    int size;
{
    int count[257], top[257];
    int i, j;
    register unsigned char curr;

    /* initialized counters */
    for (i = 0; i < 257; i++) {
        count[i] = 0;
    }

    /* count the number of occurance of a pixel value */
    for (i = 0; i < size; i++) {
        count[ch[i].color.unpack.r+1]++;
    }
    top[0] = count[0] = 0;
    top[1] = count[1];
    for (i = 2; i < 257; i++) {
        count[i] += count[i - 1];
        top[i] = count[i];
    }

    for (i = size - 1; i >= 0; i--) {
        curr = ch[i].color.unpack.r;
        while (i >= count[curr+1] || i < count[curr]) {
            /* ch[i] is in the wrong place */
            /* swap with something in the correct place */
            /* look for something to swap with */
            for (j = top[curr+1] - 1; j >= count[curr]; j--) {
                if (ch[j].color.unpack.r != curr) {
                    break;
                }
            }
            /* swap position i and j */
            top[curr+1] = j + 1;
            swap(ch[i].color.pack, ch[j].color.pack);
            swap(ch[i].value, ch[j].value);
            
            curr = ch[i].color.unpack.r;
        }
    }
}

static void 
ColorHistogramSortGreen(ch, size)
    ColorHashEntry *ch;
    int size;
{
    int count[257], top[257];
    int i, j;
    register unsigned char curr;

    /* initialized counters */
    for (i = 0; i < 257; i++) {
        count[i] = 0;
    }

    /* count the number of occurance of a pixel value */
    for (i = 0; i < size; i++) {
        count[ch[i].color.unpack.g+1]++;
    }
    top[0] = count[0] = 0;
    top[1] = count[1];
    for (i = 2; i < 257; i++) {
        count[i] += count[i - 1];
        top[i] = count[i];
    }

    for (i = size - 1; i >= 0; i--) {
        curr = ch[i].color.unpack.g;
        while (i >= count[curr+1] || i < count[curr]) {
            /* ch[i] is in the wrong place */
            /* swap with something in the correct place */
            /* look for something to swap with */
            for (j = top[curr+1] - 1; j >= count[curr]; j--) {
                if (ch[j].color.unpack.g != curr) {
                    break;
                }
            }
            /* swap position i and j */
            top[curr+1] = j + 1;
            swap(ch[i].color.pack, ch[j].color.pack);
            swap(ch[i].value, ch[j].value);
            
            curr = ch[i].color.unpack.g;
        }
    }
}


static void 
ColorHistogramSortBlue(ch, size)
    ColorHashEntry *ch;
    int size;
{
    int count[257], top[257];
    int i, j;
    register unsigned char curr;

    /* initialized counters */
    for (i = 0; i < 257; i++) {
        count[i] = 0;
    }

    /* count the number of occurance of a pixel value */
    for (i = 0; i < size; i++) {
        count[ch[i].color.unpack.b+1]++;
    }
    top[0] = count[0] = 0;
    top[1] = count[1];
    for (i = 2; i < 257; i++) {
        count[i] += count[i - 1];
        top[i] = count[i];
    }

    for (i = size - 1; i >= 0; i--) {
        curr = ch[i].color.unpack.b;
        while (i >= count[curr+1] || i < count[curr]) {
            /* ch[i] is in the wrong place */
            /* swap with something in the correct place */
            /* look for something to swap with */
            for (j = top[curr+1] - 1; j >= count[curr]; j--) {
                if (ch[j].color.unpack.b != curr) {
                    break;
                }
            }
            /* swap position i and j */
            top[curr+1] = j + 1;
            swap(ch[i].color.pack, ch[j].color.pack);
            swap(ch[i].value, ch[j].value);
            
            curr = ch[i].color.unpack.b;
        }
    }
}


static int
ComputeHashTable( rBuf, gBuf, bBuf, table )
    ByteImage *rBuf, *gBuf, *bBuf;
    ColorHashTable *table;
{
    int w, h, col, row, index, value, status;
    unsigned char *currR, *currG, *currB;

    /* Go through the entire image, building a hash table of colors. */
    w = rBuf->width;
    h = rBuf->height;
    currR = rBuf->firstByte;
    currG = gBuf->firstByte;
    currB = bBuf->firstByte;

    for (row = 0; row < h; ++row) {
        for (col = 0; col < w; ++col) {
            status = ColorHashTableFind(table, *currR, *currG, *currB, &index, &value);
            /* in current bucket. increment counter */
            if (status == DVM_COLOR_NOT_FOUND) {
                status = ColorHashTableAddAt(table, index, *currR, *currG, *currB, 1);
            } else if (status == DVM_COLOR_HASH_TABLE_FULL) {
                return DVM_COLOR_HASH_TABLE_FULL;
            } else {
                ColorHashTableSet(table, index, value + 1);
            }
            currR++;
            currG++;
            currB++;
        }
        currR += rBuf->parentWidth - rBuf->width;
        currG += gBuf->parentWidth - gBuf->width;
        currB += bBuf->parentWidth - bBuf->width;
    }
    return DVM_COLOR_HASH_TABLE_OK;
}
