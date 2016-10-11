/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _JPEG_H_
#define _JPEG_H_

#include "dvmbasic.h"
#include "bitparser.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 *----------------------------------------------------------------------
 *
 *
 *     A ScImage represents a rectangular array of SC blocks in
 *     a JPEG image.  Each ScBlock consists of a DC component
 *     and one or more quantized AC components.  This structure
 *     allows the VM to decompress part of the image without
 *     decompressing the whole image.
 *
 *     Like ByteImages, ScImages can be physical or virtual.
 *
 *     JpegHdr represents the parsed header of a JPEG stream.  It
 *     contains an array of Huffman tables, an array of quantization
 *     tables, and information about each scan in the image.
 *
 *----------------------------------------------------------------------
 */

/*
 * The following structure represents a JPEG quantization table
 */
    typedef struct JpegQt {
        char precision;         /* Precision of the QT (8 or 16) */
        short v[64];            /* Quantization values in row major order */
    } JpegQt;

/*
 * The following structure represents a JPEG Huffman table
 */
    typedef struct JpegHt {
        int valid;
        unsigned char dcBits[16];       /* DC table */
        unsigned char dcVals[256];
        unsigned char acBits[16];       /* AC table */
        unsigned char acVals[256];
    } JpegHt;

    typedef struct JpegHdr {
        short width;            /* width of this image */
        short height;           /* height of this image */
        short restartInterval;  /* Number of MCUs in restart interval */
        short numOfQts;         /* Number of quantization table */
        char maxBlockWidth;     /* Max value in blockWidth[] */
        char maxBlockHeight;    /* Max value in blockHeight[] */
        char precision;         /* precision (in bits) for the samples */
        unsigned char numComps; /* Number of components in image (1 for
                                 * gray, 3 for YUV, etc.) */
        char blockWidth[3];     /* Array[numComponents] giving the number of
                                 * blocks (horiz) in this component */
        char blockHeight[3];    /* Same for the vertical part of this component */
        char qtId[3];           /* quantization table ID to use for this comp */
        char compId[3];         /* unique value identifying each component */
        JpegQt qt[4];           /* Array of quantization tables */
        JpegHt ht[4];           /* Array of Huffman tables */
    } JpegHdr;

#define JpegHdrGetWidth(hdr) (hdr)->width
#define JpegHdrGetHeight(hdr) (hdr)->height
#define JpegHdrGetNumOfComponents(hdr) (hdr)->numComps
#define JpegHdrGetPrecision(hdr) (hdr)->precision
#define JpegHdrGetRestartInterval(hdr) (hdr)->restartInterval
#define JpegHdrGetComponentId(hdr, i) (hdr)->compId[i]
#define JpegHdrGetQtId(hdr, i) (hdr)->qtId[i]
#define JpegHdrGetBlockWidth(hdr, i) (hdr)->blockWidth[i]
#define JpegHdrGetBlockHeight(hdr, i) (hdr)->blockHeight[i]
#define JpegHdrGetMaxBlockHeight(hdr) (hdr)->maxBlockHeight
#define JpegHdrGetMaxBlockWidth(hdr) (hdr)->maxBlockWidth

#define JpegHdrSetWidth(hdr, x) (hdr)->width = x
#define JpegHdrSetHeight(hdr, x) (hdr)->height = x
#define JpegHdrSetNumOfComponents(hdr, x) (hdr)->numComps = x
#define JpegHdrSetPrecision(hdr, x) (hdr)->precision = x
#define JpegHdrSetRestartInterval(hdr, x) (hdr)->restartInterval = x
#define JpegHdrSetComponentId(hdr, i, x) (hdr)->compId[i] = x
#define JpegHdrSetQtId(hdr, i, x) (hdr)->qtId[i] = x
#define JpegHdrSetBlockWidth(hdr, i, x) (hdr)->blockWidth[i] = x
#define JpegHdrSetBlockHeight(hdr, i, x) (hdr)->blockHeight[i] = x
#define JpegHdrSetMaxBlockHeight(hdr, x) (hdr)->maxBlockHeight = x
#define JpegHdrSetMaxBlockWidth(hdr, x) (hdr)->maxBlockWidth = x

/*
 * JpegScanHdr
 *
 * The following structure is used to capture the data
 * contained in the "Scan header" (delimited by the SOS marker
 * in the JPEG stream) in the JPEG file.
 */
    typedef struct JpegScanHdr {
        int numComps;           /* Number of components */
        char scanid[3];         /* array of component ids */
        char dcid[3];           /* array of DC huffman table ids */
        char acid[3];           /* array of AC huffman table ids */
    } JpegScanHdr;

#define JpegScanHdrGetNumOfComponents(hdr) (hdr)->numComps
#define JpegScanHdrGetScanId(hdr, i) (hdr)->scanid[i]
#define JpegScanHdrGetDcId(hdr, i) (hdr)->dcid[i]
#define JpegScanHdrGetAcId(hdr, i) (hdr)->acid[i]

#define JpegScanHdrSetNumOfComponents(hdr, x) (hdr)->numComps = x
#define JpegScanHdrSetScanId(hdr, i, x) (hdr)->scanid[i] = x
#define JpegScanHdrSetAcId(hdr, i, x) (hdr)->acid[i] = x
#define JpegScanHdrSetDcId(hdr, i, x) (hdr)->dcid[i] = x

/*
 * JpegHuffTable
 *
 * The following structure is used to store the huffman table.
 * It is initialized using huffman table data from jpegHdr->ht.
 * This is used internally in JpegScanEncode, but we expose it
 * because it needs malloc and so we let the programmer explicitly
 * allocate and deallocate this structure.  (Plus, if we have 
 * incremental encoder, we only need to initialized the table once)
 */

    typedef struct JpegHuffTable {
        int numOfComponents;
        unsigned short *dcCode; /* 1D array of 4096 code */
        unsigned short *dcSymbol;       /* 1D array of 4096 symbol */
        unsigned short *acCode; /* 1D array of 256 code */
        unsigned short *acSymbol;       /* 1D array of 256 symbol */
        unsigned char *acNumOfBits;     /* 1D array of 1024 number of bits */
    } JpegHuffTable;

#define JpegHuffTableGetDcCodeTable(t,i) &(t->dcCode[i*4096])
#define JpegHuffTableGetDcSymbolTable(t,i) &(t->dcSymbol[i*4096])
#define JpegHuffTableGetAcCodeTable(t,i) &(t->acCode[i*256])
#define JpegHuffTableGetAcSymbolTable(t,i) &(t->acSymbol[i*256])
#define JpegHuffTableGetAcNumOfBitsTable(t,i) &(t->acNumOfBits[i*1024])

/*
 *  Return Codes
 */
#define DVM_JPEG_OK 0
#define DVM_JPEG_INVALID_ID -1
#define DVM_JPEG_INVALID_PRECISION -2
#define DVM_JPEG_WRONG_COMPONENTS -3
#define DVM_JPEG_INVALID_WIDTH -4
#define DVM_JPEG_INVALID_HEIGHT -5
#define DVM_JPEG_INVALID_MARKER -6
#define DVM_JPEG_AC_UNSUPPORTED -7

/*
 * C Function prototypes
 */

    JpegHdr *JpegHdrNew(void);
    void JpegHdrFree(JpegHdr * hdr);
    int JpegHdrParse(BitParser * bp, JpegHdr * hdr);
    int JpegHdrSetQt(JpegHdr * hdr, int id, int prec, short *table);
    int JpegHdrSetBlockWidths(JpegHdr *, unsigned int *, unsigned int);
    int JpegHdrSetBlockHeights(JpegHdr *, unsigned int *, unsigned int);
    int JpegHdrSetQtIds(JpegHdr *, unsigned int *, unsigned int);
    JpegScanHdr *JpegScanHdrNew(void);
    void JpegScanHdrFree(JpegScanHdr *);
    int JpegScanHdrParse(BitParser *, JpegHdr *, JpegScanHdr *);

    int JpegScanParse(BitParser *, JpegHdr *, JpegScanHdr *,
        ScImage * scans[], int);
    int JpegScanSelectiveParse(BitParser * bp, JpegHdr * hdr, JpegScanHdr * shdr,
        ScImage * scans[], int num);
    int JpegScanIncParseStart(BitParser * bp, JpegHdr * hdr, JpegScanHdr * shdr,
        ScImage * scans[], int num);
    int JpegScanIncParseEnd(BitParser * bp, JpegHdr * hdr, JpegScanHdr * shdr,
        ScImage * scans[], int num);
    int JpegScanIncParse(BitParser * bp, JpegHdr * hdr, JpegScanHdr * shdr,
        ScImage * scans[], int num, int howmany);

    int JpegHdrEncode(JpegHdr *, int, BitParser *);
    int JpegScanHdrEncode(JpegHdr *, JpegScanHdr *, BitParser *);
    int JpegScanEncode(JpegHdr *, JpegScanHdr *, JpegHuffTable *,
        ScImage * scans[], int, BitParser *);
    int JpegScanIncEncode420(JpegHdr *, JpegScanHdr *, JpegHuffTable *,
        ScImage *, ScImage *, ScImage *, BitParser *);
    int JpegScanIncEncode422(JpegHdr *, JpegScanHdr *, JpegHuffTable *,
        ScImage *, ScImage *, ScImage *, BitParser *);
    int JpegScanEncode420(JpegHdr *, JpegScanHdr *, JpegHuffTable *,
        ScImage *, ScImage *, ScImage *, BitParser *);
    int JpegScanEncode422(JpegHdr *, JpegScanHdr *, JpegHuffTable *,
        ScImage *, ScImage *, ScImage *, BitParser *);
    int JpegEndCodeEncode(BitParser * bp);
    int JpegStartCodeEncode(BitParser * bp);

    JpegHuffTable *JpegHuffTableNew(int numOfComponent);
    void JpegHuffTableInit(JpegHdr *, JpegScanHdr *, JpegHuffTable *);
    void JpegHuffTableFree(JpegHuffTable * table);

    void JpegHdrStdQtInit(JpegHdr * hdr);
    void JpegHdrStdHtInit(JpegHdr * hdr);
    int JpegHdrQtEncode(JpegHdr * hdr, BitParser * bp);
    int JpegHdrHtEncode(JpegHdr * hdr, BitParser * bp);

#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
