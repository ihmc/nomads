/*
 * CustomPolicies.h
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
#define INCL_CUSTUM_RANKER_H

#include "CustomPolicy.h"
#include "Match.h"

#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"
#include "Json.h"

class TiXmlElement;

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
    class Writer;
}

namespace IHMC_VOI
{
    class MetadataInterface;
}

namespace IHMC_ACI
{
    class CustomPolicyImpl : public IHMC_VOI::CustomPolicy
    {
        public:
            enum Type
            {
                STATIC   = 0x01,
                DISTANCE = 0x02,
                ONTOLOGY = 0x03
            };

            virtual ~CustomPolicyImpl (void);

            virtual int init (TiXmlElement *pXmlField) = 0;

            const char * getAttribute (void) const;
            float getRankWeight (void) const;
            Type getType (void) const;
            int operator == (const CustomPolicyImpl &rhsPolicy) const;

            virtual IHMC_VOI::Match rank (IHMC_VOI::MetadataInterface *pMetadata) = 0;

            virtual int read (NOMADSUtil::Reader *pReader, bool bSkip);
            virtual int write (NOMADSUtil::Writer *pWriter);

            virtual int fromJson (NOMADSUtil::JsonArray *pMatches) = 0;
            virtual NOMADSUtil::JsonObject * toJson (void) const = 0;

        protected:
            explicit CustomPolicyImpl (Type type);
            CustomPolicyImpl (Type type, float rankWeight, const NOMADSUtil::String &attributeName);

        private:
            const Type _type;
            float _fRankWeight;

        protected:
            NOMADSUtil::String _attributeName;
    };

    class CustomPolicies : public NOMADSUtil::PtrLList<CustomPolicyImpl>
    {
        public:
            CustomPolicies (void);
            ~CustomPolicies (void);

            int add (CustomPolicyImpl *pPolicy);
            int add (const char *pszCustomPolicyXML);
            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            void prepend (CustomPolicyImpl *pPolicy);
            void removeAll (void);

            int skip (NOMADSUtil::Reader *pReader);
            int read (NOMADSUtil::Reader *pReader);
            int write (NOMADSUtil::Writer *pWriter);

            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            // Hide these functions
            void append (IHMC_VOI::CustomPolicy *pel);
            void insert (IHMC_VOI::CustomPolicy *pel);
            int readInternal (NOMADSUtil::Reader *pReader, bool bSkip);
            CustomPolicyImpl * remove (CustomPolicyImpl *pel);
            CustomPolicyImpl * getTail (void) const;
            CustomPolicyImpl * search (const CustomPolicyImpl * const pel) const;

        private:
            uint8 _ui8Count;
    };

    class StaticPolicy : public CustomPolicyImpl
    {
        public:
            StaticPolicy (void);
            StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName);
            ~StaticPolicy (void);

            int init (TiXmlElement *pXmlField);

            IHMC_VOI::Match rank (IHMC_VOI::MetadataInterface *pMetadata);

            int read (NOMADSUtil::Reader *pReader, bool bSkip);
            int write (NOMADSUtil::Writer *pWriter);

            int fromJson (NOMADSUtil::JsonArray *pMatches);
            NOMADSUtil::JsonObject * toJson (void) const;

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

#endif    /* CUSTUMRANKER_H */
