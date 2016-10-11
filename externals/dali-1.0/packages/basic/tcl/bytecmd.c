/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1999 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * bytecmd.c
 * 
 * Hooks to Tcl Functions that manipulate ByteImages.
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_new <w> <h>
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
ByteNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;
    int width, height;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s width height", argv[0]);

    if ((Tcl_GetInt(interp, argv[1], &width) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[2], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    buf = ByteNew(width, height);
    PutByteImage(interp, buf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_free <buf>
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
ByteFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);
    buf = RemoveByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    ByteFree(buf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_clip <buf> <x> <y> <w> <h>
 *
 * Creates a virtual byte image, whicch is a rectangular subset of a
 * physical byteimage
 *
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
ByteClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf, *newBuf;
    int x, y, width, height;

    ReturnErrorIf1(argc != 6,
        "wrong # args: should be %s byteImage x y width height", argv[0]);
    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &y) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[4], &width) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[5], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    newBuf = ByteClip(buf, x, y, width, height);
    PutByteImage(interp, newBuf);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_reclip <buf> <x> <y> <w> <h> <clipped>
 *
 * Update virtual byte image, whicch is a rectangular subset of a
 * physical byteimage
 *
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
ByteReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf, *clipped;
    int x, y, width, height;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s byteImage x y width height clippedImage",
        argv[0]);
    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &y) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[4], &width) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[5], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    clipped = GetByteImage(argv[6]);
    ReturnErrorIf2(clipped == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);

    ByteReclip(buf, x, y, width, height, clipped);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_copy <src> <dest>
 * precond 
 *     Both buffer _src_ and _dest_ exists, and have the same shape.
 *     The buffers cannot overlap.
 * return 
 *     None
 * side effect :
 *     Content of buffer _src_ is copied into _dest_.
 *
 *----------------------------------------------------------------------
 */

int
ByteCopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s src dest", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    destBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    ByteCopy(srcBuf, destBuf);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_copy_mux_1 src soffset sstride dest doffset dstride 
 *     byte_copy_mux_2 src soffset sstride dest doffset dstride 
 *     byte_copy_mux_4 src soffset sstride dest doffset dstride 
 * precond 
 *     Both buffer _src_ and _dest_ exists, and have the same shape.
 *     The buffers cannot overlap.
 * return 
 *     None
 * side effect :
 *     Content of buffer _src_ is copied into _dest_, starting from
 *     _offset_ byte of each row, copy 1/2/4 byte, and skipping every 
 *     _stride_ byte.
 *
 *----------------------------------------------------------------------
 */

int
ByteCopyMuxCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int srcOffset, srcStride, destOffset, destStride, status, mux;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s src srcOffset srcStride dest destOffset destStride", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &srcOffset);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &srcStride);
    ReturnErrorIf (status != TCL_OK);

    destBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    status = Tcl_GetInt(interp, argv[5], &destOffset);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[6], &destStride);
    ReturnErrorIf (status != TCL_OK);

    mux = (int) clientData;
    switch (mux) {
        case 1 : ByteCopyMux1(srcBuf, srcOffset, srcStride, destBuf,
            destOffset, destStride);
            break;
        case 2 : ByteCopyMux2(srcBuf, srcOffset, srcStride, destBuf,
            destOffset, destStride);
            break;
        case 4 : ByteCopyMux4(srcBuf, srcOffset, srcStride, destBuf,
            destOffset, destStride);
            break;
    }

    return TCL_OK;
}


int
ByteCopyWithMaskCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    BitImage *bitmask;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src bitMask dest", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    bitmask = GetBitImage(argv[2]);
    ReturnErrorIf2(bitmask == NULL,
        "%s: no such bit image %s", argv[0], argv[2]);

    destBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    ByteCopyWithMask(srcBuf, bitmask, destBuf);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_set <buf> <value>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     None
 * side effect :
 *     Content of buffer _buf_ is set to _value_
 *
 *----------------------------------------------------------------------
 */

int
ByteSetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;
    int value;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s byteImage value", argv[0]);

    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    if (Tcl_GetInt(interp, argv[2], &value) != TCL_OK) {
        return TCL_ERROR;
    }
    ByteSet(buf, (unsigned char) value);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_set_mux_1 buf offset stride value 
 *     byte_set_mux_2 buf offset stride value 
 *     byte_set_mux_4 buf offset stride value 
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     None
 * side effect :
 *     Content of buffer _buf_ is set to _value_, starting from offset
 *     buf, sets 1/2/4 bytes, and skips _stride_ bytes.
 *
 *----------------------------------------------------------------------
 */

int
ByteSetMuxCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;
    int value, status, offset, stride, mux;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s byteImage offset stride value", argv[0]);

    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &offset);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &stride);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &value);
    ReturnErrorIf (status != TCL_OK);

    
    mux = (int) clientData;
    switch (mux) {
        case 1 : ByteSetMux1(buf, offset, stride, (unsigned char) value);
            break;
        case 2 : ByteSetMux2(buf, offset, stride, (unsigned char) value);
            break;
        case 4 : ByteSetMux4(buf, offset, stride, (unsigned char) value);
            break;
    }
    return TCL_OK;
}

int
ByteSetWithMaskCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;
    BitImage *mask;
    int value;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s byteImage bitMask value", argv[0]);

    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    mask = GetBitImage(argv[2]);
    ReturnErrorIf2(mask == NULL,
        "%s: no such bit image %s", argv[0], argv[2]);

    if (Tcl_GetInt(interp, argv[3], &value) != TCL_OK) {
        return TCL_ERROR;
    }
    ByteSetWithMask(buf, mask, (unsigned char) value);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_extend <buf> bw bh
 * precond 
 *     Buffer _buf_ exists.
 *     _bw_ and _bh_ are less than _buf_'s dimensions.
 * return 
 *     None
 * side effect :
 *     The borders of _buf_ (with total width _bw_ and height _bh_)
 *     are initialized by replicating pixels from just inside the border.
 *
 *----------------------------------------------------------------------
 */

int
ByteExtendCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *buf;
    int bw, bh;

    /*
     * Check args, retrieve buffer from hash table, parse rest of args
     */

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s byteImage bw nh", argv[0]);
    buf = GetByteImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    if ((Tcl_GetInt(interp, argv[2], &bw) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &bh) != TCL_OK)) {
        return TCL_ERROR;
    }
    ByteExtend(buf, bw, bh);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_get_width <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     width of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ByteGetWidth(srcBuf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_height <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     height of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ByteGetHeight(srcBuf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_x <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     x coordinate of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteGetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ByteGetX(srcBuf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_y <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     y coordinate of the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteGetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ByteGetY(srcBuf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_virtual <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     "1" if the buffer is virtual, "0" otherwise
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteGetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ByteGetVirtual(srcBuf));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_info <buf>
 * precond 
 *     Buffer _buf_ exists.
 * return 
 *     list of information about the buffer
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
ByteInfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf;

    /*
     * Check args, retrieve buffer from hash table
     */

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d",
        srcBuf->x, srcBuf->y, srcBuf->width, srcBuf->height,
        srcBuf->isVirtual);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_add <src1> <src2> <dest>
 * precond 
 *     Both buffer _src_ and _dest_ exists, and have the same shape.
 *     The buffers cannot overlap.
 * return 
 *     None
 * side effect :
 *     Add the content of buffer _src1_ and _src2, and output into _dest_.
 *
 *----------------------------------------------------------------------
 */

int
ByteAddCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *src1Buf, *src2Buf, *destBuf;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]);

    src1Buf = GetByteImage(argv[1]);
    ReturnErrorIf2(src1Buf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    src2Buf = GetByteImage(argv[2]);
    ReturnErrorIf2(src2Buf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    destBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    ByteAdd(src1Buf, src2Buf, destBuf);
    return TCL_OK;
}


int
ByteMultiplyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *srcBuf, *destBuf;
    int status;
    double k;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src k dest", argv[0]);

    srcBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &k);
    ReturnErrorIf(status != TCL_OK);

    destBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    ByteMultiply (srcBuf, (float)k, destBuf);
    return TCL_OK;
}
