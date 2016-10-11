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
 *     mpeg_sys_hdr_new 
 * return 
 *     a sys hdr handle
 * side effect :
 *     memory is allocated for sys hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegSysHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = MpegSysHdrNew();
    PutMpegSysHdr(interp, hdr);
    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_parse <bitparser> <sys hdr>
 * precond 
 *     bitstream is just before the beginning of the sys hdr.
 * return 
 *     a sys hdr handle
 * side effect :
 *     sys_hdr is initialized to the hdr of the current sys.
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSysHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser sysHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    hdr = GetMpegSysHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sys hdr %s", argv[0], argv[2]);

    len = MpegSysHdrParse(bp, hdr);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid system header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_encode <sys hdr> <bitparser>
 * precond 
 *     bitstream is just before the beginning of the sys hdr.
 * return 
 *     a sys hdr handle
 * side effect :
 *     sys_hdr is initialized to the hdr of the current sys.
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSysHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s sysHdr bitParser", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sys hdr %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    len = MpegSysHdrEncode(hdr, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_find <bitparser>
 * side effect :
 *     bitstream is skip up to the beginning of the next mpeg sys hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrFindCmd(cd, interp, argc, argv)
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

    len = MpegSysHdrFind(bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_dump <inbp> <outbp>
 * precond 
 *     the bitparser is at the beginning of the sys hdr.
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg sys hdr
 * postcond
 *     the bitparser is at the end of the sys hdr.
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrDumpCmd(cd, interp, argc, argv)
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

    len = MpegSysHdrDump(inbp, outbp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid system header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_skip <bitparser>
 * precond 
 *     the bitparser is at the beginning of the sys hdr.
 * side effect :
 *     bitstream is skip (ignored) up to the end of the mpeg sys hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrSkipCmd(cd, interp, argc, argv)
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
    len = MpegSysHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid system header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_sys_hdr_free <hdr>
 * precond 
 *     hdr is a valid mpeg sys hdr structure
 * side effect :
 *     memory occupied by the sys hdr is freed
 *
 *----------------------------------------------------------------------
 */

int
MpegSysHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s sysHdr", argv[0]);

    hdr = RemoveMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such sys hdr %s", argv[0], argv[1]);

    MpegSysHdrFree(hdr);

    return TCL_OK;
}
int
MpegSysHdrGetRateBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetRateBound(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetAudioBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetAudioBound(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetVideoBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetVideoBound(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetFixedFlagCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetFixedFlag(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetCspsFlagCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetCspsFlag(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetAudioLockCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetAudioLock(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetVideoLockCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetVideoLock(hdr));
    return TCL_OK;
}

int
MpegSysHdrGetNumOfStreamInfoCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSysHdrGetNumOfStreamInfo(hdr));
    return TCL_OK;
}


int
MpegSysHdrGetBufferSizeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status, id;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s mpegSysHdr id", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[1], &id);
    ReturnErrorIf(status != TCL_OK);

    sprintf(interp->result, "%d", MpegSysHdrGetBufferSize(hdr, id));
    return TCL_OK;
}


int
MpegSysHdrSetBufferSizeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status, id, value;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s mpegSysHdr id value", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[1], &id);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &value);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetBufferSize(hdr, id, value);
    return TCL_OK;
}



int
MpegSysHdrSetRateBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int rateBound;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr rateBound", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &rateBound);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetRateBound(hdr, rateBound);
    return TCL_OK;
}

int
MpegSysHdrSetAudioBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int audioBound;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr audioBound", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &audioBound);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetAudioBound(hdr, audioBound);
    return TCL_OK;
}

int
MpegSysHdrSetVideoBoundCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int videoBound;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr videoBound", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &videoBound);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetVideoBound(hdr, videoBound);
    return TCL_OK;
}

int
MpegSysHdrSetFixedFlagCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int fixedFlag;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr fixedFlag", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &fixedFlag);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetFixedFlag(hdr, fixedFlag);
    return TCL_OK;
}

int
MpegSysHdrSetCspsFlagCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int cspsFlag;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr cspsFlag", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &cspsFlag);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetCspsFlag(hdr, cspsFlag);
    return TCL_OK;
}

int
MpegSysHdrSetAudioLockCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int audioLock;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr audioLock", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &audioLock);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetAudioLock(hdr, audioLock);
    return TCL_OK;
}

int
MpegSysHdrSetVideoLockCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int videoLock;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr videoLock", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &videoLock);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetVideoLock(hdr, videoLock);
    return TCL_OK;
}

int
MpegSysHdrSetNumOfStreamInfoCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysHdr *hdr;
    int status;
    int numOfStreamInfo;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s mpegSysHdr numOfStreamInfo", argv[0]);

    hdr = GetMpegSysHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such system header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &numOfStreamInfo);
    ReturnErrorIf(status != TCL_OK);

    MpegSysHdrSetNumOfStreamInfo(hdr, numOfStreamInfo);
    return TCL_OK;
}

int
MpegSysHdrInfoCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    return TCL_OK;
}

int
MpegSysHdrInitCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    return TCL_OK;
}
