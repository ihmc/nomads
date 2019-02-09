/**
 * Versions.h
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
 * Created on December 22, 2016, 12:45 PM
 */

#ifndef INCL_VERSIONS_H
#define INCL_VERSIONS_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class JsonObject;
    class Reader;
    class Writer;
}

namespace IHMC_ACI
{
    struct Versions
    {
        static const char * VERSIONS_OBJECT_NAME;

        static const char * STARTING_TIME;

        static const char * INFO_VERSION;
        static const char * PATH_VERSION;
        static const char * WAYPOINT_VERSION;
        static const char * CLASSIFIER_VERSION;
        static const char * MATCHMAKING_VERSION;

        Versions (void);
        Versions (int64 i64StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                  uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion,
                  uint16 ui16MatchmakerQualifierVersion, uint16 ui16AreasOfInterestVersion);
        Versions (const Versions &version);
        ~Versions (void);

        void reset (void);
        void set (int64 i64StartingTime, uint16 ui16InfoVersion, uint16 ui16PathVersion,
                  uint16 ui16WaypointVersion, uint16 ui16ClassifierVersion,
                  uint16 ui16MatchmakerQualifierVersion, uint16 ui16AreasOfInterestVersion);

        bool greaterThan (const Versions &versions, bool bExcludeWayPointVersion) const;
        bool lessThan (const Versions &versions, bool bExcludeWayPointVersion) const;

        bool pathGreater (const Versions &version) const;

        int fromJson (const NOMADSUtil::JsonObject *pJson);
        NOMADSUtil::JsonObject * toJson (void) const;

        int read (NOMADSUtil::Reader *pReader);
        int write (NOMADSUtil::Writer *pWriter);

        int64 _i64StartingTime;
        uint16 _ui16InfoVersion;
        uint16 _ui16PathVersion;
        uint16 _ui16WaypointVersion;
        uint16 _ui16ClassifierVersion;
        uint16 _ui16MatchmakerQualifierVersion;
        uint16 _ui16AreasOfInterestVersion;
    };
}

#endif  /* INCL_VERSIONS_H */
