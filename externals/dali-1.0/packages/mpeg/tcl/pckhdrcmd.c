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
 *     mpeg_pck_hdr_new 
 * return 
 *     a pck hdr handle
 * side effect :
 *     memory is allocated for pck hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegPckHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = MpegPckHdrNew();
    PutMpegPckHdr(interp, hdr);
    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_parse <bitparser> <pck hdr>
 * precond 
 *     bitstream is just before the beginning of the pck hdr.
 * return 
 *     a pck hdr handle
 * side effect :
 *     pck_hdr is initialized to the hdr of the current pck.
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPckHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser pckHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    hdr = GetMpegPckHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pck hdr %s", argv[0], argv[2]);

    len = MpegPckHdrParse(bp, hdr);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid pack header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_encode <pck hdr> <bitparser>
 * precond 
 *     bitstream is just before the beginning of the pck hdr.
 * return 
 *     a pck hdr handle
 * side effect :
 *     pck_hdr is initialized to the hdr of the current pck.
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPckHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s pckHdr bitParser", argv[0]);

    hdr = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pck hdr %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    len = MpegPckHdrEncode(hdr, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_find <bitparser>
 * side effect :
 *     bitstream is skip up to the beginning of the next mpeg pck hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrFindCmd(cd, interp, argc, argv)
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

    len = MpegPckHdrFind(bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_dump <inbp> <outbp>
 * precond 
 *     the bitparser is at the beginning of the pck hdr.
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg pck hdr
 * postcond
 *     the bitparser is at the end of the pck hdr.
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrDumpCmd(cd, interp, argc, argv)
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

    len = MpegPckHdrDump(inbp, outbp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid pack header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_skip <bitparser>
 * precond 
 *     the bitparser is at the beginning of the pck hdr.
 * side effect :
 *     bitstream is skip (ignored) up to the end of the mpeg pck hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrSkipCmd(cd, interp, argc, argv)
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
    len = MpegPckHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid pack header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_free <hdr>
 * precond 
 *     hdr is a valid mpeg pck hdr structure
 * side effect :
 *     memory occupied by the pck hdr is freed
 *
 *----------------------------------------------------------------------
 */

int
MpegPckHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s pckHdr", argv[0]);

    hdr = RemoveMpegPckHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such pck hdr %s", argv[0], argv[1]);

    MpegPckHdrFree(hdr);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_get_mux_rate <hdr>
 * return
 *     multiplex rate of the system stream
 *
 *----------------------------------------------------------------------
 */
int
MpegPckHdrGetMuxRateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s pckHdr", argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegPckHdrGetMuxRate(pck));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pck_hdr_get_sys_clock_ref <hdr>
 * return
 *     the temporal reference of the pck hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegPckHdrGetSysClockRefCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s pckHdr", argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    sprintf(interp->result, "%f", MpegPckHdrGetSysClockRef(pck));

    return TCL_OK;
}


int
MpegPckHdrSetMuxRateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;
    int status, muxRate;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s pckHdr muxRate", argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &muxRate);
    ReturnErrorIf(status != TCL_OK);

    MpegPckHdrSetMuxRate(pck, muxRate);
    return TCL_OK;
}


int
MpegPckHdrSetSysClockRefCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;
    int status;
    double clock;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s pckHdr sysClockRef", argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &clock);
    ReturnErrorIf(status != TCL_OK);

    MpegPckHdrSetSysClockRef(pck, clock);

    return TCL_OK;
}



int
MpegPckHdrInfoCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s pckHdr", argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d %f",
        MpegPckHdrGetMuxRate(pck), MpegPckHdrGetSysClockRef(pck));

    return TCL_OK;
}


int
MpegPckHdrInitCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPckHdr *pck;
    int status, arg;
    int muxRate;
    double clock;

    ReturnErrorIf1(argc < 2,
        "wrong # args : %s pckHdr [-muxRate muxRate |-sysClockRef sysClockRef]",
        argv[0]);

    pck = GetMpegPckHdr(argv[1]);
    ReturnErrorIf2(pck == NULL,
        "%s : no such pack header %s", argv[0], argv[1]);

    for (arg = 2; arg < argc; arg++) {
        if (!strcmp(argv[arg], "-muxRate")) {
            status = Tcl_GetInt(interp, argv[++arg], &muxRate);
            ReturnErrorIf(status != TCL_OK);
            MpegPckHdrSetMuxRate(pck, muxRate);
        } else if (!strcmp(argv[arg], "-sysClockRef")) {
            status = Tcl_GetDouble(interp, argv[++arg], &clock);
            ReturnErrorIf(status != TCL_OK);
            MpegPckHdrSetSysClockRef(pck, clock);
        } else {
            sprintf(interp->result, "%s: illegal option %s", argv[0], argv[arg]);
            return TCL_ERROR;
        }
    }

    return TCL_OK;
}
