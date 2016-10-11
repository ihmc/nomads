/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/* Documentation updated 10/10/98 by Jiesang */

#include "tclDvmColor.h"

int 
ColorHashTableNewCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ColorHashTable *table;
    int size, status;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s bits", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &size);
    ReturnErrorIf (status != TCL_OK);

    table = ColorHashTableNew(size);
    PutColorHashTable(interp, table);

    return TCL_OK;
}


int 
ColorHashTableClearCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ColorHashTable *table;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s colorHashTable", argv[0]);

    table = GetColorHashTable (argv[1]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[1]);

    ColorHashTableClear(table);

    return TCL_OK;
}


int 
ColorHashTableFreeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ColorHashTable *table;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s colorHashTable", argv[0]);

    table = RemoveColorHashTable (argv[1]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[1]);

    ColorHashTableFree(table);

    return TCL_OK;
}


int 
ColorHashTableGetSizeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ColorHashTable *table;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s colorHashTable", argv[0]);

    table = GetColorHashTable (argv[1]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ColorHashTableGetSize(table));
    return TCL_OK;
}


int 
ColorHashTableGetNumOfEntryCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ColorHashTable *table;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s colorHashTable", argv[0]);

    table = GetColorHashTable (argv[1]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", ColorHashTableGetNumOfEntry(table));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_256 <red buf> <green buf> <blue buf> <out r map>
                  <out green map> <out blue map>
 * purpose
 *     reduce the colors of rgb image to 256 colors and store the
 *     256 colors in image map
 * return 
 *     nothing
 * side effect :
 *     the image maps are modified
 *
 *----------------------------------------------------------------------
 */

int
RgbTo256Cmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf;
    ImageMap  *rMap, *gMap, *bMap;
    ColorHashTable *table;

    ReturnErrorIf1 (argc != 8,
        "wrong # of args: should be %s r g b table rmap gmap bmap", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);
 
    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    table = GetColorHashTable(argv[4]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[4]);

    rMap = GetImageMap(argv[5]);
    ReturnErrorIf2(rMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[5]);

    gMap = GetImageMap(argv[6]);
    ReturnErrorIf2(gMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[6]);

    bMap = GetImageMap(argv[7]);
    ReturnErrorIf2(bMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[7]);

    RgbTo256(rBuf, gBuf, bBuf, table, rMap, gMap, bMap);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_quant_with_hash_table rByte gByte bByte rMap gMap bMap indexByte
 * purpose
 *     quatize the input rgb image into a 256 color image, based on 
 *     the 256 colors exists in image maps
 * return 
 *     nothing
 * side effect :
 *     indexByte is initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbQuantWithHashTableCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf;
    ByteImage *outByte;
    ColorHashTable *table;
    ImageMap  *rMap, *gMap, *bMap;

    ReturnErrorIf1 (argc != 9,
        "wrong # of args: should be %s r g b table rmap gmap bmap out", argv[0]);
  
    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);
 
    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    table = GetColorHashTable(argv[4]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[4]);

    rMap = GetImageMap(argv[5]);
    ReturnErrorIf2(rMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[5]);

    gMap = GetImageMap(argv[6]);
    ReturnErrorIf2(gMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[6]);

    bMap = GetImageMap(argv[7]);
    ReturnErrorIf2(bMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[7]);

    outByte = GetByteImage(argv[8]);
    ReturnErrorIf2(outByte == NULL,
        "%s: no such byte image %s", argv[0], argv[9]);

    RgbQuantWithHashTable (rBuf, gBuf, bBuf, table, rMap, gMap, bMap, outByte);

    return TCL_OK;
}


int
RgbQuantWithVpTreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf;
    ByteImage *outByte;
    VpNode *tree;
    ColorHashTable *table;
    ImageMap  *rMap, *gMap, *bMap;

    ReturnErrorIf1 (argc != 10,
        "wrong # of args: should be %s r g b tree table rmap gmap bmap out", argv[0]);
  
    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);
 
    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    tree = GetVpTree(argv[4]);
    ReturnErrorIf2(tree == NULL,
        "%s: no such color vp tree %s", argv[0], argv[4]);

    table = GetColorHashTable(argv[5]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such color hash table %s", argv[0], argv[5]);

    rMap = GetImageMap(argv[6]);
    ReturnErrorIf2(rMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[6]);

    gMap = GetImageMap(argv[7]);
    ReturnErrorIf2(gMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[7]);

    bMap = GetImageMap(argv[8]);
    ReturnErrorIf2(bMap == NULL,
        "%s: no such byte image map %s", argv[0], argv[8]);

    outByte = GetByteImage(argv[9]);
    ReturnErrorIf2(outByte == NULL,
        "%s: no such byte image %s", argv[0], argv[9]);

    RgbQuantWithVpTree(rBuf, gBuf, bBuf, tree, table, rMap, gMap, bMap, outByte);

    return TCL_OK;
}

int
YuvToRgb444Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s y u v r g b", argv[0]);

    yBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    uBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    vBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    rBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    gBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    bBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    YuvToRgb444(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf);

    return TCL_OK;
}

int
YuvToRgb422Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s y u v r g b", argv[0]);

    yBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    uBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    vBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    rBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    gBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    bBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    YuvToRgb422(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf);

    return TCL_OK;
}

int
YuvToRgb411Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s y u v r g b", argv[0]);

    yBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    uBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    vBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    rBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    gBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    bBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    YuvToRgb411(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf);

    return TCL_OK;
}


int
YuvToRgb420Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s y u v r g b", argv[0]);

    yBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    uBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    vBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    rBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    gBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    bBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    YuvToRgb420(yBuf, uBuf, vBuf, rBuf, gBuf, bBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_yuv_420 <rbuf> <gbuf> <bbuf> <ybuf> <ubuf> <vbuf>
 *
 * precond 
 *      All buffers exists.  the Y, R, G, B buffer must be of the same
 *      size.  The U and V buffers are half the width and half the height
 *      of Y
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the YUV buffers are initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbToYuv420Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s r g b y u v", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    yBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    uBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    vBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    RgbToYuv420(rBuf, gBuf, bBuf, yBuf, uBuf, vBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_yuv_411 <rbuf> <gbuf> <bbuf> <ybuf> <ubuf> <vbuf>
 *
 * precond 
 *      All buffers exists.  the Y, R, G, B buffer must be of the same
 *      size.  The U and V buffers are of the same height but only 1/4
 *      the width.
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the YUV buffers are initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbToYuv411Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s r g b y u v", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    yBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    uBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    vBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    RgbToYuv411(rBuf, gBuf, bBuf, yBuf, uBuf, vBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_yuv_422 <rbuf> <gbuf> <bbuf> <ybuf> <ubuf> <vbuf>
 *
 * precond 
 *      All buffers exists.  the Y, R, G, B buffer must be of the same
 *      size.  The U and V buffers are of the same height but only 1/4
 *      the width.
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the YUV buffers are initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbToYuv422Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s r g b y u v", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    yBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    uBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    vBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);


    RgbToYuv422(rBuf, gBuf, bBuf, yBuf, uBuf, vBuf);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_yuv_444 <rbuf> <gbuf> <bbuf> <ybuf> <ubuf> <vbuf>
 *
 * precond 
 *      all buffers exists and have same dimensions. 
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the YUV buffers are initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbToYuv444Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf, *uBuf, *vBuf;

    ReturnErrorIf1 (argc != 7,
        "wrong # args : should be %s r g b y u v", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    yBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    uBuf = GetByteImage(argv[5]);
    ReturnErrorIf2(uBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    vBuf = GetByteImage(argv[6]);
    ReturnErrorIf2(vBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);

    RgbToYuv444(rBuf, gBuf, bBuf, yBuf, uBuf, vBuf);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 *
 * usage   
 *     rgb_to_y <rbuf> <gbuf> <bbuf> <ybuf> 
 *
 * precond 
 *      all buffers exists.  the Y, R, G, B buffer must be of the same
 *      size. 
 *      
 * return 
 *     none
 * 
 * side effect :
 *     the Y buffers are initialized.
 *
 *----------------------------------------------------------------------
 */

int
RgbToYCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf, *yBuf;

    ReturnErrorIf1 (argc != 5,
        "wrong # args : should be %s r g b y", argv[0]);

    rBuf = GetByteImage(argv[1]);
    ReturnErrorIf2(rBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    gBuf = GetByteImage(argv[2]);
    ReturnErrorIf2(gBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bBuf = GetByteImage(argv[3]);
    ReturnErrorIf2(bBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);

    yBuf = GetByteImage(argv[4]);
    ReturnErrorIf2(yBuf == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    RgbToY(rBuf, gBuf, bBuf, yBuf);

    return TCL_OK;
}


int
VpTreeInitCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ImageMap  *rMap, *gMap, *bMap;
    VpNode *tree;

    if (argc != 5) {
        sprintf(interp->result, 
            "wrong # of args: should be %s rImap gImap bImap tree",
            argv[0]);
        return TCL_ERROR;
    }

    rMap = GetImageMap(argv[1]);
    if (rMap == NULL) {
        sprintf(interp->result, "%s: no such image map %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    gMap = GetImageMap(argv[2]);
    if (gMap == NULL) {
        sprintf(interp->result, "%s: no such image map %s", argv[0], argv[2]);
        return TCL_ERROR;
    }

    bMap = GetImageMap(argv[3]);
    if (bMap == NULL) {
        sprintf(interp->result, "%s: no such image map %s", argv[0], argv[3]);
        return TCL_ERROR;
    }

    tree = GetVpTree(argv[4]);
    ReturnErrorIf2 (tree == NULL,
        "%s: no such vp tree %s", argv[0], argv[4]);
    
    VpTreeInit( rMap, gMap, bMap, tree );

    return TCL_OK;
}


int
VpTreeNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VpNode *tree;

    ReturnErrorIf1 (argc != 1,
            "wrong # of args: should be %s", argv[0]);

    tree = VpTreeNew();
    PutVpTree(interp, tree);

    return TCL_OK;
}

int
VpTreeFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    VpNode *tree;

    ReturnErrorIf1 (argc != 2,
            "wrong # of args: should be %s vpTree", argv[0]);

    tree = GetVpTree(argv[1]);
    VpTreeFree(tree);

    return TCL_OK;
}
