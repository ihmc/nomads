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

#include "PtrLList.h"
#include "StrClass.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class MetadataInterface;

    class CustumPolicy
    {
        public:
            enum Type
            {
                STATIC   = 0x01,
                DISTANCE = 0x02,
                ONTOLOGY = 0x03
            };

            virtual ~CustumPolicy (void);

            float getRankWeight (void) const;
            Type getType (void) const;

            /**
             * Returns a value between 0.0 and 10.0
             */
            virtual float rank (MetadataInterface *pMetadata) = 0;

            virtual int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen);
            virtual int write (NOMADSUtil::Writer *pWriter, uint32 i32MaxLen);
            virtual int writeLength (void);

        protected:
            CustumPolicy (Type type);
            CustumPolicy (Type type, float rankWeight, const NOMADSUtil::String &attributeName);

        private:
            const Type _type;
            float _rankWeight;

        protected:               
            NOMADSUtil::String _attributeName;
    };

    class CustumPolicies : public NOMADSUtil::PtrLList<CustumPolicy>
    {
        public:
            CustumPolicies (void);
            ~CustumPolicies (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            void prepend (CustumPolicy *pPolicy);

            int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen);
            int write (NOMADSUtil::Writer *pWriter, uint32 i32MaxLen);
            int getWriteLength (void);

        private:
            // Hide these functions
            void append (CustumPolicy *pel);
            void insert (CustumPolicy *pel);
            CustumPolicy * remove (CustumPolicy *pel);
            CustumPolicy * getTail (void);
            CustumPolicy * search (const CustumPolicy * const pel) const;

        private:
            uint8 _ui8Count;
    };

    class StaticPolicy : public CustumPolicy
    {
        public:
            StaticPolicy (void);
            StaticPolicy (float rankWeight, const NOMADSUtil::String &attributeName);
            ~StaticPolicy (void);

            float rank (MetadataInterface *pMetadata);

            int read (NOMADSUtil::Reader *pReader, uint32 i32MaxLen);
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

