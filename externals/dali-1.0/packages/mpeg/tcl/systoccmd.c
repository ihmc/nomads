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
MpegSysTocNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;

    ReturnErrorIf1(argc != 1,
        "wrong # args: should be %s", argv[0]);

    toc = MpegSysTocNew();
    PutMpegSysToc(interp, toc);
    return TCL_OK;
}


int
MpegSysTocAddCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    BitParser *bp;
    int offset, status;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s <bp> <toc> <offset>", argv[0]);

    bp = GetBitParser(argv[1]);
    ReturnErrorIf2(bp == NULL,
        "%s: no such bitparser %s", argv[0], argv[1]);
    toc = GetMpegSysToc(argv[2]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[2]);
    status = Tcl_GetInt(interp, argv[3], &offset);
    ReturnErrorIf(status != TCL_OK);

    MpegSysTocAdd(bp, toc, offset);
    return TCL_OK;
}


int
MpegSysTocGetOffsetCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    int offset, status, id;
    double time;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s <toc> <id> <time>", argv[0]);

    toc = GetMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &id);
    ReturnErrorIf(status != TCL_OK);
    status = Tcl_GetDouble(interp, argv[3], &time);
    ReturnErrorIf(status != TCL_OK);

    offset = MpegSysTocGetOffset(toc, id, time);
    sprintf(interp->result, "%d", offset);

    return TCL_OK;
}

int
MpegSysTocGetFilterCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    int id, status;
    BitStreamFilter *index;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s <toc> <id>", argv[0]);

    toc = GetMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);
    status = Tcl_GetInt(interp, argv[2], &id);
    ReturnErrorIf(status != TCL_OK);

    index = MpegSysTocGetFilter(toc, id);

    ReturnErrorIf2(index == NULL,
        "%s: no such stream %s", argv[0], argv[2]);

    PutBitStreamFilter(interp, index);

    return TCL_OK;
}

int
MpegSysTocListFiltersCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    int i;
    char str[32];

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s <toc>", argv[0]);

    toc = GetMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);

    Tcl_ResetResult(interp);
    for (i = 0; i < 48; i++) {
        if ((toc->streamInfo[i] != NULL) &&
            (toc->streamInfo[i]->numOfPacket > 0)) {
            sprintf(str, "%d", i);
            Tcl_AppendElement(interp, str);
        }
    }

    return TCL_OK;
}


int
MpegSysTocFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s toc", argv[0]);

    toc = RemoveMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);

    MpegSysTocFree(toc);

    return TCL_OK;
}

int
MpegSysTocWriteCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    char *fileName;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s toc fileName", argv[0]);

    toc = GetMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);

    fileName = argv[2];
    status = MpegSysTocWrite(toc, fileName);

    ReturnErrorIf2(status == 0,
        "%s: no such file %s", argv[0], fileName);

    return TCL_OK;
}

int
MpegSysTocReadCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    MpegSysToc *toc;
    char *fileName;
    int status;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s toc fileName", argv[0]);

    toc = GetMpegSysToc(argv[1]);
    ReturnErrorIf2(toc == NULL,
        "%s: no such mpeg system toc %s", argv[0], argv[1]);

    fileName = argv[2];
    status = MpegSysTocRead(toc, fileName);

    ReturnErrorIf2(status == 0,
        "%s: no such file %s", argv[0], fileName);

    return TCL_OK;
}
