/*
 * LocalNodeContext.h
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
 */

#ifndef INCL_LOCAL_NODE_CONTEXT_H
#define INCL_LOCAL_NODE_CONTEXT_H

#include "NodeContextImpl.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class Writer;
}

namespace IHMC_C45
{
    class Classifier;
    class C45AVList;
}

namespace IHMC_ACI
{
    class MetadataConfigurationImpl;

    class LocalNodeContext : public NodeContextImpl
    {
        public:
            static const char * TEAM_ID_PROPERTY;
            static const char * MISSION_ID_PROPERTY;
            static const char * ROLE_PROPERTY;
            static const char * NODE_TYPE_PROPERTY;
            static const char * LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY;
            static const char * MATCHMAKING_QUALIFIERS;

            virtual ~LocalNodeContext (void);

            static LocalNodeContext * getInstance (const char *pszNodeId,
                                                   NOMADSUtil::ConfigManager *pCfgMgr,
                                                   MetadataConfigurationImpl *pMetadataConf);

            int addCustomPolicies (const char **ppszCustomPoliciesXML);
            int addCustomPolicy (CustomPolicyImpl *pPolicy);

            /**
             * This information may change during time.
             *
             * NOTE: "nullptr" is a valid value.  If nullptr is passed to any of the
             *       parameters, the parameter is set to nullptr, even if it was set
             *       to a non-nullptr value.
             */
            int configure (NOMADSUtil::ConfigManager *pCfgMgr);
            int configureMetadataRanker (float coordRankWeight, float timeRankWeight,
                                         float expirationRankWeight, float impRankWeight,
                                         float sourceReliabilityRankWeigth,
                                         float informationContentRankWeigth,
                                         float predRankWeight, float targetWeight,
                                         bool bStrictTarget, bool bConsiderFuturePathSegmentForMatchmacking);
            void configureNodeContext (const char *pszTeamId, const char *pszMissionId, const char *pszRole);
            int parseAndSetUsefulDistanceByType (const char *pszUsefulDistanceValues);

            // get methods

            uint16 getClassifierVersion (void);

            int64 getStartTime (void) const;
            int getCurrentLatitude (float &latitude);
            int getCurrentLongitude (float &longitude);
            int getCurrentTimestamp (uint64 &timestamp);
            int getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                    const char *&pszLocation, const char *&pszNote,
                                    uint64 &timeStamp);
            LocationInfo * getLocationInfo (void);

            bool setCurrentPosition (float latitude, float longitude, float altitude,
                                     const char *pszLocation, const char *pszNote,
                                     uint64 timeStamp);

            bool setMatchmakingThreshold (float fMatchmakingThreshold);

            /**
             * Returns the specified path if it exists. Returns nullptr otherwise.
             * Returns the current path if exists.
             */
            IHMC_VOI::NodePath * getPath (const char *pszPathId);
            IHMC_VOI::NodePath * getPath (void);

            bool isPeerActive (void);

            // modify the node context
            void setBatteryLevel (uint8 ui8BatteryLevel);
            void setMemoryAvailable (uint8 ui8MemoryAvailable);
            void setMissionId (const char *pszMissionId);
            void setTeam (const char *pszTeam);
            void setRole (const char *pszRole);
            void setNodeType (const char *pszType);
            void setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            void setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);
            void setRangeOfInfluence (const char *pszNodeType, uint32 ui32RangeOfInfluenceInMeters);

            /**
             * "probability" is the probability that the user will that path.
             * It must be in the range [0 - 1] or the default value 0 is set.
             */
            int setPathProbability (const char *pszPathID, float probability);

            /**
             * Set as actual path one of the paths. Returns -1 in case of error,
             * 0 otherwise.
             */
            int setCurrentPath (const char *pszPathId);

            /**
             * Returns 0 if the path id added with success, returns -1 otherwise
             */
            int addPath (IHMC_VOI::NodePath *pNodePath);

            /**
             * Returns -1 if the specified path id doesn't exist or if you are
             * trying to delete the current path or the past path.
             * If there are no errors returns 0.
             */
            int deletePath (const char *pszPathId);

            /**
             * Use the given dataset as input for the classifier. In case of
             * error returns -1, else returns the new version of the classifier.
             */
            int updateClassifier (IHMC_C45::C45AVList *pDataset);

        protected:
            LocalNodeContext (const char *pszNodeID, IHMC_C45::Classifier *pClassifier,
                              double dTooFarCoeff, double dApproxCoeff);
    };
}

#endif // INCL_LOCAL_NODE_CONTEXT_H

