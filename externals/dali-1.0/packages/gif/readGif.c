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
 * readGif.c
 * Routines to parse gifs
 * Steve Weiss
 */

/* Derived from code which carries the following copyright */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.                          | */
/* | Copyright 1996 Torsten Martinsen.                                 | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

#include "gifInt.h"

#define TRUE    1
#define FALSE   0

#define CM_RED          0
#define CM_GREEN        1
#define CM_BLUE         2

#define MAX_LWZ_BITS            12

#define INTERLACE               0x40
#define LOCALCOLORMAP   0x80

#define ReadOK(bp,buffer,len)  Bp_GetByteArray(bp, len, (buffer))

#define LM_to_uint(a,b)                 (((b)<<8)|(a))


/*struct {
    unsigned int Width;
    unsigned int Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
    int GrayScale;
} GifScreen;

static struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89 = {
    -1, -1, -1, 0
};*/

/*
static int ReadColorMap(FILE * fd, int number,
                        unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(FILE * fd, int label); */
static int GetDataBlock(BitParser *bp, unsigned char *buf);
static int GetCode(BitParser *bp, int code_size, int flag);
static int LWZReadByte(int flag, int input_code_size, BitParser *bp);

#if 0
static int
DoExtension(FILE *fd, int label)
{
    static char buf[256];
    char *str;

    switch (label) {
    case 0x01:                  /* Plain Text Extension */
        str = "Plain Text Extension";
        break;
    case 0xff:                  /* Application Extension */
        str = "Application Extension";
        break;
    case 0xfe:                  /* Comment Extension */
        str = "Comment Extension";
        while (GetDataBlock(fd, (unsigned char *) buf) != 0);
        return FALSE;
    case 0xf9:                  /* Graphic Control Extension */
        str = "Graphic Control Extension";
        (void) GetDataBlock(fd, (unsigned char *) buf);
        Gif89.disposal = (buf[0] >> 2) & 0x7;
        Gif89.inputFlag = (buf[0] >> 1) & 0x1;
        Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
        if ((buf[0] & 0x1) != 0)
            Gif89.transparent = buf[3];

        while (GetDataBlock(fd, (unsigned char *) buf) != 0);
        return FALSE;
    default:
        str = buf;
        sprintf(buf, "UNKNOWN (0x%02x)", label);
        break;
    }

    while (GetDataBlock(fd, (unsigned char *) buf) != 0);

    return FALSE;
}
#endif

static int ZeroDataBlock = FALSE;

static int
GetDataBlock(BitParser *bp, unsigned char *buf)
{
    unsigned char count;
    /*if (!ReadByteOK(bp, &count)) {
        return -1;
    }*/
    ReadOK(bp, &count, 1);

    ZeroDataBlock = count == 0;

/*    if ((count != 0) && (!ReadOK(bp, buf, count))) {
        return -1;
    }*/
    ReadOK(bp, buf, count);

    return count;
}

static int
GetCode(BitParser *bp, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, ret1;
    unsigned char count;

    if (flag) {
        curbit = 0;
        lastbit = 0;
        done = FALSE;
        return 0;
    }
    if ((curbit + code_size) >= lastbit) {
        if (done) {
            if (curbit >= lastbit)
            return -1;
        }
        buf[0] = buf[last_byte - 2];
        buf[1] = buf[last_byte - 1];

        if ((count = GetDataBlock(bp, &buf[2])) == 0)
            done = TRUE;

        last_byte = 2 + count;
        curbit = (curbit - lastbit) + 16;
        lastbit = (2 + count) * 8;
    }
    ret1 = 0;
    /*
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
            ret |= ((buf[i >> 3] & (1 << (i & 0x07))) != 0) << j;
    curbit += code_size;
    */
    i = curbit + code_size - 1;
    DO_N_TIMES(code_size,
        if (buf[i>>3] & (1 << (i & 0x07))) 
            ret1 |= 0x01;
        ret1 <<= 1;
        i--;
        );
    ret1 >>= 1;
    curbit += code_size;

    return ret1;
}

static int
LWZReadByte(int flag, int input_code_size, BitParser *bp)
{
    static int fresh = FALSE;
    int code, incode;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) << 1], *sp;
    register int i;

    if (flag) {
        set_code_size = input_code_size;
        code_size = set_code_size + 1;
        clear_code = 1 << set_code_size;
        end_code = clear_code + 1;
        max_code_size = clear_code << 1;
        max_code = clear_code + 2;

        GetCode(bp, 0, TRUE);

        fresh = TRUE;

        for (i = 0; i < clear_code; ++i) {
            table[0][i] = 0;
            table[1][i] = i;
        }
        for (; i < (1 << MAX_LWZ_BITS); ++i)
            table[0][i] = table[1][0] = 0;

        sp = stack;

        return 0;
    } else if (fresh) {
        fresh = FALSE;
        do {
            firstcode = oldcode = GetCode(bp, code_size, FALSE);
        } while (firstcode == clear_code);
        return firstcode;
    }
    if (sp > stack)
        return *--sp;

    while ((code = GetCode(bp, code_size, FALSE)) >= 0) {
        if (code == clear_code) {
            for (i = 0; i < clear_code; ++i) {
                table[0][i] = 0;
                table[1][i] = i;
            }
            for (; i < (1 << MAX_LWZ_BITS); ++i)
                table[0][i] = table[1][i] = 0;
            code_size = set_code_size + 1;
            max_code_size = clear_code << 1;
            max_code = clear_code + 2;
            sp = stack;
            firstcode = oldcode = GetCode(bp, code_size, FALSE);
            return firstcode;
        } else if (code == end_code) {
            int count;
            unsigned char buf[260];

            if (ZeroDataBlock)
                return -2;

            while ((count = GetDataBlock(bp, buf)) > 0);

            if (count != 0) {
                /*
                 * pm_message("missing EOD in data stream (common occurence)");
                 */
            }
            return -2;
        }
        incode = code;

        if (code >= max_code) {
            *sp++ = firstcode;
            code = oldcode;
        }
        while (code >= clear_code) {
            *sp++ = table[1][code];
            code = table[0][code];
        }

        *sp++ = firstcode = table[1][code];

        if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
            table[0][code] = oldcode;
            table[1][code] = firstcode;
            ++max_code;
            if ((max_code >= max_code_size) &&
                (max_code_size < (1 << MAX_LWZ_BITS))) {
                max_code_size <<= 1;
                ++code_size;
            }
        }
        oldcode = incode;

        if (sp > stack)
            return *--sp;
    }
    return code;
}


int
IgnoreGifImage(bp)
    BitParser *bp;
{
    unsigned char c = 0;

    while (LWZReadByte(FALSE, c, bp) < 0);
    return DVM_GIF_OK;
}


int
ReadNonInterlacedGifImage(bp, hdr, byte)
    BitParser *bp;
    GifImgHdr *hdr;
    ByteImage *byte;
{
    unsigned char c, *curr;
    int i;
    /*
     *  Initialize the compression routines
     */
    ReadOK(bp, &c, 1);

    if (LWZReadByte(TRUE, c, bp) < 0) {
        return DVM_GIF_IMG_READ_ERROR;
    }

    if (byte->width != hdr->width) {
        return DVM_GIF_BAD_WIDTH;
    }
    if (byte->height != hdr->height) {
        return DVM_GIF_BAD_HEIGHT;
    }

    curr = byte->firstByte;
    for (i = 0; i < byte->height; i++) {
        DO_N_TIMES (byte->width,
            *curr++ = LWZReadByte(FALSE, c, bp);
        );
        curr += byte->parentWidth - byte->width;
    }
    return DVM_GIF_OK;
}

int
ReadInterlacedGifImage(bp, hdr, byte)
    BitParser *bp;
    GifImgHdr *hdr;
    ByteImage *byte;
{
    unsigned char c, *destBuf, *curr;
    int i;
    destBuf = byte->firstByte;


    /*
    **  Initialize the compression routines
     */
    ReadOK(bp, &c, 1);

    if (LWZReadByte(TRUE, c, bp) < 0) {
        return DVM_GIF_IMG_READ_ERROR;
    }

    /*
     * Pass 0
     */
    curr = byte->firstByte;
    for (i = 0; i < byte->height; i+=8) {
        DO_N_TIMES (byte->width,
            *curr++ = LWZReadByte(FALSE, c, bp);
        );
        curr += (byte->parentWidth << 3) - byte->width;
    }
    /*
     * Pass 1
     */
    curr = byte->firstByte + (byte->parentWidth << 2);
    for (i = 0; i < byte->height; i+=8) {
        DO_N_TIMES (byte->width,
            *curr++ = LWZReadByte(FALSE, c, bp);
        );
        curr += (byte->parentWidth << 3) - byte->width;
    }
    /*
     * Pass 2
     */
    curr = byte->firstByte + (byte->parentWidth << 1);
    for (i = 0; i < byte->height; i+=4) {
        DO_N_TIMES (byte->width,
            *curr++ = LWZReadByte(FALSE, c, bp);
        );
        curr += (byte->parentWidth << 2) - byte->width;
    }
    /*
     * Pass 3
     */
    curr = byte->firstByte + byte->parentWidth;
    for (i = 0; i < byte->height; i+=2) {
        DO_N_TIMES (byte->width,
            *curr++ = LWZReadByte(FALSE, c, bp);
        );
        curr += (byte->parentWidth << 1) - byte->width;
    }

    return DVM_GIF_OK;
}
