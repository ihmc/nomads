/**
 * NodeContext.h
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
 * Created on November 12, 2010, 4:56 PM
 */

#ifndef INCL_NODE_CONTEXT_H
#define INCL_NODE_CONTEXT_H

#include "AreasOfIntInfo.h"
#include "MatchmakingQualifier.h"
#include "CustomPolicies.h"
#include "LocationInfo.h"
#include "MatchmakingInfo.h"
#include "NodeGenInfo.h"
#include "PathInfo.h"
#include "Versions.h"
#include "NodeContext.h"

namespace IHMC_C45
{
    class Classifier;
}

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_VOI
{
    class AdjustedPathWrapper;
    class NodePath;
    class Path;
    struct MetadataRankerConfiguration;
}

namespace IHMC_ACI
{
    class NodeContextImpl : public IHMC_VOI::NodeContext
    {
        public:
            virtual ~NodeContextImpl (void);

            virtual void display (void);

            void reset (void);
            int addUserId (const char *pszUserId);
            int addAreaOfInterest (const char *pszAreaName, NOMADSUtil::BoundingBox &bb,
                                   int64 i64StatTime, int64 int64EndTime);

            NOMADSUtil::String getNodeId (void) const;
            NOMADSUtil::String getTeamId (void) const;
            NOMADSUtil::String getMissionId (void) const;
            NOMADSUtil::String getRole (void) const;
            unsigned int getBatteryLevel (void) const;
            unsigned int getMemoryAvailable (void) const;
            bool getLimitToLocalMatchmakingOnly (void) const;
            float getMatchmakingThreshold (void) const;
            Versions getVersions (void);

            /**
             * Returns the current decision tree if it exists. Returns nullptr
             * otherwise.
             */
            IHMC_C45::Classifier * getClassifier (void) const;

            /**
             * Returns WAYPOINT_UNSET id the first position has not yet been set.
             * Returns the current way point in path if the node is exactly on
             * the way point or relatively close to it.
             * Returns the start index of the closest segment otherwise.
             */
            int getCurrentWayPointInPath (void) const;

            NodeStatus getStatus (void) const;

            float getClosestPointOnPathLatitude (void) const;
            float getClosestPointOnPathLongitude (void) const;

            /**
             * Return the latest position from/to the past path.
             *
             * NOTE: do not deallocate pszLocation and pszNote
             */
            virtual int getCurrentLatitude (float &latitude) = 0;
            virtual int getCurrentLongitude (float &longitude) = 0;
            virtual int getCurrentTimestamp (uint64 &timestamp) = 0;
            virtual int getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                            const char *&pszLocation, const char *&pszNote,
                                            uint64 &timeStamp) = 0;
            uint32 getRangeOfInfluence (const char *pszNodeType);
            uint32 getMaximumRangeOfInfluence (void);

            MatchmakingQualifiers * getMatchmakingQualifiers (void);
            IHMC_VOI::MetadataRankerConfiguration * getMetaDataRankerConfiguration (void) const;

            /**
             * Returns the current path if set, returns nullptr otherwise.
             */
            virtual IHMC_VOI::NodePath * getPath (void) = 0;

            /**
             * Returns the current path if set, enhanced with the segments to
             * return returns nullptr otherwise.
             */
            IHMC_VOI::AdjustedPathWrapper * getAdjustedPath (void);
            PositionApproximationMode getPathAdjustingMode (void) const;

            NOMADSUtil::PtrLList<IHMC_VOI::CustomPolicy> * getCustomPolicies (void);
            IHMC_VOI::AreaOfInterestList * getAreasOfInterest (void) const;

            //int64 getStartTime (void) const;

            uint32 getMaximumUsefulDistance (void);
            uint32 getUsefulDistance (const char *pszInformationMIMEType) const;

            bool hasUserId (const char *pszUserId);
            virtual bool isPeerActive (void) = 0;

            NOMADSUtil::JsonObject * toJson (const Versions *pIncomingVersions, bool bCurrwaypointOnly=false);

        protected:
            NodeContextImpl (const char *pszNodeId, double dTooFarCoeff, double dApproxCoeff);

            int64 _i64StartingTime;
            NodeGenInfo _nodeInfo;
            LocationInfo _locationInfo;
            MatchmakingInfo _matchmakingInfo;
            PathInfo _pathInfo;
            AreasOfInterestsInfo _areasOfInterestInfo;
            IHMC_C45::Classifier *_pClassifier;
    };

    inline NOMADSUtil::String NodeContextImpl::getNodeId (void) const
    {
        return _nodeInfo.getNodeId();
    }

    inline NOMADSUtil::String NodeContextImpl::getTeamId (void) const
    {
        return _nodeInfo.getTeamId();
    }

    inline NOMADSUtil::String NodeContextImpl::getMissionId (void) const
    {
        return _nodeInfo.getMisionId();
    }

    inline NOMADSUtil::String NodeContextImpl::getRole (void) const
    {
        return _nodeInfo.getRole();
    }

    inline unsigned int NodeContextImpl::getBatteryLevel (void) const
    {
        return _nodeInfo.getBatteryLevel();
    }

    inline unsigned int NodeContextImpl::getMemoryAvailable (void) const
    {
        return _nodeInfo.getMemoryAvailable();
    }

    inline bool NodeContextImpl::getLimitToLocalMatchmakingOnly (void) const
    {
        return _matchmakingInfo.getLimitToLocalMatchmakingOnly();
    }

    inline float NodeContextImpl::getMatchmakingThreshold (void) const
    {
        return _matchmakingInfo.getMatchmakingThreshold();
    }

    inline IHMC_C45::Classifier * NodeContextImpl::getClassifier (void) const
    {
        return _pClassifier;
    }

    inline int NodeContextImpl::getCurrentWayPointInPath (void) const
    {
        return _locationInfo.getCurrentWayPointInPath();
    }

    inline uint32 NodeContextImpl::getUsefulDistance (const char *pszInformationMIMEType) const
    {
        return _matchmakingInfo.getUsefulDistance (pszInformationMIMEType);
    }

    inline uint32 NodeContextImpl::getMaximumUsefulDistance (void)
    {
        return _matchmakingInfo.getMaximumUsefulDistance();
    }

    inline IHMC_VOI::MetadataRankerConfiguration * NodeContextImpl::getMetaDataRankerConfiguration (void) const
    {
        return _matchmakingInfo.getMetaDataRankerConfiguration();
    }

    inline MatchmakingQualifiers * NodeContextImpl::getMatchmakingQualifiers (void)
    {
        return _matchmakingInfo.getMatchmakingQualifiers();
    }

    inline NodeContextImpl::PositionApproximationMode NodeContextImpl::getPathAdjustingMode (void) const
    {
        return _locationInfo.getPathAdjustingMode();
    }

    inline IHMC_VOI::NodeContext::NodeStatus NodeContextImpl::getStatus (void) const
    {
        return _locationInfo.getStatus();
    }

    inline float NodeContextImpl::getClosestPointOnPathLatitude (void) const
    {
        return _locationInfo.getClosestPointOnPathLatitude();
    }

    inline float NodeContextImpl::getClosestPointOnPathLongitude (void) const
    {
        return _locationInfo.getClosestPointOnPathLongitude();
    }
}

#endif  // INCL_NODE_CONTEXT_IMPL_H
