/**
 * UsefulDistance.h
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
 */

#ifndef INCL_USEFUL_DISTANCE_H
#define INCL_USEFUL_DISTANCE_H

#include "FTypes.h"
#include "StringHashtable.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class JsonObject;
}

namespace IHMC_ACI
{
    class UsefulDistance
    {
        public:
            static const unsigned int DEFAULT_USEFUL_DISTANCE;
            static const char * USEFUL_DISTANCE_PROPERTY;
            static const char * USEFUL_DISTANCE_BY_TYPE_PROPERTY;

            UsefulDistance (void);
            ~UsefulDistance (void);

            //int init (NOMADSUtil::ConfigManager *pCfgMgr);

            uint32 getMaximumUsefulDistance (void);
            uint32 getUsefulDistance (const char *pszDataMIMEType=nullptr) const;

            bool setDefaultUsefulDistance (uint32 ui32UsefulDistanceInMeters);
            bool setUsefulDistance (const char *pszDataMIMEType, uint32 ui32UsefulDistanceInMeters);

            void reset (void);

            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            uint32 _ui32UsefulDistanceInMeters; // Indicates the maximum distance
                                                // between the path and the useful
                                                // data coordinates
            NOMADSUtil::StringHashtable<uint32> _usefulDistanceByMimeType;
    };
}

#endif  /* INCL_USEFUL_DISTANCE_H */
