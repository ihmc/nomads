/*
 * NodePath.h
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
 */

#ifndef INCL_NODE_PATH_H
#define INCL_NODE_PATH_H

#include "BoundingBox.h"
#include "Path.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class JsonObject;
    class Reader;
    class Writer;
}

namespace IHMC_VOI
{
    class NodePath : public Path
    {
        public:
            static const char * PAST_PATH; // Path id used to identify the past path.
            // constants used to identify the path type
            static const int MAIN_PATH_TO_OBJECTIVE;
            static const int ALTERNATIVE_PATH_TO_OBJECTIVE;
            static const int MAIN_PATH_TO_BASE;
            static const int ALTERNATIVE_PATH_TO_BASE;
            static const int FIXED_LOCATION; // If the node if fixed on the ground.

            /**
             * "pathType" have to be one of the declared constants.
             * "probability" is the probability that the user will
             * that path. It may change during the time. It must be
             * in the range [0 - 1] or the default value 0 is set.
             */
            NodePath (void);
            NodePath (const char *pszPathId, int pathType, float probability);

            virtual ~NodePath (void);

            // If the path has type "_FIXED_PATH" the following methods substitutes
            // the only WayPoint present in the path.  For others types the method
            // append a new WayPoint at the end of the Path.

            /**
             * This version of the method used decimal degrees for coordinates and
             * the time stamp is the number of seconds from 1970.
             */
            int appendWayPoint (float latitude, float longitude, float altitude,
                                const char *pszLocation, const char *pszNote,
                                uint64 timeStamp);

            /**
             * This version uses coordinates expressed in degrees,
             * minutes, seconds and a timeStamp.
             */
            int appendWayPoint (int8 latitudeDeg, uint8 latitudeMin, uint8 latitudeSec,
                                int8 longitudeDeg, uint8 longitudeMin, uint8 longitudeSec,
                                float altitude, const char *pszLocation, const char *pszNote,
                                uint64 timeStamp);

            /**
             * This version uses decimal degrees for coordinates
             * and a date. If the date is unknown, set the
             * parameters related to the date as "-1".
             *
             * Allowed ranges for the date:
             * hour [0, 23]
             * min [0, 59]
             * sec [0, 59]
             * day [1, 31]
             * mon [1, 12]
             * year [1900, ... ]
             */
            int appendWayPoint (float latitude, float longitude, float altitude,
                                const char *pszLocation, const char *pszNote,
                                int hour, int min, int sec, int day, int mon, int year);

            /**
             * This version uses coordinates expressed in degrees,
             * minutes, seconds and a date.
             *
             * Allowed ranges for the date:
             * hour [0, 23]
             * min [0, 59]
             * sec [0, 59]
             * day [1, 31]
             * mon [1, 12]
             * year [1900, ... ]
             */
            int appendWayPoint (int8 latitudeDeg, uint8 latitudeMin, uint8 latitudeSec,
                                int8 longitudeDeg, uint8 longitudeMin, uint8 longitudeSec,
                                float altitude, const char *pszLocation, const char *pszNote,
                                int hour, int min, int sec, int day, int mon, int year);

            /**
             * This version uses azimuth and date. An azimuth is
             * an horizontal angle measured clockwise from a north
             * base line.
             *
             * Allowed ranges for the date:
             * hour [0, 23]
             * min [0, 59]
             * sec [0, 59]
             * day [1, 31]
             * mon [1, 12]
             * year [1900, ... ]
             */
            int appendWayPoint (int8 azimuthDeg, uint8 azimuthMin, uint8 azimuthSec,
                                uint16 distanceInMeters, float altitude,
                                const char *pszLocation, const char *pszNote,
                                int hour, int min, int sec, int day,
                                int mon, int year);

            /**
             * - Returns the minimun rectangular area containing the whole path
             * - Returns the minimun rectangular area containing the whole path
             *   incremented of fPadding
             */
            int getBoundingBox (float &maxLat, float &minLat, float &maxLong, float &minLong);
            int getBoundingBox (float &maxLat, float &minLat, float &maxLong, float &minLong, float fPadding);
            NOMADSUtil::BoundingBox getBoundingBox (float fPadding = 0);

            /**
             * Read and parses a DisservicePro Path R File.
             *
             * .dpr files are comma separated files where each line contains the
             * attribute of a waypoint, except the first line, which contains
             * path id, a flag that indicates whether the time will be specified
             * in absolute or relative (from the prevois waypoint, or from the
             * current time if it's the first waypoint) terms, the node type and
             * the path probability.
             * The apth terminates with a line that contains only -1
             *
             * Example:
             *     PathID, RelTime/AbsTime, 1, 1.0
             *     time, latitude, longitude, altitude, LOCATION, NOTE
             *     ...
             *     -1
             *
             * Returns 0 if the path was read correctly, 0 otherwise
             */
            int readDPRFile (NOMADSUtil::Reader *pReader);

            /**
             * Reads a path with the given ConfigManager. Returns a number > 0 if
             * there are no errors. The return value specify the number of way
             * ponts that were read and added to the path.
             * Returns -1 if the Reader made an error.
             */
            int64 read (NOMADSUtil::ConfigManager *pConfMgr, uint32 ui32MaxSize);

            int setProbability (float probability);

            // get methods
            float getLatitude (int wayPointIndex);
            float getLongitude (int wayPointIndex);
            float getAltitude (int wayPointIndex);
            NOMADSUtil::String getLocation (int wayPointIndex);
            NOMADSUtil::String getNote (int wayPointIndex);
            uint64 getTimeStamp (int wayPointIndex);

            int getPathLength (void);
            int getPathType (void);
            float getPathProbability (void);
            NOMADSUtil::String getPathId (void);

            void display (void);

            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            void updatePathBoundingBox (float latitude, float longitude);

            enum WayPointAppendType {
                DECDEG_TIMESTP = 0x00,
                DEG_TIMESTP    = 0x01,
                DECDEG_DATE    = 0x02,
                DEG_DATE       = 0x03,
                AZM_DATE       = 0x04
            };

            NOMADSUtil::JsonObject *_pPath;

            float _maxLat;
            float _minLat;
            float _maxLong;
            float _minLong;

            static const float LATITUDE_UNSET;
            static const float LONGITUDE_UNSET;
    };
}

#endif // INCL_NODE_PATH_H
