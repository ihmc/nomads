/*
 * MPEG1Parser.h
 *
 * This file is part of the IHMC Misc Media Library
 * Copyright (c) 2015-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#ifndef INCL_MPEG_1_PARSER_H
#define INCL_MPEG_1_PARSER_H

#include <stdio.h>
#include "FTypes.h"

namespace IHMC_MISC
{
    class MPEG1Parser
    {
        public:
            MPEG1Parser (void);
            virtual ~MPEG1Parser (void);

            virtual int64 getSize (void) = 0;

            virtual int goToNextSequence (void) = 0;
            virtual int goToNextSequenceGOP (void) = 0;
            virtual int goToNextGOPFrame (void) = 0;

            virtual int readAll (void *pBuf, unsigned int uiBufLen) = 0;

            virtual int readToNextSequence (void *pBuf, unsigned int uiBufLen) = 0;
            virtual int readToNextSequenceGOP (void *pBuf, unsigned int uiBufLen) = 0;
            virtual int readToNextGOPFrame (void *pBuf, unsigned int uiBufLen) = 0;

            virtual int readToEOF (void *pBuf, unsigned int uiBufLen) = 0;

            virtual int seek (long ui64Pos) = 0;
            virtual int reset (void) = 0;
    };

    class MPEG1ParserFactory
    {
        public:
            static MPEG1Parser * newParser (const char *pszFileName);
            static MPEG1Parser * newParser (FILE *pFile, int64 i64FileSize);
            static MPEG1Parser * newParser (const void *pBuf, uint32 ui32Size);
    };
}

#endif    /* INCL_MPEG_1_PARSER_H */

