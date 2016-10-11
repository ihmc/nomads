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

#include "DArray2.h"
#include "NodeContext.h"

namespace NOMADSUtil
{
    class ConfigManager;
}

namespace IHMC_C45
{
    class Classifier;
    class C45AVList;
}

namespace IHMC_ACI
{
    class MetadataConfiguration;

    class LocalNodeContext : public NodeContext
    {
        public:
            static const char * TEAM_ID_PROPERTY;
            static const char * MISSION_ID_PROPERTY;
            static const char * ROLE_PROPERTY;
            static const char * USEFUL_DISTANCE_PROPERTY;
            static const char * USEFUL_DISTANCE_BY_TYPE_PROPERTY;
            static const char * LIMIT_PRESTAGING_TO_LOCAL_DATA_PROPERTY;
            static const char * MATCHMAKING_QUALIFIERS;

            virtual ~LocalNodeContext (void);

            static LocalNodeContext * getInstance (const char *pszNodeId,
                                                   NOMADSUtil::ConfigManager *pCfgMgr,
                                                   MetadataConfiguration *pMetadataConf);

            int addCustomPolicies (const char **ppszCustomPoliciesXML);

            /**
             * This information may change during time.
             *
             * NOTE: "NULL" is a valid value.  If NULL is passed to any of the
             *       parameters, the parameter is set to NULL, even if it was set
             *       to a non-NULL value.
             */
            int configure (NOMADSUtil::ConfigManager *pCfgMgr);
            int configureMetadataRanker (NOMADSUtil::ConfigManager *pCfgMgr);
            int configureMetadataRanker (float coordRankWeight, float timeRankWeight,
                                         float expirationRankWeight, float impRankWeight,
                                         float sourceReliabilityRankWeigth,
                                         float informationContentRankWeigth,
                                         float predRankWeight, float targetWeight,
                                         bool bStrictTarget, bool bConsiderFuturePathSegmentForMatchmacking);
            void configureNodeContext (const char *pszTeamID, const char *pszMissionID, const char *pszRole);
            int parseAndSetUsefulDistanceByType (const char *pszUsefulDistanceValues);

            // get methods

            uint16 getClassifierVersion (void);

            int getCurrentLatitude (float &latitude);
            int getCurrentLongitude (float &longitude);
            int getCurrentTimestamp (uint64 &timestamp);
            int getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                    const char *&pszLocation, const char *&pszNote,
                                    uint64 &timeStamp);
            bool setCurrentPosition (float latitude, float longitude, float altitude,
                                     const char *pszLocation, const char *pszNote,
                                     uint64 timeStamp);

            /**
             * Returns the specified path if it exists. Returns NULL otherwise.
             * Returns the current path if exists.
             */
            NodePath * getPath (const char *pszPathID);
            NodePath * getPath (void);

            bool isPeerActive (void);

            // modify the node context
 
            void setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            void setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);

            /**
             * "probability" is the probability that the user will that path.
             * It must be in the range [0 - 1] or the default value 0 is set.
             */
            int setPathProbability (const char *pszPathID, float probability);

            /**
             * Set as actual path one of the paths. Returns -1 in case of error,
             * 0 otherwise.
             */
            int setCurrentPath (const char *pszPathID);

            /**
             * Returns 0 if the path id added with success, returns -1 otherwise
             */
            int addPath (NodePath *pNodePath);

            /**
             * Returns -1 if the specified path id doesn't exist or if you are
             * trying to delete the current path or the past path.
             * If there are no errors returns 0.
             */
            int deletePath (const char *pszPathID);

            /**
             * Use the given dataset as input for the classifier. In case of
             * error returns -1, else returns the new version of the classifier.
             */
            int updateClassifier (IHMC_C45::C45AVList *pDataset);

        protected:
            LocalNodeContext (const char *pszNodeID, IHMC_C45::Classifier *pClassifier,
                              double dTooFarCoeff, double dApproxCoeff);

            // The position at which the path that has been
            // actually covered by the node (may be
            // different than the registered path(s)) is
            // stored in _pPaths.
            // It could be used to adjust the prediction
            // parameters, and off line, as an important
            // piece of information about the deployment.
            static const unsigned short int ACTUAL_COVERED_PATH_INDEX;

            NOMADSUtil::DArray2<NodePath *> *_pPaths; // Collect the paths of the node.
            uint16 _ui16PathsNumber; // Number of paths present in the array.
            int _iCurrPath;          // Index of the current path.
    };
}

#endif // INCL_LOCAL_NODE_CONTEXT_H

