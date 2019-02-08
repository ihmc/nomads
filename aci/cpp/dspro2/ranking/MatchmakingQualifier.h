/*
 * MatchmakingQualifier.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on May 13, 2013, 10:40 PM
 */

#ifndef INCL_MATCHMAKING_QUALIFIER_H
#define INCL_MATCHMAKING_QUALIFIER_H

#include "FTypes.h"
#include "PtrLList.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class MatchmakingQualifier
    {
        public:
            MatchmakingQualifier (void);
            MatchmakingQualifier (const char *pszAttribute, const char *pszValue,
                                  const char *pszOperation);
            virtual ~MatchmakingQualifier (void);

            const char * getAttribute (void);
            const char * getValue (void);
            char * getAsString (void);

            static int parseQualifier (const char *pszQualifier, MatchmakingQualifier * &pMatchmakingQualifier);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

            bool operator == (const MatchmakingQualifier &rhMq);

        public:
            static const NOMADSUtil::String EQUALS;
            static const NOMADSUtil::String GREATER;
            static const NOMADSUtil::String LESS;
            static const NOMADSUtil::String MATCH;
            static const NOMADSUtil::String TOP;

            NOMADSUtil::String _attribute;
            NOMADSUtil::String _value;
            NOMADSUtil::String _operation;
    };

    class ComplexMatchmakingQualifier
    {
        public:
            ComplexMatchmakingQualifier (void);
            ComplexMatchmakingQualifier (NOMADSUtil::PtrLList<MatchmakingQualifier> &qualifiers);
            ~ComplexMatchmakingQualifier (void);

            int addQualifier (const char *pszAttribute, const char *pszValue,
                              const char *pszOperation);
            const char * getDataFormat (void);
            char * getAsString (void);

            static int parseQualifier (const char *pszLine,
                                       ComplexMatchmakingQualifier * &pComplexMatchmakingQualifier);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        public:
            NOMADSUtil::PtrLList<MatchmakingQualifier> _qualifiers;
    };

    class MatchmakingQualifiers
    {
        public:
            MatchmakingQualifiers (void);
            ~MatchmakingQualifiers (void);

            char * getAsString (void);
            int parseAndAddQualifiers (const char *pszLine);

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);

        public:
            NOMADSUtil::PtrLList<ComplexMatchmakingQualifier> _qualifiers;
    };

    inline const char * MatchmakingQualifier::getAttribute (void)
    {
        return _attribute.c_str();
    }

    inline const char * MatchmakingQualifier::getValue (void)
    {
        return _attribute.c_str();
    }
}

#endif    /* INCL_MATCHMAKING_QUALIFIER_H */
