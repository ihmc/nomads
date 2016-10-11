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
 * audioconvcmd.c
 *
 * tcl interface to audioconv.c
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audioconv_new codec freq bps nc
 * precond 
 *     codec has to one that we recognize
 * return 
 *     Handle to a audioconv structure
 * 
 * side effect :
 *     Opens acm driver and acm conversion stream
 *
 *----------------------------------------------------------------------
 */

int
AudioConvNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    int freq, bps, nc;
    int codec;
    AudioConv *ac;

    ReturnErrorIf1 (argc != 5, 
                    "%s: wrong # args: should be %s <codec> <freq> <bps> <nc>",
                    argv[0]);

    /* Add codec names here */
    if (strcmp(argv[1], "truespeech") == 0) {
        codec = WAVE_FORMAT_DSPGROUP_TRUESPEECH;
    } else {
        sprintf(interp->result,"%s: no such codec %s", argv[0], argv[1]);
        return TCL_ERROR;
    }

    ReturnErrorIf2 (Tcl_GetInt(interp, argv[2], &freq) != TCL_OK,
                    "%s: expected int got %s", argv[0], argv[2]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[3], &bps) != TCL_OK,
                    "%s: expected int got %s", argv[0], argv[3]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[4], &nc) != TCL_OK,
                    "%s: expected int got %s", argv[0], argv[4]);

    ac = AudioConvNew(codec, freq, (short)bps, (short)nc);
    ReturnErrorIf1 (ac == NULL,
                    "%s: Couldn't open conversion stream",
                    argv[0]);
    PutAudioConv(interp, ac);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audioconv_free <ac>
 * precond 
 *     The audioconv handle <ac> exists
 * return 
 *     none
 * 
 * side effect :
 *     The audioconv data structure and additional allocated acm
 *     handles are freed.
 *----------------------------------------------------------------------
 */

int
AudioConvFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioConv *ac;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s <audioconv>", argv[0]);

    ac = RemoveAudioConv(argv[1]);
    ReturnErrorIf2 (ac == NULL,
        "%s: no such audioconv %s", argv[0], argv[1]);

    AudioConvFree (ac);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     audioconv_encode <ac> <audiobuf> <bitparser>
 * precond 
 *     The audioconv handle <ac> exists, <audiobuf> is a valid audio 
 *     buffer and <bitparser> has enough size left to accommodate
 *     the written data.
 * return 
 *     The number of source samples actually encoded
 * 
 * side effect :
 *     The audioconv data structure and additional allocated acm
 *     handles are freed.
 *----------------------------------------------------------------------
 */

int
AudioConvEncodeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AudioConv *ac;
    Audio *aud;
    BitParser *bp;
    int status, numused;

    /* Get args, retrieve structures from handles */
    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s <audioconv> <audiobuf> <bp>", argv[0]);

    ac = GetAudioConv(argv[1]);
    ReturnErrorIf2 (ac == NULL,
        "%s: no such audioconv %s", argv[0], argv[1]);

    aud = GetAudio(argv[2]);
    ReturnErrorIf2 (aud == NULL,
        "%s: no such audio buffer %s", argv[0], argv[2]);

    bp = GetBitParser(argv[3]);
    ReturnErrorIf2 (bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[3]);
    
    status = AudioConvEncode(ac, aud, bp, &numused);
    if (status) {
        sprintf(interp->result,
                "%s: Error encoding data -- %s", argv[0], AviTranslateError(status));
        return TCL_ERROR;
    } else {
        sprintf(interp->result, "%d", numused);
        return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     wave_hdr_new_from_audioconv <ac>
 *
 * precond 
 *     audioconv handle <ac> exists
 *
 * return 
 *     a new WaveHdr structure with contents from <ac> output
 * 
 * side effect :
 *     memory is allocated for a new wave header
 *
 *----------------------------------------------------------------------
 */
int WaveHdrNewFromAudioConvCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    WaveHdr *hdr;
    AudioConv *ac;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <audioconv>", argv[0]);

    ac = GetAudioConv(argv[1]);
    ReturnErrorIf2 (ac == NULL,
        "%s: no such audioconv %s", argv[0], argv[1]);
        
    /*
     * Allocate and initialize the new header.
     */
    hdr = WaveHdrNewFromAudioConv(ac);
    ReturnErrorIf1(hdr == NULL,
        "%s: error while allocating memory for header", argv[0]);
    PutWaveHdr(interp, hdr);

    return TCL_OK;
}
