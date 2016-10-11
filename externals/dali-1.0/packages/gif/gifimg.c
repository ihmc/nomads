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
 * gifimg.c
 *
 * Steve Weiss January 98
 *
 * Routines relating to Gif Images and Gif Image Headers
 *
 *----------------------------------------------------------------------
 */

#include "gifInt.h"

GifImgHdr *
GifImgHdrNew ()
{
    GifImgHdr *gifHdr;

    gifHdr = NEW (GifImgHdr);

    /*
     * gifHdr->width = 0;
     * gifHdr->height = 0;
     * gifHdr->ctFlag = 0;
     * gifHdr->ctSize = 0;
     * gifHdr->bitsPerPixel = 0;
     * gifHdr->ctSorted = 0;
     * gifHdr->leftPosition = 0;
     * gifHdr->topPosition = 0;
     * gifHdr->interlaced = 0;
     * 
     * gifHdr->graphicControlFlag = 0;
     * gifHdr->disposalMethod = 0;
     * gifHdr->userInputFlag = 0;
     * gifHdr->transparentColorFlag = 0;
     * gifHdr->delayTime = 0;
     * gifHdr->transparentColorIndex = 0;
     */

    return gifHdr;
}

void
GifImgHdrFree (gifHdr)
    GifImgHdr *gifHdr;
{
    FREE ((char *) gifHdr);
}


int
GifImgHdrParse (bp, gifHdr)
    BitParser *bp;
    GifImgHdr *gifHdr;
{
    unsigned char byte;

    /*
     * Read in the image descriptor
     */

    Bp_GetByte (bp, byte);
    while (byte == 0) {
        Bp_GetByte (bp, byte);
    }

    if (byte == GIF_EXTENSION_INTRO) {
        Bp_GetByte (bp, byte);
    }
    if (byte == GIF_GRAPHIC_CONTROL_LABEL) {
        gifHdr->graphicControlFlag = 1;
        Bp_FlushByte (bp);
        Bp_GetByte (bp, byte);
        gifHdr->disposalMethod = (byte & 0x1c) >> 2;
        gifHdr->userInputFlag = BIT1 (byte);
        gifHdr->transparentColorFlag = BIT0 (byte);
        Bp_GetShort (bp, gifHdr->delayTime);
        Bp_GetByte (bp, gifHdr->transparentColorIndex);
        Bp_FlushByte (bp);
        Bp_GetByte (bp, byte);
    }
    if (byte == GIF_TRAILER)
        return DVM_GIF_EOF_ERROR;

    if (byte != GIF_IMG_SEPARATOR)
        return DVM_GIF_IMG_SEPARATOR_ERROR;

    Bp_GetShort (bp, gifHdr->leftPosition);
    Bp_GetShort (bp, gifHdr->topPosition);
    Bp_GetLittleShort (bp, gifHdr->width);
    Bp_GetLittleShort (bp, gifHdr->height);

    Bp_GetByte (bp, byte);
    gifHdr->ctFlag = (byte & 0x80) >> 7;
    gifHdr->interlaced = (byte & 0x40) >> 6;

    if (gifHdr->ctFlag) {
        gifHdr->ctSorted = (byte & 0x08) >> 3;
        gifHdr->bitsPerPixel = (byte & 0x07) + 1;
        gifHdr->ctSize = (1 << gifHdr->bitsPerPixel);
    }
    return DVM_GIF_OK;
}


int
GifImgInterlacedParse (bp, seqHdr, imgHdr, byte)
    BitParser *bp;
    GifSeqHdr *seqHdr;
    GifImgHdr *imgHdr;
    ByteImage *byte;
{
    if (!imgHdr->interlaced)
        return DVM_GIF_NOT_INTERLACED;
    return ReadInterlacedGifImage (bp, imgHdr, byte);
}


int
GifImgNonInterlacedParse (bp, seqHdr, imgHdr, byte)
    BitParser *bp;
    GifSeqHdr *seqHdr;
    GifImgHdr *imgHdr;
    ByteImage *byte;
{
    if (imgHdr->interlaced)
        return DVM_GIF_INTERLACED;
    return ReadNonInterlacedGifImage (bp, imgHdr, byte);
}



void
GifImgHdrEncode (imgHdr, bp)
    GifImgHdr *imgHdr;
    BitParser *bp;
{
    unsigned char byte;

    /* 
     * If a graphic control flag is present, encode it before encoding the image
     */

    if (imgHdr->graphicControlFlag) {
        Bp_PutByte (bp, GIF_EXTENSION_INTRO);
        Bp_PutByte (bp, GIF_GRAPHIC_CONTROL_LABEL);
        Bp_PutByte (bp, 4);

        byte = (imgHdr->disposalMethod << 2) |
            (imgHdr->userInputFlag << 1) |
            (imgHdr->transparentColorFlag);
        Bp_PutByte (bp, byte);
        Bp_PutLittleShort (bp, imgHdr->delayTime);
        Bp_PutByte (bp, imgHdr->transparentColorIndex);
        Bp_PutByte (bp, 0);
    }
    /*
     * Write an Image separator
     */
    Bp_PutByte (bp, ',');

    /*
     * Write the Image header
     */

    Bp_PutLittleShort (bp, imgHdr->leftPosition);
    Bp_PutLittleShort (bp, imgHdr->topPosition);
    Bp_PutLittleShort (bp, imgHdr->width);
    Bp_PutLittleShort (bp, imgHdr->height);

    byte = 0;
    if (imgHdr->interlaced)
        byte |= 0x40;

    if (imgHdr->ctFlag) {
        byte |= 0x80;
        if (imgHdr->ctSorted)
            byte |= 0x20;
        if (imgHdr->bitsPerPixel == 0)
            imgHdr->bitsPerPixel = COLORBPP (imgHdr->ctSize);
        byte |= imgHdr->bitsPerPixel - 1;
    }
    Bp_PutByte (bp, byte);
}


void
GifImgEncode (gifHdr, imgHdr, srcBuf, bp)
    GifSeqHdr *gifHdr;
    GifImgHdr *imgHdr;
    ByteImage *srcBuf;
    BitParser *bp;
{
    int initCodeSize;
    int bitsPerPixel;

    /*
     * The initial code size
     */

    if (imgHdr->ctFlag) {
        bitsPerPixel = imgHdr->bitsPerPixel;
    } else {
        bitsPerPixel = gifHdr->bitsPerPixel;
    }

    if (bitsPerPixel <= 1)
        initCodeSize = 2;
    else
        initCodeSize = bitsPerPixel;

    /*
     * Write out the initial code size
     */
    Bp_PutByte (bp, initCodeSize);
    Compress (initCodeSize + 1, bp, srcBuf, imgHdr);

    /*
     * Write out a Zero-length packet (to end the series)
     */
    Bp_PutByte (bp, 0);

}


int
GifImgSkip (bp)
    BitParser *bp;
{
    unsigned char byte;
    int size;

    /*
     * Read in the image descriptor and graphical control extension (if present)
     */

    Bp_GetByte (bp, byte);
    if (byte == GIF_EXTENSION_INTRO) {
        Bp_GetByte (bp, byte);
    }
    /*
     * Ignore any extension
     */
    while (byte != GIF_IMG_SEPARATOR) {
        if (byte == GIF_GRAPHIC_CONTROL_LABEL) {
            Bp_FlushBytes (bp, 6);
            Bp_GetByte (bp, byte);
        } else if (byte == GIF_APPLICATION_LABEL) {
            Bp_FlushBytes (bp, 11);
            while (byte) {
                Bp_GetByte (bp, byte);
            }
            Bp_GetByte (bp, byte);
        } else if (byte == GIF_COMMENT_LABEL) {
            while (byte) {
                Bp_GetByte (bp, byte);
            }
            Bp_GetByte (bp, byte);
        }
    }

    if (byte != GIF_IMG_SEPARATOR) {
        return DVM_GIF_IMG_SEPARATOR_ERROR;
    }
    Bp_FlushBytes (bp, 8);
    Bp_GetByte (bp, byte);

    /* Parse the color table, if present */
    if (BIT7 (byte)) {
        size = (1 << ((byte & 0x07) + 1));
        Bp_FlushBytes (bp, 3 * size);
    }
    IgnoreGifImage (bp);

    return DVM_GIF_OK;
}


int
GifImgFind (bp)
    BitParser *bp;
{
    unsigned char byte;

    /*
     * Ignore Application and Comment Extensions,
     * Stop when it reaches a graphic control label or an image separator
     */

    Bp_PeekByte (bp, byte);
    while (1) {
        switch (byte) {

        case GIF_IMG_SEPARATOR:
            return DVM_GIF_OK;

        case GIF_EXTENSION_INTRO:
            Bp_GetByte (bp, byte);
            break;

        case GIF_GRAPHIC_CONTROL_LABEL:
            return DVM_GIF_OK;

        case GIF_APPLICATION_LABEL:
            Bp_FlushBytes (bp, 11);
            while (byte) {
                Bp_GetByte (bp, byte);
            }
            break;

        case GIF_COMMENT_LABEL:
            while (byte) {
                Bp_GetByte (bp, byte);
            }
            break;

        case GIF_TRAILER:
            return DVM_GIF_EOF_ERROR;
            break;

        case 0:
            Bp_GetByte (bp, byte);
            break;

        default:
            return DVM_GIF_EOF_ERROR;
        }

        Bp_PeekByte (bp, byte);
    }

    return DVM_GIF_OK;
}

void 
GifImgHdrSetCtSize(imgHdr, size)
    GifImgHdr *imgHdr;
    int size;
{
    imgHdr->ctSize = size;
    imgHdr->bitsPerPixel = COLORBPP(size);
}
