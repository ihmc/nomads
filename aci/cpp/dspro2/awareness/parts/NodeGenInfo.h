/**
 * NodeGenInfo.h
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

#ifndef INCL_NODE_GENERAL_INFO_H
#define INCL_NODE_GENERAL_INFO_H

#include "StrClass.h"
#include "Part.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_ACI
{
    class NodeGenInfo : public Part
    {
        public:
            static const char * NODE_INFO_OBJECT_NAME;

            explicit NodeGenInfo (const char *pszNodeId);
            ~NodeGenInfo (void);

            bool hasUserId (const char *pszUserId) const;

            unsigned int getBatteryLevel (void) const;
            unsigned int getMemoryAvailable (void) const;
            NOMADSUtil::String getNodeId (void) const;
            NOMADSUtil::String getMisionId (void) const;
            NOMADSUtil::String getRole (void) const;
            NOMADSUtil::String getTeamId (void) const;

            /*
             * Return true if the value was updated, false otherwise
             */
            // it ranges from 0 to 10
            bool setBatteryLevel (uint8 ui8BatteryLevel);
            // it ranges from 0 to 10
            bool setMemoryAvailable (uint8 ui8MemoryAvailable);
            bool addUserId (const char *pzUserId);
            bool setMisionId (const char *pszMissionId);
            bool setRole (const char *pszRole);
            bool setTeamId (const char *pszTeamId);
            bool setNodeType (const char *pszType);

            void reset (void);

            /*
             * Serialization
             */
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            NOMADSUtil::JsonObject *_pJson;
    };
}

#endif  /* INCL_NODE_GENERAL_INFO_H */
