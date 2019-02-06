/*
 * NLFLib.cpp
 *
 *This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#include "NLFLib.h"

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>

#if defined (WIN32)
    #include <sys/timeb.h>
    #include <io.h>
    #define NOMINMAX
    #include <winsock2.h>
    #include <windows.h>
    #define PATH_MAX _MAX_PATH
    #define PATH_SEP '\\'
    #define PATH_SEP_STR "\\"

#elif defined (UNIX)
    #include <unistd.h>
    #define PATH_SEP '/'
    #define PATH_SEP_STR "/"
    #if defined (LINUX) || defined (MACOSX)
    	#include <sys/time.h>
    #endif
#endif

#include <limits.h>
#include <stdlib.h>

using namespace NOMADSUtil;

uint32 NOMADSUtil::ceiling (uint32 ui32Input, uint32 ui32Multiplier)
{
    uint32 ui32ModValue = ui32Input % ui32Multiplier;
    if (ui32ModValue == 0) {
        // Nothing to be done
        return ui32Input;
    }
    else {
        return ui32Input + (ui32Multiplier - ui32ModValue);
    }
}

int64 NOMADSUtil::getTimeInMilliseconds (void)
{
#if defined (LINUX) || defined (MACOSX)
  // using BSD4.3 calls; more reliable
  struct timeval tv;
  if (gettimeofday (&tv,0) == -1)
    // this call failed.. what do we do?  not sure.
    return 0;
  else
    return (int64)tv.tv_sec* (int64) 1000 + (int64) tv.tv_usec / (int64) 1000;
#else
    int64 i64Time;
    struct timeb timebuffer;
    ftime (&timebuffer);
    i64Time = timebuffer.time;
    i64Time *= 1000;
    i64Time += timebuffer.millitm;
    return i64Time;
#endif
}

void NOMADSUtil::sleepForMilliseconds (int64 i64MilliSec)
{
    #if defined (WIN32)
        Sleep ((uint32)i64MilliSec);
    #elif defined (UNIX)
        usleep ((uint32) i64MilliSec*1000);
    #endif
}

uint32 NOMADSUtil::atoui32 (const char *pszValue)
{
    const char *pszStart = pszValue;
    while (*pszValue) {
        pszValue++;
    }
    uint32 ui32PosValue = 1;
    uint32 ui32Value = 0;
    while (pszValue > pszStart) {
        pszValue--;
        if (!isdigit (*pszValue)) {
            break;
        }
        ui32Value += ui32PosValue * (*pszValue - '0');
        ui32PosValue *= 10;
    }
    return ui32Value;
}

uint64 NOMADSUtil::atoui64 (const char *pszValue)
{
    const char *pszStart = pszValue;
    while (*pszValue) {
        pszValue++;
    }
    uint64 ui64PosValue = 1;
    uint64 ui64Value = 0;
    while (pszValue > pszStart) {
        pszValue--;
        if (!isdigit (*pszValue)) {
            break;
        }
        ui64Value += ui64PosValue * (*pszValue - '0');
        ui64PosValue *= 10;
    }
    return ui64Value;
}

int64 NOMADSUtil::atoi64 (const char *pszValue)
{
    int64 i64Value;
    #if defined (WIN32)
        bool negative = false;
        const char *pszStart = pszValue;
        while (*pszValue) {
            if((*pszValue) == '-') negative = true;
            pszValue++;
        }
        int64 i64PosValue = 1;
        i64Value = 0;
        while (pszValue > pszStart) {
            pszValue--;
            if (!isdigit (*pszValue)) {
                break;
            }
            i64Value += i64PosValue * (*pszValue - '0');
            i64PosValue *= 10;
        }
        if(negative) i64Value = - i64Value;
    #elif defined (UNIX)
        i64Value = atoll(pszValue);
    #endif
    return i64Value;
}

bool NOMADSUtil::atod (const char *pszValue, double &dValue)
{
    char *p = (char *) pszValue;
    errno = 0;
    dValue = strtod (pszValue, &p);
    if ((errno != 0) || (pszValue == p) || (*p != 0)) {
        // error (EINVAL, ERANGE) || (no characters consumed) || (trailing data)
        return false;
    }

    return true;
}

char * NOMADSUtil::itoa (char *pszValue, int iValue)
{
    char szBuf[11];
    char *pszTemp = szBuf;
    char *pszValueOrig = pszValue;
    if (iValue < 0) {
        *pszValue++ = '-';
        iValue = -iValue;
    }
    do {
        *pszTemp++ = (iValue % 10) + '0';
        iValue /= 10;
    } while (iValue > 0);
    *pszTemp--;
    while (pszTemp >= szBuf) {
        *pszValue++ = *pszTemp--;
    }
    *pszValue = '\0';
    return pszValueOrig;
}

char * NOMADSUtil::i64toa (char *pszValue, int64 i64Value)
{
    char szBuf[21];
    char *pszTemp = szBuf;
    char *pszValueOrig = pszValue;
    if (i64Value < 0) {
        *pszValue++ = '-';
        i64Value = -i64Value;
    }
    do {
        *pszTemp++ = ((char) (i64Value % 10)) + '0';
        i64Value /= 10;
    } while (i64Value > 0);
    *pszTemp--;
    while (pszTemp >= szBuf) {
        *pszValue++ = *pszTemp--;
    }
    *pszValue = '\0';
    return pszValueOrig;
}

char * NOMADSUtil::strtok_mt (const char *s1, const char *s2, char **ppszTemp)
{
    #if defined (WIN32)
        return strtok ((char*) s1, s2);
    #elif defined (UNIX)
        return strtok_r ((char *) s1, s2, ppszTemp);
    #endif
}

char * NOMADSUtil::strDup (const char *pszStr)
{
    if (pszStr == NULL) {
        return NULL;
    }
    char *pszCopy = (char*) malloc (strlen (pszStr)+1);
    if (pszCopy != NULL) {
        strcpy (pszCopy, pszStr);
    }
    return pszCopy;
}

bool NOMADSUtil::strNotNullAndEqual (const char *pszStringA, const char *pszStringB)
{
    if ((pszStringA == NULL) || (pszStringB == NULL)) {
        return false;
    }
    int iLen = (int) strlen (pszStringA);
    if (iLen != ((int) strlen (pszStringB))) {
        // The strings have different lengths, they can not be equal
        return false;
    }
    return (0 == strncmp (pszStringA, pszStringB, iLen));
}

bool NOMADSUtil::wildcardStringCompare (const char *pszString, const char *pszTemplate)
{
    if ((pszString == NULL) || (pszTemplate == NULL)) {
        return false;
    }

    // Check if there is a wildcard at the beginning of the template
    if (*pszTemplate == '*') {
        // For a wild card at the beginning

        // Check if there is a wildcard at the end
        if ((strlen (pszTemplate) > 1) && (pszTemplate[strlen (pszTemplate)-1] == '*')) {
            // We have a wildcard at the end also
            // Make a copy of the template
            char *pszTemplateCopy = new char [strlen (pszTemplate)];
            strcpy (pszTemplateCopy, pszTemplate+1);

            // Take out the ending wildcard
            pszTemplateCopy [strlen (pszTemplateCopy) - 1] = '\0';

            if (strstr (pszString, pszTemplateCopy) != NULL) {
                delete[] pszTemplateCopy;
                return true;
            }
            else {
                delete[] pszTemplateCopy;
                return false;
            }
        }
        else {
            // No wildcard at the end
            const char *pszTmp1 = pszString;
            const char *pszTmp2 = pszTemplate;
            while (*pszTmp1 != '\0') {
                pszTmp1++;
            }
            pszTmp1--;
            while (*pszTmp2 != '\0') {
                pszTmp2++;
            }
            pszTmp2--;
            for (; (pszTmp1 >= pszString) && (pszTmp2 >= pszTemplate); pszTmp1--, pszTmp2--) {
                // Returns true if the template contains a wild card
                if (*pszTmp2 == '*') {
                    return true;
                }
                // Checks each character of the string and template. It returns
                // true if they are the same or false if they are not.
                else if (tolower (*pszTmp1) != tolower (*pszTmp2)) {
                    return false;
                }
            }
            if ((pszTmp1 >= pszString) || (pszTmp2 >= pszTemplate)) {
                return false;
            }
            return true;
        }
    }
    else {
        // For a wild card at the end
        for (; (*pszTemplate != '\0') && (*pszString != '\0'); pszString++, pszTemplate++) {
            if (*pszTemplate == '*') {
                return true;
            }
            // Checks each character of the string and template. It returns
            // true if they are the same or false if they are not.
            else if (tolower (*pszTemplate) != tolower (*pszString)) {
                return false;
            }
        }
        // If the characters of the template are the same but the template's length is
        // less than the string, it returns false.
        if (*pszTemplate != *pszString) {
            return false;
        }
   }
   return true;
}

const char * NOMADSUtil::encodeSpacesInString (const char *pszSrc, char *pszDest)
{
    const char *pszTemp = pszDest;
    while (*pszSrc != '\0') {
        if (*pszSrc == ' ') {
            *pszDest++ = '%';
            *pszDest++ = '2';
            *pszDest++ = '0';
        }
        else if (*pszSrc == '%') {
            *pszDest++ = '%';
            *pszDest++ = '2';
            *pszDest++ = '5';
        }
        else {
            *pszDest++ = *pszSrc;
        }
        pszSrc++;
    }
    *pszDest = '\0';
    return pszTemp;
}

const char * NOMADSUtil::getProgPath (const char *argv0)
{
    static char buf [PATH_MAX];

    if (buf [0]) {
        // Must have already been initialized
        return buf;
    }

    #if defined (WIN32)
        int rc;
        if (0 == (rc = GetModuleFileName (NULL, (LPSTR)buf, PATH_MAX))) {
            return NULL;
        }
    #elif defined (UNIX)
        if (argv0 == NULL) {
            return NULL;
        }
        if (argv0 [0] == '/') {
            // The program was invoked with an absolute path, so that is the
            //     path to the program
            strcpy (buf, argv0);
        }
        else if (argv0 [0] == '.') {
            // The program was invoked with a relative path - prefix with the
            //     current working directory
            if (NULL == realpath (argv0, buf)) {
                buf[0] = '\0';
                return NULL;
            }
        }
        else {
            // The program must have been found in the path so search the path
            const char *pszPath = getenv ("PATH");
            if (pszPath == NULL) {
                // No path specified - try with the path just set to .
                pszPath = ".";
            }
            // Search each entry in the path
            char *pszPathBuf = new char [strlen (pszPath) + 1];
            strcpy (pszPathBuf, pszPath);
            char *pszPathEntry = pszPathBuf;
            while (*pszPathEntry) {
                char *pszDelim = strchr (pszPathEntry, ':');
                if (pszDelim) {
                    *pszDelim = '\0';
                }
                char szTempPath[PATH_MAX];
                strcpy (szTempPath, pszPathEntry);
                strcat (szTempPath, "/");
                strcat (szTempPath, argv0);
                if (NULL != realpath (szTempPath, buf)) {
                    if (0 == access (buf, 00)) {
                        // We have found the executable
                        break;
                    }
                }
                buf[0] = '\0';
                pszPathEntry = pszDelim + 1;
            }
            if (buf[0] == '\0') {
                return NULL;
            }
        }
    #endif
    return buf;
}

const char * NOMADSUtil::getProgHomeDir (const char *argv0)
{
    static char buf [PATH_MAX];

    if (buf[0]) {
        // Must have already been initialized
        return buf;
    }

    // Get the path to the program
    const char *pszProgPath = NOMADSUtil::getProgPath (argv0);
    if (pszProgPath == NULL) {
        return NULL;
    }
    strcpy (buf, pszProgPath);

    // Strip the executable file name
    #if defined (WIN32)
        char *pszTemp = strrchr (buf, '\\');
    #elif defined (UNIX)
        char *pszTemp = strrchr (buf, '/');
    #endif
    if (pszTemp == NULL) {
        buf[0] = '\0';
        return NULL;
    }
    else {
        *pszTemp = '\0';
    }

    return buf;
}

int NOMADSUtil::getMaxPathLen (void)
{
    return PATH_MAX;
}

uint32 NOMADSUtil::getPID (void)
{
    #if defined (WIN32)
        return (uint32) GetCurrentProcessId();
    #elif defined (UNIX)
        return (uint32) getpid();
    #else
        #error Must define WIN32 or UNIX
    #endif
}

const char * NOMADSUtil::generateTimestamp (char *pszBuf, uint32 ui32BufSize)
{
    if (ui32BufSize < 15) {
        return NULL;
    }
    time_t currTime = time (NULL);
    struct tm *ptm = localtime (&currTime);
    sprintf (pszBuf, "%04d%02d%02d%02d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    return pszBuf;
}

/** Expect a pointer to an empty string of PATH_MAX. If the dir exists, make sure that you can create a
 *  temporary file in it, and return a unique temp file name, with prefix defined by pszPrefix.
 */
const char * NOMADSUtil::getTempFilePath (char *pszBuf, const char *pszDirectory, const char *pszPrefix)
{
    if (pszBuf == NULL) {
        return NULL;
    }
    if (pszDirectory == NULL) {
        #if defined (WIN32)
            return tmpnam (pszBuf);
        #else
            strncpy (pszBuf, "tempXXXXXX", 10);
            int fd = -1;
            if ((fd = mkstemp (pszBuf)) < 0) {
                return NULL;
            }
            close (fd);
            return pszBuf;
        #endif
    }
    else {
        #if defined (WIN32)
            char *pszName = _tempnam (pszDirectory, pszPrefix);
        #else
            char *pszName = new char [PATH_MAX];
            const char *tmpDir = getenv ("TMP");
            if (tmpDir == NULL) {
                sprintf (pszName, "%s%c%sXXXXXX", pszDirectory, PATH_SEP, pszPrefix);
            }
            else {
                sprintf (pszName, "%s%c%sXXXXXX", tmpDir, PATH_SEP, pszPrefix);
            }
            int fd = -1;
            if ((fd = mkstemp (pszName)) < 0) {
                    return NULL;
            }
            close (fd);
        #endif
        if (pszName != NULL) {
            strcpy (pszBuf, pszName);
            free (pszName);
            return pszBuf;
        }
        return NULL;
    }
}

FILE * NOMADSUtil::getTempFile (const char *pszDirectory, const char *pszPrefix)
{
    if (pszDirectory == NULL) {
        return tmpfile();
    }
    else {
        #if defined (WIN32)
            char *pszName = _tempnam (pszDirectory, pszPrefix);
        #else
            char *pszName = new char [PATH_MAX];
            const char *tmpDir = getenv ("TMP");
            if (tmpDir == NULL) {
                sprintf (pszName, "%s%c%sXXXXXX", pszDirectory, PATH_SEP, pszPrefix);
            }
            else {
                sprintf (pszName, "%s%c%sXXXXXX", tmpDir, PATH_SEP, pszPrefix);
            }
            int fd = -1;
            if ((fd = mkstemp (pszName)) < 0) {
                return NULL;
            }
            close (fd);
        #endif
        if (pszName) {
            FILE *fileTemp = fopen (pszName, "wb+");
            free (pszName);
            return fileTemp;
        }
        return NULL;
    }
}

int64 NOMADSUtil::getFileSize (const char *pszPath)
{
    struct stat sb;
    if (0 != (stat (pszPath, &sb))) {
        return -1;
    }
    if (sb.st_size <= 0) {
        return -2;
    }
    return sb.st_size;
}

void NOMADSUtil::stripCRLF (char *pszBuf)
{
    int i = (int) strlen (pszBuf) - 1;
    while (i >= 0) {
        if (pszBuf[i] == '\r') {
            pszBuf[i] = '\0';
        }
        else if (pszBuf[i] == '\n') {
            pszBuf[i] = '\0';
        }
        i--;
    }
}

const char * NOMADSUtil::getLastOSErrorAsString (int iErrNum)
{
    #if defined (WIN32)
        __declspec (thread) static char szBuf [256];
        szBuf[0] = '\0';
        FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       (DWORD) iErrNum,
                       0,
                       (LPSTR) szBuf,
                       sizeof (szBuf),
                       NULL);
        return szBuf;
    #elif defined (UNIX)
        return NULL;
    #endif
}

const char * NOMADSUtil::getLastOSErrorAsString (void)
{
    #if defined (WIN32)
        __declspec (thread) static char szBuf [256];
        szBuf[0] = '\0';
        FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       GetLastError(),
                       0,
                       (LPSTR) szBuf,
                       sizeof (szBuf),
                       NULL);
        return szBuf;
    #elif defined (UNIX)
        return NULL;
    #endif
}


int NOMADSUtil::getline (char *s, int n, FILE *f) {
    register int i=0;

    while(1) {
        s[i] = (char)fgetc (f);

        if (s[i] == CR)
            s[i] = fgetc (f);

        if ((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) {
            s[i] = '\0';
            return (feof(f) ? 1 : 0);
        }
        ++i;
    }
}

/**
 *
 */
void NOMADSUtil::printByteArray (const char *name, unsigned char *puc, int iLen)
{
    fprintf (stderr, "\n%s of length: %d\n", name, iLen);
    for (int i = 0; i < iLen;i++) {
        fprintf (stderr, "0x%x ", puc[i]);
        if (i != 0 && (i + 1) % 12 == 0) {
            fprintf (stderr, "\n\n");
        }
    }
    fprintf (stderr, "\n");
}

int NOMADSUtil::copyFile (const char *pszSrcFile, const char *pszDestFile)
{
    FILE *pSrcFile, *pDestFile;
    char szLine[512];
    char *pszReturn;

    if ((pSrcFile = fopen (pszSrcFile, "r")) == NULL) {
        return -1;
    }

    if ((pDestFile = fopen (pszDestFile, "w")) == NULL) {
        fclose (pSrcFile);
        return -2;
    }

    while ((pszReturn = fgets (szLine, sizeof (szLine), pSrcFile)) != NULL) {
        fprintf (pDestFile, "%s", szLine);
    }

    fclose (pSrcFile);
    fclose (pDestFile);
    return 0;
}

/** Replace occurrences of searchPattern with replacePattern in searchString.  If global is true,
 *  replace all occurrences, otherwise replace only the first.
 *  This returns a new string; the caller should free it. */
char * NOMADSUtil::strsub (char *pszSearchString, char *pszSearchPattern, char *pszReplacePattern, bool bGlobal)
{
    int iTempLen = 0;
    int i;
    bool bReplace = true;
    char *pszTmpInitial = (char *)NULL;
    char *pszTempReplacePattern;
    char *pszTmpFinal;

    int iSearchPatternLen = (int) strlen (pszSearchPattern);
    int iReplacePatternLen = (int) strlen (pszReplacePattern);
    pszTmpInitial = (char *) malloc (3 * strlen (pszSearchString) +
        (((iReplacePatternLen - iSearchPatternLen) > 0)? 10 * (iReplacePatternLen - iSearchPatternLen):0));
    for (i = 0; pszSearchString[i]; ) {
        if (bReplace && strncmp (pszSearchString + i, pszSearchPattern, iSearchPatternLen) == 0) {
            for (pszTempReplacePattern = pszReplacePattern; *pszTempReplacePattern; ) {
                pszTmpInitial[iTempLen++] = *pszTempReplacePattern++;
            }

            i += iSearchPatternLen ? iSearchPatternLen : 1;	/* avoid infinite recursion */
            if (bGlobal) {
                bReplace = true;
            }
        }
        else {
            pszTmpInitial[iTempLen++] = pszSearchString[i++];
        }
    }
    pszTmpInitial[iTempLen] = 0;
    pszTmpFinal = (char *) malloc (iTempLen);
    strcpy (pszTmpFinal, pszTmpInitial);
    delete[] pszTmpInitial;
    return pszTmpFinal;
}

int NOMADSUtil::minimum (int iFirstVal, int iSecondVal)
{
    if (iFirstVal < iSecondVal) {
        return iFirstVal;
    }
    return iSecondVal;
}

uint16 NOMADSUtil::minimum (uint16 ui16FirstVal, uint16 ui16SecondVal)
{
    if (ui16FirstVal < ui16SecondVal) {
        return ui16FirstVal;
    }
    return ui16SecondVal;
}

uint32 NOMADSUtil::minimum (uint32 ui32FirstVal, uint32 ui32SecondVal)
{
    if (ui32FirstVal < ui32SecondVal) {
        return ui32FirstVal;
    }
    return ui32SecondVal;
}

uint64 NOMADSUtil::minimum (uint64 ui64FirstVal, uint64 ui64SecondVal)
{
    if (ui64FirstVal < ui64SecondVal) {
        return ui64FirstVal;
    }
    return ui64SecondVal;
}

float NOMADSUtil::minimum (float fFirstVal, float fSecondVal)
{
    if (fFirstVal < fSecondVal) {
        return fFirstVal;
    }
    return fSecondVal;
}

int NOMADSUtil::maximum (int iFirstVal, int iSecondVal)
{
    if (iFirstVal > iSecondVal) {
        return iFirstVal;
    }
    return iSecondVal;
}

uint16 NOMADSUtil::maximum (uint16 ui16FirstVal, uint16 ui16SecondVal)
{
    if (ui16FirstVal > ui16SecondVal) {
        return ui16FirstVal;
    }
    return ui16SecondVal;
}

uint32 NOMADSUtil::maximum (uint32 ui32FirstVal, uint32 ui32SecondVal)
{
    if (ui32FirstVal > ui32SecondVal) {
        return ui32FirstVal;
    }
    return ui32SecondVal;
}

float NOMADSUtil::maximum (float fFirstVal, float fSecondVal)
{
    if (fFirstVal > fSecondVal) {
        return fFirstVal;
    }
    return fSecondVal;
}

float NOMADSUtil::scale (float fValue, float fOldMin, float fOldMax, float fNewMin, float fNewMax, bool &bError)
{
    if ((fValue < fOldMin) || (fValue > fOldMax) ||
        (fOldMin > fOldMax) || (fNewMin > fNewMax)) {
        bError = true;
        return 0.0;
    }
    if (fOldMax == fOldMin) {
        bError = false;
        return fNewMax;
    }
    bError = false;
    return (fValue * (fNewMax - fNewMin) / (fOldMax - fOldMin)) + fNewMin;
}

int NOMADSUtil::getRandomBytes (void *pBuf, uint32 ui32BufSize)
{
    // The following code is borrowed from an implementation by Jesse Kovach @ ARL
    #if defined (WIN32)
        static HCRYPTPROV hCryptProv;
        if (hCryptProv == NULL) {
            if (!CryptAcquireContext (&hCryptProv, NULL, NULL, PROV_DSS, 0)) {
                if (!CryptAcquireContext (&hCryptProv, NULL, NULL, PROV_DSS, CRYPT_NEWKEYSET)) {
                    // Error - could not initialize crypto provider
                    return -1;
                }
            }
        }
        memset (pBuf, 0, ui32BufSize);
        CryptGenRandom (hCryptProv, ui32BufSize, (BYTE*)pBuf);
        return 0;
    #elif defined (UNIX)
        static FILE *fileRandom;
        if (fileRandom == NULL) {
            if (NULL == (fileRandom = fopen ("/dev/random", "rb"))) {
                return -1;
            }
        }
        memset (pBuf, 0, ui32BufSize);
        if (ui32BufSize != fread (pBuf, 1, ui32BufSize, fileRandom)) {
            return -2;
        }
        return 0;
    #endif
}

void NOMADSUtil::deallocateNullTerminatedPtrArray (char **ptrArray)
{
    if (ptrArray == NULL) {
        return;
    }
    for (unsigned int i = 0; ptrArray[i] != NULL; i++) {
        free (ptrArray[i]);
        ptrArray[i] = NULL;
    }
    free (ptrArray);
    ptrArray = NULL;
}

