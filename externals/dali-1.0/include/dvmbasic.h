/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * dvmbasic.h
 *
 * Wei Tsang Jan 98
 *
 * contains definitions for DVM core data structures and their primitives
 * - ByteImage, BitImage, BitImageScan
 *
 *----------------------------------------------------------------------
 */

#ifndef _DVM_BASIC_
#define _DVM_BASIC_


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "macro.h"

/* quantization tables */
extern int ZAG[];
extern int default_jpeg_y_quant[];
extern int default_jpeg_uv_quant[];

extern int default_mpeg_intra_quant[];
extern int default_mpeg_non_intra_quant[];

#define JPEG_LUM        default_jpeg_y_quant
#define JPEG_CHROM      default_jpeg_uv_quant
#define MPEG_INTRA      default_mpeg_intra_quant
#define MPEG_NON_INTRA  default_mpeg_non_intra_quant

/*
 *----------------------------------------------------------------------
 *
 * type ByteImage
 *
 *     An abstraction of a 2D array of bytes.
 *     There are two types of ByteImages: physical and virtual.
 *     For physical ByteImages, the data buffer is contiguous.
 *     In virtual ByteImages, the data belongs to a "parent"
 *     byte image.  In other words, a virtual ByteImage is a
 *     portion of a parent ByteImage, and shares its memory.
 *     The VM should never write outside the bounds of the
 *     virtual buffer.
 *
 *     Note : Wei Tsang removed parentHeight, parentName, buffer on 30/1/98
 *            and changed virtual to unsigned char.
 *            We should change parentWidth to rootWidth if we have time..
 *
 *----------------------------------------------------------------------
 */

typedef struct ByteImage {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    unsigned char *firstByte;       /* pointer to the first byte for this ByteImage */
} ByteImage;

/*
 *----------------------------------------------------------------------
 * Now define a bunch of macro to get the attributes of ByteImage.
 * parentWidth and firstByte are really for internal used so no need to
 * retrive them.
 *----------------------------------------------------------------------
 */

#define DVM_BYTE_OK 0
#define DVM_BYTE_ERROR 1

#define ByteGetWidth(b) (b)->width
#define ByteGetHeight(b) (b)->height
#define ByteGetX(b) (b)->x
#define ByteGetY(b) (b)->y
#define ByteGetVirtual(b) (b)->isVirtual

/*
 *----------------------------------------------------------------------
 *
 * type BitImage
 *
 *     An abstraction of a 2D array of bits.
 *     Just like ByteImage, it can be either physical and virtual.
 *
 *----------------------------------------------------------------------
 */

typedef struct BitImage {

    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int height;             /* height of this buffer in bytes */
    int byteWidth;          /* width of this buffer in bytes, excluding partial bytes */
    int unitWidth;          /* width of this buffer in bit */
    int parentWidth;        /* usedWidth of the parent buffer in bytes */
    unsigned char isVirtual;/* 0 iff buffer is physical, 1 indicates virtual */
    unsigned char firstBit; /* Offset 0-7 within the first byte on every row */
    unsigned char lastBit;  /* Offset 1-7,0 within the last byte on every row */
    unsigned char *firstByte;/* pointer to the first byte for this buffer
                              * in the buffer array */
} BitImage;

#define BitGetX(bit) (bit)->x
#define BitGetY(bit) (bit)->y
#define BitGetWidth(bit) (bit)->unitWidth
#define BitGetHeight(bit) (bit)->height
#define BitGetVirtual(bit) (bit)->isVirtual
#define BitIsAligned(bit) ((bit)->firstBit == 0 && (bit)->lastBit == 0)
#define BitIsLeftAligned(bit) ((bit)->firstBit == 0)

#define DVM_BIT_OK   0
#define DVM_BIT_IS_BYTE_ALIGN -1
#define DVM_BIT_NOT_BYTE_ALIGN -2

/*
 *----------------------------------------------------------------------
 *
 * type BitImageScan
 *
 *     A structure that keep the current cursor position in BitImage
 *
 *----------------------------------------------------------------------
 */

typedef struct BitImageScan {
    unsigned char *currByte;/* curr byte to start scan on next get extent call */
    int currBit;            /* cutt bit (0 - 7) in curr byte to start scan on
                             * the next get extent call */
    BitImage *region;       /* bitmap region to be used to scan the buffer */
    int x, y;               /* x and y coordinates of the currXXXByte in their
                             * respective buffers. */
    int regionSkip;         /* precomputed values for offset to add when
                             * skipping to the next row */
    int unitWidth;
    int byteWidth;
    int height;
    int lastBit;
} BitImageScan;

/*
 *----------------------------------------------------------------------
 *
 * type ScBlock
 *
 *     An ScBlock stores an block of semi-compress (huffman decoded
 *     but not idct) data and is used in JPEG and MPEG package.
 *
 *----------------------------------------------------------------------
 */

typedef struct ScBlock {
    /* the first 3 field are meaningless for blocks from I pic/JPEG */
    int intracoded;         /* whether a block is intracoded. */
    char skipMB;            /* whether this is a block from a skipped MB */
    char skipBlock;         /* whether this block is skipped */
    short dc;               /* DC value of this block */
    char numOfAC;           /* number of AC component */
    char index[63];         /* arrays of max 63 (index, value) pairs */
    short value[63];
} ScBlock;

/*
 *----------------------------------------------------------------------
 *
 * type ScImage
 *
 *     The sc image is very similar to byte image, except that
 *     each atomic component is a semi-compress block instead of a byte.
 *     See byte.h for details description of an _image_ in general.
 *
 *----------------------------------------------------------------------
 */

typedef struct ScImage {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    ScBlock *firstBlock;    /* pointer to the first sc-block */
} ScImage;

#define DVM_SC_OK   0
#define DVM_SC_ERROR 1

#define ScGetWidth(b) (b)->width
#define ScGetHeight(b) (b)->height
#define ScGetX(b) (b)->x
#define ScGetY(b) (b)->y
#define ScGetVirtual(b) (b)->isVirtual

/*
 *----------------------------------------------------------------------
 *
 * type VectorImage
 *
 *     The vector image is very similar to byte image, except that
 *     each atomic component is two integer (x,y) instead of a byte.
 *     See byte.h for details description of an _image_ in general.
 *
 *----------------------------------------------------------------------
 */
typedef struct Vector {
    char exists;            /* 0 iff no vector */
    char down;
    char right;
} Vector;

typedef struct VectorImage {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    Vector *firstVector;    /* pointer to the first vector */
} VectorImage;

#define VectorGetWidth(v) (v)->width
#define VectorGetHeight(v) (v)->height
#define VectorGetX(v) (v)->x
#define VectorGetY(v) (v)->y
#define VectorGetVirtual(v) (v)->isVirtual

/*
 *----------------------------------------------------------------------
 *
 * type Byte16Image, Byte32Image, and FloatImage
 *
 * These type are similar in nature to ByteImage.  Each element is now
 * a short, an int and a float respectively.
 *
 *----------------------------------------------------------------------
 */

typedef struct Byte16Image {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    unsigned short *firstByte;       /* pointer to the first byte for this ByteImage */
} Byte16Image;

typedef struct Byte32Image {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    unsigned int *firstByte;       /* pointer to the first byte for this ByteImage */
} Byte32Image;

typedef struct FloatImage {
    int width;              /* width of this buffer */
    int height;             /* height of this buffer */
    int x;                  /* x origin of buffer w.r.t. parent */
    int y;                  /* y origin of buffer w.r.t. parent */
    int parentWidth;        /* height and width of the parent buffer */
    unsigned char isVirtual;        /* 0 indicates buffer is real, 1 indicates virtual */
    float *firstByte;       /* pointer to the first byte for this ByteImage */
} FloatImage;

#define DVM_BYTE_16_OK 0
#define DVM_BYTE_16_ERROR 1
#define DVM_BYTE_32_OK 0
#define DVM_BYTE_32_ERROR 1
#define DVM_FLOAT_OK 0
#define DVM_FLOAT_ERROR 1

#define Byte16GetWidth(b) (b)->width
#define Byte16GetHeight(b) (b)->height
#define Byte16GetX(b) (b)->x
#define Byte16GetY(b) (b)->y
#define Byte16GetVirtual(b) (b)->isVirtual
#define Byte32GetWidth(b) (b)->width
#define Byte32GetHeight(b) (b)->height
#define Byte32GetX(b) (b)->x
#define Byte32GetY(b) (b)->y
#define Byte32GetVirtual(b) (b)->isVirtual
#define FloatGetWidth(b) (b)->width
#define FloatGetHeight(b) (b)->height
#define FloatGetX(b) (b)->x
#define FloatGetY(b) (b)->y
#define FloatGetVirtual(b) (b)->isVirtual

/*
 *----------------------------------------------------------------------
 *
 * type BitStream
 *
 *     just a memory buffer
 *
 *----------------------------------------------------------------------
 */
typedef struct BitStream {
    unsigned char *buffer;  /* pointer to first byte of memory buffer */
    unsigned char *endDataPtr;      /* pointer to last valid byte in buffer */
    unsigned char *endBufPtr;       /* pointer to last byte in buffer */
    unsigned char isVirtual;        /* 1 iff this is a virtual BitStream */
    int size;               /* size of buffer in bytes     */
} BitStream;


/*
 *-----------------------------------------------------------------------
 *
 * type BitParser
 *
 *     BitParser reads from / write to buffer in BitStream and
 *     provides bit/byte level access to the data in buffer.
 *
 *-----------------------------------------------------------------------
 */
typedef struct BitParser {
    BitStream *bs;          /* bitstream to work on */
    unsigned char *offsetPtr;       /* cursor in the buffer */
    int currentBits;        /* 32 bits from buffer */
    int bitCount;           /* number of bits left in currentBits */
} BitParser;


/*
 *-----------------------------------------------------------------------
 *
 * type BitStreamFilter
 *
 *     BitStreamFilter maintains a sequence of (offset, length)
 *     to indicate interesting region from files or buffer.
 *
 *-----------------------------------------------------------------------
 */
typedef struct FilterEntry {
    unsigned int offset;
    unsigned int length;
} FilterEntry;

typedef struct BitStreamFilter {
    int maxEntry;           /* maximum number of entry allocated in table */
    int lastEntry;          /* last entry used in the table */
    int currEntry;          /* last entry read */
    int currOffset;         /* next offset to starting reading */
    int currLength;         /* next number of bytes to read */
    FilterEntry *table;     /* array of entries */
} BitStreamFilter;

#define DVM_STREAMS_OK 0
#define DVM_STREAMS_FILTER_FULL -1

/*
 * 8-bit and 16-bit audio buffer
 * to use as 16-bit audio buffer, just type-cast the
 * (unsigned char *) to (short *)
 */
typedef struct Audio {
    int start;              /* number of samples skipped from the physical buffer */
    int length;             /* number of samples in the current buffer */
    unsigned char *firstSample;     /* ptr to the first audio data sample */
    unsigned char *buffer;  /* ptr to the beginning of the physical buffer */
    char isVirtual;         /* 1 iff it's a virtual buffer */
} Audio;

#define DVM_AUDIO_OK 0
#define DVM_AUDIO_ERROR 1

#define AudioGetStartOffset(a) (a)->start
#define AudioGetNumOfSamples(a) (a)->length

/*
 * Basic ByteImage primitives
 */
ByteImage *ByteNew(int, int);
void ByteFree(ByteImage *);
void ByteReclip(ByteImage *, int, int, int, int, ByteImage *);
ByteImage *ByteClip(ByteImage *, int, int, int, int);
void ByteCopy(ByteImage *, ByteImage *);
void ByteCopyMux1(ByteImage *, int, int, ByteImage *, int, int);
void ByteCopyMux2(ByteImage *, int, int, ByteImage *, int, int);
void ByteCopyMux4(ByteImage *, int, int, ByteImage *, int, int);
void ByteSet(ByteImage *, unsigned char);
void ByteSetMux1(ByteImage *, int, int, unsigned char);
void ByteSetMux2(ByteImage *, int, int, unsigned char);
void ByteSetMux4(ByteImage *, int, int, unsigned char);
void ByteExtend(ByteImage *, int, int);
void ByteCopyWithMask(ByteImage *, BitImage *, ByteImage *);
void ByteSetWithMask(ByteImage *, BitImage *, unsigned char);
void ByteToSc(ByteImage *, ScImage *);
void ByteToScI(ByteImage *, ByteImage *, int *, ScImage *);
void ByteYToScP(ByteImage *, ByteImage *, VectorImage *, ByteImage *, int *, int *, ScImage *);
void ByteUVToScP(ByteImage *, ByteImage *, VectorImage *, ByteImage *, int *, int *, ScImage *);
void ByteYToScB(ByteImage *, ByteImage *, ByteImage *, VectorImage *, VectorImage *, ByteImage *, int *, int *, ScImage *);
void ByteUVToScB(ByteImage *, ByteImage *, ByteImage *, VectorImage *, VectorImage *, ByteImage *, int *, int *, ScImage *);
void ByteAdd (ByteImage *, ByteImage *, ByteImage *);
void ByteMultiply (ByteImage *, double, ByteImage *);


/*
 * Basic BitImage primitives
 */
BitImage *BitNew(int, int);
void BitFree();
BitImage *BitClip(BitImage *, int, int, int, int);
void BitReclip(BitImage *, int, int, int, int, BitImage *);
int BitSet(BitImage *, unsigned char);
int BitSet8(BitImage *, unsigned char);
int BitCopy(BitImage *, BitImage *);
int BitCopy8(BitImage *, BitImage *);
int BitUnion(BitImage *, BitImage *, BitImage *);
int BitUnion8(BitImage *, BitImage *, BitImage *);
int BitIntersect8(BitImage *, BitImage *, BitImage *);
int BitIntersect(BitImage *, BitImage *, BitImage *);
void BitMakeFromKey(ByteImage *, unsigned char, unsigned char, BitImage *);
int BitGetSize(BitImage *);

/*
 * Basic ScImage primitives
 */
ScImage *ScNew(int w, int h);
void ScFree(ScImage *);
ScImage *ScClip(ScImage *, int x, int y, int w, int h);
void ScReclip(ScImage *, int, int, int, int, ScImage *);
void ScCopy(ScImage *, ScImage *);
void ScCopyDcAc(ScImage *, ScImage *);
int ScIToByte(ScImage *, ByteImage *);
int ScPToY(ScImage *, VectorImage *, ByteImage *, ByteImage *);
int ScPToUV(ScImage *, VectorImage *, ByteImage *, ByteImage *);
int ScBToY(ScImage *, VectorImage *, VectorImage *, ByteImage *, ByteImage *, ByteImage *);
int ScBToUV(ScImage *, VectorImage *, VectorImage *, ByteImage *, ByteImage *, ByteImage *);
void ScQuantize(ScImage *, ByteImage *, int *, ScImage *);
void ScDequantize(ScImage *, ByteImage *, int *, ScImage *);
void ScNonIDequantize(ScImage *, ByteImage *, int *, int *, ScImage *);
void ScAdd (ScImage *, ScImage *, ScImage *);
void ScMultiply (ScImage *, double, ScImage *);

/*
 * Basic VectorImage primitives
 */
VectorImage *VectorNew(int w, int h);
void VectorFree(VectorImage *);
VectorImage *VectorClip(VectorImage *, int, int, int, int);
void VectorReclip(VectorImage *, int, int, int, int, VectorImage *);
void VectorCopy(VectorImage *, VectorImage *);

/*
 * Misc Images primitives
 */
Byte16Image *Byte16New(int w, int h);
void Byte16Free(Byte16Image *);
Byte16Image *Byte16Clip(Byte16Image *, int, int, int, int);
void Byte16Reclip(Byte16Image *, int, int, int, int, Byte16Image *);
void Byte16Copy(Byte16Image *, Byte16Image *);

Byte32Image *Byte32New(int w, int h);
void Byte32Free(Byte32Image *);
Byte32Image *Byte32Clip(Byte32Image *, int, int, int, int);
void Byte32Reclip(Byte32Image *, int, int, int, int, Byte32Image *);
void Byte32Copy(Byte32Image *, Byte32Image *);

FloatImage *FloatNew(int w, int h);
void FloatFree(FloatImage *);
FloatImage *FloatClip(FloatImage *, int, int, int, int);
void FloatReclip(FloatImage *, int, int, int, int, FloatImage *);
void FloatCopy(FloatImage *, FloatImage *);

/*
 * Streams (BitStream, BitParser, BitStreamFilter) primitives
 */
BitParser *BitParserNew();
void BitParserFree(BitParser *);
int BitParserTell(BitParser *);
void BitParserSeek(BitParser *, int);
void BitParserReset(BitParser *);

BitStream *BitStreamNew(int);
void BitStreamFree(BitStream *);
BitStream *BitStreamMmapReadNew(char *);
void BitStreamMmapReadFree(BitStream *);
void BitStreamShift(BitStream *, int);
void BitStreamShareBuffer(BitStream *, BitStream *);
void BitStreamResize(BitStream *, int);
int BitStreamBytesLeft(BitStream *, int);
void BitStreamDump(BitStream *, int, BitStream *, int, int);
void BitStreamDumpSegments(BitStream *, int, BitStream *, int, int, int, int);

void BitParserWrap(BitParser *, BitStream *);

BitStreamFilter *BitStreamFilterNew(int);
void BitStreamFilterFree(BitStreamFilter *);
int BitStreamFilterAdd(BitStreamFilter *, unsigned int, unsigned int);
void BitStreamFilterResize(BitStreamFilter *, unsigned int);
void BitStreamFilterWrite(FILE * file, BitStreamFilter * index);
void BitStreamFilterRead(FILE * file, BitStreamFilter * index);
void BitStreamFilterStartScan(BitStreamFilter * index);
int BitStreamDumpUsingFilter(BitStream * src, int srcOff, BitStream * dest,
    int destOff, int length, BitStreamFilter * index);

int BitStreamFileRead(BitStream *, FILE *, int);
int BitStreamFileReadSegment(BitStream *, FILE *, int, int);
int BitStreamFileReadSegments(BitStream *, FILE *, int, int, int, int);
int BitStreamFileFilterIn(BitStream *, FILE *, int, BitStreamFilter *);
int BitStreamFileWrite(BitStream *, FILE *, int);
int BitStreamFileWriteSegment(BitStream *, FILE *, int, int);
int BitStreamFileWriteSegments(BitStream *, FILE *, int, int, int, int);

Audio *Audio8New(int);
Audio *Audio16New(int);
void AudioFree(Audio *);
Audio *Audio8Clip(Audio *, int, int);
Audio *Audio16Clip(Audio *, int, int);
void Audio8Reclip(Audio *, int, int, Audio *);
void Audio16Reclip(Audio *, int, int, Audio *);
void Audio8Copy(Audio *, Audio *);
void Audio8CopySome(Audio *, Audio *, int, int, int, int);
void Audio16Copy(Audio *, Audio *);
void Audio16CopySome(Audio *, Audio *, int, int, int, int);
void Audio8Set(Audio *, unsigned char);
void Audio8SetSome(Audio *, unsigned char, int, int);
void Audio16SetSome(Audio *, short, int, int);
void Audio8Split(Audio *, Audio *, Audio *);
void Audio16Split(Audio *, Audio *, Audio *);
void Audio8Merge(Audio *, Audio *, Audio *);
void Audio16Merge(Audio *, Audio *, Audio *);
Audio *BitStreamCastToAudio8(BitStream * bs, int offset, int length);
Audio *BitStreamCastToAudio16(BitStream * bs, int offset, int length);
BitStream *Audio8CastToBitStream(Audio * audio);
BitStream *Audio16CastToBitStream(Audio * audio);

int Audio8ChunkAbsSum(Audio * audio, int chunkSize, int **sums);
int Audio16ChunkAbsSum(Audio * audio, int chunkSize, int **sums);
void Audio8ResampleHalf(Audio * in, Audio * out);
void Audio16ResampleHalf(Audio * in, Audio * out);
void Audio8ResampleQuarter(Audio * in, Audio * out);
void Audio16ResampleQuarter(Audio * in, Audio * out);
void Audio8ResampleLinear(Audio *, int, int, Audio *);
void Audio16ResampleLinear(Audio *, int, int, Audio *);
void Audio8ResampleDecimate(Audio *, int, int, Audio *);
void Audio16ResampleDecimate(Audio *, int, int, Audio *);
int Audio8MaxAbs(Audio * in);
int Audio16MaxAbs(Audio * in);


/*
 * Utilities functions for bit manipulation
 */
void CopyRowEqual(unsigned char*, int, int, unsigned char*, int, int, int);
void CopyRowSrcOnTheLeft(unsigned char*, int, int, unsigned char*, int, int, int);
void CopyRowSrcOnTheRight(unsigned char*, int, int, unsigned char*, int, int, int);


#ifdef HAVE_MMAP
#define DVM_MMAP_READ 1
#define DVM_MMAP_WRITE 2
#define DVM_MMAP_MAP_SHARED 1
#define DVM_MMAP_MAP_PRIVATE 2
extern void *Dvm_Mmap(void *, int *, int, int, char *, int);
extern int Dvm_Munmap(void *, int);
#endif                          /* HAVE_MMAP */

#ifdef __cplusplus
}

#endif                          /* __cplusplus */
#endif
