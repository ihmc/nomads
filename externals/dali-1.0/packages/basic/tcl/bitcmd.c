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
 * bitcmd.c
 *
 * Wei Tsang May 97
 *
 * Defines hooks for BitImage primitives
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_new <w> <h>
 * precond 
 *     none
 * return 
 *     the buffer number
 * side effect :
 *     a new buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */

int
BitNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;
    int w, h;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s width height", argv[0]);
    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    buf = BitNew(w, h);
    PutBitImage(interp, buf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_free bitImage
 * precond 
 *      _buf_ exists
 * return 
 *     none
 * side effect :
 *     the memory allocated for _buf_ is freed. All virtual buffer that 
 *     refers to this becomes invalid and must be freed as well.
 *
 *----------------------------------------------------------------------
 */

int
BitFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage", argv[0]);
    buf = RemoveBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    BitFree(buf);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_clip bitImage <x> <y> <w> <h>
 * precond 
 *     buf is a valid buffer, and the clip window is within the boundary
 *     of buf.  
 * return 
 *     the buffer number
 * side effect :
 *     a new virtual buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */

int
BitClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf, *newBuf;
    int x, y, w, h;
    int status;

    ReturnErrorIf1(argc != 6,
        "wrong # args: should be %s bitImage x y width height", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    newBuf = BitClip(buf, x, y, w, h);
    PutBitImage(interp, newBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_reclip bitImage <x> <y> <w> <h>
 * precond 
 *     buf is a valid buffer, and the clip window is within the boundary
 *     of buf.  
 * return 
 *     the buffer number
 * side effect :
 *     a new virtual buffer is created and is added into _theBufferTable_
 *
 *----------------------------------------------------------------------
 */

int
BitReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf, *clipped;
    int x, y, w, h;
    int status;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s bitImage x y width height clipped",
        argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);
    clipped = GetBitImage(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s: no such bit image %s", argv[0], argv[6]
        );

    BitReclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_copy_8 <src> <dest>
 * precond 
 *     Both buffer _src_ and _dest_ exists, and have the same shape.
 *     The buffers cannot overlap.  Both _src_ and _dest_ must also
 *     be byte align.
 * return 
 *     None
 * side effect :
 *     Content of buffer _src_ is copied into _dest_.
 *
 *----------------------------------------------------------------------
 */

int
BitCopy8Cmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *srcBuf, *destBuf;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcBitImage destBitImage", argv[0]
        );
    srcBuf = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    destBuf = GetBitImage(argv[2]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );

    status = BitCopy8(srcBuf, destBuf);

    ReturnErrorIf3(status == DVM_BIT_NOT_BYTE_ALIGN,
        "%s: one of srcBitImage %s and destBitImage %s is not byte-aligned.",
        argv[0], argv[1], argv[2]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_copy <src> <dest>
 *
 * precond 
 *     Both buffer _src_ and _dest_ exists, and have the same shape.
 *     The buffers cannot overlap.
 *
 * return 
 *     None
 *
 * side effect :
 *     Content of buffer _src_ is copied into _dest_.
 *
 *----------------------------------------------------------------------
 */

int
BitCopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *srcBuf, *destBuf;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s srcBitImage destBitImage", argv[0]);

    srcBuf = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    destBuf = GetBitImage(argv[2]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    status = BitCopy(srcBuf, destBuf);
    ReturnErrorIf3(status == DVM_BIT_IS_BYTE_ALIGN,
        "%s: both srcBitImage %s and destBitImage %s is byte-aligned.",
        argv[0], argv[1], argv[2]);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_set_8 bitImage <value>
 *
 * precond 
 *     bitImage exists and is byte align.
 *
 * return 
 *     None
 *
 * side effect :
 *     Content of bitImage is set to _value_
 *
 *----------------------------------------------------------------------
 */

int
BitSet8Cmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;
    int status, value;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitImage value", argv[0]);

    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);

    status = BitSet8(buf, (unsigned char) value);

    ReturnErrorIf2(status == DVM_BIT_NOT_BYTE_ALIGN,
        "%s: %s is not byte-aligned.",
        argv[0], argv[1]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_set bitImage <value>
 * precond 
 *     bitImage exists and is byte align.
 * return 
 *     None
 * side effect :
 *     Content of bitImage is set to _value_
 *
 *----------------------------------------------------------------------
 */

int
BitSetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;
    int status, value;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitImage value", argv[0]);

    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);

    status = BitSet(buf, (unsigned char) value);

    ReturnErrorIf2(status == DVM_BIT_IS_BYTE_ALIGN,
        "%s: %s is byte-aligned.", argv[0], argv[1]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_union_8 <dest> <src1> <src2>
 * precond 
 *     all three buffers have been created, and of the same size.
 *     all three of them are byte-align. 
 * return 
 *     none
 * side effect :
 *     _dest_ will contain the union of both region in src1 and src2.
 *
 *----------------------------------------------------------------------
 */


int
BitUnion8Cmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *destBuf, *srcBuf1, *srcBuf2;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]
        );
    srcBuf1 = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf1 == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    srcBuf2 = GetBitImage(argv[2]);
    ReturnErrorIf2(srcBuf2 == NULL,
        "%s: no such bit image %s", argv[0], argv[2]
        );
    destBuf = GetBitImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[3]
        );

    status = BitUnion8(srcBuf1, srcBuf2, destBuf);

    ReturnErrorIf4(status == DVM_BIT_NOT_BYTE_ALIGN,
        "%s: one of the inputs %s, %s and %s is not byte-aligned",
        argv[0], argv[1], argv[2], argv[3]);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_union <dest> <src1> <src2>
 * precond 
 *     all three buffers have been created, and of the same size.
 * return 
 *     none
 * side effect :
 *     _dest_ will contain the union of both region in src1 and src2.
 *
 *----------------------------------------------------------------------
 */


int
BitUnionCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *destBuf, *srcBuf1, *srcBuf2;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]
        );
    srcBuf1 = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf1 == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    srcBuf2 = GetBitImage(argv[2]);
    ReturnErrorIf2(srcBuf2 == NULL,
        "%s: no such bit image %s", argv[0], argv[2]
        );
    destBuf = GetBitImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[3]);

    status = BitUnion(srcBuf1, srcBuf2, destBuf);

    ReturnErrorIf4(status == DVM_BIT_IS_BYTE_ALIGN,
        "%s: all three input %s, %s and %s is byte-aligned.",
        argv[0], argv[1], argv[2], argv[3]);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_intersect_8 <dest> <src1> <src2>
 * precond 
 *     all three buffers have been created, and of the same size.
 * return 
 *     none
 * side effect :
 *     _dest_ will contain the union of both region in src1 and src2.
 *
 *----------------------------------------------------------------------
 */

int
BitIntersect8Cmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *destBuf, *srcBuf1, *srcBuf2;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]
        );
    srcBuf1 = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf1 == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    srcBuf2 = GetBitImage(argv[2]);
    ReturnErrorIf2(srcBuf2 == NULL,
        "%s: no such bit image %s", argv[0], argv[2]
        );
    destBuf = GetBitImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[3]);

    status = BitIntersect8(srcBuf1, srcBuf2, destBuf);
    ReturnErrorIf4(status == DVM_BIT_NOT_BYTE_ALIGN,
        "%s: one of the inputs %s, %s and %s is not byte-aligned",
        argv[0], argv[1], argv[2], argv[3]);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_intersect <dest> <src1> <src2>
 *
 * precond 
 *     all three buffers have been created, and of the same size.
 *
 * return 
 *     none
 * 
 * side effect :
 *     _dest_ will contain the union of both region in src1 and src2.
 *
 *----------------------------------------------------------------------
 */


int
BitIntersectCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *destBuf, *srcBuf1, *srcBuf2;
    int status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]
        );
    srcBuf1 = GetBitImage(argv[1]);
    ReturnErrorIf2(srcBuf1 == NULL,
        "%s: no such bit image %s", argv[0], argv[1]
        );
    srcBuf2 = GetBitImage(argv[2]);
    ReturnErrorIf2(srcBuf2 == NULL,
        "%s: no such bit image %s", argv[0], argv[2]
        );
    destBuf = GetBitImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such bit image %s", argv[0], argv[3]);

    status = BitIntersect(srcBuf1, srcBuf2, destBuf);

    ReturnErrorIf4(status == DVM_BIT_IS_BYTE_ALIGN,
        "%s: all three input %s, %s and %s is byte-aligned.",
        argv[0], argv[1], argv[2], argv[3]);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_get_width bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     width of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    sprintf(interp->result, "%d", BitGetWidth(buf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_get_height bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     height of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    sprintf(interp->result, "%d", BitGetHeight(buf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_get_x bitImage
 *
 * precond 
 *     bitImage exists.
 *
 * return 
 *     x coordinate of the top left corner of the buffer
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitGetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    sprintf(interp->result, "%d", BitGetX(buf));

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_get_y bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     y coordinate of the top left corner of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitGetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    sprintf(interp->result, "%d", BitGetY(buf));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_get_virtual bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     1 if the buffer is virtual, 0 if not.
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitGetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    sprintf(interp->result, "%d", BitGetVirtual(buf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_is_aligned bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     1 if the buffer is byte-aligned, 0 if not.
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitIsAlignedCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    if (BitIsAligned(buf))
        sprintf(interp->result, "1");
    else
        sprintf(interp->result, "0");
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_is_aligned bitImage
 * precond 
 *     bitImage exists.
 * return 
 *     1 if the buffer is left-byte-aligned, 0 if not.
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
BitIsLeftAlignedCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    if (BitIsLeftAligned(buf))
        sprintf(interp->result, "1");
    else
        sprintf(interp->result, "0");
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_info bitImage
 *
 * precond 
 *     bitImage exists.
 *
 * return 
 *     list of information about the bit image
 *
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
BitInfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *buf;
    char str[32];

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage ", argv[0]);
    buf = GetBitImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d %s",
        buf->x, buf->y, buf->unitWidth, buf->height,
        buf->isVirtual, str);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_image_make_from_key <byteImage> <low> <high> <bitImage>
 * precond 
 *     bitImage and byteImage must have the same dimension.
 * return 
 *     none
 * side effect :
 *     _bitImage_ is initialized to 1 where _byteImage_ value is between _low_
 *     and _high_ (inclusive) and 0 elsewhere.
 *
 *----------------------------------------------------------------------
 */

int
BitMakeFromKeyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *bitImage;
    ByteImage *byteImage;
    int low, high;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s byteImage low high bitImage", argv[0]);

    byteImage = GetByteImage(argv[1]);
    ReturnErrorIf2(byteImage == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    if (Tcl_GetInt(interp, argv[2], &low) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &high) != TCL_OK) {
        return TCL_ERROR;
    }
    bitImage = GetBitImage(argv[4]);
    ReturnErrorIf2(bitImage == NULL,
        "%s: no such bit image %s", argv[0], argv[4]);

    BitMakeFromKey(byteImage, (unsigned char) low,
        (unsigned char) high, bitImage);

    return TCL_OK;
}

int
BitGetSizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitImage *bitImage;
    int size;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitImage", argv[0]);

    bitImage = GetBitImage(argv[1]);
    ReturnErrorIf2(bitImage == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    size = BitGetSize(bitImage);
    sprintf(interp->result, "%d", size);

    return TCL_OK;
}
