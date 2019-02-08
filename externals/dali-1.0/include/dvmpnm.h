/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef DVM_PNM_H
#define DVM_PNM_H

#include "dvmbasic.h"
#ifdef __cplusplus
extern "C" {
#endif

#define PBM_TEXT (1)
#define PGM_TEXT (2)
#define PPM_TEXT (3)
#define PBM_BIN  (4)
#define PGM_BIN  (5)
#define PPM_BIN  (6)

    typedef struct PnmHdr {
        int type;
        int width;
        int height;
        int maxVal;
    } PnmHdr;

#define PnmHdrGetWidth(hdr)         ((hdr)->width)
#define PnmHdrGetHeight(hdr)        ((hdr)->height)
#define PnmHdrGetType(hdr)          ((hdr)->type)
#define PnmHdrGetMaxVal(hdr)        ((hdr)->maxVal)
#define PnmHdrSetWidth(hdr, w)      (hdr)->width = w
#define PnmHdrSetHeight(hdr, h)     (hdr)->height = h
#define PnmHdrSetType(hdr, t)       (hdr)->type = t
#define PnmHdrSetMaxVal(hdr, val)   (hdr)->maxVal = val

/*
 * Error codes
 */
#define DVM_PNM_OK 0
#define DVM_PNM_INVALID_HDR -1
#define DVM_PNM_BS_UNDERFLOW -2
#define DVM_PNM_IS_BYTE_ALIGN -3
#define DVM_PNM_NOT_BYTE_ALIGN -4

    PnmHdr *PnmHdrNew();
    void PnmHdrFree(PnmHdr *);
    void PnmHdrCopy(PnmHdr *, PnmHdr *);
    int PnmHdrParse(BitParser *, PnmHdr *);
    int PnmHdrEncode(PnmHdr *, BitParser *);
    BitStream *ByteCastToBitStream(ByteImage * byte);
    ByteImage *BitStreamCastToByte(BitStream * bs, PnmHdr * hdr, int offset);
    BitStream *BitCastToBitStream(BitImage * bit);
    BitImage *BitStreamCastToBit(BitStream * bs, PnmHdr * hdr, int offset);

    int PpmParse(BitParser *, ByteImage *, ByteImage *, ByteImage *);
    int PpmEncode(ByteImage *, ByteImage *, ByteImage *, BitParser *);
    int PgmParse(BitParser *, ByteImage *);
    int PgmEncode(ByteImage *, BitParser *);
    int PbmParse(BitParser *, BitImage *);
    int PbmParse8(BitParser *, BitImage *);
    int PbmEncode(BitImage *, BitParser *);
    int PbmEncode8(BitImage *, BitParser *);
#ifdef __cplusplus
}                               /* extern "C" */

#endif                          /* __cplusplus */
#endif
