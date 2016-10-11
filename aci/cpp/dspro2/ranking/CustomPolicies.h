/* 
 * CustumRanker.h
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
 * Created on February 12, 2015, 7:09 PM
 */

#ifndef INCL_CUSTUM_RANKER_H
#define	INCL_CUSTUM_RANKER_H

#include "Match.h"

#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

class TiXmlElement;

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class MetadataInterface;

    class CustomPolicy
    {
        public:
            enum Type
            {
                STATIC   = 0x01,
                DISTANCE = 0x02,
                ONTOLOGY = 0x03
            };

            virtual ~CustomPolicy (void);

            virtual int init (TiXmlElement *pXmlField) = 0;

            const char * getAttribute (void) const;
            float getRankWeight (void) const;
            Type getType (void) const;
            int operator == (const CustomPolicy &rhsPolicy) const;

            virtual Match rank (MetadataInterface *pMetadata) = 0;

            virtual int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen, bool bSkip);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 i32MaxLen);
            virtual int writeLength (void);

        protected:
            CustomPolicy (Type type);
            CustomPolicy (Type type, float rankWeight, const NOMADSUtil::String &attributeName);

        private:
            const Type _type;
            float _fRankWeight;

        protected:               
            NOMADSUtil::String _attributeName;
    };

    class CustomPolicies : public NOMADSUtil::PtrLList<CustomPolicy>
    {
        public:
            CustomPolicies (void);
            ~CustomPolicies (void);

            int add (const char *pszCustomPolicyXML);
            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            void prepend (CustomPolicy *pPolicy);
            void removeAll (void);

            int skip (NOMADSUtil::Reader *pReader, uint32 i32MaxLen);
            int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen);
            int write (NOMADSUtil::Writer *pWriter, uint32 i32MaxLen);
            int getWriteLength (void);

        private:
            // Hide these functions
            void append (CustomPolicy *pel);
            void insert (CustomPolicy *pel);
            int readInternal (NOMADSUtil::Reader *pReader, uint32 i32MaxLen, bool bSkip);
            CustomPolicy * remove (CustomPolicy *pel);
            CustomPolicy * getTail (void);
            CustomPolicy * search (const CustomPolicy * const pel) const;

        private:
            uint8 _ui8Count;
    };

    class StaticPolicy : public CustomPolicy
    {
        public:
            StaticPolicy (void);
            StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName);
            ~StaticPolicy (void);

            int init (TiXmlElement *pXmlField);

            Match rank (MetadataInterface *pMetadata);

            int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen, bool bSkip);
            int write (NOMADSUtil::Writer *pWriter, uint32 i32MaxLen);
            int writeLength (void);

        private:
            struct Rank
            {
                Rank (float rank);
                ~Rank (void);

                const float _rank;
            };
            NOMADSUtil::StringHashtable<Rank> _valueToRank;
    };
}

#endif	/* CUSTUMRANKER_H */

