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
 * gifInt.h
 *
 *----------------------------------------------------------------------
 */

#ifndef _GIF_INT_
#define _GIF_INT_

#include "dvmgif.h"
#include "dvmbasic.h"
#include "bitparser.h"

extern int bitmask[];

#define BIT7(x) (((x)&0x80)>>7)
#define BIT6(x) (((x)&0x40)>>6)
#define BIT5(x) (((x)&0x20)>>5)
#define BIT4(x) (((x)&0x10)>>4)
#define BIT3(x) (((x)&0x08)>>3)
#define BIT2(x) (((x)&0x04)>>2)
#define BIT1(x) (((x)&0x02)>>1)
#define BIT0(x) ((x)&0x01)



#define GIF_IMG_SEPARATOR 0x2c
#define GIF_EXTENSION_INTRO 0x21
#define GIF_GRAPHIC_CONTROL_LABEL 0xF9
#define GIF_APPLICATION_LABEL 0xFF
#define GIF_COMMENT_LABEL 0xFE
#define GIF_PLAIN_TEXT_LABEL 0x01
#define GIF_TRAILER 0x3B

#define Bp_PeekNextByte(bp, retval)  retval = *(bp->offsetPtr+1)



int ReadInterlacedGifImage (BitParser *, GifImgHdr * hdr, ByteImage *);
int ReadNonInterlacedGifImage (BitParser *, GifImgHdr * hdr, ByteImage *);
int IgnoreGifImage (BitParser *);
void Compress (int init_bits, BitParser * bp, ByteImage * srcBuf, GifImgHdr * imgHdr);

extern char theBppTable[];
#define COLORBPP(i) (int)theBppTable[i]


#endif
