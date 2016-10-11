/*------------------------------------------------------------------------
 *
 * Copyright (c) 1997-1998 by Cornell University.
 * 
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * sccmd.c
 *
 * Tcl hooks to sc image functions. 
 *
 * weitsang Nov 97
 *
 *----------------------------------------------------------------------
 */

#include "tclDvmBasic.h"

int
ScNewCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *new;
    int status, w, h;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s width height", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[2], &h);
    ReturnErrorIf(status != TCL_OK);

    new = ScNew(w, h);
    PutScImage(interp, new);

    return TCL_OK;
}

int
ScFreeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *img;

    ReturnErrorIf1(argc != 2,
        "wrong # args : should be %s scImage", argv[0]);

    img = GetScImage(argv[1]);
    ReturnErrorIf2(img == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    ScFree(img);
    return TCL_OK;
}

int
ScClipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *buf, *new;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s scImage x y width height", argv[0]);

    buf = GetScImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    new = ScClip(buf, x, y, w, h);
    PutScImage(interp, new);

    return TCL_OK;
}

int
ScReclipCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *buf, *clipped;
    int x, y, w, h, status;

    ReturnErrorIf1(argc != 7,
        "wrong # args : should be %s scImage x y width height clipped", argv[0]);

    buf = GetScImage(argv[1]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    status = Tcl_GetInt(interp, argv[2], &x);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[3], &y);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[4], &w);
    ReturnErrorIf(status != TCL_OK);

    status = Tcl_GetInt(interp, argv[5], &h);
    ReturnErrorIf(status != TCL_OK);

    clipped = GetScImage(argv[6]);
    ReturnErrorIf2(buf == NULL,
        "%s : no such sc image %s\n", argv[0], argv[6]);

    ScReclip(buf, x, y, w, h, clipped);

    return TCL_OK;
}

int
ScIToByteCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;
    ByteImage *byte;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s scImage byteImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    byte = GetByteImage(argv[2]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[2]);

    ScIToByte(sc, byte);

    return TCL_OK;

}


int
ScPToYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;
    VectorImage *mv;
    ByteImage *prev;
    ByteImage *byte;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s scImage v prev dest", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    mv = GetVectorImage(argv[2]);
    ReturnErrorIf2(mv == NULL,
        "%s : no such vector image %s\n", argv[0], argv[2]);

    prev = GetByteImage(argv[3]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[3]);

    byte = GetByteImage(argv[4]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[4]);

    ScPToY(sc, mv, prev, byte);

    return TCL_OK;

}

int
ScPToUVCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;
    VectorImage *mv;
    ByteImage *prev;
    ByteImage *byte;

    ReturnErrorIf1(argc != 5,
        "wrong # args: should be %s scImage v prev dest", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    mv = GetVectorImage(argv[2]);
    ReturnErrorIf2(mv == NULL,
        "%s : no such mv image %s\n", argv[0], argv[2]);

    prev = GetByteImage(argv[3]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[3]);

    byte = GetByteImage(argv[4]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[4]);

    ScPToUV(sc, mv, prev, byte);

    return TCL_OK;

}


int
ScBToYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;
    VectorImage *fwdmv;
    VectorImage *bwdmv;
    ByteImage *future;
    ByteImage *prev;
    ByteImage *byte;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s scImage fwd bwd past future dest", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    fwdmv = GetVectorImage(argv[2]);
    ReturnErrorIf2(fwdmv == NULL,
        "%s : no such mv image %s\n", argv[0], argv[2]);

    bwdmv = GetVectorImage(argv[3]);
    ReturnErrorIf2(bwdmv == NULL,
        "%s : no such mv image %s\n", argv[0], argv[3]);

    prev = GetByteImage(argv[4]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[4]);

    future = GetByteImage(argv[5]);
    ReturnErrorIf2(future == NULL,
        "%s : no such byte image %s\n", argv[0], argv[5]);

    byte = GetByteImage(argv[6]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[6]);

    ScBToY(sc, fwdmv, bwdmv, prev, future, byte);

    return TCL_OK;

}


int
ScBToUVCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;
    VectorImage *fwdmv;
    VectorImage *bwdmv;
    ByteImage *future;
    ByteImage *prev;
    ByteImage *byte;

    ReturnErrorIf1(argc != 7,
        "wrong # args: should be %s scImg fwdVectorImg bwdVectorImg pastByteImg futureByteImg outByteImg", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    fwdmv = GetVectorImage(argv[2]);
    ReturnErrorIf2(fwdmv == NULL,
        "%s : no such mv image %s\n", argv[0], argv[2]);

    bwdmv = GetVectorImage(argv[3]);
    ReturnErrorIf2(bwdmv == NULL,
        "%s : no such mv image %s\n", argv[0], argv[3]);

    prev = GetByteImage(argv[4]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[4]);

    future = GetByteImage(argv[5]);
    ReturnErrorIf2(future == NULL,
        "%s : no such byte image %s\n", argv[0], argv[5]);

    byte = GetByteImage(argv[6]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[6]);

    ScBToUV(sc, fwdmv, bwdmv, prev, future, byte);

    return TCL_OK;

}


int
ScCopyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s src dest", argv[0]);

    src = GetScImage(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    dest = GetScImage(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such sc image %s\n", argv[0], argv[2]);

    ScCopy(src, dest);
    return TCL_OK;
}


int
ScCopyDcAcCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *src, *dest;

    ReturnErrorIf1(argc != 3,
        "wrong # args: should be %s src dest", argv[0]);

    src = GetScImage(argv[1]);
    ReturnErrorIf2(src == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    dest = GetScImage(argv[2]);
    ReturnErrorIf2(dest == NULL,
        "%s : no such sc image %s\n", argv[0], argv[2]);

    ScCopyDcAc(src, dest);
    return TCL_OK;
}


int
ScGetXCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", ScGetX(sc));
    return TCL_OK;
}


int
ScGetYCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", ScGetY(sc));
    return TCL_OK;
}


int
ScGetWidthCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", ScGetWidth(sc));
    return TCL_OK;
}


int
ScGetHeightCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", ScGetHeight(sc));
    return TCL_OK;
}


int
ScGetVirtualCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d", ScGetVirtual(sc));
    return TCL_OK;
}


int
ScInfoCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *sc;

    ReturnErrorIf1(argc != 2,
        "wrong # args: should be %s scImage", argv[0]);

    sc = GetScImage(argv[1]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    sprintf(interp->result, "%d %d %d %d %d", sc->x, sc->y, sc->width,
        sc->height, sc->isVirtual);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_to_sc <scImage> <byteImage>
 * precond 
 *     Both buffers exist
 * return 
 *     none
 * side effect :
 *     Converts <byteImage> to <scImage> without quantization.
 *
 *----------------------------------------------------------------------
 */

int
ByteToScCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *byte;
    ScImage *sc;

    ReturnErrorIf1(argc != 3,
        "wrong # args : should be %s byteImage scImage", argv[0]);

    byte = GetByteImage(argv[1]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such byte image %s\n", argv[0], argv[1]);

    sc = GetScImage(argv[2]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[2]);

    ByteToSc(byte, sc);
    return TCL_OK;
}



/*
 * Parse a Q Table passed as a Tcl list with 64 elements.  if the
 * list is null, use the default Quant Table
 */
static int
ParseQTable(interp, str, qTable)
    Tcl_Interp *interp;         /* For error reporting */
    char *str;                  /* String to parse -- should be ... or list of 64 ints */
    int **qTable;               /* Target -- result is stored here */
{
    int argc, i, status;
    char **argv;

    if (strcmp(str, "mpeg-intra") == 0) {
        *qTable = MPEG_INTRA;
    } else if (strcmp(str, "mpeg-non-intra") == 0) {
        *qTable = MPEG_NON_INTRA;
    } else if (strcmp(str, "jpeg-y") == 0) {
        *qTable = JPEG_LUM;
    } else if (strcmp(str, "jpeg-uv") == 0) {
        *qTable = JPEG_CHROM;
    } else {
        status = Tcl_SplitList(interp, str, &argc, &argv);
        ReturnErrorIf(status != TCL_OK);
        if (argc == 64) {
            for (i = 0; i < 64; i++) {
                status = Tcl_GetInt(interp, argv[i], (*qTable) + i);
                ReturnErrorIf(status != TCL_OK);
            }
        } else {
            sprintf(interp->result,
                "Error parsing quantization table -- expected list of 64 integers");
            return TCL_ERROR;
        }
        FREE((char *) argv);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_to_sc_i byteImage qScale qTable scImage
 * precond 
 *     Both buffers exist
 * return 
 *     none
 * side effect :
 *     Converts a color ByteImage (represented by <yImage> <uImage> <vImage>) to a
 *     color ScImage, quantizing using the standard MPEG quantization tables scaled
 *     by qScale.
 *
 *----------------------------------------------------------------------
 */

int
ByteToScICmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *byte, *qScale;
    ScImage *sc;
    int *qTable;
    int status;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s byteImage qTable qScale scImage", argv[0]);

    byte = GetByteImage(argv[1]);
    ReturnErrorIf2(byte == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[1]);

    qScale = GetByteImage(argv[2]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    status = ParseQTable(interp, argv[3], &qTable);
    ReturnErrorIf(status != TCL_OK);

    sc = GetScImage(argv[4]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such ScImage %s\n", argv[0], argv[4]);

    ByteToScI(byte, qScale, qTable, sc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_y_to_sc_p <currImage> <prevImage> <scImage> <vecImage> <qScale>
 *
 * precond 
 *     curr and prev images contain meaningful data.
 *
 * return 
 *     none
 *
 * side effect :
 *     Calculates the motion vectors and residual for the luminance component
 *     of an MPEG P-frame, quantizing using the standard MPEG quantization tables 
 *     scaled by <qScale>.
 *
 *----------------------------------------------------------------------
 */

int
ByteYToScPCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *curr, *prev, *qScale;
    ScImage *sc;
    VectorImage *fmv;
    int status;
    int *qTable, *niqTable;

    ReturnErrorIf1(argc != 8,
        "wrong # args : should be %s currImage prevImage vecImage qScale qTable niqTable scImage", argv[0]);

    curr = GetByteImage(argv[1]);
    ReturnErrorIf2(curr == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[1]);

    prev = GetByteImage(argv[2]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    fmv = GetVectorImage(argv[3]);
    ReturnErrorIf2(fmv == NULL,
        "%s : no such VectorImage %s\n", argv[0], argv[3]);

    qScale = GetByteImage(argv[4]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[4]);

    status = ParseQTable(interp, argv[5], &qTable);
    ReturnErrorIf(status != TCL_OK);

    status = ParseQTable(interp, argv[6], &niqTable);
    ReturnErrorIf(status != TCL_OK);

    sc = GetScImage(argv[7]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such ScImage %s\n", argv[0], argv[7]);

    ByteYToScP(curr, prev, fmv, qScale, qTable, niqTable, sc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_uv_to_sc_p <currImage> <prevImage> <scImage> <vecImage> <qScale>
 *
 * precond 
 *     curr and prev images contain meaningful data.
 *
 * return 
 *     none
 *
 * side effect :
 *     Calculates the residual for one chrominance component of an
 *     MPEG P-frame, quantizing using the standard MPEG quantization tables 
 *     scaled by <qScale>.
 *
 *----------------------------------------------------------------------
 */

int
ByteUVToScPCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *curr, *prev, *qScale;
    ScImage *sc;
    VectorImage *fmv;
    int status;
    int *qTable, *niqTable;

    ReturnErrorIf1(argc != 8,
        "wrong # args : should be %s currImage prevImage vecImage qScale qTable niqTable scImage", argv[0]);

    curr = GetByteImage(argv[1]);
    ReturnErrorIf2(curr == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[1]);

    prev = GetByteImage(argv[2]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    fmv = GetVectorImage(argv[3]);
    ReturnErrorIf2(fmv == NULL,
        "%s : no such VectorImage %s\n", argv[0], argv[3]);

    qScale = GetByteImage(argv[4]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[4]);

    status = ParseQTable(interp, argv[5], &qTable);
    ReturnErrorIf(status != TCL_OK);

    status = ParseQTable(interp, argv[6], &niqTable);
    ReturnErrorIf(status != TCL_OK);

    sc = GetScImage(argv[7]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such ScImage %s\n", argv[0], argv[7]);

    ByteUVToScP(curr, prev, fmv, qScale, qTable, niqTable, sc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_y_to_sc_b <currImage> <prevImage> <nextImage> <scImage>
 *                    <fwdVecImage> <backVecImage> <qScale>
 *
 * precond 
 *     curr, prev, and next buffers contain useful data
 *
 * return 
 *     none
 *
 * side effect :
 *     Calculates the forward/backward motion vectors and the residual for
 *     the luminance component of an MPEG B-frame, quantizing using the
 *     standard MPEG quatization tables scaled by <qScale>.
 *
 *----------------------------------------------------------------------
 */

int
ByteYToScBCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *curr, *prev, *next, *qScale;
    ScImage *sc;
    VectorImage *fmv, *bmv;
    int status;
    int *qTable, *niqTable;

    ReturnErrorIf1(argc != 10,
        "wrong # args : should be %s byteImage past future fmv bmv qScale qTable niqTable scImage",
        argv[0]);

    curr = GetByteImage(argv[1]);
    ReturnErrorIf2(curr == NULL,
        "%s : no such byte image %s\n", argv[0], argv[1]);

    prev = GetByteImage(argv[2]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[2]);

    next = GetByteImage(argv[3]);
    ReturnErrorIf2(next == NULL,
        "%s : no such byte image %s\n", argv[0], argv[3]);

    fmv = GetVectorImage(argv[4]);
    ReturnErrorIf2(fmv == NULL,
        "%s : no such vector image %s\n", argv[0], argv[4]);

    bmv = GetVectorImage(argv[5]);
    ReturnErrorIf2(bmv == NULL,
        "%s : no such vector image %s\n", argv[0], argv[5]);

    qScale = GetByteImage(argv[6]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[6]);

    status = ParseQTable(interp, argv[7], &qTable);
    ReturnErrorIf(status != TCL_OK);

    status = ParseQTable(interp, argv[8], &niqTable);
    ReturnErrorIf(status != TCL_OK);

    sc = GetScImage(argv[9]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[9]);

    ByteYToScB(curr, prev, next, fmv, bmv, qScale, qTable, niqTable, sc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     byte_uv_to_sc_b byteImage past future fmv bmv qTable qScale scImage
 *
 * precond 
 *     yteImage, past, and future buffers contain useful data, fmv & bmv motion
 *     vectors are initialized.
 *
 * return 
 *     none
 *
 * side effect:
 *     Calculates the residual for one chrominance component of an MPEG
 *     B-frame
 *
 *----------------------------------------------------------------------
 */

int
ByteUVToScBCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ByteImage *curr, *prev, *next, *qScale;
    ScImage *sc;
    VectorImage *fmv, *bmv;
    int *qTable, *niqTable;
    int status;

    ReturnErrorIf1(argc != 10,
        "wrong # args : should be %s byteImage past future fmv bmv qScale qTable niqTable scImage",
        argv[0]);

    curr = GetByteImage(argv[1]);
    ReturnErrorIf2(curr == NULL,
        "%s : no such byte image %s\n", argv[0], argv[1]);

    prev = GetByteImage(argv[2]);
    ReturnErrorIf2(prev == NULL,
        "%s : no such byte image %s\n", argv[0], argv[2]);

    next = GetByteImage(argv[3]);
    ReturnErrorIf2(next == NULL,
        "%s : no such byte image %s\n", argv[0], argv[3]);

    fmv = GetVectorImage(argv[4]);
    ReturnErrorIf2(fmv == NULL,
        "%s : no such vector image %s\n", argv[0], argv[4]);

    bmv = GetVectorImage(argv[5]);
    ReturnErrorIf2(bmv == NULL,
        "%s : no such vector image %s\n", argv[0], argv[5]);

    qScale = GetByteImage(argv[6]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[6]);

    status = ParseQTable(interp, argv[7], &qTable);
    ReturnErrorIf(status != TCL_OK);

    status = ParseQTable(interp, argv[8], &niqTable);
    ReturnErrorIf(status != TCL_OK);

    sc = GetScImage(argv[9]);
    ReturnErrorIf2(sc == NULL,
        "%s : no such sc image %s\n", argv[0], argv[9]);

    ByteUVToScB(curr, prev, next, fmv, bmv, qScale, qTable, niqTable, sc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     sc_quantize src qScale qTable dest
 *
 * precond 
 *     src contains unscaled DCT values
 *     qTable is a list of 64 ints or one of the strings "jpeg-y", "jpeg-uv" "mpeg-y", "mpeg-uv" 
 *     qScale is a ByteImage of the same size as src containing scaling factors
 *     dest is as large as src
 *
 * return 
 *     none
 *
 * side effect :
 *     
 *----------------------------------------------------------------------
 */

int
ScQuantizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *scIn, *scOut;
    ByteImage *qScale;
    int *qTable;
    int status;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s src qScale qTable dest", argv[0]);

    scIn = GetScImage(argv[1]);
    ReturnErrorIf2(scIn == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    qScale = GetByteImage(argv[2]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    status = ParseQTable(interp, argv[3], &qTable);
    ReturnErrorIf(status != TCL_OK);

    scOut = GetScImage(argv[4]);
    ReturnErrorIf2(scOut == NULL,
        "%s : no such sc image %s\n", argv[0], argv[4]);

    ScQuantize(scIn, qScale, qTable, scOut);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     sc_dequantize 
 *
 * precond 
 *
 * return 
 *
 * side effect :
 *----------------------------------------------------------------------
 */

int
ScDequantizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *scIn, *scOut;
    ByteImage *qScale;
    int *qTable;
    int status;

    ReturnErrorIf1(argc != 5,
        "wrong # args : should be %s src qScale qTable dest", argv[0]);

    scIn = GetScImage(argv[1]);
    ReturnErrorIf2(scIn == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    qScale = GetByteImage(argv[2]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    status = ParseQTable(interp, argv[3], &qTable);
    ReturnErrorIf(status != TCL_OK);

    scOut = GetScImage(argv[4]);
    ReturnErrorIf2(scOut == NULL,
        "%s : no such sc image %s\n", argv[0], argv[4]);

    ScDequantize(scIn, qScale, qTable, scOut);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     sc_non_i_dequantize 
 *
 * precond 
 *
 * return 
 *
 * side effect :
 *----------------------------------------------------------------------
 */

int
ScNonIDequantizeCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *scIn, *scOut;
    ByteImage *qScale;
    int *qTable, *niqTable;
    int status;

    ReturnErrorIf1(argc != 6,
        "wrong # args : should be %s src qScale qTable niqTable dest", argv[0]);

    scIn = GetScImage(argv[1]);
    ReturnErrorIf2(scIn == NULL,
        "%s : no such sc image %s\n", argv[0], argv[1]);

    qScale = GetByteImage(argv[2]);
    ReturnErrorIf2(qScale == NULL,
        "%s : no such ByteImage %s\n", argv[0], argv[2]);

    status = ParseQTable(interp, argv[3], &qTable);
    ReturnErrorIf(status != TCL_OK);

    status = ParseQTable(interp, argv[4], &niqTable);
    ReturnErrorIf(status != TCL_OK);

    scOut = GetScImage(argv[5]);
    ReturnErrorIf2(scOut == NULL,
        "%s : no such sc image %s\n", argv[0], argv[5]);

    ScNonIDequantize(scIn, qScale, qTable, niqTable, scOut);
    return TCL_OK;
}

int
ScAddCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *src1Buf, *src2Buf, *destBuf;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src1 src2 dest", argv[0]);

    src1Buf = GetScImage(argv[1]);
    ReturnErrorIf2(src1Buf == NULL,
        "%s: no such sc image %s", argv[0], argv[1]);

    src2Buf = GetScImage(argv[2]);
    ReturnErrorIf2(src2Buf == NULL,
        "%s: no such sc image %s", argv[0], argv[2]);

    destBuf = GetScImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such sc image %s", argv[0], argv[3]);

    ScAdd(src1Buf, src2Buf, destBuf);
    return TCL_OK;
}


int
ScMultiplyCmd(clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    ScImage *srcBuf, *destBuf;
    int status;
    double k;

    ReturnErrorIf1(argc != 4,
        "wrong # args: should be %s src k dest", argv[0]);

    srcBuf = GetScImage(argv[1]);
    ReturnErrorIf2(srcBuf == NULL,
        "%s: no such sc image %s", argv[0], argv[1]);

    status = Tcl_GetDouble(interp, argv[2], &k);
    ReturnErrorIf(status != TCL_OK);

    destBuf = GetScImage(argv[3]);
    ReturnErrorIf2(destBuf == NULL,
        "%s: no such sc image %s", argv[0], argv[3]);

    ScMultiply (srcBuf, (float) k, destBuf);
    return TCL_OK;
}
