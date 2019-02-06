/*
 * StringTokenizer.cpp
 *
 * This file is part of the IHMC Util Library
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
 *
 * The StringTokenizer class is used to retrieve tokens from a string.
 *
 */

#include "StringTokenizer.h"

#include <stdio.h>

#if defined (WIN32)
    #define strdup _strdup
#endif

using namespace NOMADSUtil;

StringTokenizer::StringTokenizer (void)
{
    pszBuffer = pszNextToken = NULL;
}

StringTokenizer::StringTokenizer (const char *pszSource, char chLeftDelimiter,
                                  char chRightDelimiter)
{
    pszBuffer = pszNextToken = NULL;
    init (pszSource, chLeftDelimiter, chRightDelimiter);
}

StringTokenizer::~StringTokenizer (void)
{
    if (pszBuffer) {
        free (pszBuffer);
        pszBuffer = pszNextToken = NULL;
    }
}

int StringTokenizer::init (const char *pszSource, char chLeftDelimiter,
                           char chRightDelimiter)
{
    if (pszBuffer) {
        free (pszBuffer);
        pszBuffer = NULL;
    }
    pszBuffer = strdup (pszSource?pszSource:"");
    pszNextToken = pszBuffer;
    chLeftDelim = chLeftDelimiter;
    chRightDelim = chRightDelimiter;
    return 0;
}

void StringTokenizer::setDelimiter (char chDelimiter)
{
    chLeftDelim = chDelimiter;
    chRightDelim = (char) 255;
}

void StringTokenizer::setLeftDelimiter (char chLeftDelimiter)
{
    chLeftDelim = chLeftDelimiter;
}

void StringTokenizer::setRightDelimiter (char chRightDelimiter)
{
    chRightDelim = chRightDelimiter;
}

const char * StringTokenizer::getNextToken (void)
{
    char chEnd, *pszCurrToken;
    if (!pszBuffer) {
        return NULL;
    }
    while (pszNextToken [0] == ' ') {
        pszNextToken++;
    }
    if (pszNextToken [0] == '\0') {
        return NULL; // Not really required
    }
    if ((chLeftDelim != (char) 255) && (pszNextToken [0] == chLeftDelim)) {
        pszNextToken++;
    }
    if (pszNextToken [0] == '\0') {
        return NULL;
    }
    pszCurrToken = pszNextToken;
    if (chRightDelim != (char) 255) {
        chEnd = chRightDelim;
    }
    else if (chLeftDelim != (char) 255) {
        chEnd = chLeftDelim;
    }
    else {
        chEnd = ' ';
    }
    while ((pszNextToken [0] != '\0') && (pszNextToken [0] != chEnd)) {
        pszNextToken++;
    }
    if (pszNextToken [0] != '\0') {
        pszNextToken [0] = '\0';
        pszNextToken++;
    }
    if (pszCurrToken == pszNextToken) {
        return NULL;
    }
    return pszCurrToken;
}
