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
MpegAudioGraDataNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioGraData *data;

    data = MpegAudioGraDataNew();
    PutMpegAudioGraData(interp, data);
    return TCL_OK;
}

int 
MpegAudioSynDataNewCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioSynData *data;

    data = MpegAudioSynDataNew();
    PutMpegAudioSynData(interp, data);
    return TCL_OK;
}


int 
MpegAudioSynDataFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioSynData *data;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s audioSynData", argv[1]);

    data = RemoveMpegAudioSynData(argv[1]);
    ReturnErrorIf2(data == NULL,
        "%s: no such MPEG audio syn data %s", argv[0], argv[1]);

    MpegAudioSynDataFree(data);
    return TCL_OK;
}

int 
MpegAudioGraDataFreeCmd(cd, interp, argc, argv)
    ClientData cd;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegAudioGraData *data;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s audioGranuleData", argv[1]);

    data = RemoveMpegAudioGraData(argv[1]);
    ReturnErrorIf2(data == NULL,
        "%s: no such MPEG audio granule data %s", argv[0], argv[1]);

    MpegAudioGraDataFree(data);
    return TCL_OK;
}
