/*
 * MetaDataRankerConfiguration.h
 *
 * This file is part of the IHMC Voi Library/Component
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

#include "StringStringWildMultimap.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class JsonObject;
}

namespace IHMC_VOI
{
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

        bool configure (NOMADSUtil::ConfigManager *pCfgMgr);
        bool configure (float coordRankWeight, float timeRankWeight,
                        float expirationRankWeight, float impRankWeight,
                        float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                        float predRankWeight, float targetWeight, bool bStrictTarget,
                        bool bConsiderFuturePathSegmentForMatchmacking);

        int fromJson (const NOMADSUtil::JsonObject *pJson);
        NOMADSUtil::JsonObject * toJson (void);

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
}

#endif	/* INCL_METADATA_RANKER_CONFIGURATION_H */

