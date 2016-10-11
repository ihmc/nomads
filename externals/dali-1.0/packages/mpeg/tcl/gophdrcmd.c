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
 *     mpeg_gop_hdr_new 
 * precond 
 *     none
 * return 
 *     a handle to a new allocated gop header
 * side effect :
 *     memory is allocated for gop hdr
 *
 *----------------------------------------------------------------------
 */

int 
MpegGopHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *gh;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    gh = MpegGopHdrNew();
    ReturnErrorIf1(gh == NULL,
        "%s : error allocating gop header.", argv[0]);

    PutMpegGopHdr(interp, gh);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_parse bitParser gopHdr
 * precond 
 *     bitparser is just before the gop header
 * return 
 *     none
 * side effect :
 *     gop_hdr is initialized to the hdr of the current gop
 *
 *----------------------------------------------------------------------
 */

int 
MpegGopHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegGopHdr *gh;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser gopHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    gh = GetMpegGopHdr(argv[2]);
    ReturnErrorIf2(gh == NULL,
        "%s: no such GOP header %s", argv[0], argv[2]);

    len = MpegGopHdrParse(bp, gh);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not at GOP start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_encode <gop_hdr> bitParser
 * precond 
 *     all structures are valid, allocated, and initialized.
 * return 
 *     none
 * side effect :
 *     gop header is written into bitstream assoc. with the bitparser.
 *
 *----------------------------------------------------------------------
 */

int 
MpegGopHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegGopHdr *gh;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr bitParser", argv[0]);

    gh = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(gh == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);
    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    len = MpegGopHdrEncode(gh, bp);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_find bitParser
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     bitstream is skip up to the beginning of the next mpeg gop hdr
 *
 *----------------------------------------------------------------------
 */
int
MpegGopHdrFindCmd(cd, interp, argc, argv)
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

    len = MpegGopHdrFind(bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_skip bitParser
 * precond 
 *     the bitparser is at the beginning of the gop hdr.
 * return 
 *     number of bytes skipped
 * side effect :
 *     bitstream is skip (ignored) up to the end of the mpeg gop hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSkipCmd(cd, interp, argc, argv)
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
    len = MpegGopHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not a valid gop header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_dump <input bitparser> <output bitparser>
 * precond 
 *     the bitparser is at the beginning of the gop hdr.
 * return 
 *     number of bytes dumped
 * side effect :
 *     bits are removed from input bitparser and put into output 
 *     bitparser.
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrDumpCmd(cd, interp, argc, argv)
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

    len = MpegGopHdrDump(inbp, outbp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not a valid gop header start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_free <hdr>
 * precond 
 *      <hdr> exists
 * return 
 *     none
 * side effect :
 *     the memory allocated for the gop hdr <hdr> is free'd.
 *
 *----------------------------------------------------------------------
 */
int
MpegGopHdrFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s gopHdr", argv[0]);

    hdr = RemoveMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such mpeg gop hdr %s", argv[0], argv[1]);

    MpegGopHdrFree(hdr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_get_broken_link <hdr> 
 * precond 
 *     <hdr> exists.
 * return 
 *     the broken link field of a GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrGetBrokenLinkCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s gopHdr", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such mpeg gop header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegGopHdrGetBrokenLink(hdr));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_get_closed_gop <hdr> 
 * precond 
 *     <hdr> exists.
 * return 
 *     the closed gop field for the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrGetClosedGopCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s gopHdr", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such mpeg gop header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegGopHdrGetClosedGop(hdr));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_drop_frame_flag <hdr> <drop_frame>
 * return 
 *     set the drop frame flag of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetDropFrameFlagCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, dropFrame;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr dropFrame", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &dropFrame);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetDropFrameFlag(hdr, dropFrame);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_hours <hdr> <hours>
 * return 
 *     set the time code hours of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetHoursCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, hours;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr hours", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &hours);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetHours(hdr, hours);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_minutes <hdr> <minutes>
 * return 
 *     set the time code minutes of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetMinutesCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, minutes;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr minutes", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &minutes);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetMinutes(hdr, minutes);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_seconds <hdr> <seconds>
 * return 
 *     set the time code seconds of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetSecondsCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, seconds;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr seconds", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &seconds);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetSeconds(hdr, seconds);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_pictures <hdr> <pictures>
 * return 
 *     set the time code pictures of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetPicturesCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, pictures;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr pictures", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &pictures);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetPictures(hdr, pictures);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_closed_gop <hdr> <closed_gop>
 * return 
 *     set the closed gop flag of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetClosedGopCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, closedGop;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr closedGop", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &closedGop);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetClosedGop(hdr, closedGop);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_gop_hdr_set_broken_link <hdr> <broken_link>
 * return 
 *     set the broken link of the GOP header
 *
 *----------------------------------------------------------------------
 */

int
MpegGopHdrSetBrokenLinkCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegGopHdr *hdr;
    int status, brokenLink;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s gopHdr brokenLink", argv[0]);

    hdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such gop header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &brokenLink);
    ReturnErrorIf(status != TCL_OK);

    MpegGopHdrSetBrokenLink(hdr, brokenLink);
    return TCL_OK;
}


/***TEMPORARY***/
int
MpegGopHdrSetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int dropFrame;
    int hours, minutes, seconds, pictures;
    int closed, broken;
    MpegGopHdr *gopHdr;

    ReturnErrorIf1(argc != 9,
        "wrong # args: should be %s gopHdr", argv[0]);

    gopHdr = GetMpegGopHdr(argv[1]);
    ReturnErrorIf2(gopHdr == NULL,
        "%s: no such mpeg gop header %s", argv[0], argv[1]);

    if ((Tcl_GetInt(interp, argv[2], &dropFrame) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &hours) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[4], &minutes) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[5], &seconds) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[6], &pictures) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[7], &closed) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[8], &broken) != TCL_OK)) {
        return TCL_ERROR;
    }
    MpegGopHdrSet(gopHdr, (char) 0, (char) hours, (char) minutes, (char) seconds, (char) pictures, (char) 0, (char) 0);
    return TCL_OK;
}
