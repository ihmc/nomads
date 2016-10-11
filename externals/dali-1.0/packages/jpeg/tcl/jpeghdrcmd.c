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
 *     jpeg_hdr_new 
 * precond 
 *     allocate a new jpeg header
 * return 
 *     a handle to the new jpeg header
 * side effect :
 *     memory is allocated for a new jpeg header
 *
 *----------------------------------------------------------------------
 */
int
JpegHdrNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1 (argc != 1,
        "wrong # args: should be %s", argv[0]);
    hdr = JpegHdrNew();
    PutJpegHdr(interp, hdr);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_free <hdr>
 * precond 
 *      <hdr> exists
 * side effect :
 *     the memory allocated for the jpeg image hdr <hdr> is free'd.
 *
 *----------------------------------------------------------------------
 */
int
JpegHdrFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s jpegHdr", argv[0]);

    hdr = RemoveJpegHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such jpeg header %s", argv[0], argv[1]);

    JpegHdrFree (hdr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_parse bitstream hdr
 * precond 
 *     bitstream is at the beginning of the jpeg hdr
 * return 
 *     none
 * side effect :
 *     jpeg hdr is parsed off the bitstream
 *
 *----------------------------------------------------------------------
 */
int
JpegHdrParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    int len;

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s bitParser hdr", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);

    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);

    len = JpegHdrParse(bp, hdr);

    ReturnErrorIf1 ( len == DVM_JPEG_INVALID_MARKER,
        "%s: error parsing JPEG header: invalid marker", argv[0]);
    ReturnErrorIf1 ( len == DVM_JPEG_AC_UNSUPPORTED,
        "%s: error parsing JPEG header: arithmetic coding is unsupported", 
        argv[0]);
    
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_encode <bitstream> <hdr> <numqt> <baseline>
 * precond 
 *     hdr exists
 * return 
 *     none
 * side effect :
 *     The header is encoded onto the bitstream
 *
 *----------------------------------------------------------------------
 */
int
JpegHdrEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    int status, baseline;
    int len;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s jpegHdr baseLine bitParser", 
        argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &baseline);
    ReturnErrorIf2 (status != TCL_OK,
        "%s: expected integer got %s", argv[0], argv[2]);

    bp = GetBitParser (argv[3]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[3]);

    len = JpegHdrEncode(hdr, baseline, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


int
JpegHdrGetWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetWidth(hdr));
    return TCL_OK;
}

int
JpegHdrSetWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int width;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr width", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &width);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetWidth(hdr, width);
    return TCL_OK;
}

int
JpegHdrGetHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetHeight(hdr));
    return TCL_OK;
}

int
JpegHdrSetHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int height;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr height", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &height);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetHeight(hdr, height);
    return TCL_OK;
}


int
JpegHdrGetNumOfComponentsCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetNumOfComponents(hdr));
    return TCL_OK;
}

int
JpegHdrSetNumOfComponentsCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int numOfComponents;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr numOfComponents", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &numOfComponents);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetNumOfComponents(hdr, numOfComponents);
    return TCL_OK;
}

int
JpegHdrGetPrecisionCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetPrecision(hdr));
    return TCL_OK;
}

int
JpegHdrSetPrecisionCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int precision;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr precision", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &precision);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetPrecision(hdr, precision);
    return TCL_OK;
}


int
JpegHdrGetComponentIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr component", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    sprintf(interp->result, "%d", JpegHdrGetComponentId(hdr, component));
    return TCL_OK;
}

int
JpegHdrSetComponentIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, componentId, component;

    ReturnErrorIf1(argc != 4,
        "wrong # args : %s jpegHdr component componentId", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &componentId);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetComponentId(hdr, component, componentId);
    return TCL_OK;
}


int
JpegHdrGetQtIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int component, status;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr component", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    sprintf(interp->result, "%d", JpegHdrGetQtId(hdr, component));
    return TCL_OK;
}


int
JpegHdrSetQtIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;
    int qtId;
    ReturnErrorIf1(argc != 4,
        "wrong # args : %s jpegHdr component qtId", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &qtId);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetQtId(hdr, component, qtId);
    return TCL_OK;
}


int
JpegHdrGetRestartIntervalCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetRestartInterval(hdr));
    return TCL_OK;
}


int
JpegHdrSetRestartIntervalCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int restartInterval;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr restartInterval", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &restartInterval);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetRestartInterval(hdr, restartInterval);
    return TCL_OK;
}


int
JpegHdrGetMaxBlockHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetMaxBlockHeight(hdr));
    return TCL_OK;
}


int
JpegHdrSetMaxBlockHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int maxBlockHeight;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr maxBlockHeight", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &maxBlockHeight);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetMaxBlockHeight(hdr, maxBlockHeight);
    return TCL_OK;
}


int
JpegHdrGetBlockHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr component", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    sprintf(interp->result, "%d", JpegHdrGetBlockHeight(hdr, component));
    return TCL_OK;
}


int
JpegHdrSetBlockHeightCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;
    int blockHeight;

    ReturnErrorIf1(argc != 4,
        "wrong # args : %s jpegHdr component blockHeight", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &blockHeight);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetBlockHeight(hdr, component, blockHeight);
    return TCL_OK;
}


int
JpegHdrGetBlockWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;

    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr component", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);

    sprintf(interp->result, "%d", JpegHdrGetBlockWidth(hdr, component));
    return TCL_OK;
}


int
JpegHdrSetBlockWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status, component;
    int blockWidth;
    ReturnErrorIf1(argc != 4,
        "wrong # args : %s jpegHdr component blockWidth", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetInt(interp, argv[3], &blockWidth);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetBlockWidth(hdr, component, blockWidth);
    return TCL_OK;
}


int
JpegHdrGetMaxBlockWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args : %s jpegHdr", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegHdrGetMaxBlockWidth(hdr));
    return TCL_OK;
}


int
JpegHdrSetMaxBlockWidthCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int status;
    int maxBlockWidth;
    ReturnErrorIf1(argc != 3,
        "wrong # args : %s jpegHdr maxBlockWidth", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2(hdr == NULL,
        "%s : no such jpeg header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &maxBlockWidth);
    ReturnErrorIf(status != TCL_OK);

    JpegHdrSetMaxBlockWidth(hdr, maxBlockWidth);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_get_qt <hdr> <qtid>
 * return 
 *     A list giving the values in quantization table of the
 *     header <hdr> installed at location qtid, which must be
 *     between 0 and 3.
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrGetQtCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int j, qtId, status;
    char str[32];

    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s jpegHdr qtId", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp,argv[2],&qtId);
    ReturnErrorIf (status != TCL_OK);

    ReturnErrorIf1 (qtId < 0 || qtId > 3,
        "%s: qtId must lie between 0 and 3", argv[0]);

    Tcl_ResetResult (interp);
    for (j=0; j<64; j++) {
        sprintf(str, "%d", hdr->qt[qtId].v[j]);
        Tcl_AppendElement (interp, str);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_set_qt <hdr> <id> <precision> <qtvallist>
 * precond 
 *     JPEG hdr <hdr> exists.
 * return 
 *     The values in quantization table <id> of the
 *     header <hdr> are set to the vals in qtvallist
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrSetQtCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    int id, j, prec, numvals,n;
    char **valstr;
    short vals[64];
    int status;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1( argc != 5,
         "wrong # args: should be %s header id precision qList", argv[0]);

    hdr  = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
         "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &id);
    ReturnErrorIf2( status != TCL_OK,
         "%s: expected int got %s", argv[0], argv[2]);

    status = Tcl_GetInt(interp, argv[3], &prec);
    ReturnErrorIf(status != TCL_OK);
    
    status = Tcl_SplitList(interp, argv[4], &numvals, &valstr);
    ReturnErrorIf1(status != TCL_OK,
        "%s: cannot parse Q table elements", argv[0]);

    if (numvals > 64)
        numvals = 64;
    
    for (j = 0; j < numvals; j++) {
        status = Tcl_GetInt(interp, valstr[j], &n);
        ReturnErrorIf1 (status != TCL_OK,
            "%s: cannot parse Q table elements",argv[0]);
        vals[j] = n;
    }

    for (j=numvals; j<64; j++)
        vals[j] = 1;

    status = JpegHdrSetQt(hdr, id, prec, vals);

    ReturnErrorIf1 (status == DVM_JPEG_INVALID_ID,
        "%s : illegal id value. Valid values are 0, 1, 2 and 3", argv[0]);
    ReturnErrorIf1 (status == DVM_JPEG_INVALID_PRECISION,
        "%s : illegal precision value. Valid values are 8 and 16", argv[0]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_set_blk_widths <hdr> <bwlist>
 * precond 
 *     JPEG hdr <hdr> exists.
 *     The bwlist has as many elements as the number of components
 * side effect :
 *     Sets the block widths and max widths field in the JPEG header
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrSetBlockWidthsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    char **valstr;
    int w[3];
    int status, numvals, i;

    ReturnErrorIf1( argc != 3,
         "wrong # args: should be %s header bwlist", argv[0]);

    hdr  = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
         "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1(status != TCL_OK,
           "%s: cannot parse widths",argv[0]);

    ReturnErrorIf2(numvals != hdr->numComps,
           "%s: must specify %d elements", argv[0], hdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &w[i]);
        ReturnErrorIf(status != TCL_OK);
    }

    JpegHdrSetBlockWidths(hdr, w, numvals);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_set_blk_heights <hdr> <bhlist>
 *
 * precond 
 *     JPEG hdr <hdr> exists.
 *     The bhlist has as many elements as the number of components
 *
 * return 
 *     none
 *
 * side effect :
 *     Sets the block heights and max heights field in the JPEG header
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrSetBlockHeightsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    char **valstr;
    int h[3];
    int status, numvals, i;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1( argc != 3,
         "wrong # args: should be %s header heightList", argv[0]);

    hdr  = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
         "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1(status != TCL_OK,
           "%s: cannot parse heights",argv[0]);

    ReturnErrorIf2(numvals != hdr->numComps,
           "%s: must specify %d elements", argv[0], hdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &h[i]);
        ReturnErrorIf(status != TCL_OK);
    }

    JpegHdrSetBlockHeights(hdr, h, numvals);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_set_qtid <hdr> <qtidlist>
 *
 * precond 
 *     JPEG hdr <hdr> exists.
 *     The qtidlist has as many elements as the number of components
 *
 * return 
 *     none
 *
 * side effect :
 *     Sets the qtid field in the JPEG header
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrSetQtIdsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    char **valstr;
    int qt[3];
    int numvals, status, i, n;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1( argc != 3,
         "wrong # args: should be %s header qtidlist", argv[0]);

    hdr  = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
         "%s: no such JPEG header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1 (status != TCL_OK,
           "%s: cannot parse qtids", argv[0]);

    ReturnErrorIf2 (numvals != hdr->numComps,
           "%s: must specify %d elements", argv[0], hdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &n);
        ReturnErrorIf1(status != TCL_OK,
            "%s: cannot parse qtids", argv[0]);
        qt[i] = n;
    }

    JpegHdrSetQtIds(hdr, qt, numvals);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_standardize_qt <hdr>
 *
 * precond 
 *     JPEG hdr <hdr> 
 *
 * return 
 *     Makes the header <hdr> 
 *     get the default QT tables
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrStdQtInitCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1( argc != 2,
            "wrong # args: should be %s header", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
            "%s: no such JPEG header %s", argv[0], argv[1]);

    /* Copy quantization tables */

    JpegHdrStdQtInit(hdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_std_ht_init <hdr>
 *
 * precond 
 *     JPEG hdr <hdr> 
 *
 * return 
 *     Makes the header <hdr> 
 *     get the default HT tables
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrStdHtInitCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1( argc != 2,
            "wrong # args: should be %s header", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
            "%s: no such JPEG header %s", argv[0], argv[1]);

    /* Copy quantization tables */

    JpegHdrStdHtInit(hdr);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_ht_encode header bp
 * return 
 *     number of bytes encoded
 * side effect :
 *     huffman table gets encoded into bitstream through bp
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrHtEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    BitParser *bp;
    int len;

    ReturnErrorIf1( argc != 3,
            "wrong # args: should be %s header bitparser", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
            "%s: no such JPEG header %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2( bp == NULL,
            "%s: no such bitparser %s", argv[0], argv[2]);

    len = JpegHdrHtEncode(hdr, bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_hdr_qt_encode header bp
 * return 
 *     number of bytes encoded
 * side effect :
 *     quantization table gets encoded into bitstream through bp
 *
 *----------------------------------------------------------------------
 */

int
JpegHdrQtEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHdr *hdr;
    BitParser *bp;
    int len;

    ReturnErrorIf1( argc != 3,
            "wrong # args: should be %s header bitparser", argv[0]);

    hdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2( hdr == NULL,
            "%s: no such JPEG header %s", argv[0], argv[1]);

    bp = GetBitParser(argv[2]);
    ReturnErrorIf2( bp == NULL,
            "%s: no such bitparser %s", argv[0], argv[2]);

    len = JpegHdrQtEncode(hdr, bp);
    sprintf(interp->result, "%d", len);
    return TCL_OK;
}

