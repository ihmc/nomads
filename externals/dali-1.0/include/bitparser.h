/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#ifndef _BITPARSER_H_
#define _BITPARSER_H_

extern int bitmask[];

/*
 * Bitparser helper macros
 */

#ifndef JPEG_BYTE_STUFF
#define bitfill(bp)            \
{                              \
    unsigned short _x;         \
    unsigned char _z, _y;      \
    if (bp->bitCount < 16) {   \
        _z = *(bp->offsetPtr++); \
        _y = *(bp->offsetPtr++); \
        _x = (_z << 8) | _y;   \
        bp->currentBits <<= 16;\
        bp->currentBits |= _x; \
        bp->bitCount += 16;    \
    }                          \
}
#else /* JPEG_BYTE_STUFF */
#define bitfill(bp)                          \
{                                            \
    while (bp->bitCount < 16) {              \
        unsigned char _x;                    \
        Bp_GetByte(bp, _x);                  \
        bp->currentBits <<= 8;               \
        bp->currentBits |= _x;               \
        bp->bitCount += 8;                   \
        if (_x == 0xff) {                    \
            Bp_GetByte(bp, _x);              \
            if (_x != 0) {                   \
                bp->currentBits <<= 8;       \
                bp->currentBits |= _x;       \
                bp->bitCount += 8;           \
            }                                \
        }                                    \
    }                                        \
}
#endif /* JPEG_BYTE_STUFF */
#ifndef JPEG_BYTE_STUFF
#define bitflush(bp)            \
{                               \
    unsigned char _x;           \
    if (bp->bitCount >= 16) {   \
        bp->bitCount -= 16;     \
        _x = ((bp->currentBits) & 0xff000000) >> 24;    \
        *(bp->offsetPtr++) = _x;\
        _x = ((bp->currentBits) & 0x00ff0000) >> 16;    \
        *(bp->offsetPtr++) = _x;\
        bp->currentBits <<= 16; \
        bp->bs->endDataPtr = bp->offsetPtr; \
    }                           \
}

#define bitflushall(bp)            \
{                                  \
    unsigned char _x;              \
    bitflush(bp);                  \
    if (bp->bitCount >= 8) {       \
        bp->bitCount -= 8;         \
        _x = ((bp->currentBits) & 0xff000000) >> 24;    \
        *(bp->offsetPtr++) = _x;   \
        bp->currentBits <<= 8;     \
    }                              \
    if (bp->bitCount > 0) {        \
       bp->currentBits = (bp->currentBits >> 24)|bitmask[(int)(8-bp->bitCount)]; \
       _x = bp->currentBits&0xff;  \
        *(bp->offsetPtr++) = _x;   \
       bp->bitCount = 0;           \
    }                              \
}

#else /* JPEG_BYTE_STUFF */

#define bitflush(bp)            \
{                               \
    unsigned char _x;           \
    if (bp->bitCount >= 16) {   \
        bp->bitCount -= 16;     \
        _x = ((bp->currentBits) & 0xff000000) >> 24;    \
        *(bp->offsetPtr++) = _x;\
        if (_x == 0xff) {       \
            *(bp->offsetPtr++) = 0x0; \
        }                       \
        _x = ((bp->currentBits) & 0x00ff0000) >> 16;    \
        *(bp->offsetPtr++) = _x;\
        if (_x == 0xff) {       \
            *(bp->offsetPtr++) = 0x0; \
        }                       \
        bp->currentBits <<= 16; \
    }                           \
}

#define bitflushall(bp)            \
{                                  \
    unsigned char _x;              \
    bitflush(bp);                  \
    if (bp->bitCount >= 8) {       \
        bp->bitCount -= 8;         \
        _x = ((bp->currentBits) & 0xff000000) >> 24;    \
        *(bp->offsetPtr++) = _x;   \
        if (_x == 0xff) {          \
            *(bp->offsetPtr++) = 0x0;                   \
        }                          \
        bp->currentBits <<= 8;     \
    }                              \
    if (bp->bitCount > 0) {        \
       bp->currentBits = (bp->currentBits >> 24)|bitmask[(int)(8-bp->bitCount)]; \
       _x = bp->currentBits&0xff;  \
        *(bp->offsetPtr++) = _x;   \
        if (_x == 0xff) {          \
            *(bp->offsetPtr++) = 0x0;                   \
        }                          \
       bp->bitCount = 0;           \
    }                              \
}

#endif /* JPEG_BYTE_STUFF */


/*
 *------------------------------------------------------------
 *
 * Bits stuff
 *
 *------------------------------------------------------------
 */

#define Bp_GetBits(bp, num, retval)                     \
{                                                       \
    bitfill(bp);                                        \
    bp->bitCount -= (num);                              \
    (retval) = (bp->currentBits >> bp->bitCount) & bitmask[(int)num];    \
}

#define Bp_PeekBits(bp, num, retval)                    \
{                                                       \
    bitfill(bp);                                        \
    (retval) = (bp->currentBits >> (bp->bitCount-(num))) & bitmask[(int)num];    \
}

#define Bp_FlushBits(bp, num)    bp->bitCount -= (num)
#define Bp_UngetBits(bp, num)    bp->bitCount += (num)
#define Bp_RestoreBits(bp, num)  bp->bitCount += (num)


#define Bp_PutBits(bp, num, value)                              \
{                                                               \
    bp->currentBits |= ((int)(value & bitmask[(int)num])) << (32 - bp->bitCount - (num));\
    bp->bitCount += num;                                        \
    bitflush(bp);                                               \
}

#define Bp_UnputBits(bp, num)  bp->bitCount += (num)

#define Bp_MoveBits(inbp, outbp, num)      \
{                                 \
    unsigned int _x;              \
    Bp_GetBits(inbp,(num),_x);      \
    Bp_PutBits(outbp,(num),_x);     \
}


/*
 *------------------------------------------------------------
 *
 * Byte stuff
 *
 *------------------------------------------------------------
 */

#define Bp_GetByte(bp, retval)   retval = *(bp->offsetPtr)++
#define Bp_PeekByte(bp, retval)  retval = *(bp->offsetPtr)
#define Bp_FlushByte(bp)         bp->offsetPtr++
#define Bp_UngetByte(bp)         bp->offsetPtr--
#define Bp_RestoreByte(bp)       bp->offsetPtr--

#define Bp_PutByte(bp, value) {                         \
    *(bp->offsetPtr)++ = (value);                       \
    bp->bs->endDataPtr = bp->offsetPtr;                 \
}

#define Bp_UnputByte(bp) {                              \
    *(bp->offsetPtr)--;                                 \
    bp->bs->endDataPtr = bp->offsetPtr;                 \
}

#define Bp_MoveByte(outbp, inbp) {                      \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    outbp->bs->endDataPtr = outbp->offsetPtr;           \
}


/*
 *------------------------------------------------------------
 *
 * Short stuff
 *
 *------------------------------------------------------------
 */

#define Bp_GetShort(bp, retval) {\
    unsigned char _x, _y;        \
    Bp_GetByte(bp, _x);          \
    Bp_GetByte(bp, _y);          \
    retval = (_x << 8) | _y;     \
}

#define Bp_PeekShort(bp, retval) \
{                                \
    unsigned char _x, _y;        \
    Bp_GetByte(bp, _x);          \
    Bp_GetByte(bp, _y);          \
    retval = (_x << 8) | _y;     \
    bp->offsetPtr -= 2;          \
}

#define Bp_FlushShort(bp)    bp->offsetPtr += 2
#define Bp_UngetShort(bp)    bp->offsetPtr -= 2
#define Bp_RestoreShort(bp)  bp->offsetPtr -= 2

#define Bp_PutShort(bp, value)   \
{                                \
    Bp_PutByte(bp, (((value)&0xff00) >> 8));            \
    Bp_PutByte(bp, (value)&0x00ff);                     \
}

#define Bp_UnputShort(bp, value) \
{                                \
    bp->offsetPtr -= 2;          \
    bp->bs->endDataPtr = bp->offsetPtr;                 \
}

#define Bp_MoveShort(outbp, inbp)                       \
{                                                       \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    outbp->bs->endDataPtr = outbp->offsetPtr;           \
}


/*
 *------------------------------------------------------------
 *
 * LittleShort stuff
 *
 *------------------------------------------------------------
 */

#define Bp_GetLittleShort(bp, retval) {\
    unsigned char _x, _y;        \
    Bp_GetByte(bp, _x);          \
    Bp_GetByte(bp, _y);          \
    retval = _x | (_y << 8);     \
}

#define Bp_PeekLittleShort(bp, retval)\
{                                \
    unsigned char _x, _y;        \
    Bp_GetByte(bp, _x);          \
    Bp_GetByte(bp, _y);          \
    retval = _x | (_y << 8);     \
    bp->offsetPtr -= 2;          \
}

#define Bp_PutLittleShort(bp, value)                \
{                                                   \
    *(bp->offsetPtr)++ = (value) & 0x00ff;          \
    *(bp->offsetPtr)++ = ((value) & 0xff00) >> 8;   \
    bp->bs->endDataPtr = bp->offsetPtr;             \
}

/*
 *------------------------------------------------------------
 *
 * Int stuff
 *
 *------------------------------------------------------------
 */



#define Bp_GetInt(bp, retval)\
{                            \
    retval = 0;              \
    retval |= *(bp->offsetPtr++); \
    retval <<= 8;            \
    retval |= *(bp->offsetPtr++); \
    retval <<= 8;            \
    retval |= *(bp->offsetPtr++); \
    retval <<= 8;            \
    retval |= *(bp->offsetPtr++); \
}

#define Bp_PeekInt(bp, retval)\
{\
    unsigned char *_c;\
    _c = bp->offsetPtr;\
    retval = (*_c++ << 24);\
    retval |= (*_c++ << 16);\
    retval |= (*_c++ << 8);\
    retval |= (*_c);\
}

#define Bp_FlushInt(bp)        bp->offsetPtr += 4
#define Bp_UngetInt(bp)        bp->offsetPtr -= 4
#define Bp_RestoreInt(bp)      bp->offsetPtr -= 4

#define Bp_PutInt(bp, value)         \
{                                    \
    *(bp->offsetPtr)++ = ((value) & 0xff000000)>>24; \
    *(bp->offsetPtr)++ = ((value) & 0x00ff0000)>>16; \
    *(bp->offsetPtr)++ = ((value) & 0x0000ff00)>>8;  \
    *(bp->offsetPtr)++ = ((value) & 0x000000ff);     \
    bp->bs->endDataPtr = bp->offsetPtr;              \
}

#define Bp_UnputInt(bp)              \
{                                    \
    bp->offsetPtr -= 4;              \
    bp->bs->endDataPtr = bp->offsetPtr;\
}

#define Bp_MoveInt(outbp, inbp)      \
{                                    \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    *(outbp->offsetPtr)++ = *(inbp->offsetPtr)++;       \
    outbp->bs->endDataPtr = outbp->offsetPtr;           \
}


/*
 *------------------------------------------------------------
 *
 * LittleInt stuff
 *
 *------------------------------------------------------------
 */

#define Bp_GetLittleInt(bp, retval) \
{                                   \
    int _x;                         \
                                    \
    retval = 0;                     \
    Bp_GetByte(bp, _x);             \
    retval |= _x;                   \
    Bp_GetByte(bp, _x);             \
    retval |= (_x << 8);            \
    Bp_GetByte(bp, _x);             \
    retval |= (_x << 16);           \
    Bp_GetByte(bp, _x);             \
    retval |= (_x << 24);           \
}

#define Bp_PeekLittleInt(bp, retval)    \
{                                                                               \
    unsigned char *_c;\
    _c = bp->offsetPtr;\
    retval = (*_c++);\
    retval |= (*_c++ << 8);\
    retval |= (*_c++ << 16);\
    retval |= (*_c << 24);\
}

#define Bp_PutLittleInt(bp, value)   \
{                                    \
    *(bp->offsetPtr)++ = ((value) & 0x000000ff);        \
    *(bp->offsetPtr)++ = ((value) & 0x0000ff00)>>8;     \
    *(bp->offsetPtr)++ = ((value) & 0x00ff0000)>>16;    \
    *(bp->offsetPtr)++ = ((value) & 0xff000000)>>24;    \
    bp->bs->endDataPtr = bp->offsetPtr;                 \
}

#define Bp_GetByteArray(bp, bytes, array)               \
{                                                       \
    memcpy(array,bp->offsetPtr,bytes);                  \
    bp->offsetPtr += (bytes);                           \
}

#define Bp_PutByteArray(bp, bytes, array)               \
{                                                       \
    memcpy(bp->offsetPtr,array,bytes);                  \
    bp->offsetPtr += (bytes);                           \
    bp->bs->endDataPtr = bp->offsetPtr;                 \
}

#define Bp_MoveBytes(inbp, outbp, bytes)                \
{                                                       \
    memcpy(outbp->offsetPtr, inbp->offsetPtr, bytes);   \
    outbp->offsetPtr += bytes;                          \
    outbp->bs->endDataPtr = outbp->offsetPtr;                 \
    inbp->offsetPtr += bytes;                           \
}

#define Bp_FlushBytes(bp, n)     bp->offsetPtr += n
#define Bp_RestoreBytes(bp, n)   bp->offsetPtr -= n


/*
 * this one only works in input mode only.
 * so should really be called Bp_InByteAlign
 */
#define Bp_ByteAlign(bp)                                \
{                                                       \
    int _bytesLeft;                                     \
    _bytesLeft = bp->bitCount/8;                        \
    bp->offsetPtr -= _bytesLeft;                        \
    bp->bitCount = 0;                                   \
}

#define Bp_InByteAlign(bp) Bp_ByteAlign(bp)

#define Bp_OutByteAlign(bp)                             \
{                                                       \
    if (bp->bitCount <= 8 && bp->bitCount > 0) {\
        *(bp->offsetPtr)++ = (unsigned char)(bp->currentBits >> 24);\
    } else if (bp->bitCount > 8 && bp->bitCount <= 16) {\
        *(bp->offsetPtr)++ = (unsigned char)(bp->currentBits >> 24);\
        *(bp->offsetPtr)++ = (unsigned char)((bp->currentBits & 0x00FF0000) >> 16);\
    }\
    bp->currentBits = 0;                                \
    bp->bitCount = 0;                                   \
    bp->bs->endDataPtr = bp->offsetPtr;\
}


#define Bp_GetOffset(bp, retval)                        \
{                                                       \
    retval = bp->offsetPtr - (bp->bs->buffer);          \
}

#define Bp_ShiftOffset(bp, bytes)                       \
{                                                       \
    bp->offsetPtr += bytes;                             \
}

#define Bp_SetOffset(bp, offset)                        \
{                                                       \
    bp->offsetPtr = (bp->bs->buffer) + offset;          \
}

/*
 * this macro only makes sense for input bitparsers
 */
#define Bp_DataRemain(bp) ((bp)->bs->endDataPtr-(bp)->offsetPtr)
#define Bp_Underflow(bp) (bp->offsetPtr > bp->bs->endDataPtr)

#endif
