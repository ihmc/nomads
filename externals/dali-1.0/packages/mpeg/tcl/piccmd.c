/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmMpeg.h"


#define GET_SLICE_INFO(interp, argv0, argvX, sliceInfo, sliceInfoLen) {\
    int i;\
    char **list;\
    if (Tcl_SplitList(interp, argvX, &sliceInfoLen, &list) != TCL_OK) {\
        sprintf(interp->result, "%s: expecting list of slice lengths", argv0);\
        return TCL_ERROR;\
    }\
    sliceInfo = (int*)malloc(sliceInfoLen*sizeof(int));\
    for (i = 0; i < sliceInfoLen; i++) {\
        if (Tcl_GetInt(interp, list[i], &sliceInfo[i]) != TCL_OK) {\
            sprintf(interp->result,"%s: error in parsing %s", argv0, argvX);\
            return TCL_ERROR;\
        }\
    }\
}

#define GET_INTERMEDIATES(interp, argv0, argvX, intermediates) {\
    int i;\
    char **intermArgv;\
    \
    if (Tcl_SplitList(interp, argvX, &i, &intermArgv) != TCL_OK) {\
        sprintf(interp->result, "%s: expecting a list of 3 byte images", argv0);\
        return TCL_ERROR;\
    }\
    ReturnErrorIf2 (i != 3,\
        "%s: %s should contain 3 byteImages.", argv0, argvX);\
    \
    for (i = 0; i < 3; i++) {\
        intermediates[i] = GetByteImage(intermArgv[i]); \
        ReturnErrorIf3 (intermediates[i] == NULL,\
            "%s: no such byte image (%d) of %s", argv0, i, argvX);\
    }\
}


int 
MpegPicIParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y;
    ScImage *u;
    ScImage *v;
    int status;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s <bitparser> <seq hdr> <pic hdr> <y sc> <u sc> <v sc>", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    seq_hdr = GetMpegSeqHdr(argv[2]);
    ReturnErrorIf2(seq_hdr == NULL,
        "%s: no such mpeg hdr %s", argv[0], argv[2]);
    pic_hdr = GetMpegPicHdr(argv[3]);
    ReturnErrorIf2(pic_hdr == NULL,
        "%s: no such mpeg pic hdr %s", argv[0], argv[3]);
    y = GetScImage(argv[4]);
    ReturnErrorIf2(y == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    u = GetScImage(argv[5]);
    ReturnErrorIf2(u == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    v = GetScImage(argv[6]);
    ReturnErrorIf2(v == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);

    status = MpegPicIParse(bp, seq_hdr, pic_hdr, y, u, v);
    ReturnErrorIf2(status == 0,
        "%s: Unexpected end of data while reading from bitstream %s", argv[0], argv[1]);

    return TCL_OK;
}

int 
MpegPicPParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y, *u, *v;
    VectorImage *mv;
    int status;

    ReturnErrorIf1(argc != 8,
        "wrong # args: should be\n %s <bitparser> <seq hdr> <pic hdr> <y sc> <u sc> <v sc> <y mv>", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    seq_hdr = GetMpegSeqHdr(argv[2]);
    ReturnErrorIf2(seq_hdr == NULL,
        "%s: no such mpeg hdr %s", argv[0], argv[2]);
    pic_hdr = GetMpegPicHdr(argv[3]);
    ReturnErrorIf2(pic_hdr == NULL,
        "%s: no such mpeg pic hdr %s", argv[0], argv[3]);
    y = GetScImage(argv[4]);
    ReturnErrorIf2(y == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    u = GetScImage(argv[5]);
    ReturnErrorIf2(u == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    v = GetScImage(argv[6]);
    ReturnErrorIf2(v == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);
    mv = GetVectorImage(argv[7]);
    ReturnErrorIf2(mv == NULL,
        "%s: no such vector image %s", argv[0], argv[7]);

    status = MpegPicPParse(bp, seq_hdr, pic_hdr, y, u, v, mv);
    ReturnErrorIf2(status == 0,
        "%s: Unexpected end of data while reading from bitstream %s", argv[0], argv[1]);

    return TCL_OK;
}

int 
MpegPicBParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSeqHdr *seq_hdr;
    MpegPicHdr *pic_hdr;
    ScImage *y, *u, *v;
    VectorImage *fwd, *bwd;
    int status;

    ReturnErrorIf1(argc != 9,
        "wrong # args: should be\n%s <bitparser> <seq_hdr> <pic_hdr> <y sc> <u sc> <v sc> <fwd mv> <bwd mv>", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    seq_hdr = GetMpegSeqHdr(argv[2]);
    ReturnErrorIf2(seq_hdr == NULL,
        "%s: no such mpeg hdr %s", argv[0], argv[2]);
    pic_hdr = GetMpegPicHdr(argv[3]);
    ReturnErrorIf2(pic_hdr == NULL,
        "%s: no such mpeg pic hdr %s", argv[0], argv[3]);

    y = GetScImage(argv[4]);
    ReturnErrorIf2(y == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    u = GetScImage(argv[5]);
    ReturnErrorIf2(u == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    v = GetScImage(argv[6]);
    ReturnErrorIf2(v == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);

    fwd = GetVectorImage(argv[7]);
    ReturnErrorIf2(fwd == NULL,
        "%s: no such vector image %s", argv[0], argv[7]);
    bwd = GetVectorImage(argv[8]);
    ReturnErrorIf2(bwd == NULL,
        "%s: no such vector image %s", argv[0], argv[8]);

    status = MpegPicBParse(bp, seq_hdr, pic_hdr, y, u, v, fwd, bwd);
    ReturnErrorIf2(status == 0,
        "%s: Unexpected end of data while reading from bitstream %s", argv[0], argv[1]);

    return TCL_OK;
}




/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_dump <inbp> <outbp>
 *
 * precond 
 *     the bitparser is at the beginning of the pic.
 *      
 * return 
 *     none
 * 
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg pic
 *
 * postcond
 *     the bitparser is at the end of the pic hdr.
 *
 *----------------------------------------------------------------------
 */

int 
MpegPicDumpCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *inbp, *outbp;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <in bitstream> <out bitstream>", argv[0]);

    inbp = GetBitParser(argv[1]);
    ReturnErrorIf2(inbp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    outbp = GetBitParser(argv[2]);
    ReturnErrorIf2(outbp == NULL,
        "%s: no such bitstream %s", argv[0], argv[2]);

    len = MpegPicDump(inbp, outbp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_skip <inbp>
 * precond 
 *     the bitparser is at the beginning of the pic.
 * return 
 *     none
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg pic
 * postcond
 *     the bitparser is at the end of the pic hdr.
 *
 *----------------------------------------------------------------------
 */

int 
MpegPicSkipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *inbp;
    int len;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitStream", argv[0]);

    inbp = GetBitParser(argv[1]);
    ReturnErrorIf2(inbp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    len = MpegPicSkip(inbp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

int 
MpegPicIEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPicHdr *picHdr;
    ScImage *scY;
    ScImage *scU;
    ScImage *scV;
    ByteImage *qScale;
    int sliceInfoLen;
    int *sliceInfo;

    ReturnErrorIf1(argc != 8,
        "wrong # args: should be %s <picHdr> <scY> <scU> <scV> <qScale> <sliceInfo> <bitparser>", argv[0]);

    picHdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(picHdr == NULL,
        "%s: no such mpeg picHdr %s", argv[0], argv[1]);
    scY = GetScImage(argv[2]);
    ReturnErrorIf2(scY == NULL,
        "%s: no such sc image %s", argv[0], argv[2]);
    scU = GetScImage(argv[3]);
    ReturnErrorIf2(scU == NULL,
        "%s: no such sc image %s", argv[0], argv[3]);
    scV = GetScImage(argv[4]);
    ReturnErrorIf2(scV == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    qScale = GetByteImage(argv[5]);
    ReturnErrorIf2(qScale == NULL,
        "%s: no such byte image %s", argv[0], argv[5]);

    GET_SLICE_INFO(interp, argv[0], argv[6], sliceInfo, sliceInfoLen);

    bp = GetBitParser(argv[7]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[7]);

    MpegPicIEncode(picHdr, scY, scU, scV, qScale, sliceInfo, sliceInfoLen, bp);
    free(sliceInfo);
    return TCL_OK;
}

int 
MpegPicPEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPicHdr *picHdr;
    ScImage *scY;
    ScImage *scU;
    ScImage *scV;
    VectorImage *fmv;
    ByteImage *qScale;
    int sliceInfoLen;
    int *sliceInfo;

    ReturnErrorIf1(argc != 9,
        "wrong # args: should be %s <picHdr> <scY> <scU> <scV> <fmv> <sliceInfo> <qScale> <bitparser>", argv[0]);

    picHdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(picHdr == NULL,
        "%s: no such mpeg picHdr %s", argv[0], argv[1]);
    scY = GetScImage(argv[2]);
    ReturnErrorIf2(scY == NULL,
        "%s: no such sc image %s", argv[0], argv[2]);
    scU = GetScImage(argv[3]);
    ReturnErrorIf2(scU == NULL,
        "%s: no such sc image %s", argv[0], argv[3]);
    scV = GetScImage(argv[4]);
    ReturnErrorIf2(scV == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    fmv = GetVectorImage(argv[5]);
    ReturnErrorIf2(fmv == NULL,
        "%s: no such vector image %s", argv[0], argv[5]);
    qScale = GetByteImage(argv[6]);
    ReturnErrorIf2(qScale == NULL,
        "%s: no such byte image %s", argv[0], argv[6]);

    GET_SLICE_INFO(interp, argv[0], argv[7], sliceInfo, sliceInfoLen);

    bp = GetBitParser(argv[8]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[8]);

    MpegPicPEncode(picHdr, scY, scU, scV, fmv, qScale, sliceInfo, sliceInfoLen, bp);
    free(sliceInfo);
    return TCL_OK;
}


int 
MpegPicBEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPicHdr *picHdr;
    ScImage *scY;
    ScImage *scU;
    ScImage *scV;
    VectorImage *fmv, *bmv;
    ByteImage *qScale;
    int sliceInfoLen;
    int *sliceInfo;

    ReturnErrorIf1(argc != 10,
        "wrong # args: should be %s <picHdr> <scY> <scU> <scV> <fmv> <bmv> <sliceInfo> <qScale> <bitparser>", argv[0]);

    picHdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(picHdr == NULL,
        "%s: no such mpeg picHdr %s", argv[0], argv[1]);
    scY = GetScImage(argv[2]);
    ReturnErrorIf2(scY == NULL,
        "%s: no such sc image %s", argv[0], argv[2]);
    scU = GetScImage(argv[3]);
    ReturnErrorIf2(scU == NULL,
        "%s: no such sc image %s", argv[0], argv[3]);
    scV = GetScImage(argv[4]);
    ReturnErrorIf2(scV == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    fmv = GetVectorImage(argv[5]);
    ReturnErrorIf2(fmv == NULL,
        "%s: no such vector image %s", argv[0], argv[5]);
    bmv = GetVectorImage(argv[6]);
    ReturnErrorIf2(bmv == NULL,
        "%s: no such vector image %s", argv[0], argv[6]);
    qScale = GetByteImage(argv[7]);
    ReturnErrorIf2(qScale == NULL,
        "%s: no such byte image %s", argv[0], argv[7]);

    GET_SLICE_INFO(interp, argv[0], argv[8], sliceInfo, sliceInfoLen);

    bp = GetBitParser(argv[9]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[9]);

    MpegPicBEncode(picHdr, scY, scU, scV, fmv, bmv, qScale, sliceInfo, sliceInfoLen, bp);
    free(sliceInfo);
    return TCL_OK;
}



int 
BytePMotionVecSearchCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *picHdr;
    ByteImage *curr, *prev;
    ByteImage *intermediates[3];
    VectorImage *fmv;

    ReturnErrorIf1(argc != 6,
        "wrong # args: should be %s <picHdr> <curr> <prev> <intermediates> <fmv>", argv[0]);

    picHdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(picHdr == NULL,
        "%s: no such mpeg pic header %s", argv[0], argv[1]);
    curr = GetByteImage(argv[2]);
    ReturnErrorIf2(curr == NULL,
        "%s: no such byteImage %s", argv[0], argv[2]);
    prev = GetByteImage(argv[3]);
    ReturnErrorIf2(prev == NULL,
        "%s: no such byteImage %s", argv[0], argv[3]);

    if (strcmp(argv[4], "") != 0) {
        GET_INTERMEDIATES(interp, argv[0], argv[4], intermediates);
    }
    fmv = GetVectorImage(argv[5]);
    ReturnErrorIf2(fmv == NULL,
        "%s: no such vectorImage %s", argv[0], argv[5]);

    BytePMotionVecSearch(picHdr, curr, prev, intermediates, fmv);
    return TCL_OK;
}


int 
ByteBMotionVecSearchCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *picHdr;
    ByteImage *curr, *prev, *next;
    ByteImage *interPrev[3], *interNext[3];
    VectorImage *fmv, *bmv;
    int *sliceInfo;
    int sliceInfoLen;

    ReturnErrorIf1(argc != 10,
        "wrong # args: should be %s <picHdr> <curr> <prev> \
        <interPrev> <interNext> <sliceInfo> <fmv> <bmv>", argv[0]);

    picHdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(picHdr == NULL,
        "%s: no such mpeg pic header %s", argv[0], argv[1]);
    curr = GetByteImage(argv[2]);
    ReturnErrorIf2(curr == NULL,
        "%s: no such byteImage %s", argv[0], argv[2]);
    prev = GetByteImage(argv[3]);
    ReturnErrorIf2(prev == NULL,
        "%s: no such byteImage %s", argv[0], argv[3]);
    next = GetByteImage(argv[4]);
    ReturnErrorIf2(next == NULL,
        "%s: no such byteImage %s", argv[0], argv[4]);

    if (strcmp(argv[5], "") != 0) {
        GET_INTERMEDIATES(interp, argv[0], argv[5], interPrev);
    }
    if (strcmp(argv[6], "") != 0) {
        GET_INTERMEDIATES(interp, argv[0], argv[6], interNext);
    }
    GET_SLICE_INFO(interp, argv[0], argv[7], sliceInfo, sliceInfoLen);

    fmv = GetVectorImage(argv[8]);
    ReturnErrorIf2(fmv == NULL,
        "%s: no such vectorImage %s", argv[0], argv[8]);
    bmv = GetVectorImage(argv[9]);
    ReturnErrorIf2(bmv == NULL,
        "%s: no such vectorImage %s", argv[0], argv[9]);

    ByteBMotionVecSearch(picHdr, curr, prev, next, interPrev, interNext, sliceInfo, sliceInfoLen, fmv, bmv);
    free(sliceInfo);
    return TCL_OK;
}

int 
ByteComputeIntermediatesCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *original;
    ByteImage *intermediates[3];

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <original> <intermediates>", argv[0]);

    original = GetByteImage(argv[1]);
    ReturnErrorIf2(original == NULL,
        "%s: no such mpeg pic header %s", argv[0], argv[1]);

    GET_INTERMEDIATES(interp, argv[0], argv[2], intermediates);

    ByteComputeIntermediates(original, intermediates);
    return TCL_OK;
}
