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

int 
MpegAudioHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    hdr = MpegAudioHdrNew();
    PutMpegAudioHdr(interp, hdr);
    return TCL_OK;
}


int 
MpegAudioHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s mpgAudioHdr", argv[0]);

    hdr = RemoveMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    MpegAudioHdrFree(hdr);

    return TCL_OK;
}


int 
MpegAudioHdrFindCmd(cd, interp, argc, argv)
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

    len = MpegAudioHdrFind(bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


int 
MpegAudioHdrDumpCmd(cd, interp, argc, argv)
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

    len = MpegAudioHdrDump(inbp, outbp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: invalid audio header sync word", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


int 
MpegAudioHdrSkipCmd(cd, interp, argc, argv)
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

    len = MpegAudioHdrSkip(bp);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: invalid audio header sync word", argv[0]);

    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


int 
MpegAudioHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    BitParser *bp;
    int len;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s bitParser mpgAudioHdr", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    hdr = GetMpegAudioHdr(argv[2]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[2]);

    len = MpegAudioHdrParse(bp, hdr);
    ReturnErrorIf1(len == DVM_MPEG_INVALID_START_CODE,
        "%s: invalid MPEG Audio start code.", argv[0]);

    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


int 
MpegAudioHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *hdr;
    BitParser *bp;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s mpgAudioHdr bitParser", argv[0]);

    hdr = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    status = MpegAudioHdrEncode(hdr, bp);
    ReturnErrorIf1(status == DVM_MPEG_INVALID_START_CODE,
        "%s: invalid MPEG Audio start code.", argv[0]);

    return TCL_OK;
}


int 
MpegAudioHdrGetLayerCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *header;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s mpgAudioHdr", argv[0]);

    header = GetMpegAudioHdr(argv[1]);

    ReturnErrorIf2(header == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", MpegAudioHdrGetLayer(header));

    return TCL_OK;
}


int 
MpegAudioHdrGetModeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *header;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s mpgAudioHdr", argv[0]);

    header = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(header == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    switch (MpegAudioHdrGetMode(header)) {
    case MPEG_AUDIO_STEREO:
        sprintf(interp->result, "stereo");
        break;
    case MPEG_AUDIO_JOINT_STEREO:
        sprintf(interp->result, "joint stereo");
        break;
    case MPEG_AUDIO_DUAL_CHANNEL:
        sprintf(interp->result, "dual channel");
        break;
    case MPEG_AUDIO_SINGLE_CHANNEL:
        sprintf(interp->result, "single channel");
        break;
    }
    return TCL_OK;
}


int 
MpegAudioHdrGetBitRateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *header;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s mpgAudioHdr", argv[0]);

    header = GetMpegAudioHdr(argv[1]);
    /*sprintf(interp->result, "0"); */
    /*return TCL_OK; */
    ReturnErrorIf2(header == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    sprintf(interp->result, "%f", MpegAudioHdrGetBitRate(header));
    return TCL_OK;
}


int 
MpegAudioHdrGetSamplingRateCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioHdr *header;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s mpgAudioHdr", argv[0]);

    header = GetMpegAudioHdr(argv[1]);
    ReturnErrorIf2(header == NULL,
        "%s: no such MPEG audio header %s", argv[0], argv[1]);

    sprintf(interp->result, "%f", MpegAudioHdrGetSamplingRate(header));
    return TCL_OK;
}
