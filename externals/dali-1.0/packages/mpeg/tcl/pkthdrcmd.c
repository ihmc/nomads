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

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_new 
 * return 
 *     a pkt hdr handle
 * side effect :
 *     memory is allocated for pkt hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegPktHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = MpegPktHdrNew();
    PutMpegPktHdr(interp, hdr);
    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_parse <bitparser> <pkt hdr>
 * precond 
 *     bitstream is just before the beginning of the pkt hdr.
 * return 
 *     a pkt hdr handle
 * side effect :
 *     pkt_hdr is initialized to the hdr of the current pkt.
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPktHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser pktHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    hdr = GetMpegPktHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pkt hdr %s", argv[0], argv[2]);

    len = MpegPktHdrParse(bp, hdr);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid packet header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_encode <pkt hdr> <bitparser>
 * precond 
 *     bitstream is just before the beginning of the pkt hdr.
 * return 
 *     a pkt hdr handle
 * side effect :
 *     pkt_hdr is initialized to the hdr of the current pkt.
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPktHdr *hdr;
    int len, size, status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s pktHdr pktSize bitParser", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pkt hdr %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &size);
    ReturnErrorIf(status != TCL_OK);

    bp = GetBitParser(argv[3]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[3]);

    len = MpegPktHdrEncode(hdr, size, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_find <bitparser>
 * side effect :
 *     bitstream is skip up to the beginning of the next mpeg pkt hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrFindCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int len;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitParser", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    len = MpegPktHdrFind(bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_dump <inbp> <outbp>
 * precond 
 *     the bitparser is at the beginning of the pkt hdr.
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg pkt hdr
 * postcond
 *     the bitparser is at the end of the pkt hdr.
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrDumpCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *inbp, *outbp;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s inBitParser outBitParser", argv[0]);

    inbp = GetBitParser(argv[1]);
    ReturnErrorIf2(inbp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    outbp = GetBitParser(argv[2]);
    ReturnErrorIf2(outbp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    len = MpegPktHdrDump(inbp, outbp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid packet header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_skip <bitparser>
 * precond 
 *     the bitparser is at the beginning of the pkt hdr.
 * side effect :
 *     bitstream is skip (ignored) up to the end of the mpeg pkt hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrSkipCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int len;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitParser", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    len = MpegPktHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid packet header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pkt_hdr_free <hdr>
 * precond 
 *     hdr is a valid mpeg pkt hdr structure
 * side effect :
 *     memory occupied by the pkt hdr is freed
 *
 *----------------------------------------------------------------------
 */

int
MpegPktHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s pktHdr", argv[0]);

    hdr = RemoveMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such pkt hdr %s", argv[0], argv[1]);

    MpegPktHdrFree(hdr);

    return TCL_OK;
}
int
MpegPktHdrGetLengthCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegPktHdrGetLength(hdr));
    return TCL_OK;
}

int
MpegPktHdrGetStreamIdCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegPktHdrGetStreamId(hdr));
    return TCL_OK;
}

int
MpegPktHdrGetBufferSizeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegPktHdrGetBufferSize(hdr));
    return TCL_OK;
}

int
MpegPktHdrGetPtsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    sprintf(interp->result, "%f", MpegPktHdrGetPts(hdr));
    return TCL_OK;
}

int
MpegPktHdrGetDtsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    sprintf(interp->result, "%f", MpegPktHdrGetDts(hdr));
    return TCL_OK;
}

int
MpegPktHdrSetLengthCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;
    int status;
    int length;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr length", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &length);
    ReturnErrorIf(status != TCL_OK);

    MpegPktHdrSetLength(hdr, length);
    return TCL_OK;
}

int
MpegPktHdrSetStreamIdCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;
    int status;
    int streamId;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr streamId", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &streamId);
    ReturnErrorIf(status != TCL_OK);

    MpegPktHdrSetStreamId(hdr, streamId);
    return TCL_OK;
}

int
MpegPktHdrSetBufferSizeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;
    int status;
    int bufferSize;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr bufferSize", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &bufferSize);
    ReturnErrorIf(status != TCL_OK);

    MpegPktHdrSetBufferSize(hdr, bufferSize);
    return TCL_OK;
}

int
MpegPktHdrSetPtsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;
    int status;
    double pts;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr pts", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &pts);
    ReturnErrorIf(status != TCL_OK);

    MpegPktHdrSetPts(hdr, pts);
    return TCL_OK;
}

int
MpegPktHdrSetDtsCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPktHdr *hdr;
    int status;
    double dts;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegPktHdr dts", argv[0]);

    hdr = GetMpegPktHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such packet header %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &dts);
    ReturnErrorIf(status != TCL_OK);

    MpegPktHdrSetDts(hdr, dts);
    return TCL_OK;
}

int
MpegPktHdrInitCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    return TCL_OK;
}


int
MpegPktHdrInfoCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    return TCL_OK;
}
