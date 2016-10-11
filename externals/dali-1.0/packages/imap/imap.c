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
 *---------------------------------------------------------------------------
 * Image Maps (C Source File)
 *
 *---------------------------------------------------------------------------
 */

#include "dvmimap.h"

ImageMap *
ImageMapNew ()
{
    ImageMap *buf;

    buf = NEW (ImageMap);
    buf->dimension = 1;

    return buf;
}

void
ImageMapInit (map, array)
    ImageMap *map;
    unsigned char *array;
{
    /*
     * copies the values from Array into the image map table
     */
    unsigned char *tablePtr;

    tablePtr = map->table;
    DO_N_TIMES(256,
        *tablePtr++ = *array++;
    );
}


void
ImageMapFree (map)
    ImageMap *map;
{
    FREE((char *)map);
}

void
ImageMapCopy (src, dest)
    ImageMap *src;
    ImageMap *dest;
{
    dest->dimension = src->dimension;
    memcpy(dest->table,src->table,256*sizeof(unsigned char));
}

void
ImageMapCompose(map1, map2, dest)
    ImageMap *map1;
    ImageMap *map2;
    ImageMap *dest;
{
    unsigned char *arrayPos, *newArrayPos, *lookup;

    /* 
     * create a new buffer and initialize it by composing the two tables 
     * in buf1 and buf2
     */
    dest->dimension = 1;

    arrayPos = map1->table;
    lookup = map2->table;
    newArrayPos = dest->table;
    DO_N_TIMES(256,
        *newArrayPos++ = lookup[*arrayPos++];
    );
}

void
ImageMapApply(map, srcImage, destImage)
    ImageMap *map;
    ByteImage *srcImage;
    ByteImage *destImage;
{

    int i,h,w,srcSkip,destSkip;
    unsigned char *currSrc,*currDest;
    unsigned char *lookup;

    /* 
     * apply the image map to each pixel in srcimage and store the result 
     * in destimage
     */
    h = min(srcImage->height, destImage->height);
    w = min(srcImage->width, destImage->width);
    srcSkip = srcImage->parentWidth - w;
    destSkip = destImage->parentWidth - w;
    currSrc = srcImage->firstByte;
    currDest = destImage->firstByte;
    lookup = map->table;
    
    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *currDest++ = lookup[*currSrc++];
        );
        currSrc += srcSkip;
        currDest += destSkip;
    }
}

void
ImageMapInitHistoEqual(image, newbuf)
    ByteImage *image;
    ImageMap *newbuf;
{
    int w,h,skip,min,max,newvalue,quot,i;
    float sum,average;

    unsigned int histogram[256];
    unsigned char *currLoc;
    unsigned int *arrayLoc;
    unsigned char *tableLoc;
    
    /* 
     * make 1st-pass through the image to generate histogram of intensity
     * values
     */
    w = image->width;
    h = image->height;
    skip = image->parentWidth - w;
    currLoc = image->firstByte;
    arrayLoc = histogram;

    memset (histogram, 0, 256*sizeof(unsigned int));
    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            histogram[*currLoc++]++;
        );
        currLoc += skip;
    }

    /* 
     * generate table of values according to algorithm given in Pavlidis, 
     * Algorithms for Graphics and Image Processing, p. 52.
     */

    arrayLoc = histogram;
    tableLoc = newbuf->table;
    average = (float)((w*h)/256.0);
    max = 0;
    sum = 0;
    
    for (i=1;i<=256;i++) {
        min = max;
        sum += *arrayLoc++;
        /*
         * the following lines given in his algorithm have been replaced

            while (sum > average) {
                max++;
                sum -= average;
            }
         *
         */

        if (sum > average) {
            quot = (int)(sum/average);
            max += quot;
            sum -= quot * average;
        }

        newvalue = (min + max)/2;
        if (newvalue > 255) {
            newvalue = 255;
            /*
             * I believe I am justified in doing this because in his 
             * algorithm newvalue can equal 256.  for example, if all of 
             * the pixels are in 1 bin then quot will be 256. The appearance 
             * of a newvalue of 256 is not significant towards the processing 
             * of the image.
             */
        }
        *tableLoc++ = newvalue;
    }
}

void
ImageMapInitIdentity(map)
    ImageMap *map;
{
    unsigned char *arrayPos;
    int i;

    /*
     * Create a new buffer, initialize it with the identity map
     */
    map->dimension = 1;
    arrayPos = map->table;

    for (i = 0; i < 256; i++) {
        *arrayPos++ = (unsigned char)i;
    }
}


void
ImageMapInitInverse(map)
    ImageMap *map;
{
    unsigned char *arrayPos;
    int i;

    /*
     * Create a new buffer, initialize it with the inverse map
     */
    map->dimension = 1;
    arrayPos = map->table;

    for (i = 255; i>=0; i--) {
        *arrayPos++ = (unsigned char)i;
    }
}
