/*
 * NLFLib.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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
 *
 * Niranjan's Little Function Library
 *
 * Defines a set of commonly used functions
 */

#ifndef INCL_NLF_LIB_H
#define INCL_NLF_LIB_H

#if !defined (WIN32) && !defined (UNIX)
    #error Must Define WIN32 or UNIX
#endif

#if defined (WIN32)
    #define _USE_MATH_DEFINES    // For M_PI - see math.h
#endif

#include "FTypes.h"

#define LF 10
#define CR 13

#ifdef __linux__
    #include <linux/limits.h>
#endif

#include <math.h>
#include <stdio.h>

namespace NOMADSUtil
{

    int64 getTimeInMilliseconds (void);
    void sleepForMilliseconds (int64 i64MilliSec);

    // Convert from degrees to radians
    inline double degToRad (double deg)
    {
        return (deg * M_PI) / 180.0;
    }

    // Convert from radians to degrees
    inline double radToDeg (double rad)
    {
        return (rad * 180.0) / M_PI;
    }

    // Return the ceiling of the specified number, rounded up to the nearest multiple of the multiplier
    uint32 ceiling (uint32 ui32Input, uint32 ui32Multiplier);

    // Convert a string representation of a 32-bit unsigned int value
    //     into the corresponding numeric representation
    uint32 atoui32 (const char *pszValue);

    // Convert a string representation of a 64-bit unsigned int value
    //     into the corresponding numeric representation
    uint64 atoui64 (const char *pszValue);

    // Convert a string representation of a 64-bit int value
    //     into the corresponding numeric representation
    int64 atoi64 (const char *pszValue);

    // Convert a string representation of a double floating point number
    //     into the corresponding numerica representation.
    // Returns true if the conversion was successful, false otherwise
    bool atod (const char *pszValue, double &dValue);

    // Convert a numeric representation of a 32-bit signed int value
    //     into the corresponding string representation
    // NOTE: The pszValue buffer may need to be upto 12 characters long
    //       depending on the value
    // NOTE: This differs from the standard itoa() in the c library by
    //       not taking a radix argument.
    //       The main reason for the definition of this function in this
    //       library is that Linux does not seem to have itoa()
    char * itoa (char *pszValue, int iValue);

    // Convert a numeric representation of a 64-bit signed int value
    //     into the corresponding string representation
    // NOTE: The pszValue buffer may need to be upto 22 characters long
    //     depending on the value
    char * i64toa (char *pszValue, int64 i64Value);

    // A version of strtok() that is safe to use in multi-threaded programs
    // NOTE: The third variable - ppszTemp - is used by the C library to
    //       hold state information about the parsing. It should not be used
    //       by the application. The application should declare a variable of
    //       type char* and pass in the address of that variable.
    // EXAMPLE:
    //     char *pszTemp;
    //     char *pszInput = "hello, world";
    //     char *pszToken;
    //     pszToken = strtok_mt (pszInput, ",", &pszTemp);
    //     printf ("%s\n", pszToken);
    //     pszToken = strtok_mt (NULL, ",", &pszTemp);
    //     printf ("%s\n", pszToken);
    char * strtok_mt (const char *s1, const char *s2, char **ppszTemp);

    // A replacement for the strdup function in the C library
    // This is to fix a problem with the Microsoft C Runtime Library that seems
    // to cause a problem when free() is called on a string that has been
    // created using strdup()
    char * strDup (const char *pszStr);

    // Returns true if the two strings are not null and they are equal - false
    // otherwise
    bool strNotNullAndEqual (const char *pszStringA, const char *pszStringB);

    // Compares a string against a template which can contain wildcards
    // Returns true if the string matches the template, or false otherwise
    // Note that the template may contain a wildcard at the beginning, the end, or both
    // Examples: "*.coginst.uwf.edu", "143.88.7.*", "*.uwf.*"
    bool wildcardStringCompare (const char *pszString, const char *pszTemplate);

    // Encode spaces in a string into %20 (and % into %25) so that the resulting
    // string has no spaces
    // Returns a pointer to the encoded string
    // NOTE: pszDest must point to a string buffer that has sufficient space to
    //       hold the resultant string. The size of the resultant string depends
    //       on the number of spaces and percent signs in the source string. The
    //       maximum length is 3*n where n is the number of characters in the
    //       source string
    const char * encodeSpacesInString (const char *pszSrc, char *pszDest);

    const char * getProgPath (const char *argv0);
    const char * getProgHomeDir (const char *argv0);

    // Returns the system dependent maximum path length
    int getMaxPathLen (void);

    // Returns the id of the current process
    uint32 getPID (void);

    inline char getPathSepChar (void)
    {
        #if defined (WIN32)
            return '\\';
        #elif defined (UNIX)
            return '/';
        #endif
    }

    inline const char * getPathSepCharAsString (void)
    {
        #if defined (WIN32)
            return "\\";
        #elif defined (UNIX)
            return "/";
        #endif
    }

    // Generates a string in the format YYYYMMDDHHMMSS
    // Returns a pointer to pszBuf if successful, or NULL in case of failure
    // Buffer must be at least 15 bytes
    const char * generateTimestamp (char *pszBuf, uint32 ui32BufSize);

    // Get a pathname to a temporary file
    // If pszDirectory is NULL, the pszPrefix is ignored and tmpnam() is used to generate the name
    // if pszDirectory is specified, tempnam() is used to generate the name
    // NOTE: pszBuf must be large enough to accommodate the file path
    // Returns a pointer to pszBuf if successful or NULL otherwise
    const char * getTempFilePath (char *pszBuf, const char *pszDirectory = NULL, const char *pszPrefix = NULL);

    // Create and open a temporary file and return a FILE handle to the file
    // If pszDirectory is NULL, the pszPrefix is ignored and tmpfile() is used to create and open the file
    // if pszDirectory is specified, tempnam() is used to generate a filename which is then opened using
    //     fopen() with a mode of "wb+"
    // Returns NULL in case of error
    FILE * getTempFile (const char *pszDirectory = NULL, const char *pszPrefix = NULL);

    // Returns the size of the file, or a negative number in case of error
    int64 getFileSize (const char *pszPath);

    // Replace CRLF combination in string with nulls
    void stripCRLF (char *pszBuf);

    // Returns the last operating system error as a string message
    // The returned string is in a thread-specific static string of size 256 bytes
    // which will be overwritten the next time the same thread calls this function
    const char * getLastOSErrorAsString (void);

    // Will not work directly with windows sockets unless errnum passed in. can use WSAGetLastError() as arg
    const char * getLastOSErrorAsString (int iErrNum);

    // Reads a line in from the given file pointed at by the file descriptor f, and place it in 
    // char buf pointed to by s, but not exceeding n characters, which presumably is the size
    // of the string allocated to s; Returns a 0 if successful, a 1 if EOF encountered.
    int getline (char *s, int n, FILE *f);

    //Prints out a bytearray of name as hex columns
    void printByteArray (const char *name, unsigned char *puc, int iLen);

    // Copies an entire file from src to dest, geared toward text files
    int copyFile (const char *pszSrcFile, const char *pszDestFile);

    /** Replace occurrences of searchPattern with replacePattern in searchString.  If global is true,
     *  replace all occurrences, otherwise replace only the first.
     *  This returns a new string; the caller should free it. */
    char *strsub (char *pszSearchString, char *pszSearchPattern, char *pszReplacePattern, bool bGlobal);

    // STL <algorithm> replacement functions
    // Returns the lesser of iFirstVal and iSecondVal.
    int minimum (int iFirstVal, int iSecondVal);
    uint16 minimum (uint16 ui16FirstVal, uint16 ui16SecondVal);
    uint32 minimum (uint32 ui32FirstVal, uint32 ui32SecondVal);
    float minimum (float fFirstVal, float fSecondVal);

    // STL <algorithm> replacement functions
    // Returns the greater of iFirstVal and iSecondVal.
    int maximum (int iFirstVal, int iSecondVal);
    uint16 maximum (uint16 ui16FirstVal, uint16 ui16SecondVal);
    uint32 maximum (uint32 ui32FirstVal, uint32 ui32SecondVal);
    float maximum (float fFirstVal, float fSecondVal);

    // Returns the relative difference between two floats
    inline float relDiff (float a, float b)
    {
        float absA = fabs (a);
        float absB = fabs (b);

        absB = maximum (absA, absB);

        return absB > 0.0f ? (fabs (a - b) / absB) : 0.0f;
    }

    /**
     * Rounds up or down a decimal value to the closest integer
     */
    inline long int roundUpOrDown (float fValue)
    {
        long int intPart = (long int) fValue;
        return (fValue - intPart >= 0.5 ? ++intPart : intPart);
    }

    /**
     * Scale fValue, included in ]fOldMin, fOldMax[  into the range
     * ]fNewMin, fNewMax[.
     *
     * - fValue is the value to be scaled.
     * - fOldMin is the minimum value that fValue can assume
     * - fOldMax is the maximum value that fValue can assume
     * - fNewMin is the minimum returned value
     * - fNewMax is the maximum returned value
     */
    float scale (float fValue, float fOldMin, float fOldMax, float fNewMin, float fNewMax, bool &bError);

    /**
     * Analogous to the previous method, but it scales fValue, included
     * in ]fOldMin, fOldMax[  into the range ]fNewMax, fNewMin[.
     */
    inline float inverseScale (float fValue, float fOldMin, float fOldMax,
                               float fNewMin, float fNewMax, bool &bError)
    {
        return (fNewMax - (scale (fValue, fOldMin, fOldMax, fNewMin, fNewMax, bError) - fNewMin));
    }

    // Compares two floats for equality (using relative difference method)
    inline bool fEquals (float a, float b, float tolerance)
    {
        return (relDiff (a, b) < tolerance);
    }

    // Generates random bytes and stores them in the specified buffer
    // Returns 0 if successful or a negative value in case of error
    int getRandomBytes (void *pBuf, uint32 ui32BufSize);

    void deallocateNullTerminatedPtrArray (char **ptrArray);
}

#endif   // #ifndef INCL_NLF_LIB_H
