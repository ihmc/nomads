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
 * jpegInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _JPEG_INT_
#define _JPEG_INT_

#define JPEG_BYTE_STUFF

#include "dvmjpeg.h"
#include "dvmbasic.h"

#undef JPEG_BYTE_STUFF

extern int bitmask[];


/*
 * The following macro decodes a number is the format that jpeg stores it.
 *   v is the value to devode
 *   t is the length of the value (in bits);
 *   res is the variable to write into.
 */
#define EXT_CONST 8


#define  DECIMATION_411  1
#define  DECIMATION_422  2

#define PutMarker(bp,marker)                                        \
{                                                                   \
    Bp_PutByte(bp,0xff);                                            \
    Bp_PutByte(bp,marker);                                          \
}

/*
 * JPEG Markers
 */
#define SOF0      0xc0
#define SOF1      0xc1
#define SOF2      0xc2
#define SOF3      0xc3

#define SOF5      0xc5
#define SOF6      0xc6
#define SOF7      0xc7

#define JPG       0xc8
#define SOF9      0xc9
#define SOF10     0xca
#define SOF11     0xcb
#define SOF13     0xcd
#define SOF14     0xce
#define SOF15     0xcf

#define DHT       0xc4

#define DAC       0xcc

#define RST0      0xd0
#define RST1      0xd1
#define RST2      0xd2
#define RST3      0xd3
#define RST4      0xd4
#define RST5      0xd5
#define RST6      0xd6
#define RST7      0xd7

#define SOI       0xd8
#define EOI       0xd9
#define SOS       0xda
#define DQT       0xdb
#define DNL       0xdc
#define DRI       0xdd
#define DHP       0xde
#define EXP       0xdf

#define APP0      0xe0
#define APP15     0xef

#define JPG0      0xf0
#define JPG13     0xfd
#define COM       0xfe

#define TEM       0x01

#define ERROR     0x100
/*
 * To undo the effect of the level shift and make sc buffers compatible
 * between jpeg nd mpeg
 */

#define NORMAL_OFFSET (1<<10) 


/*
 *  The following structure tracks information about an MCU.
 */
typedef struct MCU {
    int compNum; /* component number to which this MCU belongs. */
    int x, y;    /* x,y offset of a block within an mcu.
                  * resets to (0,0) at beginning of new scan */
} MCU;

typedef struct EncInfo {
    short predictor;              /* DC predictor */
    short bw, bh;                 /* Block width and height (JPEG
                                   * sampling factors) */
    int w, h;                     /* width and height, in blocks */
    int numBlocks;                /* number of blocks in this component */
    int scanpad;
    unsigned short *dcxhufco, *dcxhufsi;  /* Huffman tables */
    unsigned short *acehufco, *acehufsi;
    unsigned char *acnbits;
    unsigned short *qvals;        /* Quant. table */
    ScBlock *blockPtr;            /* Tracks the block in this component */
} EncInfo;

typedef struct CompInfo {
    short ignored;                /* Are we ignoring this component */
    short predictor;              /* DC predictor */
    short bw, bh;                 /* Block width and height (JPEG
                                   * sampling factors) */
    int w, h;                     /* width and height, in blocks */
    int numBlocks;                /* number of blocks in this component */
    int scanpad;
    unsigned short *dcht, *acht;  /* Huffman tables */
    unsigned short *qvals;        /* Quant. table */
    ScBlock *blockPtr;            /* Tracks the block in this component */
} CompInfo;

#endif
