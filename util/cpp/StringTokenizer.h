/*
 * StringTokenizer.h
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
 * C++ Header file that implements the Tokenizer class
 *
 * The Tokenizer class is used to retrieve tokens from a string.
 */

#ifndef INCL_STRING_TOKENIZER_H
#define INCL_STRING_TOKENIZER_H

#include <stdlib.h>
#include <string.h>

namespace NOMADSUtil
{
    class StringTokenizer
    {
        public:
             StringTokenizer (void);
             StringTokenizer (const char *pszSource, char chLeftDelimiter = (char) 255,
                              char chRightDelimiter = (char) 255);
             ~StringTokenizer (void);
             int init (const char *pszSource, char chLeftDelimiter = (char) 255,
                       char chRightDelimiter = (char) 255);
             void setDelimiter (char chDelimiter);
             void setLeftDelimiter (char chLeftDelimiter);
             void setRightDelimiter (char chRightDelimiter);
             const char * getNextToken (void);
        private:
             char *pszBuffer;
             char *pszNextToken;
             char chLeftDelim;
             char chRightDelim;
    };
}

#endif // #ifndef INCL_STRING_TOKENIZER_H
