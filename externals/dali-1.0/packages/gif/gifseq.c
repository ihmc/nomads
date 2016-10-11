/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * gifseq.c
 *
 * Steve Weiss January 98
 *
 * Routines to allocate and manipulate gif sequences and sequence headers
 *
 *------------------------------------------------------------------------
 */

#include "gifInt.h"

GifSeqHdr *
GifSeqHdrNew ()
{
    GifSeqHdr *gifHdr;

    gifHdr = NEW (GifSeqHdr);

    /*
     * gifHdr->width = 0;
     * gifHdr->height = 0;
     * gifHdr->ctFlag = 0;
     * gifHdr->bitsPerPixel = 0;
     * gifHdr->ctSize = 0;
     * gifHdr->ctSorted = 0;
     * gifHdr->resolution = 0;
     * gifHdr->backgroundColor = 0;
     * gifHdr->aspectRatio = 0;
     * strcpy(gifHdr->version, "   ");
     */

    return gifHdr;
}


void
GifSeqHdrFree (gifHdr)
    GifSeqHdr *gifHdr;
{
    FREE ((char *) gifHdr);
}


int
GifSeqHdrParse (bp, gifHdr)
    BitParser *bp;
    GifSeqHdr *gifHdr;
{
    char hdr[6];
    unsigned char byte;

    /*
     * Read the gif's signature and version number.
     */

    Bp_GetByteArray (bp, 6, hdr);

    if (strncmp (hdr, "GIF", 3) != 0) {
        return DVM_GIF_BAD_HEADER;
    }
    if ((strncmp (hdr + 3, "87a", 3) != 0) && (strncmp (hdr + 3, "89a", 3) != 0)) {
        return DVM_GIF_BAD_VERSION;
    }
    strncpy (gifHdr->version, hdr + 3, 3);

    /*
     * Read in the GIF's logical screen descriptor
     */
    Bp_GetLittleShort (bp, gifHdr->width);
    Bp_GetLittleShort (bp, gifHdr->height);
    Bp_GetByte (bp, byte);

    gifHdr->ctFlag = (byte & 0x80) >> 7;
    gifHdr->resolution = ((byte & 0x70) >> 4) + 1;

    if (gifHdr->ctFlag) {
        gifHdr->ctSorted = (byte & 0x08) >> 3;
        gifHdr->bitsPerPixel = (byte & 0x07) + 1;
        gifHdr->ctSize = (1 << gifHdr->bitsPerPixel);
    }
    Bp_GetByte (bp, gifHdr->backgroundColor);
    Bp_GetByte (bp, gifHdr->aspectRatio);

    Bp_PeekByte (bp, byte);
    if (byte == GIF_EXTENSION_INTRO) {
        Bp_PeekNextByte (bp, byte);
        if (byte == GIF_APPLICATION_LABEL) {
            Bp_FlushBytes (bp, 13);
            while (byte)
                Bp_GetByte (bp, byte);
        }
    }
    return DVM_GIF_OK;

}


void
GifSeqHdrEncode (gifHdr, bp)
    GifSeqHdr *gifHdr;
    BitParser *bp;
{
    unsigned char b;
    char bytes[4] = "GIF";

    /*
     * Write the Magic header
     */
    Bp_PutByteArray (bp, 3, bytes);
    Bp_PutByteArray (bp, 3, gifHdr->version);

    /*
     * Write out the screen width and height
     */
    Bp_PutLittleShort (bp, gifHdr->width);
    Bp_PutLittleShort (bp, gifHdr->height);

    /*
     * Indicate if there is a global colour map
     */
    if (gifHdr->ctFlag)
        b = 0x80;               /* Yes, there is a color map */
    else
        b = 0;

    /*
     * OR in the resolution
     * XXX : ctSorted not encoded ? Why ?
     */
    b |= (gifHdr->resolution - 1) << 4;
    b |= (COLORBPP(gifHdr->ctSize) - 1);

    /*
     * Write it out
     */
    Bp_PutByte (bp, b);

    /*
     * Write out the Background colour and aspect ratio
     */
    Bp_PutByte (bp, gifHdr->backgroundColor);
    Bp_PutByte (bp, gifHdr->aspectRatio);

}


void
GifSeqTrailerEncode (bp)
    BitParser *bp;
{
    /*
     * Write the GIF file terminator
     */
    Bp_PutByte (bp, ';');
}


void 
GifSeqLoopEncode (bp)
    BitParser *bp;
{
    /*
     * Encode the extension introduction, the application label, and the
     * size of the application name
     */
    Bp_PutByte (bp, GIF_EXTENSION_INTRO);
    Bp_PutByte (bp, GIF_APPLICATION_LABEL);
    Bp_PutByte (bp, 11);

    /*
     * Encode the string "Netscape2.0"
     */
    Bp_PutByte (bp, 78);
    Bp_PutByte (bp, 69);
    Bp_PutByte (bp, 84);
    Bp_PutByte (bp, 83);
    Bp_PutByte (bp, 67);
    Bp_PutByte (bp, 65);
    Bp_PutByte (bp, 80);
    Bp_PutByte (bp, 69);
    Bp_PutByte (bp, 50);
    Bp_PutByte (bp, 46);
    Bp_PutByte (bp, 48);

    /*
     * Encode the loop constants
     */
    Bp_PutByte (bp, 3);
    Bp_PutByte (bp, 1);
    Bp_PutByte (bp, 0);
    Bp_PutByte (bp, 0);
    Bp_PutByte (bp, 0);
}


void 
GifSeqHdrSetCtSize(seqHdr, size)
    GifSeqHdr *seqHdr;
    int size;
{
    seqHdr->ctSize = size;
    seqHdr->bitsPerPixel = COLORBPP(size);
}
