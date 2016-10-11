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
 * aviutil.c
 *
 * misc utility functions 
 *
 *----------------------------------------------------------------------
 */

#include "tclAviInt.h"

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_get_num_of_codecs 
 * precond 
 *     none
 * return 
 *     a list of vfw compatible codecs that the system supports. Any
 *     of these can be used as an argument to avi_video_start_encode
 *     or avi_audio_start_encode
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
AviGetNumOfCodecsCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ReturnErrorIf1 (argc != 1, "wrong # args: should be %s", argv[0]);
    sprintf (interp->result, "%d", AviGetNumOfCodecs());
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     avi_codec_info <codecNum>
 * precond 
 *     none
 *      
 * return 
 *     The descriptor string for the codec
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
AviCodecInfoCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    AviCodec codecInfo;
    int codecNum;
    int error;
    char tmp[32];

    ReturnErrorIf1 (argc != 2, "wrong # args: should be %s <codecNumber>",
                    argv[0]);
    ReturnErrorIf2 (Tcl_GetInt(interp, argv[1], &codecNum) != TCL_OK,
        "%s: expected int got %s", argv[0], argv[1]);

    error = AviCodecInfo(&codecInfo, codecNum);
    ReturnErrorIf2 (error, "%s -- invalid codec number %d", argv[0], codecNum);

    Tcl_ResetResult (interp);

    /*
     * Type
     */
    Tcl_AppendElement (interp, "-type");
    if (codecInfo.video) {
        char str[5];
        Tcl_AppendElement (interp, "video");
        Tcl_AppendElement (interp, "-fcc");
        strncpy (str, (char *)(&codecInfo.id), 4);
        str[4] = 0;
        Tcl_AppendElement (interp, str);
    } else {
        Tcl_AppendElement (interp, "audio");
    }

    /*
     * Version
     */
    sprintf(tmp, "%d ", codecInfo.version);
    Tcl_AppendElement (interp, "-version");
    Tcl_AppendElement (interp, tmp);

    /*
     * Name
     */
    Tcl_AppendElement (interp, "-name");
    Tcl_AppendElement (interp, codecInfo.name);

    /*
     * Decsription
     */
    Tcl_AppendElement (interp, "-description");
    Tcl_AppendElement (interp, codecInfo.description);

    return TCL_OK;
}
