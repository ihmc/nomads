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

#ifndef INCL_METADATA_RANKER_LOCAL_CONFIGURATION_H
#define	INCL_METADATA_RANKER_LOCAL_CONFIGURATION_H

#include "StringStringWildMultimap.h"

namespace IHMC_VOI
{
    struct MetadataRankerLocalConfiguration
    {
        static const float MAX_RANK;
        static const float MIN_RANK;
        static const float DEF_RANK;    // Default rank

        MetadataRankerLocalConfiguration (const char *pszNodeId);
        virtual ~MetadataRankerLocalConfiguration (void);

        /**
         * Messages of pszDataType will not be matched for pszPeerNodeId.
         * If pszPeerNodeId is NULL, then messages of pszDataType will not
         * be matched for any peer.
         */
        void addFilter (const char *pszDataType, const char *pszPeerNodeId = NULL);
        virtual bool getLimitToLocalMatchmakingOnly (void) = 0;
        bool hasFilterForTypeAndPeer (const char *pszNodeId, const char *pszDataType);

        NOMADSUtil::String getRangeOfInfluenceAttributeName (void);
        void setRangeOfInfluenceAttributeName (const NOMADSUtil::String &rangeOfInfluenceKeyAttributeName);

        bool _bInstrumented;
        const NOMADSUtil::String _nodeId;

        struct TrackVoIConf
        {
            TrackVoIConf (void);
            ~TrackVoIConf (void);

            static const float DEFAULT_INSIGNIFICANT_MOVEMENT_PERC;

            /**
             * Percentage of the moving vector's module under which the movement
             * is considerend insignificant
             */
            bool enabled (void);
            float getInsignificantTrackMovementFactor (void) const;
            void setInsignificantTrackMovementFactor (float fPerc);

            private:
                bool _enable;
                float _fInsignificantMovementPerc;
        };

        struct LogStatVoIConf
        {
            LogStatVoIConf (void);
            ~LogStatVoIConf (void);

            static const float DEFAULT_INSIGNIFICANT_UPDATE_PERC;

            /**
             * Percentage of the moving vector's module under which the movement
             * is considerend insignificant
             */
            bool enabled (void);
            float getInsignificantUpdatePerc (void) const;
            void setInsignificantUpdatePerc (float fPerc);

            private:
                bool _enable;
                float _fInsignificantUpdatePerc;
        };

        TrackVoIConf _track;
        LogStatVoIConf _logStat;

        private:
            NOMADSUtil::String _rangeOfInfluenceKeyAttributeName;
            NOMADSUtil::StringHashset _dataTypesToFilter; // this metadata type will not be matched
            NOMADSUtil::StringStringWildMultimap _dataTypesNotToMatchByNodeId;
    };
}

#endif  /* INCL_METADATA_RANKER_LOCAL_CONFIGURATION_H */

