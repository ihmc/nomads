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

#include "aviInt.h"


/*
 *----------------------------------------------------------------------
 *
 * Support for Audio codecs --
 *
 *    Video for Windows uses a callback scheme to enurmerate the audio
 *    codecs.  We do this once, when first called, and cache all the
 *    information in a static array of ACMDRIVERDETAILS structure.
 *
 * The ACMDRIVERDETAILS structure is used by VFW audio compression manager
 * to return codec info.  We enumerate all the audio codecs on the system,
 * and store the information in a static array (up to 64 codecs, which should
 * be plenty).
 *
 * Here's the typedef, for reference:
 *    typedef struct { 
 *        DWORD  cbStruct; 
 *        FOURCC fccType; 
 *        FOURCC fccComp; 
 *        WORD   wMid; 
 *        WORD   wPid; 
 *        DWORD  vdwACM; 
 *        DWORD  vdwDriver; 
 *        DWORD  fdwSupport; 
 *        DWORD  cFormatTags; 
 *        DWORD  cFilterTags; 
 *        HICON  hicon; 
 *        char  szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS]; 
 *        char  szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS]; 
 *        char  szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS]; 
 *        char  szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS]; 
 *        char  szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS]; 
 *    } ACMDRIVERDETAILS; 
 */

#define MAX_AUDIO_CODECS 64
static ACMDRIVERDETAILS audioCodecInfo[MAX_AUDIO_CODECS];
static HACMDRIVERID driverid[MAX_AUDIO_CODECS];

/* static BOOL CALLBACK */
static BOOL CALLBACK
AudioEnumCallback(h, cd, flags)
    HACMDRIVERID h;
    DWORD cd;
    DWORD flags;
{
    int *nPtr, n;
    ACMDRIVERDETAILS *adPtr;
    int ret;

    nPtr = (int *) cd;
    n = *nPtr;
    if (n >= MAX_AUDIO_CODECS) {
        return TRUE;
    }
    driverid[n] = h;
    adPtr = &audioCodecInfo[n++];
    adPtr->cbStruct = sizeof(ACMDRIVERDETAILS);
    ret = acmDriverDetails(h, adPtr, 0);
    if (ret == 0) {
        *nPtr = n;
    }
    return TRUE;
}

static int
NumberOfAudioCodecs()
{
    static int numberFound = -1;

    if (numberFound <= 0) {
        numberFound = 0;
        acmDriverEnum((ACMDRIVERENUMCB)AudioEnumCallback, (DWORD) & numberFound, 0);
    }
    return numberFound;
}

/*
 *----------------------------------------------------------------------
 *
 * AviNumCodecs --
 *
 *     Return the number of codecs installed in the system
 *
 * precond 
 *     none
 *      
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
AviGetNumOfCodecs()
{
    int i;
    ICINFO ici;

    for (i = 0; ICInfo(ICTYPE_VIDEO, i, &ici); i++) {
        /* Do nothing */
    }
    return i + NumberOfAudioCodecs();
}

/*
 *----------------------------------------------------------------------
 *
 * AviCodecInfo --
 *
 *     Retrieve information about the specified codec
 *     Returned information is placed in codecPtr
 *      
 * return 
 *     0 for success, 1 for invalid codec number
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

int
AviCodecInfo(codecPtr, codecNumber)
    AviCodec *codecPtr;
    int codecNumber;
{
    ICINFO ici;
    ACMDRIVERDETAILS *adPtr;

    /*
     * The Audio codecs are 0 .. NumberOfAudioCodecs()-1
     * The Video codecs are NumberOfAudioCodecs() .. AviNumCodecs()-1
     */
    if (codecNumber < NumberOfAudioCodecs()) {
        adPtr = &audioCodecInfo[codecNumber];
        codecPtr->id = (adPtr->wMid << 16) | adPtr->wPid;
        codecPtr->video = 0;
        codecPtr->version = (short) adPtr->vdwDriver;
        strncpy(codecPtr->name, adPtr->szShortName, sizeof(codecPtr->name));
        codecPtr->name[sizeof(codecPtr->name) - 1] = 0;
        strncpy(codecPtr->description, adPtr->szLongName, sizeof(codecPtr->description));
        codecPtr->description[sizeof(codecPtr->description) - 1] = 0;
        return 0;
    } else if (ICInfo(ICTYPE_VIDEO, codecNumber - NumberOfAudioCodecs(), &ici)) {
        codecPtr->id = ici.fccHandler;
        codecPtr->video = 1;
        codecPtr->version = (short) ici.dwVersion;
        strcpy(codecPtr->name, (char *) ici.szName);
        strcpy(codecPtr->description, (char *) ici.szDescription);
        return 0;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * AviTranslateError
 *      
 *     Translates an AVI error code to a human readable string
 *
 * precond 
 *     none
 *      
 * return 
 *     A string that contains the error message.  The string is
 *     allocated in static memory, and should be considered read-only
 * 
 * side effect :
 *     none
 *
 *----------------------------------------------------------------------
 */

char *
AviTranslateError(errorCode)
    int errorCode;              /* The AVI error code */
{
    switch (errorCode) {
    case DVM_AVI_UNINITIALIZED:
        return "Stream not initialized";
    case DVM_AVI_GET_FRAME_FAILED:
        return "StartDecode not called";
    case DVM_AVI_NOT_VIDEO:
        return "Not a video stream";
    case DVM_AVI_BAD_SIZE:
        return "Buffers are not the same size as the video stream";
    case AVIERR_UNSUPPORTED:
        return "AVIERR_UNSUPPORTED: unsupported format";
    case AVIERR_BADFORMAT:
        return "AVIERR_BADFORMAT: corrupt file or unrecognized format";
    case AVIERR_MEMORY:
        return "AVIERR_MEMORY: insufficient memory";
    case AVIERR_INTERNAL:
        return "AVIERR_INTERNAL: Internal error";
    case AVIERR_BADFLAGS:
        return "AVIERR_BADFLAGS: ";
    case AVIERR_BADPARAM:
        return "AVIERR_BADPARAM: ";
    case AVIERR_BADSIZE:
        return "AVIERR_BADSIZE: ";
    case AVIERR_BADHANDLE:
        return "AVIERR_BADHANDLE: ";
    case AVIERR_FILEREAD:
        return "AVIERR_FILEREAD: disk error while reading file";
    case AVIERR_FILEWRITE:
        return "AVIERR_FILEWRITE: disk error while writing file";
    case AVIERR_FILEOPEN:
        return "AVIERR_FILEOPEN: disk error while opening the file";
    case AVIERR_COMPRESSOR:
        return "AVIERR_COMPRESSOR: ";
    case AVIERR_NOCOMPRESSOR:
        return "AVIERR_NOCOMPRESSOR: suitable compressor cannot be found";
    case AVIERR_READONLY:
        return "AVIERR_READONLY: ";
    case AVIERR_BUFFERTOOSMALL:
        return "AVIERR_BUFFERTOOSMALL: Buffer size smaller than generated filter specification";
    case AVIERR_CANTCOMPRESS:
        return "AVIERR_CANTCOMPRESS: ";
    case AVIERR_USERABORT:
        return "AVIERR_USERABORT: ";
    case AVIERR_ERROR:
        return "AVIERR_ERROR: ";
    case REGDB_E_CLASSNOTREG:
        return "REGDB_E_CLASSNOTREG: file does not have a handler to process it in the registry";
    case DVM_AVI_BAD_STREAM_NUM:
        return "Requested non-existent stream";
    case DVM_AVI_NOT_RGB:
        return "Video stream is not RGB";
    }
    return "Unknown error code (passed to AviTranslateError)";
}
