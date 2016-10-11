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
#define	INCL_NODE_CONTEXT_H

#include "LList.h"
#include "MatchmakingQualifier.h"

#include "CustomPolicies.h"

#include "StringHashtable.h"

namespace IHMC_C45
{
    class Classifier;
}

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    class AdjustedPathWrapper;
    class NodePath;
    class Path;
    struct MetadataRankerConfiguration;

    class NodeContext
    {
        public:
            struct Versions
            {
                Versions (void);
                Versions (uint32 ui32StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                          uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion);
                Versions (const Versions &version);
                ~Versions (void);

                void set (uint32 ui32StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                          uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion);

                bool greaterThan (const NodeContext::Versions &versions, bool bExcludeWayPointVersion) const;
                bool lessThan (const NodeContext::Versions &versions, bool bExcludeWayPointVersion) const;

                uint32 _ui32StartingTime;
                uint16 _ui16InfoVersion;
                uint16 _ui16PathVersion;
                uint16 _ui16WaypointVersion;
                uint16 _ui16ClassifierVersion;
                uint16 _ui16MatchmakerQualifierVersion;
            };

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

            static const unsigned int DEFAULT_USEFUL_DISTANCE;   // in meters
            static const PositionApproximationMode DEFAULT_PATH_ADJUSTING_MODE;
            static const unsigned int WAYPOINT_UNSET;

            static const char * PATH_UNSET_DESCRIPTOR;
            static const char * ON_WAY_POINT_DESCRIPTOR;
            static const char * PATH_DETOURED_DESCRIPTOR;
            static const char * PATH_IN_TRANSIT_DESCRIPTOR;
            static const char * TOO_FAR_FROM_PATH_DESCRIPTOR;

            static const double TOO_FAR_FROM_PATH_COEFF;
            static const double APPROXIMABLE_TO_POINT_COEFF;

            virtual ~NodeContext (void);

            virtual void display (void);

            int addUserId (const char *pszUserId);
            int setMissionId (const char *pszMissionName);

            const char * getNodeId (void) const;
            const char * getTeamId (void) const;
            const char * getMissionId (void) const;
            const char * getRole (void) const;
            unsigned int getBatteryLevel (void) const;
            unsigned int getMemoryAvailable (void) const;
            bool getLimitToLocalMatchmakingOnly (void) const;
            float getMatchmakingThreshold (void) const;

            /**
             * Returns the current decision tree if it exists. Returns NULL
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
            static const char * getStatusAsString (NodeStatus status);

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

            MatchmakingQualifiers * getMatchmakingQualifiers (void);
            MetadataRankerConfiguration * getMetaDataRankerConfiguration (void) const;

            /**
             * Set the current actual position on the node.
             * Returns true if the node changes its state (see values in
             * NodeStatus), or if the current waypoint changed, false otherwise.
             */
            virtual bool setCurrentPosition (float latitude, float longitude, float altitude,
                                             const char *pszLocation, const char *pszNote,
                                             uint64 timeStamp) = 0;

            /**
             * Returns the current path if set, returns NULL otherwise.
             */
            virtual NodePath * getPath (void) = 0;

            /**
             * Returns the current path if set, enhanced with the segments to
             * return returns NULL otherwise.
             */
            AdjustedPathWrapper * getAdjustedPath (void);
            PositionApproximationMode getPathAdjustingMode (void) const;

            CustomPolicies * getCustomPolicies (void);

            Versions getVersions (void);
            uint32 getStartTime (void) const;
            uint16 getInformationVersion (void) const;
            uint16 getPathVersion (void) const;
            uint16 getWaypointVersion (void) const;
            uint16 getMatchmakerQualifierVersion (void) const;
            virtual uint16 getClassifierVersion (void) = 0;

            uint32 getMaximumUsefulDistance (void);
            uint32 getUsefulDistance (const char *pszInformationMIMEType) const;

            bool hasUserId (const char *pszUserId);

            void setBatteryLevel(unsigned int uiBatteryLevel);
            void setMemoryAvailable(unsigned int uiMemoryAvailable);

            virtual bool isPeerActive (void) = 0;

            /**
             * Read and write the current position in the path.
             */
            int readCurrentWaypoint (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize,
                                     Versions &versions, bool &bPositionHasChanged);
            int writeCurrentWaypoint (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int getWriteWaypointLength (void);

            /**
             * Reads remote versions with the given Reader. The versions values
             * are returned in the given fields.
             * Returns a number > 0 if there are no errors. The returned value
             * specifies the number of bytes read.
             * Returns -1 if the Reader made an error.
             */
            static int64 readVersions (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize, bool &pbContainsVersions,
                                       Versions &versions);

            /**
             * Writes the versions of the information, path, the decision tree
             * and the current position of the node in the path.
             * - Returns a number > 0 if there are no errors. The return value
             *   specifies the number of bytes written.
             * - Returns -1 if there was a writing error. In this case the
             *   context could be partially written.
             * - Returns -2 if "ui32MaxSize" is reached and the last part
             *   of the context is not written.
             *
             * If ui32MaxSizes is not specified, the path is written completely
             * without any check on the allowed number of bytes that are written.
             */
            int64 writeEmptyVersions (NOMADSUtil::Writer *pWriter);
            int64 writeVersions (NOMADSUtil::Writer *pWriter);
            int64 writeVersions (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int getWriteVersionsLength (void);

            /**
             * Writes part of the local context based on the given
             * versions. Return values similar to writeAll().
             */
            int64 writeUpdates (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, const Versions &versions);
            int64 getWriteUpdatesLength (const Versions &versions);

            /**
             ** Writes the whole local context with the given Writer.
             * Returns a number > 0 if there are no errors. The number specifies
             * the number of bytes written.
             * Returns -1 if the Writer made an error.
             * In this case the context could be partially written.
             * Returns -2 if "ui32MaxSize" is reached and the last part of the
             * context is not written.
             */
            int64 writeAll (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int64 getWriteAllLength (void);

            int64 writeForDSProListener (NOMADSUtil::Writer *pWriter);

        protected:
            /**
             * pszNodeID - the unique identifier of the node
             * dTooFarCoeff - if a point is within a usefuldDistance/dTooFarCoeff
             *                distance from a waypoint, then it is approximable
             *                to the waypoint
             * dApproxCoeff - if a point is not within a usefuldDistance*dTooFarCoeff
             *                distance from the path, then it is too far from
             *                from the path
             */
            NodeContext (const char *pszNodeID, double dTooFarCoeff, double dApproxCoeff);

            int64 readLocalInformation (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize, bool bSkip);
            int64 writeLocalInformation (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int getWriteLocalInformationLength (void);

            int64 readPathInformation (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize, bool bSkip);
            int64 writePathInformation (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize);
            int getWritePathInformationLength (void);

            /**
             * Given the point at (latitude, longitude), returns the closest point
             * on the path.  The coordinates of the closest points are stored in
             * (closestPointLat, closestPointLong).
             * Returns the index of the way-point that ends the path segment
             * that contains the closest way-point.
             *
             * Returns a negative number in case of error (if the path is empty)
             */
            NodeStatus calculateClosestCurrentPointInPath (float latitude, float longitude, float altitude,
                                                           float &closestPointLat, float &closestPointLong,
                                                           int &segmentStartIndex);

            /**
             * Evaluate whether (latitude1, longitude1) is close enough to
             * (latitude2, longitude2) that it can be approximated to it.
             * If (latitude2, longitude2) is not specified, (latitude1, longitude1)
             * is compared with the latest position that was set.
             * 
             */
            bool isApproximableToPoint (float latitude1, float longitude1, float latitude2, float longitude2);
            bool isApproximableToPreviousPosition (float latitude, float longitude);
            bool isApproximableToPoint (double dDistance);
            bool isTooFarFromPath (float latitude1, float longitude1, float latitude2, float longitude2);
            bool isTooFarFromPath (double dDistance);

            /**
             * If a path is set, it finds _iCurrWayPointInPath,
             * _closestPointOnPathLat, and _closestPointOnPathLong and sets them
             * in the NodeContext
             *
             * This methods MUST BE EXTENDED by the extending classes in order
             * to store the current position.
             */
            virtual bool setCurrentPosition (float latitude, float longitude, float altitude);

            void setLimitToLocalMatchmakingOnly (bool bLimitPrestagingToLocalData);
            void setMatchmakingThreshold (float fMatchmakingThreshold);

        protected:
            bool _bLimitPrestagingToLocalData;
            uint8 _ui8BatteryLevel;     // range from 0 to 10
            uint8 _ui8MemoryAvailable;	// range from 0 to 10

            uint16 _ui16CurrInfoVersion;   // Current version of team, mission
                                           // and role
            uint16 _ui16CurrPathVersion;
            uint16 _ui16CurrWaypointVersion;
            uint16 _ui16CurrMatchmakerQualifierVersion;

            NodeStatus _status;
            uint32 _ui32StartingTime;
            uint32 _ui32DefaultUsefulDistance;    // Indicates the maximum distance
                                                  // between the path and the useful
                                                  // data coordinates
            int _iCurrWayPointInPath;             // Current position of the node along
                                                  // the path

            float _closestPointOnPathLat;
            float _closestPointOnPathLong;
            float _fMatchmakingThreshold;

            double _dTooFarCoeff;
            double _dApproxCoeff;

            const char *_pszNodeId;
            char *_pszTeamID;
            char *_pszMissionID;
            char *_pszRole;

            IHMC_C45::Classifier *_pClassifier;
            MetadataRankerConfiguration *_pMetaDataRankerConf;
            NOMADSUtil::StringHashtable<uint32> _usefulDistanceByMimeType;
            MatchmakingQualifiers _qualifiers;
            CustomPolicies _customPolicies;

        private:
            int64 writeVersionsInternal (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, bool bIgnoreMaxSizes);
            int64 writeUpdatesInternal (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, const Versions &versions, bool bForceWriteAll);
            int64 getWriteUpdatesLengthInternal (const Versions &versions, bool bForceWriteAll);

        private:
            PositionApproximationMode _pathAdjustingMode;
            AdjustedPathWrapper *_pPathWrapper;
            NOMADSUtil::LList<NOMADSUtil::String> _usersIds;
    };

    inline const char * NodeContext::getNodeId() const
    {
        return _pszNodeId;
    }

    inline const char * NodeContext::getTeamId() const
    {
        return _pszTeamID;
    }

    inline const char * NodeContext::getMissionId() const
    {
        return _pszMissionID;
    }

    inline const char * NodeContext::getRole() const
    {
        return _pszRole;
    }

    inline unsigned int NodeContext::getBatteryLevel() const
    {
    	return _ui8BatteryLevel;
    }

    inline unsigned int NodeContext::getMemoryAvailable() const
    {
    	return _ui8MemoryAvailable;
    }

    inline bool NodeContext::getLimitToLocalMatchmakingOnly (void) const
    {
        return _bLimitPrestagingToLocalData;
    }

    inline float NodeContext::getMatchmakingThreshold (void) const
    {
        return _fMatchmakingThreshold;
    }

    inline IHMC_C45::Classifier * NodeContext::getClassifier() const
    {
        return _pClassifier;
    }

    inline int NodeContext::getCurrentWayPointInPath() const
    {
        return _iCurrWayPointInPath;
    }

    inline uint32 NodeContext::getUsefulDistance (const char *pszInformationMIMEType) const
    {
        if (pszInformationMIMEType == NULL ||
            !_usefulDistanceByMimeType.containsKey (pszInformationMIMEType)) {
            return _ui32DefaultUsefulDistance;
        }
        uint32 *ui32 = _usefulDistanceByMimeType.get (pszInformationMIMEType);
        return *ui32;
    }

    inline uint32 NodeContext::getMaximumUsefulDistance()
    {
        if (_usefulDistanceByMimeType.getCount() == 0) {
            return _ui32DefaultUsefulDistance;
        }
        uint32 ui32Max = _ui32DefaultUsefulDistance;
        for (NOMADSUtil::StringHashtable<uint32>::Iterator i = _usefulDistanceByMimeType.getAllElements();
             !i.end(); i.nextElement()) {
            uint32 ui32Val = *(i.getValue());
            if (ui32Val > ui32Max) {
                ui32Max = ui32Val;
            }
        }
        return ui32Max;
    }

    inline NodeContext::Versions NodeContext::getVersions (void)
    {
        return Versions (getStartTime(), getInformationVersion(), getPathVersion(),
                         getWaypointVersion(), getClassifierVersion(),
                         getMatchmakerQualifierVersion());
    }

    inline uint32 NodeContext::getStartTime() const
    {
        return _ui32StartingTime;
    }

    inline MetadataRankerConfiguration * NodeContext::getMetaDataRankerConfiguration() const
    {
        return _pMetaDataRankerConf;
    }

    inline MatchmakingQualifiers * NodeContext::getMatchmakingQualifiers()
    {
        return &_qualifiers;
    }

    inline uint16 NodeContext::getInformationVersion() const
    {
        return _ui16CurrInfoVersion;
    }

    inline NodeContext::PositionApproximationMode NodeContext::getPathAdjustingMode() const
    {
        return _pathAdjustingMode;
    }

    inline CustomPolicies * NodeContext::getCustomPolicies (void)
    {
        return &_customPolicies;
    }

    inline uint16 NodeContext::getPathVersion() const
    {
        return _ui16CurrPathVersion;
    }

    inline uint16 NodeContext::getWaypointVersion() const
    {
        return _ui16CurrWaypointVersion;
    }

    inline uint16 NodeContext::getMatchmakerQualifierVersion() const
    {
        return _ui16CurrMatchmakerQualifierVersion;
    }

    inline NodeContext::NodeStatus NodeContext::getStatus() const
    {
        return _status;
    }

    inline float NodeContext::getClosestPointOnPathLatitude() const
    {
        return _closestPointOnPathLat;
    }

    inline float NodeContext::getClosestPointOnPathLongitude() const
    {
        return _closestPointOnPathLong;
    }

    inline NodeContext::Versions::Versions (void)
        : _ui32StartingTime (0), _ui16InfoVersion (0), _ui16PathVersion (0),
           _ui16WaypointVersion (0), _ui16ClassifierVersion (0), _ui16MatchmakerQualifierVersion (0)
    {
    }

    inline NodeContext::Versions::Versions (uint32 ui32StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                                            uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion)
        : _ui32StartingTime (ui32StartingTime), _ui16InfoVersion (ui16InfoVersion),
          _ui16PathVersion (ui16PathVersion), _ui16WaypointVersion (ui16WaypointVersion),
          _ui16ClassifierVersion (ui16ClassifierVersion), _ui16MatchmakerQualifierVersion (ui16MatchmakerQualifierVersion)
    {
    }

    inline NodeContext::Versions::Versions (const NodeContext::Versions &version)
        : _ui32StartingTime (version._ui32StartingTime), _ui16InfoVersion (version._ui16InfoVersion),
          _ui16PathVersion (version._ui16PathVersion), _ui16WaypointVersion (version._ui16WaypointVersion),
          _ui16ClassifierVersion (version._ui16ClassifierVersion), _ui16MatchmakerQualifierVersion (version._ui16MatchmakerQualifierVersion)
    {
        
    }

    inline NodeContext::Versions::~Versions (void)
    {
    }

    inline void NodeContext::Versions::set (uint32 ui32StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
        uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion, uint16 ui16MatchmakerQualifierVersion)
    {
        _ui32StartingTime = ui32StartingTime;
        _ui16InfoVersion = ui16InfoVersion;
        _ui16PathVersion = ui16PathVersion;
        _ui16WaypointVersion = ui16WaypointVersion;
        _ui16ClassifierVersion = ui16ClassifierVersion;
        _ui16MatchmakerQualifierVersion = ui16MatchmakerQualifierVersion;
    }

    inline bool NodeContext::Versions::greaterThan (const NodeContext::Versions &versions, bool bExcludeWayPointVersion) const
    {
        if ((_ui16InfoVersion > versions._ui16InfoVersion) ||
            (_ui16PathVersion > versions._ui16PathVersion) ||
            (_ui16ClassifierVersion > versions._ui16ClassifierVersion) ||
            (_ui16MatchmakerQualifierVersion > versions._ui16MatchmakerQualifierVersion)) {
            return true;
        }
        if (!bExcludeWayPointVersion) {
            return (_ui16PathVersion > versions._ui16PathVersion);
        }
        return false;
    }

    inline bool NodeContext::Versions::lessThan (const NodeContext::Versions &versions, bool bExcludeWayPointVersion) const
    {
        if ((_ui16InfoVersion < versions._ui16InfoVersion) ||
            (_ui16PathVersion < versions._ui16PathVersion) ||
            (_ui16ClassifierVersion < versions._ui16ClassifierVersion) ||
            (_ui16MatchmakerQualifierVersion < versions._ui16MatchmakerQualifierVersion)) {
            return true;
        }
        if (!bExcludeWayPointVersion) {
            return (_ui16PathVersion < versions._ui16PathVersion);
        }
        return false;
    }
}

#endif  // INCL_NODE_CONTEXT_H

