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
 * wavehdrcmd.c --
 *
 *      Tcl command to Wave Driver
 *
 */

#include "tclDvmWave.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_new
 *
 * precond 
 *     none
 *
 * return 
 *     a new WaveHdr structure with initialized contents
 * 
 * side effect :
 *     memory is allocated for a new wave header
 *
 *----------------------------------------------------------------------
 */
int WaveHdrNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
        WaveHdr *hdr;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);
        
    /*
     * Allocate and initialize the new header.
     */
    hdr = WaveHdrNew();
    ReturnErrorIf1(hdr == NULL,
        "%s: error while allocating memory for header", argv[0]);
    PutWaveHdr(interp, hdr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_parse <bitparser> <hdr>
 *
 * precond 
 *     bitparser is at the beginning of the wave file bitstream
 *
 * return 
 *     none
 * 
 * side effect :
 *     memory is allocated for a new wave header
 *     The next call should be to either wave_read or wave_read_some.
 *     <hdr> is stored with the contents parsed from the bitparser
 *
 *----------------------------------------------------------------------
 */
int WaveHdrParseCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    BitParser *bp;
    WaveHdr *hdr;
    int bytesRead;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <bitparser> <hdr>", argv[0]);
        bp = GetBitParser(argv[1]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    hdr = GetWaveHdr(argv[2]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[2]);
        
    /*
     * Allocate and initialize the new header.
     */
    bytesRead = WaveHdrParse(bp, hdr);
    ReturnErrorIf1 (!bytesRead,
        "%s: invalid wave file", argv[0]);
    sprintf(interp->result, "%d", bytesRead);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_free <hdr>
 *
 * precond 
 *     <hdr> exists
 *
 * return 
 *     none
 *
 * side effect :
 *     the memory allocated for the wave header is free'd
 *
 *----------------------------------------------------------------------
 */
int WaveHdrFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
        WaveHdr *hdr;

    ReturnErrorIf1(argc != 2,
        "wrong # args. should be %s <hdr>", argv[0]);
        
    hdr = RemoveWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    WaveHdrFree (hdr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_encode <hdr> <bitparser>
 *
 * precond 
 *     <hdr> is a valid wave header
 *
 * return 
 *     none
 * 
 * side effect :
 *     the bitstream referenced by <bitparser> is written with the 
 *     contents in <hdr> in canonical wave file format
 *
 *----------------------------------------------------------------------
 */
int WaveHdrEncodeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
        BitParser *bp;
        WaveHdr *hdr;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <hdr> <parse>", argv[0]);
    hdr = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
        bp = GetBitParser(argv[2]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[2]);
        
    /*
     * Allocate and initialize the new header.
     */
    sprintf(interp->result, "%d", WaveHdrEncode(hdr, bp));
        
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_format <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     format code of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetFormatCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->format);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_numofchan <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     number of channels (1 or 2) of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetNumOfChanCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->numOfChan);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_samplespersec <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     sampling rate (samples per second) of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetSamplesPerSecCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->samplesPerSec);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_bytespersec <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     bytes per second of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetBytesPerSecCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->bytesPerSec);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_blockalign <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     block alignment of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetBlockAlignCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->blockAlign);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_bitspersample <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     data length of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetBitsPerSampleCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->bitsPerSample);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_datalen <hdr>
 *
 * precond 
 *     <hdr> exists.
 *
 * return 
 *     data length of the wave header <hdr>
 *
 * side effect :
 *     None
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrGetDataLenCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s hdr", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);

    sprintf(interp->result, "%d", hdr->dataLen);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_format <hdr> <format>
 *
 * precond 
 *     <hdr> exists <format> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set format of the wave header <hdr> to be <format>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetFormatCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int format, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr format", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &format);
    ReturnErrorIf(status != TCL_OK);
    hdr->format = format;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_numofchan <hdr> <numofchan>
 *
 * precond 
 *     <hdr> exists <numofchan> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set numOfChan of the wave header <hdr> to be <numofchan>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetNumOfChanCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int numOfChan, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr nchan", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &numOfChan);
    ReturnErrorIf(status != TCL_OK);
    hdr->numOfChan = numOfChan;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_samplespersec <hdr> <samplespersec>
 *
 * precond 
 *     <hdr> exists <samplespersec> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set samplesPerSec of the wave header <hdr> to be <samplespersec>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetSamplesPerSecCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int samplesPerSec, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr format", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &samplesPerSec);
    ReturnErrorIf(status != TCL_OK);
    hdr->samplesPerSec = samplesPerSec;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_bytespersec <hdr> <bytespersec>
 *
 * precond 
 *     <hdr> exists <bytespersec> and integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set bytesPerSec of the wave header <hdr> to be <bytespersec>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetBytesPerSecCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int bytesPerSec, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr bytespersec", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &bytesPerSec);
    ReturnErrorIf(status != TCL_OK);
    hdr->bytesPerSec = bytesPerSec;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_blockalign <hdr> <blockalign>
 *
 * precond 
 *     <hdr> exists <blockalign> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set blockAlign of the wave header <hdr> to be <blockalign>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetBlockAlignCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int blockAlign, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr blockalign", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &blockAlign);
    ReturnErrorIf(status != TCL_OK);
    hdr->blockAlign = blockAlign;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_bitspersample <hdr> <bitspersample>
 *
 * precond 
 *     <hdr> exists <bitspersample> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set bitsPerSample of the wave header <hdr> to be <bitspersample>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetBitsPerSampleCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int bitsPerSample, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr bitspersample", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &bitsPerSample);
    ReturnErrorIf(status != TCL_OK);
    hdr->bitsPerSample = bitsPerSample;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_set_datalen <hdr> <datalen>
 *
 * precond 
 *     <hdr> exists <datalen> an integer
 *
 * return 
 *     none
 *
 * side effect :
 *     set dataLen of the wave header <hdr> to be <datalen>
 *
 *----------------------------------------------------------------------
 */

int
WaveHdrSetDataLenCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    int dataLen, status;

    /*
     * Check args, retrieve wave header from hash table
     */
    ReturnErrorIf1 (argc != 3,
        "wrong # args: should be %s hdr datalen", argv[0]);

    hdr  = GetWaveHdr(argv[1]);
    ReturnErrorIf2 (hdr == NULL,
        "%s: no such wave header %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &dataLen);
    ReturnErrorIf(status != TCL_OK);
    hdr->dataLen = dataLen;

    return TCL_OK;
}
