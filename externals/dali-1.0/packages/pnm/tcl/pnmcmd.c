/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/* Documentation updated by Jiesang 10/11/98 */

#include "tclDvmPnm.h"

int PnmHdrNewCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 1,
        "wrong # args: should be %s", argv[0]);

    hdr = PnmHdrNew();
    PutPnmHdr(interp, hdr);

    return TCL_OK;
}


int PnmHdrFreeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s pnmHdr", argv[0]);

    hdr = RemovePnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    PnmHdrFree(hdr);

    return TCL_OK;
}


int PnmHdrCopyCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *src, *dest;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s srcPnmHdr destPnmHdr", argv[0]);

    src = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (src == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    dest = GetPnmHdr (argv[2]);
    ReturnErrorIf2 (dest == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[2]);
    PnmHdrCopy(src, dest);

    return TCL_OK;
}

int PnmHdrParseCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    PnmHdr *hdr;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitParser pnmHdr", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[1]);
    hdr = GetPnmHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[2]);
    
    bytes = PnmHdrParse(bp, hdr);

    ReturnErrorIf1 (bytes == DVM_PNM_INVALID_HDR,
        "invalid header encountered when parsing with %s", argv[1]);
    ReturnErrorIf1 (bytes == DVM_PNM_BS_UNDERFLOW,
        "not enough data when parsing with %s", argv[1]);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}


int PnmHdrEncodeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    PnmHdr *hdr;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s pnmHdr bitParser", argv[0]);

    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    bp = GetBitParser (argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[2]);
    
    bytes = PnmHdrEncode(hdr, bp);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}


int PnmHdrGetWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s pnmHdr", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", PnmHdrGetWidth(hdr));
    return TCL_OK;
}

int PnmHdrGetHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s pnmHdr", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", PnmHdrGetHeight(hdr));
    return TCL_OK;
}

int PnmHdrGetTypeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s pnmHdr", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);

    switch (PnmHdrGetType(hdr)) {
        case 1 : sprintf(interp->result, "pbm-text"); break;
        case 2 : sprintf(interp->result, "pgm-text"); break;
        case 3 : sprintf(interp->result, "ppm-text"); break;
        case 4 : sprintf(interp->result, "pbm-bin"); break;
        case 5 : sprintf(interp->result, "pgm-bin"); break;
        case 6 : sprintf(interp->result, "ppm-bin"); break;
        default : sprintf(interp->result, "unknown"); break;
    }
    return TCL_OK;
}

int PnmHdrGetMaxValCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s pnmHdr", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", PnmHdrGetMaxVal(hdr));

    return TCL_OK;
}

int PnmHdrSetWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;
    int w, status;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s pnmHdr width", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &w);
    ReturnErrorIf (status != TCL_OK);

    PnmHdrSetWidth(hdr, w);

    return TCL_OK;
}


int PnmHdrSetHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;
    int h, status;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s pnmHdr height", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf (status != TCL_OK);

    PnmHdrSetHeight(hdr, h);
    return TCL_OK;
}



int PnmHdrSetTypeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s pnmHdr type", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    if (!strcmp(argv[2], "pbm-text")) {
        PnmHdrSetType(hdr, PBM_TEXT);
    } else if (!strcmp(argv[2], "pgm-text")) {
        PnmHdrSetType(hdr, PGM_TEXT);
    } else if (!strcmp(argv[2], "ppm-text")) {
        PnmHdrSetType(hdr, PPM_TEXT);
    } else if (!strcmp(argv[2], "pbm-bin")) {
        PnmHdrSetType(hdr, PBM_BIN);
    } else if (!strcmp(argv[2], "pgm-bin")) {
        PnmHdrSetType(hdr, PGM_BIN);
    } else if (!strcmp(argv[2], "ppm-bin")) {
        PnmHdrSetType(hdr, PPM_BIN);
    } else {
        sprintf(interp->result, "%s%s", "unknown type. valid types are\n",
        "pbm-text, pgm-text, ppm-text, pbm-bin, pgm-bin and ppm-bin.");
        return TCL_ERROR;
    }
    return TCL_OK;
}


int PnmHdrSetMaxValCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    PnmHdr *hdr;
    int status, val;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s pnmHdr type", argv[0]);
    hdr = GetPnmHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &val);
    ReturnErrorIf (status != TCL_OK);

    PnmHdrSetMaxVal(hdr, val);
    return TCL_OK;
}


int BitStreamCastToByteCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    PnmHdr *hdr;
    ByteImage *byte;
    int offset, status;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s bitstream pnmHdr offset", argv[0]);

    bs = GetBitStream (argv[1]);
    ReturnErrorIf2 (bs == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    hdr = GetPnmHdr(argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &offset);
    ReturnErrorIf(status != TCL_OK);

    byte = BitStreamCastToByte (bs, hdr, offset);

    PutByteImage(interp, byte);
    return TCL_OK;
}


int ByteCastToBitStreamCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    ByteImage *byte;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    byte = GetByteImage(argv[1]);
    ReturnErrorIf2 (byte == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);

    bs = ByteCastToBitStream (byte);
    
    PutBitStream(interp, bs);
    return TCL_OK;
}


int BitStreamCastToBitCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    PnmHdr *hdr;
    BitImage *bit;
    int offset, status;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s bitStream pnmHdr offset", argv[0]);

    bs = GetBitStream (argv[1]);
    ReturnErrorIf2 (bs == NULL,
        "%s: no such bit stream %s", argv[0], argv[1]);
    hdr = GetPnmHdr(argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such pnm hdr %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &offset);
    ReturnErrorIf(status != TCL_OK);

    bit = BitStreamCastToBit (bs, hdr, offset);

    PutBitImage(interp, bit);
    return TCL_OK;
}


int BitCastToBitStreamCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitStream *bs;
    BitImage *bit;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s byteImage", argv[0]);

    bit = GetBitImage(argv[1]);
    ReturnErrorIf2 (bit == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);

    bs = BitCastToBitStream (bit);
    PutBitStream(interp, bs);

    return TCL_OK;
}

int PpmParseCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    ByteImage *r, *g, *b;
    int bytes;

    ReturnErrorIf1 (argc != 5,
        "wrong # args: should be %s bitParser r g b", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bit parser %s", argv[0], argv[1]);
    r = GetByteImage(argv[2]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);
    g = GetByteImage(argv[3]);
    ReturnErrorIf2 (g == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);
    b = GetByteImage(argv[4]);
    ReturnErrorIf2 (b == NULL,
        "%s: no such byte image %s", argv[0], argv[4]);

    bytes = PpmParse(bp, r, g, b);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PpmEncodeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    ByteImage *r, *g, *b;
    int bytes;

    ReturnErrorIf1 (argc != 5,
        "wrong # args: should be %s r g b bitParser", argv[0]);
    r = GetByteImage(argv[1]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);
    g = GetByteImage(argv[2]);
    ReturnErrorIf2 (g == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);
    b = GetByteImage(argv[3]);
    ReturnErrorIf2 (b == NULL,
        "%s: no such byte image %s", argv[0], argv[3]);
    bp = GetBitParser(argv[4]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[4]);

    bytes = PpmEncode(r, g, b, bp);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PgmParseCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    ByteImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitParser byteImage", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    r = GetByteImage(argv[2]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such byte image %s", argv[0], argv[2]);

    bytes = PgmParse(bp, r);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PgmEncodeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    ByteImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s byteImage bitParser", argv[0]);
    r = GetByteImage(argv[1]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such byte image %s", argv[0], argv[1]);
    bp = GetBitParser(argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    bytes = PgmEncode(r, bp);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PbmParseCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    BitImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitParser bitImage", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    r = GetBitImage(argv[2]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such bit image %s", argv[0], argv[2]);

    bytes = PbmParse(bp, r);
    ReturnErrorIf2 (bytes == DVM_PNM_IS_BYTE_ALIGN,
        "%s: bit image %s is byte-aligned. Use pbm_parse_8 instead", argv[0], argv[1]);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PbmParse8Cmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    BitImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitParser bitImage", argv[0]);
    bp = GetBitParser(argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    r = GetBitImage(argv[2]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such bit image %s", argv[0], argv[2]);

    bytes = PbmParse8(bp, r);

    ReturnErrorIf2 (bytes == DVM_PNM_NOT_BYTE_ALIGN,
        "%s: bit image %s is not byte-aligned. Use pbm_parse instead.", argv[0], argv[1]);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}


int PbmEncodeCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    BitImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitImage bitParser", argv[0]);
    r = GetBitImage(argv[1]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    bp = GetBitParser(argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    bytes = PbmEncode(r, bp);

    ReturnErrorIf2 (bytes == DVM_PNM_IS_BYTE_ALIGN,
        "%s: bit image %s is byte-aligned. Use pbm_encode_8 instead", argv[0], argv[1]);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

int PbmEncode8Cmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    BitImage *r;
    int bytes;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitImage bitParser", argv[0]);
    r = GetBitImage(argv[1]);
    ReturnErrorIf2 (r == NULL,
        "%s: no such bit image %s", argv[0], argv[1]);
    bp = GetBitParser(argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);

    bytes = PbmEncode8 (r, bp);

    ReturnErrorIf2 (bytes == DVM_PNM_NOT_BYTE_ALIGN,
        "%s: bit image %s is not byte-aligned. Use pbm_encode instead.", argv[0], argv[1]);

    sprintf(interp->result, "%d", bytes);
    return TCL_OK;
}

