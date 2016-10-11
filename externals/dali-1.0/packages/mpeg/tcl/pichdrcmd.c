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
 *     mpeg_pic_hdr_new 
 * return 
 *     a pic hdr handle
 * side effect :
 *     memory is allocated for pic hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegPicHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = MpegPicHdrNew();
    PutMpegPicHdr(interp, hdr);
    return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_parse <bitparser> <pic hdr>
 * precond 
 *     bitstream is just before the beginning of the pic hdr.
 * return 
 *     a pic hdr handle
 * side effect :
 *     pic_hdr is initialized to the hdr of the current pic.
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPicHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser picHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    hdr = GetMpegPicHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pic hdr %s", argv[0], argv[2]);

    len = MpegPicHdrParse(bp, hdr);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid picture header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_encode <pic hdr> <bitparser>
 * precond 
 *     bitstream is just before the beginning of the pic hdr.
 * return 
 *     a pic hdr handle
 * side effect :
 *     pic_hdr is initialized to the hdr of the current pic.
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegPicHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr bitParser", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such pic hdr %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    len = MpegPicHdrEncode(hdr, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_find <bitparser>
 * side effect :
 *     bitstream is skip up to the beginning of the next mpeg pic hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrFindCmd(cd, interp, argc, argv)
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

    len = MpegPicHdrFind(bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_dump <inbp> <outbp>
 * precond 
 *     the bitparser is at the beginning of the pic hdr.
 * side effect :
 *     inbp is copied out to outbp up to the end of the mpeg pic hdr
 * postcond
 *     the bitparser is at the end of the pic hdr.
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrDumpCmd(cd, interp, argc, argv)
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

    len = MpegPicHdrDump(inbp, outbp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid picture header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_skip <bitparser>
 * precond 
 *     the bitparser is at the beginning of the pic hdr.
 * side effect :
 *     bitstream is skip (ignored) up to the end of the mpeg pic hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSkipCmd(cd, interp, argc, argv)
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
    len = MpegPicHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at a valid picture header start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_free <hdr>
 * precond 
 *     hdr is a valid mpeg pic hdr structure
 * side effect :
 *     memory occupied by the pic hdr is freed
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s picHdr", argv[0]);

    hdr = RemoveMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such pic hdr %s", argv[0], argv[1]);

    MpegPicHdrFree(hdr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_get_type <hdr>
 * return
 *     "i", "p", "b" or "d" based on the pic type
 *
 *----------------------------------------------------------------------
 */
int
MpegPicHdrGetTypeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *pic;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s picHdr", argv[0]);

    pic = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(pic == NULL,
        "%s : no such pic header %s", argv[0], argv[1]);

    switch (MpegPicHdrGetType(pic)) {
    case 1:
        sprintf(interp->result, "i");
        break;
    case 2:
        sprintf(interp->result, "p");
        break;
    case 3:
        sprintf(interp->result, "b");
        break;
    case 4:
        sprintf(interp->result, "d");
        break;
    default:
        sprintf(interp->result, "unknown");
        break;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_get_temporal_ref <hdr>
 * return
 *     the temporal reference of the pic hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegPicHdrGetTemporalRefCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *pic;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s picHdr", argv[0]);

    pic = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(pic == NULL,
        "%s : no such pic header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegPicHdrGetTemporalRef(pic));

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_temporal_ref <hdr> <temporal_ref>
 * return 
 *     set the temporal reference of the Picture header
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetTemporalRefCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, temporalRef;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr temporalRef", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &temporalRef);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetTemporalRef(hdr, temporalRef);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_type <hdr> <type>
 * return 
 *     set the picture coding type of the Picture header
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetTypeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr type", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    if (strcmp(argv[2], "i-frame") == 0) {
        MpegPicHdrSetType(hdr, I_FRAME);
    } else if (strcmp(argv[2], "p-frame") == 0) {
        MpegPicHdrSetType(hdr, P_FRAME);
    } else if (strcmp(argv[2], "b-frame") == 0) {
        MpegPicHdrSetType(hdr, B_FRAME);
    } else if (strcmp(argv[2], "d-frame") == 0) {
        MpegPicHdrSetType(hdr, D_FRAME);
    } else {
        sprintf(interp->result, "%s: Invalid frame type.", argv[0]);
    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_vbv_delay <hdr> <vbv_delay>
 * return 
 *     set the VBV delay of the Picture header
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetVBVDelayCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, vbvDelay;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr vbvDelay", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &vbvDelay);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetVBVDelay(hdr, vbvDelay);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_full_pel_forward <hdr> <full_pel_forward>
 * return 
 *     set the full_pel_forward_vector of the Picture header
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetFullPelForwardCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, fullPelForward;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr fullPelForward", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &fullPelForward);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetFullPelForward(hdr, fullPelForward);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_forward_f_code <hdr> <forward_f_code>
 * return 
 *     set the forward_f_code of the Picture header
 *     see Table 2-D.7 on page D-31 for valid values 
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetForwardFCodeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, forwardFCode;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr forwardFCode", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &forwardFCode);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetForwardFCode(hdr, forwardFCode);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_full_pel_backward <hdr> <full_pel_backward>
 * return 
 *     set the full_pel_backward_vector of the Picture header
 *
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetFullPelBackwardCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, fullPelBackward;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr fullPelBackward", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &fullPelBackward);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetFullPelBackward(hdr, fullPelBackward);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_pic_hdr_set_backward_f_code <hdr> <backward_f_code>
 * return 
 *     set the backward_f_code of the Picture header
 *     see Table 2-D.7 on page D-31 for valid values 
 *----------------------------------------------------------------------
 */

int
MpegPicHdrSetBackwardFCodeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegPicHdr *hdr;
    int status, backwardFCode;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s picHdr backwardFCode", argv[0]);

    hdr = GetMpegPicHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such picture header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &backwardFCode);
    ReturnErrorIf(status != TCL_OK);

    MpegPicHdrSetBackwardFCode(hdr, backwardFCode);
    return TCL_OK;
}


/***TEMPORARY***/
int
MpegPicHdrSetCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    /*MpegPicHdr *picHdr;
       int tempRef, type, vbvDelay;
       int fullPelForVec, forwardFCode, fullPelBackVec, backwardFCode;

       ReturnErrorIf1(argc != 9,
       "wrong # args : %s picHdr", argv[0]);

       picHdr  = GetMpegPicHdr(argv[1]);
       ReturnErrorIf2 (picHdr == NULL,
       "%s: no such mpeg pic header %s", argv[0], argv[1]);

       if ((Tcl_GetInt(interp, argv[2], &tempRef) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[3], &type) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[4], &vbvDelay) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[5], &fullPelForVec) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[6], &forwardFCode) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[7], &fullPelBackVec) != TCL_OK) ||
       (Tcl_GetInt(interp, argv[8], &backwardFCode) != TCL_OK) )
       return TCL_ERROR;

       MpegPicHdrSet(picHdr, tempRef, type, vbvDelay, fullPelForVec, forwardFCode, fullPelBackVec, backwardFCode); */
    return TCL_OK;
}
