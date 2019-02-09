/**
 * NodeContext.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on November 12, 2010, 4:56 PM
 */

#ifndef INCL_NODE_CONTEXT_IMPL_H
#define	INCL_NODE_CONTEXT_IMPL_H

#include "AreaOfInterest.h"
#include "CustomPolicy.h"

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

    class NodeContext
    {
        public:
            enum NodeStatus
            {
                PATH_UNSET = 0x00,
                ON_WAY_POINT = 0x01,
                PATH_DETOURED = 0x02,    // The node is far away from any path
                                         // segment, which means that is detoured
                PATH_IN_TRANSIT = 0x03,  // The node is far away from any waypoints
                TOO_FAR_FROM_PATH = 0x04
            };

            enum PositionApproximationMode
            {
                GO_TO_NEXT_WAY_POINT,
                GO_TO_PROJECTION
            };

            static const PositionApproximationMode DEFAULT_PATH_ADJUSTING_MODE;
            static const unsigned int WAYPOINT_UNSET;

            static const char * PATH_UNSET_DESCRIPTOR;
            static const char * ON_WAY_POINT_DESCRIPTOR;
            static const char * PATH_DETOURED_DESCRIPTOR;
            static const char * PATH_IN_TRANSIT_DESCRIPTOR;
            static const char * TOO_FAR_FROM_PATH_DESCRIPTOR;

            static const double TOO_FAR_FROM_PATH_COEFF;
            static const double APPROXIMABLE_TO_POINT_COEFF;

            NodeContext (void);
            virtual ~NodeContext (void);

            virtual NOMADSUtil::String getNodeId (void) const = 0;
            virtual NOMADSUtil::String getTeamId (void) const = 0;
            virtual NOMADSUtil::String getMissionId (void) const = 0;
            virtual NOMADSUtil::String getRole (void) const = 0;
            virtual unsigned int getBatteryLevel (void) const = 0;
            virtual unsigned int getMemoryAvailable (void) const = 0;
            virtual bool getLimitToLocalMatchmakingOnly (void) const = 0;
            virtual float getMatchmakingThreshold (void) const = 0;
            virtual AreaOfInterestList * getAreasOfInterest (void) const = 0;

            /**
             * Returns the current decision tree if it exists. Returns NULL
             * otherwise.
             */
            virtual IHMC_C45::Classifier * getClassifier (void) const = 0;

            /**
             * Returns WAYPOINT_UNSET id the first position has not yet been set.
             * Returns the current way point in path if the node is exactly on
             * the way point or relatively close to it.
             * Returns the start index of the closest segment otherwise.
             */
            virtual int getCurrentWayPointInPath (void) const = 0;

            virtual NodeStatus getStatus (void) const = 0;
            static const char * getStatusAsString (NodeStatus status);

            virtual float getClosestPointOnPathLatitude (void) const = 0;
            virtual float getClosestPointOnPathLongitude (void) const = 0;

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
            virtual uint32 getRangeOfInfluence (const char *pszNodeType) = 0;

            virtual MetadataRankerConfiguration * getMetaDataRankerConfiguration (void) const = 0;

            /**
             * Returns the current path if set, returns NULL otherwise.
             */
            virtual NodePath * getPath (void) = 0;

            /**
             * Returns the current path if set, enhanced with the segments to
             * return returns NULL otherwise.
             */
            virtual AdjustedPathWrapper * getAdjustedPath (void) = 0;
            virtual PositionApproximationMode getPathAdjustingMode (void) const = 0;

            virtual NOMADSUtil::PtrLList<CustomPolicy> * getCustomPolicies (void) = 0;

            virtual uint32 getMaximumUsefulDistance (void) = 0;
            virtual uint32 getUsefulDistance (const char *pszInformationMIMEType) const = 0;
            virtual uint32 getMaximumRangeOfInfluence (void) = 0;

            virtual bool hasUserId (const char *pszUserId) = 0;

            virtual bool isPeerActive (void) = 0;
    };

    typedef NOMADSUtil::PtrLList<NodeContext> NodeContextList;
}

#endif  // INCL_NODE_CONTEXT_IMPL_H

