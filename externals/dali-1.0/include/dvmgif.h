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
 * rvmgif.h
 *
 * This file contains prototypes for functions to perform
 * operations on gif images
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_GIF_
#define _DVM_GIF_

#include "dvmbasic.h"
#include "dvmimap.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct GifSeqHdr {
        int ctFlag;
        int bitsPerPixel;
        int ctSize;
        int ctSorted;
        int width;
        int height;
        int resolution;
        int backgroundColor;
        int aspectRatio;
        char version[4];
    } GifSeqHdr;

    typedef struct GifImgHdr {
        int ctFlag;
        int bitsPerPixel;
        int ctSize;
        int ctSorted;
        int leftPosition;
        int topPosition;
        int width;
        int height;
        int interlaced;

        int graphicControlFlag;
        int disposalMethod;
        int userInputFlag;
        int transparentColorFlag;
        int delayTime;
        int transparentColorIndex;
    } GifImgHdr;


    GifSeqHdr *GifSeqHdrNew();
    void GifSeqHdrFree();
    int GifSeqHdrParse(BitParser *, GifSeqHdr *);
    void GifSeqHdrEncode(GifSeqHdr *, BitParser *);
    void GifSeqHdrSetCtSize(GifSeqHdr *, int size);
    void GifSeqLoopEncode(BitParser *);
    void GifSeqTrailerEncode(BitParser *);

    int GifCtParse(BitParser *, int size, ImageMap *, ImageMap *, ImageMap *);
    void GifCtEncode(int size, ImageMap *, ImageMap *, ImageMap *, BitParser * bp);

    GifImgHdr *GifImgHdrNew();
    void GifImgHdrFree();
    int GifImgHdrParse(BitParser *, GifImgHdr *);
    int GifImgInterlacedParse(BitParser *, GifSeqHdr *, GifImgHdr *, ByteImage *);
    int GifImgNonInterlacedParse(BitParser *, GifSeqHdr *, GifImgHdr *, ByteImage *);
    void GifImgHdrEncode(GifImgHdr * imgHdr, BitParser *);
    void GifImgEncode(GifSeqHdr *, GifImgHdr *, ByteImage *, BitParser *);
    int GifImgSkip(BitParser * bp);
    int GifImgFind(BitParser * bp);
    void GifImgHdrSetCtSize(GifImgHdr *, int size);

#define GifSeqHdrGetWidth(hdr) hdr->width
#define GifSeqHdrGetHeight(hdr) hdr->height
#define GifSeqHdrGetCtFlag(hdr) hdr->ctFlag
#define GifSeqHdrGetCtSize(hdr) hdr->ctSize
#define GifSeqHdrGetCtSorted(hdr) hdr->ctSorted
#define GifSeqHdrGetResolution(hdr) hdr->resolution
#define GifSeqHdrGetBackgroundColor(hdr) hdr->backgroundColor
#define GifSeqHdrGetAspectRatio(hdr) hdr->aspectRatio
#define GifSeqHdrGetVersion(hdr) hdr->version

#define GifImgHdrGetWidth(hdr) hdr->width
#define GifImgHdrGetHeight(hdr) hdr->height
#define GifImgHdrGetCtFlag(hdr) hdr->ctFlag
#define GifImgHdrGetCtSize(hdr) hdr->ctSize
#define GifImgHdrGetLeftPosition(hdr) hdr->leftPosition
#define GifImgHdrGetTopPosition(hdr) hdr->topPosition
#define GifImgHdrGetInterlaced(hdr) hdr->interlaced
#define GifImgHdrGetGraphicControlFlag(hdr) hdr->graphicControlFlag
#define GifImgHdrGetDisposalMethod(hdr) hdr->disposalMethod
#define GifImgHdrGetUserInputFlag(hdr) hdr->userInputFlag
#define GifImgHdrGetTransparentColorFlag(hdr) hdr->transparentColorFlag
#define GifImgHdrGetDelayTime(hdr) hdr->delayTime
#define GifImgHdrGetTransparentColorIndex(hdr) hdr->transparentColorIndex


#define GifSeqHdrSetWidth(hdr, val) hdr->width=val
#define GifSeqHdrSetHeight(hdr, val) hdr->height=val
#define GifSeqHdrSetCtFlag(hdr, val) hdr->ctFlag=val
#define GifSeqHdrSetCtSorted(hdr, val) hdr->ctSorted=val
#define GifSeqHdrSetResolution(hdr, val) hdr->resolution=val
#define GifSeqHdrSetBackgroundColor(hdr, val) hdr->backgroundColor=val
#define GifSeqHdrSetAspectRatio(hdr, val) hdr->aspectRatio=val
#define GifSeqHdrSetVersion(hdr, v) strncpy(hdr->version, v, 4)

#define GifImgHdrSetWidth(hdr, val) hdr->width=val
#define GifImgHdrSetHeight(hdr, val) hdr->height=val
#define GifImgHdrSetCtFlag(hdr, val) hdr->ctFlag=val
#define GifImgHdrSetLeftPosition(hdr, val) hdr->leftPosition=val
#define GifImgHdrSetTopPosition(hdr, val) hdr->topPosition=val
#define GifImgHdrSetInterlaced(hdr, val) hdr->interlaced=val
#define GifImgHdrSetGraphicControlFlag(hdr, val) hdr->graphicControlFlag=val
#define GifImgHdrSetDisposalMethod(hdr, val) hdr->disposalMethod=val
#define GifImgHdrSetUserInputFlag(hdr, val) hdr->userInputFlag=val
#define GifImgHdrSetTransparentColorFlag(hdr, val) hdr->transparentColorFlag=val
#define GifImgHdrSetDelayTime(hdr, val) hdr->delayTime=val
#define GifImgHdrSetTransparentColorIndex(hdr, val) hdr->transparentColorIndex=val

#define DVM_GIF_OK 0
#define DVM_GIF_IMG_READ_ERROR -1
#define DVM_GIF_EOF_ERROR -2
#define DVM_GIF_IMG_SEPARATOR_ERROR -3
#define DVM_GIF_BAD_HEADER -4
#define DVM_GIF_BAD_VERSION -5
#define DVM_GIF_INTERLACED -6
#define DVM_GIF_NOT_INTERLACED -7
#define DVM_GIF_BAD_WIDTH -8
#define DVM_GIF_BAD_HEIGHT -9

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
