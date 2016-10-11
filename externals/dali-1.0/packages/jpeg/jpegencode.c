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
 * jpegencode.c
 *
 * This files contains routines to encode a JPEG image.
 *
 * Parts of this file are based on code under the following
 * copyrights.  Include these copyrights if you do anything
 * with this code
 *
 * Copyright (C) 1997-1998 Cornell University
 * Sugata Mukhopadhyay sugata@cs.cornell.edu
 * Wei Tsang Ooi weitsang@cs.cornell.edu
 *
 */

/*
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 */

/*
 *
 * Andrew Swan (aswan@cs.berkeley.edu)
 * Department of Computer Science,
 * University of California, Berkeley
 *
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 */


#include "jpegInt.h"

/*
 * Static Prototypes.
 */
static void EncodeScBlock(EncInfo *ei, ScBlock *block, BitParser *bp);

/*
 * ZigZag table
static char zz[] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18,
    11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49,
    56, 57, 50, 43, 36, 29, 22, 15, 23, 30,
    37, 44, 51, 58, 59, 52, 45, 38, 31, 39,
    46, 53, 60, 61, 54, 47, 55, 62, 63 
};
 */


int 
JpegHdrQtEncode (hdr, bp)
    JpegHdr *hdr;
    BitParser *bp;
{
    int prec, i, j, startPos;
    startPos = BitParserTell(bp);

    for (i = 0; i < hdr->numOfQts; i++) {
        PutMarker(bp,DQT);
        prec = (hdr->qt[i].precision/8) -1; /* 0 for 8 bit, 1 for 16 */
        Bp_PutShort(bp, prec ? (64 * 2 + 1 + 2) : (64 + 1 + 2)); /* length */
        Bp_PutByte(bp, i + (prec<<4));

        for (j = 0; j < 64; j++) {
            if (prec)
                Bp_PutByte (bp, (hdr->qt[i].v[j]) >> 8);
            Bp_PutByte (bp, (hdr->qt[i].v[j]) & 0xff);
        }
    }
    
    return (BitParserTell(bp) - startPos);
}

int 
JpegHdrHtEncode (hdr, bp)
    JpegHdr *hdr;
    BitParser *bp;
{
    int i, j, length, startPos;
    JpegHt *ht;
    startPos = BitParserTell(bp);

    /* Emit the Huffman tables */
    for (i = 0; i < 4; i++) {
        if (hdr->ht[i].valid & 0x01) {
            /* DC table is valid */
            length = 0;
            PutMarker(bp,DHT);
            ht = &(hdr->ht[i]);
            for (j=0; j<16; j++) {
                length += ht->dcBits[j];
            }
            Bp_PutShort(bp,length+19);
            Bp_PutByte(bp,i);
            
            for (j=0; j<16; j++) {
                Bp_PutByte(bp,ht->dcBits[j]);
            }
            for (j=0; j<length; j++) {
                Bp_PutByte(bp,ht->dcVals[j]);
            }
        }
        if (hdr->ht[i].valid & 0x02) {
            /* AC table is valid */
            length = 0;
            PutMarker(bp,DHT);
            ht = &(hdr->ht[i]);
            for (j=0; j<16; j++) {
                length += ht->acBits[j];
            }
            Bp_PutShort(bp,length+19);
            Bp_PutByte(bp,0x10+i); /* Set the AC Bit */
            
            for (j=0; j<16; j++) {
                Bp_PutByte(bp,ht->acBits[j]);
            }
            for (j=0; j<length; j++) {
                Bp_PutByte(bp,ht->acVals[j]);
            }
        }
    }

    return (BitParserTell(bp) - startPos);
}


int
JpegHdrEncode (hdr, baseline, bp)
    JpegHdr *hdr;
    int baseline;
    BitParser *bp;
{
    int i;
    int startPos;

    startPos = BitParserTell(bp);

    /*
     * The APP block should not be encoded here.
     * JPEG files encoded by Dali does not have APP section.

    PutMarker(bp,APP0);
    Bp_PutShort(bp,16);
    Bp_PutByte(bp, 0x4A);       // Identifier: ASCII "JFIF" 
    Bp_PutByte(bp, 0x46);
    Bp_PutByte(bp, 0x49);
    Bp_PutByte(bp, 0x46);
    Bp_PutByte(bp, 0);
    Bp_PutByte(bp, 1);          // Major version 
    Bp_PutByte(bp, 1);          // Minor version 
    Bp_PutByte(bp, 0);          // Pixel size information 
    Bp_PutShort(bp, 0);
    Bp_PutShort(bp, 0);
    Bp_PutByte(bp, 0);          // No thumbnail image 
    Bp_PutByte(bp, 0);
    
    */

    /* Now the quantization tables */

    /* Emit the proper SOF marker */
    if (baseline) {
        PutMarker(bp,SOF0);
    } else {
        PutMarker(bp,SOF1);
    }

    /* Now the SOF data */
    Bp_PutShort (bp, (3 * hdr->numComps + 2 + 5 + 1));  /* length */

    Bp_PutByte (bp, hdr->precision);
    Bp_PutShort (bp, (int)hdr->height);
    Bp_PutShort (bp, (int)hdr->width);

    Bp_PutByte (bp, hdr->numComps);

    for (i = 0; i < hdr->numComps; i++) {
        Bp_PutByte (bp, i); /* We dont use ids here */
        Bp_PutByte (bp, (hdr->blockWidth[i] << 4) + hdr->blockHeight[i]);
        Bp_PutByte (bp, hdr->qtId[i]);
    }

    return (BitParserTell(bp) - startPos);
}

int
JpegScanHdrEncode (hdr, scanHdr, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    BitParser *bp;
{
    int i;
    int startPos;

    startPos = BitParserTell(bp);

    /* Define restart interval if required */
    if (hdr->restartInterval) {
        PutMarker(bp,DRI);
        Bp_PutShort(bp,4);
        Bp_PutShort(bp,(int)hdr->restartInterval);
    }

    /* Now the real scan header */
    PutMarker(bp,SOS);
    Bp_PutShort(bp, (2*scanHdr->numComps + 6)); /* length */
    Bp_PutByte(bp,scanHdr->numComps);

    for (i = 0; i < scanHdr->numComps; i++) {
        Bp_PutByte(bp, scanHdr->scanid[i]);
        Bp_PutByte(bp, (scanHdr->dcid[i] << 4)
                   + scanHdr->acid[i]);
    }

    Bp_PutByte(bp, 0);          /* Spectral selection start */
    Bp_PutByte(bp, 63);         /* Spectral selection end */
    Bp_PutByte(bp, 0);          /* Successive approximation */
    return (BitParserTell(bp) - startPos);
}

int
JpegScanEncode (hdr, scanHdr, huffTable, scans, numOfScans, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    JpegHuffTable *huffTable;
    ScImage *scans[];
    int numOfScans;
    BitParser *bp;
{
    int i, j, x, y, w, h, k, r;
    int numComps, numPieces, id;
    EncInfo encInfo[4], *ei;
    MCU mcuInfo[10];
    unsigned short *dcco, *dcsi, *acco, *acsi;
    unsigned char *acnb;
    unsigned short *qval;
    short temp, tempmag, index;
    int maxbw, maxbh, wFull, hFull;
    int mcu, mcux, mcuy, maxmcu, piece;
    int mcuWidth, mcuHeight, nbits, dc;
    int startPos;
    ScBlock *block;

    ei = NULL; /* make compiler happy */
    startPos = BitParserTell(bp);

    numComps = scanHdr->numComps;

    /* Check if we have the right number of sc bufs */
    if (numComps != numOfScans) {
        return DVM_JPEG_WRONG_COMPONENTS;
    }

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in encInfo[]
     */
    numPieces = 0;
    for (i=0; i<numComps; i++) {
        ei = &encInfo[i];
        ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, i);
        ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, i);
        ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, i);
        ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, i);
        ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, i);
        ei->predictor = 0;
        ei->blockPtr = scans[i]->firstBlock;
        id = scanHdr->scanid[i];
        ei->bw = hdr->blockWidth[id];
        ei->bh = hdr->blockHeight[id];
        /* For dequantized storage */
        id = hdr->qtId[id];
        ei->qvals = (unsigned short *)(hdr->qt[id]).v;
        /* End for dequantized storage */
        for (j=0; j<ei->bw * ei->bh; j++) {
            mcuInfo[numPieces].x = j % ei->bw;
            mcuInfo[numPieces].y = j / ei->bw;
            mcuInfo[numPieces++].compNum = i;
        }
    }

    if (numComps == 1) {
        ei->bw = ei->bh = 1;
        maxbw = maxbh = 1;
        numPieces = 1;
    } else {
        maxbw = hdr->maxBlockWidth;
        maxbh = hdr->maxBlockHeight;
    }

    /*
     * Compute the parameters for each MCU:
     *  (w,h) give the dimension of the image in pixels.
     *  (mcuWidth,mcuHeight) give the dimensions of an MCU (in pixels)
     *  (wFull,hFull) give the dimensions of the image in MCUs
     *  maxmcu is the number of MCUs in this scan
     */
    mcuWidth = maxbw * 8;
    mcuHeight = maxbh * 8;
    w = hdr->width;
    h = hdr->height;

    wFull = (int)ceil((double)w/mcuWidth);
    hFull = (int)ceil((double)h/mcuHeight);
    maxmcu = wFull*hFull;  

    for (i=0; i<numComps; i++) {
        ei = &encInfo[i];
        ei->w = wFull*ei->bw;
        ei->h = hFull*ei->bh;
        if (scans[i]->width != ei->w) {
            return DVM_JPEG_INVALID_WIDTH;
        }
        if (scans[i]->height != ei->h) {
            return DVM_JPEG_INVALID_HEIGHT;
        }
        ei->numBlocks = ei->w * ei->h;
        ei->scanpad = scans[i]->parentWidth;
    }

    /*
     * Main encode loop
     */
    for (mcu = 0; mcu < maxmcu; mcu++) {
        /*
         * Compute x,y offset of first block in MCU within its
         * component's block array
         */

        mcux = (mcu % wFull);
        mcuy = (mcu / wFull);
        
        /*
         * Encode all the MCUs
         */
        for (piece = 0; piece < numPieces; piece++) {
            /*
             * Calculate everything specific to the plane we are
             * are encoding to avoid conditionals in the main loop
             */
            id = mcuInfo[piece].compNum;
            ei = &encInfo[id];
            dcco = ei->dcxhufco+2047;
            dcsi = ei->dcxhufsi+2047;
            acco = ei->acehufco;
            acsi = ei->acehufsi;
            acnb = ei->acnbits;
            qval = ei->qvals;
            x = mcux*ei->bw + mcuInfo[piece].x;
            y = mcuy*ei->bh + mcuInfo[piece].y;
            block = ei->blockPtr + x + y*ei->scanpad;
            /*
             * Encode the DC value
             */
            temp = dc = ((block->dc - NORMAL_OFFSET)/qval[0]);
            dc -= ei->predictor;
            ei->predictor = temp;
            Bp_PutBits(bp, dcsi[dc], dcco[dc]);

            /*
             * Encode the AC values
             */
            k = 1;
            if (block->numOfAC > 63) block->numOfAC = 0;
            for (i=0; i < block->numOfAC; i++) {
                
                index = block->index[i];
                tempmag = temp = (block->value[i])/qval[index];
                
                if (temp == 0) {
                    continue;
                }
        
                if (temp < 0) {
                    tempmag = -temp;
                    temp--;
                }
                r = index - k;
                k = index + 1;

                /* while (r & ~0xF) */
                while (r > 15) { 
                    Bp_PutBits(bp, acsi[0xf0], acco[0xf0]);
                    r -= 16;
                }
                nbits = acnb[tempmag];
                r = (r << 4)|nbits;
                Bp_PutBits(bp, acsi[r], acco[r]);
                Bp_PutBits(bp, nbits, temp);
            }

            /* Emit EOB if needed */
            if (k != 64) {
                Bp_PutBits(bp, acsi[0], acco[0]);
            }
        }
    }

    Bp_OutByteAlign(bp);
    return (BitParserTell(bp) - startPos);
}


int
JpegScanEncode420 (hdr, scanHdr, huffTable, scy, scu, scv, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    JpegHuffTable *huffTable;
    ScImage *scy;
    ScImage *scu;
    ScImage *scv;
    BitParser *bp;
{
    int i, j;
    int id;
    EncInfo encInfo[4], *ei;
    int startPos;
    ScBlock *currY1, *currY2, *currU, *currV;

    startPos = BitParserTell(bp);

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in encInfo[]
     */
    ei = &encInfo[0];
    ei->predictor = 0;
    id = scanHdr->scanid[0];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 0);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 0);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 0);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 0);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 0);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;
    
    ei = &encInfo[1];
    ei->predictor = 0;
    id = scanHdr->scanid[1];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 1);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 1);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 1);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 1);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 1);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;

    ei = &encInfo[2];
    ei->predictor = 0;
    id = scanHdr->scanid[2];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 2);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 2);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 2);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 2);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 2);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;

    /*
     * Main encode loop
     */
    currY1 = scy->firstBlock;
    currY2 = currY1 + scy->parentWidth;
    currU  = scu->firstBlock;
    currV  = scv->firstBlock;

    for (i = 0; i < scu->height; i++) {
        for (j = 0; j < scu->width; j++) {
            
            ei = &encInfo[0];
            EncodeScBlock(ei, currY1, bp);
            currY1++;

            EncodeScBlock(ei, currY1, bp);
            currY1++;

            EncodeScBlock(ei, currY2, bp);
            currY2++;

            EncodeScBlock(ei, currY2, bp);
            currY2++;
            ei = &encInfo[1];

            EncodeScBlock(ei, currU, bp);
            currU++;
            ei = &encInfo[2];

            EncodeScBlock(ei, currV, bp);

            currV++;
        }
        currU += scu->parentWidth - scu->width;
        currV += scv->parentWidth - scv->width;
        currY1 += 2*scy->parentWidth - scy->width;
        currY2 += 2*scy->parentWidth - scy->width;
    }

    Bp_OutByteAlign(bp);
    return (BitParserTell(bp) - startPos);
}

int
JpegScanEncode422 (hdr, scanHdr, huffTable, scy, scu, scv, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    JpegHuffTable *huffTable;
    ScImage *scy;
    ScImage *scu;
    ScImage *scv;
    BitParser *bp;
{
    int i, j;
    int id;
    EncInfo encInfo[4], *ei;
    int startPos;
    ScBlock *currY, *currU, *currV;

    startPos = BitParserTell(bp);

    /*
     * Gather up all the information about the components
     * in this scan.  This information is spread across
     * scanHdr, hdr, and scan[].  We put all the relevant
     * info in encInfo[]
     */
    ei = &encInfo[0];
    ei->predictor = 0;
    id = scanHdr->scanid[0];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 0);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 0);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 0);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 0);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 0);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;
    
    ei = &encInfo[1];
    ei->predictor = 0;
    id = scanHdr->scanid[1];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 1);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 1);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 1);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 1);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 1);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;

    ei = &encInfo[2];
    ei->predictor = 0;
    id = scanHdr->scanid[2];
    ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 2);
    ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 2);
    ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 2);
    ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 2);
    ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 2);
    id = hdr->qtId[id];
    ei->qvals = (unsigned short *)(hdr->qt[id]).v;

    /*
     * Main encode loop
     */
    currY = scy->firstBlock;
    currU  = scu->firstBlock;
    currV  = scv->firstBlock;

    for (i = 0; i < scu->height; i++) {
        for (j = 0; j < scu->width; j++) {
            
            ei = &encInfo[0];
            EncodeScBlock(ei, currY, bp);
            currY++;
            EncodeScBlock(ei, currY, bp);
            currY++;

            ei = &encInfo[1];
            EncodeScBlock(ei, currU, bp);
            currU++;

            ei = &encInfo[2];
            EncodeScBlock(ei, currV, bp);
            currV++;
        }
        currU += scu->parentWidth - scu->width;
        currV += scv->parentWidth - scv->width;
        currY += scy->parentWidth - scy->width;
    }

    Bp_OutByteAlign(bp);
    return (BitParserTell(bp) - startPos);
}


static void
EncodeScBlock(ei, block, bp)
    EncInfo *ei;
    ScBlock *block;
    BitParser *bp;
{
    int i, k, r;
    unsigned short *dcco, *dcsi, *acco, *acsi;
    unsigned char *acnb;
    unsigned short *qval;
    char  *currIndex;
    short *currValue;
    short temp, tempmag, index;
    int nbits, dc;

#ifdef DEBUG_ENCODESCBLOCK
    static int count = 0;
    fprintf(stderr, "-- BLOCK %d --\n", count++);
#endif
    /*
     * Encode 4 blocks from SC Y
     */
    dcco = ei->dcxhufco+2047;
    dcsi = ei->dcxhufsi+2047;
    acco = ei->acehufco;
    acsi = ei->acehufsi;
    acnb = ei->acnbits;
    qval = ei->qvals;

    /*
     * Encode the DC value
     */
    temp = dc = ((block->dc - NORMAL_OFFSET)/(qval[0]));
    dc -= ei->predictor;
    ei->predictor = temp;
    Bp_PutBits(bp, dcsi[dc], dcco[dc]);

    /*
     * Encode the AC values
     */
    k = 1;

    currIndex = block->index;
    currValue = block->value;
    for (i=0; i < block->numOfAC; i++) {
        short q, v;
        v     = *currValue++;
        index = *currIndex++;
        q     = qval[index];
        // q     = qval[(int)zz[index]];
        if (v < 0) {
            v -= (q >> 1);
            if (-v < q)
                continue;
            temp = v/q;
            tempmag = -temp;
            temp--;
        } else {
            v += (q >> 1);
            if (v < q)
                continue;
            tempmag = temp = v/q;
        }

        r = index - k;
        k = index + 1;

        /* while (r & ~0xF) {*/
        while (r > 15) { 
            Bp_PutBits(bp, acsi[0xf0], acco[0xf0]);
            r -= 16;
        }
        
#ifdef DEBUG_ENCODESCBLOCK
        fprintf(stderr, "%d %d\n", v, temp);
#endif
        nbits = acnb[tempmag];
        r = (r << 4)|nbits;
        Bp_PutBits(bp, acsi[r], acco[r]);
        Bp_PutBits(bp, nbits, temp);
    }

    /* Emit EOB if needed */
    if (k != 64) {
        Bp_PutBits(bp, acsi[0], acco[0]);
    }
}

int 
JpegStartCodeEncode(bp)
    BitParser *bp;
{
    PutMarker(bp,SOI);
    return 2;
}

int
JpegEndCodeEncode (bp)
    BitParser *bp;

{
    PutMarker(bp,EOI);
    return 2;
}

int
JpegScanIncEncode420 (hdr, scanHdr, huffTable, scy, scu, scv, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    JpegHuffTable *huffTable;
    ScImage *scy;
    ScImage *scu;
    ScImage *scv;
    BitParser *bp;
{
    int i, j;
    int id;
    static EncInfo encInfo[4], *ei;
    static int started = 0;
    int startPos;
    ScBlock *currY1, *currY2, *currU, *currV;


    if (scy != NULL) {
        if (!started) {
            return 0;
        }
        startPos = BitParserTell(bp);
    } else {
        if (!started) {

            /*
             * Gather up all the information about the components
             * in this scan.  This information is spread across
             * scanHdr, hdr, and scan[].  We put all the relevant
             * info in encInfo[]
             */
            ei = &encInfo[0];
            ei->predictor = 0;
            id = scanHdr->scanid[0];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 0);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 0);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 0);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 0);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 0);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;
            
            ei = &encInfo[1];
            ei->predictor = 0;
            id = scanHdr->scanid[1];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 1);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 1);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 1);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 1);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 1);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;

            ei = &encInfo[2];
            ei->predictor = 0;
            id = scanHdr->scanid[2];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 2);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 2);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 2);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 2);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 2);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;
            started = 1;
        } else {
            Bp_OutByteAlign(bp);
            started = 0;
        }
        return 0;
    }

    /*
     * Main encode loop
     */
    currY1 = scy->firstBlock;
    currY2 = currY1 + scy->parentWidth;
    currU  = scu->firstBlock;
    currV  = scv->firstBlock;

    for (i = 0; i < scu->height; i++) {
        for (j = 0; j < scu->width; j++) {
            
            ei = &encInfo[0];
            EncodeScBlock(ei, currY1, bp);
            currY1++;

            EncodeScBlock(ei, currY1, bp);
            currY1++;

            EncodeScBlock(ei, currY2, bp);
            currY2++;

            EncodeScBlock(ei, currY2, bp);
            currY2++;
            ei = &encInfo[1];

            EncodeScBlock(ei, currU, bp);
            currU++;
            ei = &encInfo[2];

            EncodeScBlock(ei, currV, bp);

            currV++;
        }
        currU += scu->parentWidth - scu->width;
        currV += scv->parentWidth - scv->width;
        currY1 += 2*scy->parentWidth - scy->width;
        currY2 += 2*scy->parentWidth - scy->width;
    }

    return (BitParserTell(bp) - startPos);
}


int
JpegScanIncEncode422 (hdr, scanHdr, huffTable, scy, scu, scv, bp)
    JpegHdr *hdr;
    JpegScanHdr *scanHdr;
    JpegHuffTable *huffTable;
    ScImage *scy;
    ScImage *scu;
    ScImage *scv;
    BitParser *bp;
{
    int i, j;
    int id;
    static EncInfo encInfo[4], *ei;
    static int started = 0;
    int startPos;
    ScBlock *currY1, *currU, *currV;


    if (scy != NULL) {
        if (!started) {
            return 0;
        }
        startPos = BitParserTell(bp);
    } else {
        if (!started) {

            /*
             * Gather up all the information about the components
             * in this scan.  This information is spread across
             * scanHdr, hdr, and scan[].  We put all the relevant
             * info in encInfo[]
             */
            ei = &encInfo[0];
            ei->predictor = 0;
            id = scanHdr->scanid[0];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 0);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 0);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 0);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 0);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 0);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;
            
            ei = &encInfo[1];
            ei->predictor = 0;
            id = scanHdr->scanid[1];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 1);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 1);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 1);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 1);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 1);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;

            ei = &encInfo[2];
            ei->predictor = 0;
            id = scanHdr->scanid[2];
            ei->dcxhufco = JpegHuffTableGetDcCodeTable(huffTable, 2);
            ei->dcxhufsi = JpegHuffTableGetDcSymbolTable(huffTable, 2);
            ei->acehufco = JpegHuffTableGetAcCodeTable(huffTable, 2);
            ei->acehufsi = JpegHuffTableGetAcSymbolTable(huffTable, 2);
            ei->acnbits = JpegHuffTableGetAcNumOfBitsTable(huffTable, 2);
            id = hdr->qtId[id];
            ei->qvals = (unsigned short *)(hdr->qt[id]).v;
            started = 1;
        } else {
            Bp_OutByteAlign(bp);
            started = 0;
        }
        return 0;
    }

    /*
     * Main encode loop
     */
    currY1 = scy->firstBlock;
    currU  = scu->firstBlock;
    currV  = scv->firstBlock;

    for (i = 0; i < scu->height; i++) {
        for (j = 0; j < scu->width; j++) {
            
            ei = &encInfo[0];
            EncodeScBlock(ei, currY1, bp);
            currY1++;
            EncodeScBlock(ei, currY1, bp);
            currY1++;
            
            ei = &encInfo[1];
            EncodeScBlock(ei, currU, bp);
            currU++;

            ei = &encInfo[2];
            EncodeScBlock(ei, currV, bp);
            currV++;
        }
        currU += scu->parentWidth - scu->width;
        currV += scv->parentWidth - scv->width;
        currY1 += scy->parentWidth - scy->width;
    }

    return (BitParserTell(bp) - startPos);
}
