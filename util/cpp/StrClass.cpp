/*
 * StrClass.cpp
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
 */

#include "StrClass.h"

#include "NLFLib.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;

String::String (void)
{
    pszBuf = NULL;
}

String::String (unsigned short usSize)
{
    pszBuf = (char*) malloc (usSize+1);
    pszBuf[0] = '\0';
}

String::String (const char *pszStr)
{
    if (pszStr) {
        pszBuf = strdup (pszStr);
    }
    else {
        pszBuf = NULL;
    }
}

String::String (const String &sourceStr)
{
    if (sourceStr.pszBuf) {
        pszBuf = strdup (sourceStr.pszBuf);
    }
    else {
        pszBuf = NULL;
    }
}

String::~String (void)
{
    if (pszBuf) {
        free (pszBuf);
        pszBuf = NULL;
    }
}

String::String (const char *pszStr, unsigned short usCount)
{
    if (pszStr) {
        pszBuf = (char*) malloc (usCount+1);
        strncpy (pszBuf, pszStr, usCount);
        pszBuf[usCount] = '\0';
    }
    else {
        pszBuf = NULL;
    }
}

String String::operator + (const String &str) const
{
    return ((*this) + str.pszBuf);
}

String String::operator + (const char *pszStr) const
{
    String newStr (*this);
    newStr += pszStr;
    return newStr;
}

String String::operator + (char c) const
{
    String newStr (*this);
    newStr += c;
    return newStr;
}

String & String::operator = (const char *pszStr)
{
    if (pszBuf) {
        free (pszBuf);
        pszBuf = NULL;
    }
    if (pszStr) {
        pszBuf = strdup (pszStr);
    }

    return *this;
}

String & String::operator += (const char *pszStr)
{
    if (pszStr) {
        if (pszBuf) {
            pszBuf = (char *) realloc (pszBuf, strlen (pszBuf) + strlen (pszStr) + 1);
            strcat (pszBuf, pszStr);
        }
        else {
            pszBuf = strdup (pszStr);
        }
    }

    return *this;
}

String & String::operator += (char c)
{
    if (pszBuf) {
        unsigned int uiOldLen = strlen (pszBuf);
        pszBuf = (char *) realloc (pszBuf, uiOldLen + 1 + 1);
        pszBuf[uiOldLen] = c;
        pszBuf[uiOldLen+1] = '\0';
    }
    else {
        pszBuf = (char*) malloc (2);
        pszBuf[0] = c;
        pszBuf[1] = '\0';
    }
    return *this;
}

String & String::operator += (uint32 ui32)
{
    char szBuf[12];
    itoa (szBuf, ui32);
    return this->operator += (szBuf);
}

char & String::operator [] (int iPos) const
{
    static char junk = '\0';
    if (!pszBuf) {
         return junk;
    }
    int iLength;
    iLength = (int) strlen (pszBuf);
    if ((iPos >= 0) && (iPos <= iLength)) {
        return pszBuf [iPos];
    }
    else if (iPos < 0) {
        iPos = -iPos;
        if ((iLength - iPos) >= 0) {
            return pszBuf [iLength - iPos];
        }
    }
    return junk;
}

int String::operator == (const char *pszrhsStr) const
{
    const char *p = pszBuf, *q = pszrhsStr;
    if ((!p) && (!q)) {
        return 1;
    }
    else if ((!p) || (!q)) {
        return 0;
    }
    for (; *p == *q; p++, q++) {
        if (*p == '\0') {
            return 1;
        }
    }
    return 0;
}

int String::operator ^= (const char *pszrhsStr) const
{
    const char *p = pszBuf, *q = pszrhsStr;
    if ((!p) && (!q)) {
        return 1;
    }
    else if ((!p) || (!q)) {
        return 0;
    }

    for (; toupper(*p) == toupper(*q); p++, q++) {
        if (*p == '\0') {
            return 1;
        }
    }
    return 0;
}

void String::setSize (int iNewSize)
{
    if (pszBuf) {
        pszBuf = (char*) realloc (pszBuf, iNewSize+1);
        pszBuf [iNewSize] = '\0';
    }
    else {
        pszBuf = (char*) malloc (iNewSize+1);
        pszBuf [0] = '\0';
    }
}

//Removes white space from both ends of this string. The following character values are
//all white spaces: "\t" (0x09 - HORIZONTAL TABULATION),
//                  "\n" (0x0A - NEW LINE),
//                  "\v" (0x0B - VERTICAL SPACE),
//                  "\f" (0x0C - FORM FEED),
//                  "\r" (0x0D - CARRIAGE RETURN), and
//                   " " (0x20 - SPACE).
int String::trim (void)
{
    int i;
    int offset = 0;

    if (pszBuf == NULL) {
        return -1;
    }

    // ----
    //char *pszTemp = pszBuf;
    for (i = (int) strlen(pszBuf) - 1; i >= 0; i--) {
        if (isspace (pszBuf[i])) {
            pszBuf[i] = 0;
        }
        else {
            break;
        }
    }
    
    for (i = 0; i < (int) strlen(pszBuf) - 1; i++) {
        if (isspace(pszBuf[i])) {
            offset++;
        }
        else {
            break;
        }
    }

    if (offset > 0) {
        for (i = 0; i < (int)strlen(pszBuf) - offset; i++) {
            pszBuf[i] = pszBuf[i + offset];
        }
        pszBuf[i] = 0;
    }
    // ----

/*
    for (i = 0; i < (int) strlen (pszBuf); i++) {
        if (isspace (pszBuf[i])) {
            pszBuf++;
        }
        else
            break;
    }
    j = (int) strlen (pszBuf) - 1;
    char *pszTemp = pszBuf;
    for (i = j; i > -1; i--) {
        if (isspace (pszTemp[i])) {
            pszTemp[i] = 0;
        }
        else
            break;
    }
*/
    return 0;
}

int String::length (void) const
{
    if (pszBuf) {
        return (int) strlen (pszBuf);
    }
    else {
        return -1;
    }
}

bool String::contains (const char *pszStr) const
{
    return (indexOf (pszStr) >= 0);
}

int String::convertToLowerCase (void)
{
    if (pszBuf == NULL) {
        return -1;
    }
    for (char *pszTemp = pszBuf; *pszTemp; pszTemp++) {
        *pszTemp = tolower (*pszTemp);
    }
    return 0;
}

int String::convertToUpperCase (void)
{
    if (pszBuf == NULL) {
        return -1;
    }
    for (char *pszTemp = pszBuf; *pszTemp; pszTemp++) {
        *pszTemp = toupper (*pszTemp);
    }
    return 0;
}

int String::indexOf (char ch) const
{
    if (pszBuf == NULL) {
        return -1;
    }
    for (unsigned int i = 0; pszBuf[i] != '\0'; i++) {
        if (pszBuf[i] == ch) {
            return i;
        }
    }
    return -2;
}

int String::indexOf (const char *pszStr) const
{
    const char *p = pszBuf, *q = pszStr;
    if (p && q) {
        char* pch = strstr (pszBuf, pszStr);
        if (pch != NULL) {
            return (int)(pch - pszBuf);
        }
    }

    return -1;
}

int String::lastIndexOf (char ch) const
{
    if (pszBuf == NULL) {
        return -1;
    }
    int iIndex = strlen (pszBuf) - 1;
    for (; iIndex >= 0; iIndex--) {
        if (pszBuf[iIndex] == ch) {
            return iIndex;
        }
    }
    return -2;
}

String String::substring (int beginIndex, int endIndex) const
{
    String newStr;
    if (pszBuf && (beginIndex < endIndex)) {
        newStr += &pszBuf[beginIndex];
        newStr.setSize(endIndex-beginIndex);
    }
    return newStr;
}

int String::startsWith (const char *pszStr) const
{
    const char *p = pszBuf, *q = pszStr;
    if (p && q) {
        if (strncmp(pszBuf, pszStr, strlen(pszStr)) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int String::endsWith (const char *pszStr) const
{
    // return 1 if the current string ends with the string in pszStr
    int res = 0;

    if ( (pszBuf != NULL) && (pszStr != NULL) ) {
        if (strlen(pszStr) > strlen(pszBuf)) {
            return 0;
        }
        char *pAux = pszBuf + strlen(pszBuf) - strlen(pszStr);
        if (strncmp (pAux, pszStr, sizeof(pszStr)) == 0) {
            res = 1;
        }
    }

    return res;
}

char * String::r_str()
{
    char *pszRet = pszBuf;
    pszBuf = NULL;
    return pszRet;
}

char * String::strdup (const char *pszStr) const
{
    if (pszStr == NULL) {
        return NULL;
    }
    char *pszCopy = (char*) malloc (strlen(pszStr)+1);
    strcpy (pszCopy, pszStr);
    return pszCopy;
}
