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
 * avivideo.c
 *
 * Functions that deal with avi video
 *
 *----------------------------------------------------------------------
 */

#include "aviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * AviVideoFrameRead
 *
 *   Get the next frame from the video input stream
 *
 * Returns
 *   0 for sucess, error code otherwise (see dvmavi.h for error codes) 
 * 
 * side effect :
 *   The frame pointer is advanced, so the next call will retrieve
 *   the next frame
 *
 *----------------------------------------------------------------------
 */
int
AviVideoFrameRead(str, rBuf, gBuf, bBuf)
    AviStream *str;
    ByteImage *rBuf, *gBuf, *bBuf;
{
    AviVideoStream *viddata;
    unsigned char *rDest, *gDest, *bDest;
    int rDelta, gDelta, bDelta, rowDelta;
    int i, w, h;
    unsigned char *data, *rowPtr;
    LPBITMAPINFOHEADER lpbi;

    if (str->type != AVI_STREAM_VIDEO) {
        return DVM_AVI_NOT_VIDEO;
    }
    viddata = (AviVideoStream *) (str->data);

    /*
     * verify sizes
     */
    w = viddata->width;
    h = viddata->height;
    if ((w != rBuf->width) || (w != gBuf->width) || (w != bBuf->width) ||
        (h != rBuf->height) || (h != gBuf->height) || (h != bBuf->height)) {
        return DVM_AVI_BAD_SIZE;
    }
    /*
     * If any buffers are virtual, we need to advance its pointer
     * by these deltas
     */
    rDelta = rBuf->parentWidth - w;
    gDelta = gBuf->parentWidth - w;
    bDelta = bBuf->parentWidth - w;

    rDest = rBuf->firstByte;
    gDest = gBuf->firstByte;
    bDest = bBuf->firstByte;

    /* Get the data */
    data = (unsigned char *) AVIStreamGetFrame(viddata->gf, str->sofar);
    if (data == NULL) {
        return DVM_AVI_GET_FRAME_FAILED;
    }
    str->sofar++;

    lpbi = (LPBITMAPINFOHEADER) data;
    data += lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);
    rowDelta = ((3 * (w + 1)) / 4) * 4;
    rowPtr = data + (h - 1) * rowDelta;         /* last row */
    data = rowPtr;

    /*
     * Now split the planes - we have to do it bottom up!
     */
    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *bDest++ = *data++;
            *gDest++ = *data++;
            *rDest++ = *data++;
            );

        /*
         * Move up to next row
         */
        rDest -= rDelta;
        gDest -= gDelta;
        bDest -= bDelta;
        rowPtr -= rowDelta;
        data = rowPtr;
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * AviVideoFrameWrite
 *
 *   Users call this function to write data to an AVI stream
 *
 * Returns
 *   0 for sucess, error code otherwise (see dvmavi.h for error codes) 
 * 
 * side effect :
 *   The data is written at the next position in the stream
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFrameWrite(str, rBuf, gBuf, bBuf)
    AviStream *str;
    ByteImage *rBuf, *gBuf, *bBuf;
{
    AviVideoStream *viddata;
    unsigned char *rSrc, *gSrc, *bSrc;
    int rDelta, gDelta, bDelta, rowDelta;
    int i, w, h, status;
    unsigned char *data, *rowPtr;

    /*
     * Make sure it's a video stream
     */
    if (str->type != AVI_STREAM_VIDEO) {
        return DVM_AVI_NOT_VIDEO;
    }
    viddata = (AviVideoStream *) (str->data);

    /*
     * check if we can write
     */
    if (viddata->fb == NULL) {
        return DVM_AVI_UNINITIALIZED;
    }
    /*
     * verify sizes
     */
    w = viddata->width;
    h = viddata->height;
    if ((w != rBuf->width) || (w != gBuf->width) || (w != bBuf->width) ||
        (h != rBuf->height) || (h != gBuf->height) || (h != bBuf->height)) {
        return DVM_AVI_BAD_SIZE;
    }
    /*
     * If any buffers are virtual, we need to advance its pointer
     * by these deltas
     */
    rDelta = rBuf->parentWidth - w;
    gDelta = gBuf->parentWidth - w;
    bDelta = bBuf->parentWidth - w;

    rSrc = rBuf->firstByte;
    gSrc = gBuf->firstByte;
    bSrc = bBuf->firstByte;

    /*
     * Reformat the data
     */
    data = viddata->fb;
    rowDelta = 3 * w;
    rowPtr = data + (h - 1) * rowDelta;         /* last row */
    data = rowPtr;

    /*
     * Merge the planes - we have to do it bottom up!
     */

    for (i = 0; i < h; i++) {
        DO_N_TIMES(w,
            *data++ = *bSrc++;
            *data++ = *gSrc++;
            *data++ = *rSrc++;
            );

        /*
         * Move to next row
         */
        rSrc += rDelta;
        gSrc += gDelta;
        bSrc += bDelta;
        rowPtr -= rowDelta;
        data = rowPtr;
    }

    if (str->sofar % viddata->keyinterval) {
        status = AVIStreamWrite(viddata->cs, str->sofar, 1, viddata->fb,
            w * h * 3, 0, NULL, NULL);
    } else {
        status = AVIStreamWrite(viddata->cs, str->sofar, 1, viddata->fb,
            w * h * 3, AVIIF_KEYFRAME, NULL, NULL);
    }

    if (!status) {
        str->sofar++;
        str->length++;
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * AviVideoFrameSkip, AviVideoFrameRewind, AviVideoFrameTell
 * AviVideoFrameSeek --
 *
 *   These three functions are used to move the cursor within the
 *   video stream
 *
 * Return 
 *   the position of the cursor (i.e., the frame number)
 * 
 * side effect :
 *   Sets the frame indicator.
 *
 *----------------------------------------------------------------------
 */

int
AviVideoFrameSkip(str)
    AviStream *str;
{
    return ++str->sofar;
}

int
AviVideoFrameRewind(str)
    AviStream *str;
{
    return (str->sofar = str->start);
}

int
AviVideoFrameTell(str)
    AviStream *str;
{
    return str->sofar;
}

int
AviVideoFrameSeek(str, frameNumber)
    AviStream *str;
    int frameNumber;
{
    return (str->sofar = frameNumber);
}
