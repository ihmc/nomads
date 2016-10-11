/* 
 * MetaDataRankerConfiguration.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 *
 * Author: Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on July 5, 2013, 12:39 PM
 */

#ifndef INCL_METADATA_RANKER_CONFIGURATION_H
#define	INCL_METADATA_RANKER_CONFIGURATION_H

#include "SymbolCodeTemplateTable.h"

#include "StringHashtable.h"
#include "StringStringWildMultimap.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class LocalNodeContext;

    struct MetadataRankerConfiguration
    {
        /**
         * Configure MetaDataRankerConfiguration with the configuration in
         * pConfigManager.
         * If a property is not found, its value is set to the default value
         * (refer to the previous constructor's comment).
         * If pConfigManager is NULL, the default configuration is used.
         * The default configuration is as follows:
         * - all the ranks are set to 1.0
         * - strict target = false (if set to true, when the target is specified,
         *   all the peer node context of peer != target will have rank 0).
         * - future path segments are considered
         */
        MetadataRankerConfiguration (NOMADSUtil::ConfigManager *pCfgMgr = NULL);
        ~MetadataRankerConfiguration (void);

        int configure (NOMADSUtil::ConfigManager *pCfgMgr);
        int configure (float coordRankWeight, float timeRankWeight,
                       float expirationRankWeight, float impRankWeight,
                       float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                       float predRankWeight, float targetWeight, bool bStrictTarget,
                       bool bConsiderFuturePathSegmentForMatchmacking);

        void display (void);
        uint32 getLength (void);

        int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxLen);
        int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxLen);

        // Ranking Weights
        float _fCoordRankWeight;              // Coordinate Ranking Weight
        float _fTimeRankWeight;               // Time Ranking Weight
        float _fExpirationRankWeight;         // Expiration Ranking Weight
        float _fImpRankWeight;                // Importance Ranking Weight
        float _fSourceReliabilityRankWeigth;  // Source Reliability Ranking Weight
        float _fInformationContentRankWeigth; // Information Content Ranking Weight
        float _fPredRankWeight;               // Prediction Ranking Weight
        float _fTargetRankWeight;             // Prediction Ranking Weight

        bool _bStrictTarget;
        bool _bConsiderFuturePathSegmentForMatchmacking;

        private:
            void logConfig (void);
    };

    struct MetadataRankerLocalConfiguration
    {
        MetadataRankerLocalConfiguration (const char *pszNodeId, LocalNodeContext *pLocalNodeCtxt);
        ~MetadataRankerLocalConfiguration (void);

        int init (NOMADSUtil::ConfigManager *pCfgMgr);

        uint32 getRangeOfInfluence (IHMC_MISC_MIL_STD_2525::SymbolCode &milSTD2525Symbol);
        bool getLimitToLocalMatchmakingOnly (void) const;

        static const float MAX_RANK;
        static const float MIN_RANK;
        static const float DEF_RANK;    // Default rank

        bool _bInstrumented;
        const NOMADSUtil::String _nodeId;
        NOMADSUtil::StringHashset _dataTypesToFilter; // this metadata type will not be matched
        NOMADSUtil::StringStringWildMultimap _dataTypesNotToMatchByNodeId;

        private:
            void addRangeOfInfluence (IHMC_MISC_MIL_STD_2525::SymbolCodeTemplate &milSTD2525SymbolTemplate,
                                      uint32 ui32RangeOfInfluence);

        private:
            static const char * NON_MATCHING_DATA_TYPES;
            static const char * FILTER_MATCHMAKING_BY_PEER;
            static const char * RANGE_OF_INFLUENCE_BY_MILSTD2525_SYMBOL_CODE;

            LocalNodeContext *_pLocalNodeCtxt;
            IHMC_MISC_MIL_STD_2525::SymbolCodeTemplateTable _symbols;
            NOMADSUtil::StringHashtable<uint32> _symbolCodeToRangeOfInfluence;
    };
}

#endif	/* INCL_METADATA_RANKER_CONFIGURATION_H */

