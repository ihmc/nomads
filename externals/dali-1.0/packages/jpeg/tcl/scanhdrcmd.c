/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *------------------------------------------------------------------------
 */
/*
 *----------------------------------------------------------------------
 *
 * scanhdrcmd.c
 *
 * This file contains tcl-hooks to scan header commands
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmJpeg.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_new 
 * precond 
 *     allocate a new JPEG scan header
 * return 
 *     a handle to the new JPEG scan header
 * side effect :
 *     memory is allocated for a new JPEG scan header
 *
 *----------------------------------------------------------------------
 */
int
JpegScanHdrNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *shdr;

    ReturnErrorIf1 (argc != 1,
        "wrong # args: should be %s", argv[0]);
    shdr = JpegScanHdrNew();
    PutJpegScanHdr(interp, shdr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_parse <bitstream> <hdr> <scanhdr>
 * precond 
 *     bitstream is at the beginning of a JPEG scanhdr
 * return 
 *     none
 * side effect :
 *     JPEG scan hdr is parsed off the bitstream
 *
 *----------------------------------------------------------------------
 */
int
JpegScanHdrParseCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    int len;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s bitParser hdr scanHdr", argv[0]);

    bp = GetBitParser (argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[1]);
    hdr = GetJpegHdr (argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[2]);
    shdr = GetJpegScanHdr (argv[3]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[3]);

    len = JpegScanHdrParse(bp, hdr, shdr);
    ReturnErrorIf1 ( len == DVM_JPEG_INVALID_MARKER,
        "%s: error parsing JPEG scan: invalid marker", argv[0]);
    ReturnErrorIf1 ( len == DVM_JPEG_AC_UNSUPPORTED,
        "%s: error parsing JPEG scan: arithmetic coding is unsupported", 
        argv[0]);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_free <shdr>
 * precond 
 *      <shdr> exists
 * side effect :
 *     the memory allocated for the JPEG scan hdr <shdr> is free'd.
 *
 *----------------------------------------------------------------------
 */
int
JpegScanHdrFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *shdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s scanHdr", argv[0]);

    shdr = RemoveJpegScanHdr(argv[1]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[1]);

    JpegScanHdrFree (shdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_encode bitstream hdr scanhdr
 * side effect :
 *     JPEG scan hdr is encoded onto the bitstream
 *
 *----------------------------------------------------------------------
 */
int
JpegScanHdrEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    JpegHdr *hdr;
    JpegScanHdr *shdr;
    int len;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s hdr scanHdr bitParser", argv[0]);

    hdr = GetJpegHdr (argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    shdr = GetJpegScanHdr (argv[2]);
    ReturnErrorIf2 (shdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    bp = GetBitParser (argv[3]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitstream %s", argv[0], argv[3]);
    len = JpegScanHdrEncode(hdr, shdr, bp);
    sprintf(interp->result, "%d", len);

    return TCL_OK;
}

int
JpegScanHdrGetNumOfComponentsCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;

    ReturnErrorIf1 (argc != 2,
        "wrong # args : %s scanHdr", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", JpegScanHdrGetNumOfComponents(hdr));
    return TCL_OK;
}

int
JpegScanHdrSetNumOfComponentsCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int status;
    int numOfComponents;
    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s scanHdr numOfComponents", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &numOfComponents);
    ReturnErrorIf (status != TCL_OK);

    JpegScanHdrSetNumOfComponents(hdr, numOfComponents);
    return TCL_OK;
}

int
JpegScanHdrGetScanIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int component, status;

    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s scanHdr component", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    sprintf(interp->result, "%d", JpegScanHdrGetScanId(hdr, component));
    return TCL_OK;
}

int
JpegScanHdrSetScanIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int status, scanId, component;

    ReturnErrorIf1 (argc != 4,
        "wrong # args : %s scanHdr component scanId", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &scanId);
    ReturnErrorIf (status != TCL_OK);

    JpegScanHdrSetScanId(hdr, component, scanId);
    return TCL_OK;
}

int
JpegScanHdrGetDcIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int component, status;

    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s scanHdr component", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    sprintf(interp->result, "%d", JpegScanHdrGetDcId(hdr,component));
    return TCL_OK;
}

int
JpegScanHdrSetDcIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int status, dcId, component;

    ReturnErrorIf1 (argc != 4,
        "wrong # args : %s scanHdr dcId", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &dcId);
    ReturnErrorIf (status != TCL_OK);

    JpegScanHdrSetDcId(hdr, component, dcId);
    return TCL_OK;
}

int
JpegScanHdrGetAcIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int component, status;

    ReturnErrorIf1 (argc != 3,
        "wrong # args : %s scanHdr component", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    sprintf(interp->result, "%d", JpegScanHdrGetAcId(hdr, component));
    return TCL_OK;
}

int
JpegScanHdrSetAcIdCmd (cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *hdr;
    int status, component, acId;

    ReturnErrorIf1 (argc != 4,
        "wrong # args : %s scanHdr component acId", argv[0]);

    hdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s : no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &component);
    ReturnErrorIf (status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &acId);
    ReturnErrorIf (status != TCL_OK);

    JpegScanHdrSetAcId(hdr, component, acId);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_set_scan_ids <scanhdr> <scanidlist>
 *
 * precond 
 *     JPEG scan hdr <scanhdr> exists.
 *     The scanidlist has as many elements as the number of components in
 *     the scan header
 *
 * return 
 *     none
 *
 * side effect :
 *     Sets the scanid field in the JPEG scan header
 *
 *----------------------------------------------------------------------
 */

int
JpegScanHdrSetScanIdsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *shdr;
    char **valstr;
    int status,numvals,i,n;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1 (argc != 3,
         "wrong # args: should be %s scanHdr scanIdList", argv[0]);

    shdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (shdr == NULL,
         "%s: no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1 (status != TCL_OK,
           "%s: cannot parse scan id list",argv[0]);

    ReturnErrorIf2 (numvals != shdr->numComps,
           "%s: must specify %d elements", argv[0], shdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &n);
        ReturnErrorIf1 (status != TCL_OK,
            "%s: cannot parse compids",argv[0]);
        shdr->scanid[i] = n;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_set_dc_ids <scanhdr> <dcidlist>
 *
 * precond 
 *     JPEG scan hdr <scanhdr> exists.
 *     The dcidlist has as many elements as the number of components in
 *     the scan header
 *
 * return 
 *     none
 *
 * side effect :
 *     Sets the dcid field in the JPEG scan header
 *
 *----------------------------------------------------------------------
 */

int
JpegScanHdrSetDcIdsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *shdr;
    char **valstr;
    int status,numvals,i,n;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1 (argc != 3,
         "wrong # args: should be %s scanHdr dcIdList", argv[0]);

    shdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (shdr == NULL,
         "%s: no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1 (status != TCL_OK,
           "%s: cannot parse dc ids",argv[0]);

    ReturnErrorIf2 (numvals != shdr->numComps,
           "%s: must specify %d elements", argv[0], shdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &n);
        ReturnErrorIf1 (status != TCL_OK,
            "%s: cannot parse dc ids",argv[0]);
        shdr->dcid[i] = n;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_scan_hdr_set_ac_ids <scanhdr> <acidlist>
 *
 * precond 
 *     JPEG scan hdr <scanhdr> exists.
 *     The acidlist has as many elements as the number of components in
 *     the scan header
 *
 * return 
 *     none
 *
 * side effect :
 *     Sets the acid field in the JPEG scan header
 *
 *----------------------------------------------------------------------
 */

int
JpegScanHdrSetAcIdsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegScanHdr *shdr;
    char **valstr;
    int status,numvals,i,n;

    /*
     * Check args, retrieve image header from hash table
     */

    ReturnErrorIf1 (argc != 3,
         "wrong # args: should be %s scanHdr acIdList", argv[0]);

    shdr = GetJpegScanHdr(argv[1]);
    ReturnErrorIf2 (shdr == NULL,
         "%s: no such JPEG scan header %s", argv[0], argv[1]);

    status = Tcl_SplitList(interp, argv[2], &numvals, &valstr);
    ReturnErrorIf1 (status != TCL_OK,
           "%s: cannot parse AC ids",argv[0]);

    ReturnErrorIf2 (numvals != shdr->numComps,
           "%s: must specify %d elements", argv[0], shdr->numComps);
    
    for (i = 0; i < numvals; i++) {
        status = Tcl_GetInt(interp, valstr[i], &n);
        ReturnErrorIf1 (status != TCL_OK,
            "%s: cannot parse AC ids",argv[0]);
        shdr->acid[i] = n;
    }
    return TCL_OK;
}
