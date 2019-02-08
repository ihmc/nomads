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
 * dvmimagemap.h
 *
 * function prototypes for the c functions
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_IMAGE_MAP
#define _DVM_IMAGE_MAP

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *----------------------------------------------------------------------
 *
 * type ImageMap
 *
 * Note that Imagemap.table was changed to type unsigned char from
 * short since the the legal values for BW maps range from 0..255.
 * This reduces the size of image maps in half although it increases
 * the complexity of implementing RGB image maps
 *
 * Matthew Chiu Jan 98
 *----------------------------------------------------------------------
 */
    typedef struct ImageMap {
        unsigned char table[256];       /* table of lookup values */
        int dimension;          /* BW = 1, RGB = 3 - RGB not implemented */
    } ImageMap;

#define IMAGEMAPSIZE 256
#define ImageMapGetValues(map)   ((map)->table)

/* c functions */
    ImageMap *ImageMapNew();
    void ImageMapInit(ImageMap *, unsigned char *);
    void ImageMapFree(ImageMap *);
    void ImageMapCopy(ImageMap *, ImageMap *);
    void ImageMapCompose(ImageMap *, ImageMap *, ImageMap *);
    void ImageMapApply(ImageMap *, ByteImage *, ByteImage *);
    void ImageMapInitHistoEqual(ByteImage * image, ImageMap *);
    void ImageMapInitIdentity(ImageMap *);
    void ImageMapInitInverse(ImageMap *);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
