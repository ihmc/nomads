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

extern float pel_aspect_ratio_table[];
extern float picture_rate_table[];

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_new 
 * precond 
 *     allocate a new seq header
 * return 
 *     a handle to the new seq header
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = MpegSeqHdrNew();
    PutMpegSeqHdr(interp, hdr);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_parse bitParser seqHdr
 * precond 
 *     bitstream is at the beginning of the mpeg sequence hdr
 * return
 *     number of bytes parsed
 * side effect :
 *     header is parsed off the bitstream and seqHdr is initialized.
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrParseCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSeqHdr *hdr;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser seqHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    hdr = GetMpegSeqHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[2]);

    len = MpegSeqHdrParse(bp, hdr);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not a valid sequence start code", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_encode seqHdr bitParser
 * return
 *     number of bytes written
 * side effect :
 *     header is written into bitstream
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrEncodeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    MpegSeqHdr *hdr;
    int bytes;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr bitParser", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such seq hdr %s", argv[0], argv[1]);
    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[2]);

    bytes = MpegSeqHdrEncode(hdr, bp);
    sprintf(interp->result, "%d", bytes);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_find <bitparser>
 * precond 
 *     none
 * side effect :
 *     put bitparser in the beginning of seq hdr
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrFindCmd(clientData, interp, argc, argv)
    ClientData clientData;
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

    len = MpegSeqHdrFind(bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_dump inbp outbp
 * precond 
 *     bitstream is before a sequence header
 * side effect :
 *     bits are dumped from inbp to outbp
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrDumpCmd(clientData, interp, argc, argv)
    ClientData clientData;
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

    len = MpegSeqHdrDump(inbp, outbp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not a valid sequence start code", argv[0]);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_skip bitParser
 * precond 
 *     bitstream is just before the sequence header
 * side effect :
 *     bitstream is moved to the beginning of next GOP header.
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSkipCmd(clientData, interp, argc, argv)
    ClientData clientData;
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
        "%s: no such bitstream %s", argv[0], argv[1]);

    len = MpegSeqHdrSkip(bp);

    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: not a valid sequence start code", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_free seqHdr
 * precond 
 *     seqHdr exists
 * side effect :
 *     the memory allocated for the jpeg image hdr <hdr> is free'd.
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = RemoveMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence hdr %s", argv[0], argv[1]);

    MpegSeqHdrFree(hdr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_width <hdr>
 * return 
 *     width of the mpeg frames (in pixels)
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSeqHdrGetWidth(hdr));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_height <hdr>
 * return 
 *     Height of the mpeg frame (in pixels)
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSeqHdrGetHeight(hdr));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_iqt <hdr> 
 * return 
 *     A list giving the values in intra quantization table of the
 *     header <hdr>
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int j;
    char str[32];

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    if (hdr->iqt) {
        Tcl_SetResult(interp, str, TCL_VOLATILE);
        for (j = 0; j < 64; j++) {
            sprintf(str, "%d", hdr->iqt[j]);
            Tcl_AppendElement(interp, str);
        }
    } else {
        sprintf(interp->result, "error : no IQT in seq hdr %s", argv[1]);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_niqt <hdr> 
 * return 
 *     A list giving the values in non intra quantization table of the
 *     header <hdr>
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetNIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int j;
    char str[32];

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    if (hdr->niqt) {
        Tcl_SetResult(interp, str, TCL_VOLATILE);
        for (j = 0; j < 64; j++) {
            sprintf(str, "%d", hdr->niqt[j]);
            Tcl_AppendElement(interp, str);
        }
    } else {
        sprintf(interp->result, "error : no nIQT in seq hdr %s", argv[1]);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_aspect_ratio <hdr> 
 * return 
 *     the pel aspect ratio of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetAspectRatioCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    sprintf(interp->result, "%g", MpegSeqHdrGetAspectRatio(hdr));

    //  pel_aspect_ratio_table[(int)hdr->pel_aspect_ratio]); 
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_pic_rate <hdr> 
 * return 
 *     the picture rate of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetPicRateCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    sprintf(interp->result, "%g", MpegSeqHdrGetPicRate(hdr));
    // picture_rate_table[(int)hdr->picture_rate]);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_bit_rate <hdr> 
 * return 
 *     the bit rate of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetBitRateCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int rate;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    rate = MpegSeqHdrGetBitRate(hdr);
    if (rate == 0x03ffff) {
        sprintf(interp->result, "variable");
    } else {
        sprintf(interp->result, "%g", (double) rate * 400.0);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_get_buffer_size <hdr> 
 * return 
 *     the bit rate of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrGetBufferSizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegSeqHdrGetBufferSize(hdr));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_width <hdr> <width>
 * purpose 
 *     set the width of the mpeg frames (in pixels)
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status, width;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr width", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &width);
    ReturnErrorIf(status != TCL_OK);
    MpegSeqHdrSetWidth(hdr, width);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_height <hdr> <height>
 * purpose 
 *     set the height of the mpeg frames (in pixels)
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status, height;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr height", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &height);
    ReturnErrorIf(status != TCL_OK);

    MpegSeqHdrSetHeight(hdr, height);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_aspect_ratio <hdr> <ratio>
 * return 
 *     the pel aspect ratio of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetAspectRatioCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status;
    double aspectRatio;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr aspectRatio", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &aspectRatio);
    ReturnErrorIf(status != TCL_OK);

    MpegSeqHdrSetAspectRatio(hdr, aspectRatio);
    /*switch (aspectRatio) {
       case 1.0000:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_10000);
       break;
       case 0.6735:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_06735);
       break;
       case 0.7031:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_07031);
       break;
       case 0.7615:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_07615);
       break;
       case 0.8055:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_08055);
       break;
       case 0.8437:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_08437);
       break;
       case 0.8935:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_08935);
       break;
       case 0.9375:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_09375);
       break;
       case 0.9815:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_09815);
       break;
       case 1.0255:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_10255);
       break;
       case 1.0695:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_10695);
       break;
       case 1.1250:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_11250);
       break;
       case 1.1575:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_11575);
       break;
       case 1.2015:
       MpegSeqHdrSetAspectRatio(hdr, PEL_ASPECT_RATIO_12015);
       break;
       default:
       sprintf(interp->result, "Invalid pel_aspect_ratio.");
       return TCL_ERROR;
       } */
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_pic_rate <hdr> 
 * return 
 *     the picture rate of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetPicRateCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status;
    double picRate;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr picRate", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &picRate);
    ReturnErrorIf(status != TCL_OK);

    MpegSeqHdrSetPicRate(hdr, picRate);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_bit_rate <hdr> <bitrate>
 * purpose 
 *     set the bit rate of the MPEG sequence in units of 400 bits/sec
 *     rounded up.  -1 is variable bit rate.
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetBitRateCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status, bitRate;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr bitRate", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &bitRate);
    ReturnErrorIf(status != TCL_OK);
    if (bitRate != -1) {
        bitRate = (bitRate + 399) / 400;
    }
    MpegSeqHdrSetBitRate(hdr, bitRate);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_buffer_size <hdr> 
 * return 
 *     set the bit rate of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetBufferSizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status, bufferSize;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr bufferSize", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &bufferSize);
    ReturnErrorIf(status != TCL_OK);

    MpegSeqHdrSetBufferSize(hdr, bufferSize);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_constrained <hdr> <constrained>
 * return 
 *     set the constrained parameters flag of the MPEG sequence
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetConstrainedCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    int status, constrained;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr constrained", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &constrained);
    ReturnErrorIf(status != TCL_OK);

    MpegSeqHdrSetConstrained(hdr, constrained);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_iqt <hdr> <qtable>
 * return 
 *     set intra quantizer matrix of the MPEG sequence header
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    char **rows, **oneRow;
    int status, listLen, i, j, k = 0;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr qTable", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &listLen, &rows);
    ReturnErrorIf(status != TCL_OK);
    ReturnErrorIf2(listLen != 8,
        "%s: %s should have 8 rows.", argv[0], argv[2]);

    for (i = 0; i < 8; i++) {
        status = Tcl_SplitList(interp, rows[i], &listLen, &oneRow);
        ReturnErrorIf(status != TCL_OK);
        ReturnErrorIf3(listLen != 8,
            "%s: row %d of %s should have 8 elements.", argv[0], i, argv[2]);

        for (j = 0; j < 8; j++) {
            status = Tcl_GetInt(interp, oneRow[j], (int *) &(hdr->iqt[k]));
            ReturnErrorIf(status != TCL_OK);
            k++;
        }
    }

    /* could just add 1 to hdr->default_qt here but it may not be initialized */
    if (hdr->default_qt == 2) {
        hdr->default_qt = 3;
    } else {
        hdr->default_qt = 1;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_niqt <hdr> <qtable>
 * return 
 *     set non-intra quantizer matrix of the MPEG sequence header
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetNIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;
    char **rows, **oneRow;
    int status, listLen, i, j, k = 0;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s seqHdr qTable", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &listLen, &rows);
    ReturnErrorIf(status != TCL_OK);
    ReturnErrorIf2(listLen != 8,
        "%s: %s should have 8 rows.", argv[0], argv[2]);

    for (i = 0; i < 8; i++) {
        status = Tcl_SplitList(interp, rows[i], &listLen, &oneRow);
        ReturnErrorIf(status != TCL_OK);
        ReturnErrorIf3(listLen != 8,
            "%s: row %d of %s should have 8 elements.", argv[0], i, argv[2]);

        for (j = 0; j < 8; j++) {
            status = Tcl_GetInt(interp, oneRow[j], (int *) &(hdr->niqt[k]));
            ReturnErrorIf(status != TCL_OK);
            k++;
        }
    }

    /* could just add 2 to hdr->default_qt here but it may not be initialized */
    if (hdr->default_qt == 1) {
        hdr->default_qt = 3;
    } else {
        hdr->default_qt = 2;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_default_iqt <hdr>
 * return 
 *     set intra quantizer matrix of the MPEG sequence header to default
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetDefaultIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    MpegSeqHdrSetDefaultIQT(hdr);

    if (hdr->default_qt == 3) {
        hdr->default_qt = 2;
    } else if (hdr->default_qt != 2) {
        hdr->default_qt = 0;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_hdr_set_default_niqt <hdr>
 * return 
 *     set non-intra quantizer matrix of the MPEG sequence header to 
 *     default
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqHdrSetDefaultNIQTCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSeqHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s seqHdr", argv[0]);

    hdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such sequence header %s", argv[0], argv[1]);

    MpegSeqHdrSetDefaultNIQT(hdr);

    if (hdr->default_qt == 3) {
        hdr->default_qt = 1;
    } else if (hdr->default_qt != 1) {
        hdr->default_qt = 0;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     mpeg_seq_ender <bp>
 * purpose 
 *     mark the end of an MPEG sequence (not sequence header)
 *
 *----------------------------------------------------------------------
 */

int
MpegSeqEnderCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bp", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[2]);

    MpegSeqEnder(bp);
    return TCL_OK;
}


/***TEMPORARY***/
int
MpegSeqHdrSetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int width, height;
    MpegSeqHdr *seqHdr;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s seqHdr w h", argv[0]);

    seqHdr = GetMpegSeqHdr(argv[1]);
    ReturnErrorIf2(seqHdr == NULL,
        "%s: no such mpeg seq header %s", argv[0], argv[1]);

    if ((Tcl_GetInt(interp, argv[2], &width) != TCL_OK) ||
        (Tcl_GetInt(interp, argv[3], &height) != TCL_OK)) {
        return TCL_ERROR;
    }
    MpegSeqHdrSet(seqHdr, (short) width, (short) height, 1, 30, -1, 16, 0, 0, 0, NULL, NULL);
    return TCL_OK;
}
