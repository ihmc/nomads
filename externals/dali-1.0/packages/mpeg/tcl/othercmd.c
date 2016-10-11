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
MpegAnyHdrFindCmd(cd, interp, argc, argv)
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
        "%s: no such bit parser %s", argv[0], argv[1]);
    len = MpegAnyHdrFind(bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

int
MpegSeqEndCodeEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitParser", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[1]);
    MpegSeqEndCodeEncode(bp);

    return TCL_OK;
}


int
MpegGetCurrStartCodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int code;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s bitParser", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[1]);
    code = MpegGetCurrStartCode(bp);

    if (code == GOP_START_CODE) {
        sprintf(interp->result, "gop-start-code");
    } else if (code == PIC_START_CODE) {
        sprintf(interp->result, "pic-start-code");
    } else if (code == SEQ_START_CODE) {
        sprintf(interp->result, "seq-start-code");
    } else if (code == SEQ_END_CODE) {
        sprintf(interp->result, "seq-end-code");
    } else if (code == SYS_START_CODE) {
        sprintf(interp->result, "sys-start-code");
    } else if (code == ISO_11172_END_CODE) {
        sprintf(interp->result, "sys-end-code");
    } else if (code == PACK_START_CODE) {
        sprintf(interp->result, "pack-start-code");
    } else if (code >= SLICE_MIN_START_CODE &&
        code <= SLICE_MAX_START_CODE) {
        sprintf(interp->result, "slice-start-code");
    } else if (code >= PACKET_MIN_START_CODE &&
        code <= PACKET_MAX_START_CODE) {
        sprintf(interp->result, "packet-start-code");
    } else if ((code & 0xFFF00000) == 0xFFF00000) {
        sprintf(interp->result, "audio-start-code");
    } else {
        sprintf(interp->result, "unknown");
    }
    return TCL_OK;
}
