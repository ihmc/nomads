/**
 * MatchmakingInfo.h
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
 * Created on December 24, 2016, 8:23 PM
 */

#ifndef INCL_MATCHMAKING_INFO_H
#define INCL_MATCHMAKING_INFO_H

#include "CustomPolicies.h"
#include "MatchmakingQualifier.h"
#include "Part.h"
#include "RangeOfInfluence.h"
#include "UsefulDistance.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_VOI
{
    struct MetadataRankerConfiguration;
}

namespace IHMC_ACI
{
    class MatchmakingInfo : public Part
    {
        public:
            static const char * MATCHMAKING_INFO_OBJECT_NAME;

            MatchmakingInfo (void);
            ~MatchmakingInfo (void);

            int init (NOMADSUtil::ConfigManager *pCfgMgr);

            CustomPolicies * getCustomPolicies (void);
            bool getLimitToLocalMatchmakingOnly (void) const;
            float getMatchmakingThreshold (void) const;
            MatchmakingQualifiers * getMatchmakingQualifiers (void);
            IHMC_VOI::MetadataRankerConfiguration * getMetaDataRankerConfiguration (void) const;
            uint32 getUsefulDistance (const char *pszInformationMIMEType) const;
            uint32 getMaximumUsefulDistance (void);
            uint32 getRangeOfInfluence (const char *pszNodeType);
            uint32 getMaximumRangeOfInfluence (void);

            /*
             * Return true if the value was updated, false otherwise
             */
            bool setCustomPolicy (const char *pszCustomPoliciesXML);
            bool setCustomPolicy (CustomPolicyImpl *pPolicy);
            bool setLimitToLocalMatchmakingOnly (bool bLimitPrestagingToLocalData);
            bool setMatchmakingThreshold (float fMatchmakingThreshold);
            bool setMetadataRankerParameters (NOMADSUtil::ConfigManager *pCfgMgr);
            bool setMetadataRankerParameters (float coordRankWeight, float timeRankWeight,
                                              float expirationRankWeight, float impRankWeight,
                                              float sourceReliabilityRankWeigth, float informationContentRankWeigth,
                                              float predRankWeight, float targetWeight, bool bStrictTarget,
                                              bool bConsiderFuturePathSegmentForMatchmacking);
            bool setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            bool setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);
            bool setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters);

            void reset (void);

            /*
             * Serialization
             */
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            friend class MatchmakingInfoHelper;
            bool _bLimitPrestagingToLocalData;                                  // Limit matchmaking to objects published by local applications
            float _fMatchmakingThreshold;
            IHMC_VOI::MetadataRankerConfiguration *_pMetaDataRankerConf;
            UsefulDistance _usefulDistance;
            RangeOfInfluence _rangeOfInfluence;
            MatchmakingQualifiers _qualifiers;
            CustomPolicies _customPolicies;
    };

    class MatchmakingInfoHelper
    {
        public:
            static bool parseAndSetUsefulDistanceByType (MatchmakingInfo *pLocationInfo, const char *pszUsefulDistanceValues);
            static bool parseAndSetRangesOfInfluence (MatchmakingInfo *pLocationInfo, const char *pszRangeOfInfluence);
            static int parseAndAddQualifiers (MatchmakingInfo *pQualifiers, const char *pszLine);
    };
}

#endif  /* INCL_MATCHMAKING_INFO_H */
