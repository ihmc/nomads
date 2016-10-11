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
 * avifile.c
 *
 * Functions that open/close/create manipulate avi files
 *
 *----------------------------------------------------------------------
 */

#include "aviInt.h"

static void
InitAviLibrary ()
{
    static int inited = 0;

    if (!inited) {
	/* Init the AviFile library */
	AVIFileInit();
	inited++;
    }
}
/*
 *----------------------------------------------------------------------
 *
 * AviFileOpen --
 *
 *    Open an AVI file for reading
 *
 * precond 
 *     filename exists
 *      
 * return 
 *     0 on success, error code otherwise
 * 
 * side effect :
 *     The file <filename> is opened for reading
 *     memory is allocated for a new aviheader object. Use AviCloseFile
 *     to free this header and close the file.
 *
 *----------------------------------------------------------------------
 */

int
AviFileOpen(filename, aviFilePtr)
    char *filename;
    AviFile **aviFilePtr;
{
    AviFile *rv;
    AVIFILEINFO fileInfo;
    int status;

    rv = NEW(AviFile);
    if (rv == NULL) {
        return AVIERR_MEMORY;
    }
    /*
     * Call video for windows to open the file for us
     */
    InitAviLibrary();
    status = AVIFileOpen(&(rv->aviHandle), filename, OF_READ, NULL);
    if (status) {
        FREE(rv);
        return status;
    }
    /* Read the header */
    memset((char *) &fileInfo, 0, sizeof(AVIFILEINFO));
    status = AVIFileInfo(rv->aviHandle, &fileInfo, sizeof(AVIFILEINFO));
    if (status) {
        FREE(rv);
        return status;
    }
    rv->length = fileInfo.dwLength;
    rv->numStreams = (short) fileInfo.dwStreams;
    rv->flags = fileInfo.dwFlags;
    *aviFilePtr = rv;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * AviFileClose --
 *
 *      Close a previously open AVI file
 *
 * precond 
 *      Avi File aviFile exists
 *      
 * return 
 *     none
 * 
 * side effect :
 *     The file associated with this header is closed
 *
 *----------------------------------------------------------------------
 */

void
AviFileClose(aviFile)
    AviFile *aviFile;
{

    /*
     * Inform video for windows, and free the memory
     */
    AVIFileRelease(aviFile->aviHandle);
    FREE(aviFile);
}

/*
 *----------------------------------------------------------------------
 *
 * AviFileCreate --
 *
 *     Create a new AVI file
 *
 * precond 
 *     none
 *      
 * return 
 *    0 for success, error code otherwise.
 * 
 * side effect :
 *     The file <filename> is opened for writing. It is created if it
 *     does not exist. Use AviFileClose to close the file.
 *
 *----------------------------------------------------------------------
 */

int
AviFileCreate(filename, aviFilePtr)
    char *filename;
    AviFile **aviFilePtr;
{
    AviFile *rv;
    int status;

    /*
     * Call video for windows.....
     */
    rv = NEW(AviFile);
    if (rv == NULL) {
        return AVIERR_MEMORY;
    }
    InitAviLibrary();
    status = AVIFileOpen(&(rv->aviHandle), filename, OF_CREATE | OF_WRITE, NULL);
    if (status) {
        return status;
    }
    rv->numStreams = rv->length = rv->flags = 0;
    *aviFilePtr = rv;
    return 0;
}
