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
 *     jpeg_huff_table_new numOfComponents
 * precond 
 *     none
 * return 
 *     A handle to the new huffman table
 * side effect :
 *     Memories are allocated for new huffman table
 *
 *----------------------------------------------------------------------
 */
int
JpegHuffTableNewCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHuffTable *table;
    int status, numOfComponents;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s numOfComponents", argv[0]);

    status = Tcl_GetInt(interp, argv[1], &numOfComponents);
    ReturnErrorIf (status != TCL_OK);

    table = JpegHuffTableNew(numOfComponents);
    PutJpegHuffTable(interp, table);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_huff_table_init jpegHdr scanHdr huffTable
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     huffTable is initialized
 *
 *----------------------------------------------------------------------
 */
int
JpegHuffTableInitCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHuffTable *huffTable;
    JpegScanHdr *scanHdr;
    JpegHdr *jpegHdr;

    ReturnErrorIf1 (argc != 4,
        "wrong # args: should be %s jpegHdr scanHdr huffTable", argv[0]);

    jpegHdr = GetJpegHdr(argv[1]);
    ReturnErrorIf2 (jpegHdr == NULL,
        "%s: no such JPEG header %s", argv[0], argv[1]);
    scanHdr = GetJpegScanHdr(argv[2]);
    ReturnErrorIf2 (scanHdr == NULL,
        "%s: no such JPEG scan header %s", argv[0], argv[2]);
    huffTable = GetJpegHuffTable(argv[3]);
    ReturnErrorIf2 (huffTable == NULL,
        "%s: no such JPEG huffman table %s", argv[0], argv[3]);

    JpegHuffTableInit(jpegHdr, scanHdr, huffTable);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * usage   
 *     jpeg_huff_table_free huffTable
 * precond 
 *     none
 * return 
 *     none
 * side effect :
 *     huffTable is initialized
 *
 *----------------------------------------------------------------------
 */
int
JpegHuffTableFreeCmd (clientData, interp, argc, argv)
    ClientData clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
    JpegHuffTable *table;

    ReturnErrorIf1 (argc != 2,
        "wrong # args: should be %s huffTable", argv[0]);

    table = RemoveJpegHuffTable(argv[1]);
    ReturnErrorIf2 (table == NULL,
        "%s: no such JPEG huffman table %s", argv[0], argv[1]);

    JpegHuffTableFree(table);

    return TCL_OK;
}

