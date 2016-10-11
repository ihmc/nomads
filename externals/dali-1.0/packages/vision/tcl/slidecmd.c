/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmVision.h"


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_make_from_bit_intersect <rBuf> <gBuf> <bBuf> <bit1> <bit2>
 *
 * precond 
 *     All buffers have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *     the ByteImages are written to visualize the intersection of the BitImages
 *
 *----------------------------------------------------------------------
 */

int
ByteMakeFromBitIntersectCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *rBuf, *gBuf, *bBuf;
    BitImage *srcBuf1, *srcBuf2;

    if (argc != 6) {
        sprintf( interp->result,
            "wrong # args: should be %s red green blue bit1 bit2", argv[0]);
        return TCL_ERROR;
    }

    rBuf = GetByteImage(argv[1]);
    if (rBuf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    gBuf = GetByteImage(argv[2]);
    if (gBuf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
       return TCL_ERROR;
    }

    bBuf = GetByteImage(argv[3]);
    if (bBuf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[3]);
       return TCL_ERROR;
    }

    srcBuf1 = GetBitImage(argv[4]);
    if (srcBuf1 == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[4]);
       return TCL_ERROR;
    }

    srcBuf2 = GetBitImage(argv[5]);
    if (srcBuf2 == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[5]);
       return TCL_ERROR;
    }

    ByteMakeFromBitIntersect(rBuf, gBuf, bBuf, srcBuf1, srcBuf2);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_make_from_bit_8 <bit> <byte>
 *
 * precond 
 *     Buffers exist and have the same dimensions
 *
 * return 
 *     nothing
 *
 * side effect :
 *      _byte_ contains the ByteImage representation of _bit_    
 *
 *----------------------------------------------------------------------
 */

int 
ByteMakeFromBit8Cmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *srcBuf;
    ByteImage *destBuf;

    srcBuf = GetBitImage(argv[1]);
    if (srcBuf == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    destBuf = GetByteImage(argv[2]);
    if (destBuf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
       return TCL_ERROR;
    }

    ByteMakeFromBit8(srcBuf, destBuf);

    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_compare <bit1> <bit2>
 *
 * precond 
 *     Buffers exist and have the same dimensions
 *
 * return 
 *     similairity of two images
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */

int
BitCompareCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *buf1, *buf2;
    float result;

    buf1 = GetBitImage(argv[1]);
    if (buf1 == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    buf2 = GetBitImage(argv[2]);
    if (buf2 == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[2]);
       return TCL_ERROR;
    }

    result = BitCompare(buf1, buf2);
    sprintf(interp->result, "%f", result);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_all_white <bit>
 *
 * precond 
 *     none
 *
 * return 
 *     1 if the image is all "0"s, 0 otherwise
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */
int
BitAllWhiteCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *buf;
    int result;

    buf = GetBitImage(argv[1]);
    if (buf == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    result = BitAllWhite(buf);
    sprintf(interp->result, "%d", result);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     bit_find_centroid <bit> <val>
 *
 * precond 
 *     _val_ is 0 or 1
 *
 * return 
 *     centroid for all pixels of value _val_
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */
int 
BitFindCentroidCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *buf;
    int val;
    int xmean, ymean;

    buf = GetBitImage(argv[1]);
    if (buf == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &val) != TCL_OK) {
        return TCL_ERROR;
    }

    BitFindCentroid(buf, val, &xmean, &ymean);

    sprintf(interp->result, "%d %d", xmean, ymean);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_find_bounding_box <buf> <px> <py>
 *
 * precond 
 *     none
 *
 * return 
 *     corners of the black bounding box containing the point (px, py)
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */
int 
ByteFindBoundingBoxCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *buf;
    int px, py;
    int x0, y0, x1, y1, x2, y2, x3, y3;

    buf = GetByteImage(argv[1]);
    if (buf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &px) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &py) != TCL_OK) {
        return TCL_ERROR;
    }

    ByteFindBoundingBox(buf, px, py, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);

    sprintf(interp->result, "%d %d %d %d %d %d %d %d", x0, y0, x1, y1, x2, y2, x3, y3);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_find_outer_corners <buf> <ix0> <iy0> ... <ix3> <iy3>
 *
 * precond 
 *     none
 *
 * return 
 *     interpolated projections corners based on the bounding box corners
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */

int 
ByteFindOuterCornersCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *buf;
    int x0, y0, x1, y1, x2, y2, x3, y3;
    int ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3;

    buf = GetByteImage(argv[1]);
    if (buf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &ix0) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &iy0) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[4], &ix1) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[5], &iy1) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[6], &ix2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[7], &iy2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[8], &ix3) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[9], &iy3) != TCL_OK) {
        return TCL_ERROR;
    }

    ByteFindOuterCorners(buf, ix0, iy0, ix1, iy1, ix2, iy2, ix3, iy3, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3);

    sprintf(interp->result, "%d %d %d %d %d %d %d %d", x0, y0, x1, y1, x2, y2, x3, y3);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_find_background_intensity <buf> 
 *
 * precond 
 *     none
 *
 * return 
 *     background intensity
 *
 * side effect :
 *      none    
 *
 *----------------------------------------------------------------------
 */

int 
ByteFindBackgroundIntensityCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    ByteImage *buf;
    int result;

    buf = GetByteImage(argv[1]);
    if (buf == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    result = ByteFindBackgroundIntensity(buf);
    
    sprintf(interp->result, "%d", result);

    return TCL_OK;
}



static char ThePreMask[9] = {
    (char)0xff, /* 11111111 */
    (char)0x7f, /* 01111111 */
    (char)0x3f, /* 00111111 */
    (char)0x1f, /* 00011111 */
    (char)0x0f, /* 00001111 */
    (char)0x07, /* 00000111 */
    (char)0x03, /* 00000011 */
    (char)0x01, /* 00000001 */
    (char)0x00, /* 00000000 */
};


static char ThePostMask[9] = {
    (char)0x00, /* 00000000 */
    (char)0x80, /* 10000000 */
    (char)0xc0, /* 11000000 */
    (char)0xe0, /* 11100000 */
    (char)0xf0, /* 11110000 */
    (char)0xf8, /* 11111000 */
    (char)0xfc, /* 11111100 */
    (char)0xfe, /* 11111110 */
    (char)0xff, /* 11111111 */
};


static char TheBitMask[8] = {
    (char)0x80, /* 10000000 */
    (char)0x40, /* 01000000 */
    (char)0x20, /* 00100000 */
    (char)0x10, /* 00010000 */
    (char)0x08, /* 00001000 */
    (char)0x04, /* 00000100 */
    (char)0x02, /* 00000010 */
    (char)0x01, /* 00000001 */
};

#define ZERO_FIRST(n, b) (unsigned char)((b) & ThePreMask[(n)])
#define ZERO_LAST(n, b)  (unsigned char)((b) & ThePostMask[8-(n)])
#define KEEP_MIDDLE(n1,n2,b) (unsigned char)(ZERO_FIRST((n1),(b)) & ZERO_LAST(7-(n2),(b)))
#define BITN(n,x) (unsigned char)(((x) & TheBitMask[(n)]) >> (7-(n)))


int 
BitFindTextAngleCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *src;
    ByteImage *dest;
    int w,h,wd, hd, parW,i,destDelta, temp, n, n1,n2;
    unsigned char *firstBuf, *currBuf, *destBuf;

    src = GetBitImage(argv[1]);
    if (src == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    dest = GetByteImage(argv[2]);
    if (dest == NULL) {
       sprintf (interp->result, "%s: no such byte image %s.", argv[0], argv[2]);
       return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &n) != TCL_OK) {
        sprintf (interp->result, "%s: expected int got %s.", argv[0], argv[3]);
        return TCL_ERROR;
    }

    if ((n != 2) && (n != 4) && (n!=8)) {
        sprintf (interp->result, "%s: bad value for n - %d.", argv[0], n);
        return TCL_ERROR;
    }

    w = src->byteWidth;
    h = src->height;
    wd = dest->width;
    hd = dest->height;

    if ((hd != h) || (wd != w*8)) {
       sprintf (interp->result, "%s: size mismatch.", argv[0]);
       return TCL_ERROR;
    }

    parW = src->parentWidth;
    firstBuf = src->firstByte;
    destBuf = dest->firstByte;
    destDelta = (dest->isVirtual)?(dest->parentWidth-wd):0;
    
    for (i=0; i < h; i++) {
        currBuf = firstBuf;
        DO_N_TIMES(w,
                   n1 = 0;
                   n2 = n-1;
                   switch (n) {
                   case 2:
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       n1 += n;
                       n2 += n;
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       n1 += n;
                       n2 += n;
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       n1 += n;
                       n2 += n;
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       n1 += n;
                       n2 += n;
                       break;
                   case 4:
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       n1 += n;
                       n2 += n;
                       temp = KEEP_MIDDLE(n1,n2,*currBuf);
                       temp = temp?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       break;
                   case 8:
                       temp = *currBuf?0:255;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       *destBuf++ = temp;
                       break;
                   };
                   currBuf++;
            );
        firstBuf += parW;
        destBuf += destDelta;
    }

    return TCL_OK;
}

int 
BitCountVertScanCmd (clientData, interp, argc, argv)
     ClientData clientData;
     Tcl_Interp *interp;
     int argc;
     char *argv[];
{
    BitImage *bit;
    unsigned int v, ibit;
    int w, h, bw, pw, val,y;
    unsigned char *src;
    int run[500], runnum=0, inrun;

    bit = GetBitImage(argv[1]);
    if (bit == NULL) {
       sprintf (interp->result, "%s: no such bit image %s.", argv[0], argv[1]);
       return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &v) != TCL_OK) {
        sprintf (interp->result, "%s: expected int got %s.", argv[0], argv[2]);
        return TCL_ERROR;
    }



    w = bit->unitWidth;
    bw = bit->byteWidth;
    h = bit->height;
    pw = bit->parentWidth;

    if ((v < 0)||(v >= (unsigned int)w)) {
        sprintf (interp->result, "%s: scanline param %d out of range.", argv[0], v);
        return TCL_ERROR;
    }


    src = bit->firstByte + ((v&0xfff8) >> 3);
    ibit = bit->firstBit + (v&0x7);
    src += (ibit & 0x8)>>3;
    ibit = ibit & 0x7;

    /* Now generate the run */

    inrun = 0;
    for (y=0; y<h; y++) {
        val = BITN(ibit,*src);

        if (val && !inrun) {
            run[runnum++]=y;
            inrun++;
        }
        if (!val && inrun) {
            run[runnum] = y-run[runnum-1];
            runnum++;
            inrun = 0;
        }
        src += pw;          
    }

    /* Now return the list */

    for (y=0; y<runnum; y++) {
        char buf[20];

        sprintf(buf," %d ", run[y]);
        Tcl_AppendResult(interp, buf, (char *)0);
    }

    return TCL_OK;
}
    


#define imRef(x,y,buf) (*((buf)->firstByte + (y)*((buf)->parentWidth) + (x)))

int
ByteCountEdgesCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *src;
    ByteImage *dest;
    int rad, w, h, x,y,xx,yy,s;

    src = GetByteImage(argv[1]);
    ReturnErrorIf2((src == NULL),
        "%s: no such byte image %s", argv[0], argv[1]);

    dest = GetByteImage(argv[2]);
    ReturnErrorIf2((src == NULL),
        "%s: no such byte image %s", argv[0], argv[2]);

    if (Tcl_GetInt(interp, argv[3], &rad) != TCL_OK) {
        sprintf(interp->result,
                "%s: expected int got %s", argv[0], argv[3]);
        return TCL_ERROR;
    }

    w = src->width;
    h = src->height;
    if ((w != dest->width) || (h != dest->height)) {
        sprintf(interp->result,
                "%s: size mismatch between src and dest image", argv[0]);
        return TCL_ERROR;
    }


    /* Start at offset (rad+1,rad+1) */
    for (y=rad; y < h-rad; y++) {
        for (x=rad; x<w-rad; x++) {
            s = 0;
            for (yy = y-rad; yy <= y+rad; yy++) {
                for (xx = x-rad; xx <= x+rad; xx++) {
                    s += imRef(xx,yy,src);
                }
            }
            imRef(x,y,dest) =  s;
        }
    }

    return TCL_OK;
}
