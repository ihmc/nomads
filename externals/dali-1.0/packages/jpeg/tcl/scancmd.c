/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
#include "tclDvmJpeg.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_parse <bitparser> <hdr> <scanhdr> <sc1> <sc2> ...
 * precond 
 *     bitparser is at the beginning of a jpeg scan
 * return 
 *     none
 * side effect :
 *     a jpeg scan parsed off the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    ScImage *sc[3];
    int num, i, ret;

    ReturnErrorIf1 (argc < 5,
        "wrong # args: should be %s bitParser hdr scanHdr sc1 sc2 ..",
        argv[0]);

    ReturnErrorIf0 (argc > 7,
        "too many args: possible too many sc images (at most 3)");

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    num = argc - 4;
    for (i=0; i<num; i++) {
        sc[i] = GetScImage(argv[i+4]);
        ReturnErrorIf2 (sc[i] == NULL,
            "%s: no such sc image %s", argv[0], argv[i+4]);
    }

    ret = JpegScanParse (bp, hdr, shdr, sc, num);
    ReturnErrorIf2 (ret == DVM_JPEG_WRONG_COMPONENTS,
        "%s: number of components passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_WIDTH,
        "%s: the width of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_HEIGHT,
        "%s: the height of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]); 

    sprintf(interp->result, "%d", ret);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_sel_parse <bitparser> <hdr> <scanhdr> <sc1> <sc2> ...
 * precond 
 *     bitparser is at the beginning of a jpeg scan
 * return 
 *     none
 * side effect :
 *     a JPEG scan parsed off the bitparser. Howver, can include null
 *     sc bufs if we want to ignore a scan
 *
 *----------------------------------------------------------------------
 */
int
JpegScanSelectiveParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    ScImage *sc[3];
    int num, i, ret;

    ReturnErrorIf1 (argc < 5,
        "wrong # args: should be %s bitParser hdr scanHdr sc1 sc2 ..",
        argv[0]);
    ReturnErrorIf0(argc > 7,
        "too many args: possible too many sc images (at most 3)");

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    num = argc - 4;
    for (i=0; i<num; i++) {
        sc[i] = GetScImage(argv[i+4]);
    }

    ret = JpegScanSelectiveParse (bp, hdr, shdr, sc, num);
    ReturnErrorIf2 (ret == DVM_JPEG_WRONG_COMPONENTS,
        "%s: number of components passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_WIDTH,
        "%s: the width of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_HEIGHT,
        "%s: the height of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]); 

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_inc_parse_start <bitparser> <hdr> <scanhdr> <sc1> ...
 * precond 
 *     bitparser is at the beginning of a JPEG scan
 * return 
 *     total number of MCU's in the scan
 * side effect :
 *     getting ready for an incremental scan parse
 *
 *----------------------------------------------------------------------
 */
int
JpegScanIncParseStartCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    ScImage *sc[3];
    int num, i, ret;

    ReturnErrorIf1 (argc < 5,
        "wrong # args: should be %s bitParser hdr scanHdr sc1 sc2 ..",
        argv[0]);
    ReturnErrorIf0(argc > 7,
        "too many args: possible too many sc images (at most 3)");

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    num = argc - 4;
    for (i=0; i<num; i++) {
        sc[i] = GetScImage(argv[i+4]);
        ReturnErrorIf2 (sc[i] == NULL,
            "%s: no such sc image %s", argv[0], argv[i+4]);
    }

    ret = JpegScanIncParseStart (bp, hdr, shdr, sc, num);
    ReturnErrorIf2 (ret == DVM_JPEG_WRONG_COMPONENTS,
        "%s: number of components passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_WIDTH,
        "%s: the width of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_HEIGHT,
        "%s: the height of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]); 

    sprintf(interp->result,"%d",ret);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_inc_parse_end <bitparser> <hdr> <scanhdr> <sc1> ...
 * precond 
 *     none
 * return 
 *     noe
 * side effect :
 *     memory alloced in ...in_parse_start is freed
 *
 *----------------------------------------------------------------------
 */
int
JpegScanIncParseEndCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    ScImage *sc[3];
    int num, i;

    ReturnErrorIf1 (argc < 5,
        "wrong # args: should be %s bitparser hdr scanHdr sc1 sc2 ..",
        argv[0]);
    ReturnErrorIf0(argc > 7,
        "too many args: possible too many sc images (at most 3)");

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    num = argc - 4;
    for (i=0; i<num; i++) {
        sc[i] = GetScImage(argv[i+4]);
        ReturnErrorIf2 (sc[i] == NULL,
            "%s: no such sc image %s", argv[0], argv[i+4]);
    }

    JpegScanIncParseEnd (bp, hdr, shdr, sc, num);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_inc_parse <bitparser> <hdr> <scanhdr> <howmany> <sc1> ..
 * precond 
 *     jpeg_scan_inc_parse_start has been called before.
 * return 
 *     the mcu number where the next parse will start
 * side effect :
 *     <howmany> mcus parsed off the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanIncParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    ScImage *sc[3];
    int num, i, howmany, status, ret;

    ReturnErrorIf1 (argc < 6,
        "wrong # args: should be %s bitParser hdr scanHdr howMany sc1 ..",
        argv[0]);
    ReturnErrorIf0(argc > 8,
        "too many args: possible too many sc images (at most 3)");

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    status = Tcl_GetInt(interp,argv[4],&howmany);
    ReturnErrorIf2 (status != TCL_OK,
        "%s: expected integer got %s", argv[0], argv[4]);

    num = argc - 5;
    for (i=0; i<num; i++) {
        sc[i] = GetScImage(argv[i+5]);
        ReturnErrorIf2 (sc[i] == NULL,
            "%s: no such sc image %s", argv[0], argv[i+5]);
    }

    ret = JpegScanIncParse (bp, hdr, shdr, sc, num, howmany);
    sprintf(interp->result,"%d",ret);
    return TCL_OK;
}




/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_encode <hdr> <scanhdr> <sc1> <sc2> ... <bitparser>
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     a JPEG scan encoded onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    JpegHuffTable *huffTable;
    ScImage *sc[3];
    int i, ret;

    ReturnErrorIf1 (argc < 6,
        "wrong # args: should be %s jpegHdr scanHdr huffTable sc1 sc2 .. bitParser",
        argv[0]);

    ReturnErrorIf0 (argc > 8,
        "wrong # args: possibly too many sc image. there can be at most three components in an image.");

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable (argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG Huffman table %s", argv[0], argv[3]);

    for (i = 4; i < argc - 1; i++) {
        sc[i-4] = GetScImage(argv[i]);
        ReturnErrorIf2 (sc[i-4] == NULL,
            "%s: no such sc image %s", argv[0], argv[i]);
    }

    bp = GetBitParser (argv[argc-1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[argc-1]);

    ret = JpegScanEncode (hdr, shdr, huffTable, sc, argc - 5, bp);

    ReturnErrorIf2 (ret == DVM_JPEG_WRONG_COMPONENTS,
        "%s: number of components passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_WIDTH,
        "%s: the width of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]);
    ReturnErrorIf2 (ret == DVM_JPEG_INVALID_HEIGHT,
        "%s: the height of SC images passed in does not match what is specified in JPEG header %s", argv[0], argv[2]); 

    sprintf(interp->result, "%d", ret);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_encode_420 hdr scanHdr huffTable scy scu scv bitparser
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     a jpeg scan encoded onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanEncode420Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    JpegHuffTable *huffTable;
    ScImage *scy, *scu, *scv;
    int ret;

    ReturnErrorIf1 (argc != 8,
        "wrong # args: should be %s jpegHdr scanHdr huffTable scy scu scv bitParser",
        argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable (argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG Huffman table %s", argv[0], argv[3]);
    scy = GetScImage(argv[4]);
    ReturnErrorIf2 (scy == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    scu = GetScImage(argv[5]);
    ReturnErrorIf2 (scu == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    scv = GetScImage(argv[6]);
    ReturnErrorIf2 (scv == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);
    bp = GetBitParser (argv[7]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[7]);

    ret = JpegScanEncode420 (hdr, shdr, huffTable, scy, scu, scv, bp);
    sprintf(interp->result, "%d", ret);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_encode_422 hdr scanHdr huffTable scy scu scv bitparser
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     a jpeg scan encoded onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanEncode422Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    JpegHuffTable *huffTable;
    ScImage *scy, *scu, *scv;
    int ret;

    ReturnErrorIf1 (argc != 8,
        "wrong # args: should be %s jpegHdr scanHdr huffTable scy scu scv bitParser",
        argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable (argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG Huffman table %s", argv[0], argv[3]);
    scy = GetScImage(argv[4]);
    ReturnErrorIf2 (scy == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    scu = GetScImage(argv[5]);
    ReturnErrorIf2 (scu == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    scv = GetScImage(argv[6]);
    ReturnErrorIf2 (scv == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);
    bp = GetBitParser (argv[7]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[7]);

    ret = JpegScanEncode422 (hdr, shdr, huffTable, scy, scu, scv, bp);
    sprintf(interp->result, "%d", ret);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_encode_done <bitparser>
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     the jpeg trailer is written onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegEndCodeEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int len;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s bitparser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    len = JpegEndCodeEncode (bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_encode_done <bitparser>
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     the jpeg trailer is written onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegStartCodeEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    int len;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s bitparser", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);

    len = JpegStartCodeEncode (bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_inc_encode_420 hdr scanHdr huffTable scy scu scv bitparser
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     a jpeg scan encoded onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanIncEncode420Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    JpegHuffTable *huffTable;
    ScImage *scy, *scu, *scv;
    int ret;

    ReturnErrorIf1 (argc != 8,
        "wrong # args: should be %s jpegHdr scanHdr huffTable scy scu scv bitParser",
        argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable (argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG Huffman table %s", argv[0], argv[3]);
    if (strcmp(argv[4], "null")) {
        scy = NULL;
    } else {
        scy = GetScImage(argv[4]);
        ReturnErrorIf2 (scy == NULL,
            "%s: no such sc image %s", argv[0], argv[4]);
    }
    scu = GetScImage(argv[5]);
    ReturnErrorIf2 (scu == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    scv = GetScImage(argv[6]);
    ReturnErrorIf2 (scv == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);
    bp = GetBitParser (argv[7]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[7]);

    ret = JpegScanIncEncode420 (hdr, shdr, huffTable, scy, scu, scv, bp);
    sprintf(interp->result, "%d", ret);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_inc_encode_422 hdr scanHdr huffTable scy scu scv bitparser
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     a jpeg scan encoded onto the bitparser
 *
 *----------------------------------------------------------------------
 */
int
JpegScanIncEncode422Cmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    JpegHuffTable *huffTable;
    ScImage *scy, *scu, *scv;
    int ret;

    ReturnErrorIf1 (argc != 8,
        "wrong # args: should be %s jpegHdr scanHdr huffTable scy scu scv bitParser",
        argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable (argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG Huffman table %s", argv[0], argv[3]);
    if (strcmp(argv[4], "null")) {
        scy = NULL;
    } else {
        scy = GetScImage(argv[4]);
        ReturnErrorIf2 (scy == NULL,
            "%s: no such sc image %s", argv[0], argv[4]);
    }
    scy = GetScImage(argv[4]);
    ReturnErrorIf2 (scy == NULL,
        "%s: no such sc image %s", argv[0], argv[4]);
    scu = GetScImage(argv[5]);
    ReturnErrorIf2 (scu == NULL,
        "%s: no such sc image %s", argv[0], argv[5]);
    scv = GetScImage(argv[6]);
    ReturnErrorIf2 (scv == NULL,
        "%s: no such sc image %s", argv[0], argv[6]);
    bp = GetBitParser (argv[7]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[7]);

    ret = JpegScanIncEncode422 (hdr, shdr, huffTable, scy, scu, scv, bp);
    sprintf(interp->result, "%d", ret);

    return TCL_OK;
}

