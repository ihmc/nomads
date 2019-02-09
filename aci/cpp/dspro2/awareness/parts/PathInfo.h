/**
 * PathInfo.h
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
 * Created on December 24, 2016, 8:23 PM
 */

#ifndef INCL_PATH_INFO_H
#define INCL_PATH_INFO_H

#include "NodePath.h"

#include "StrClass.h"
#include "StringHashtable.h"
#include "Part.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_ACI
{
    class PathInfo : public Part
    {
        public:
            static const char * PATH_INFO_OBJECT_NAME;

            PathInfo (void);
            ~PathInfo (void);

            /**
             * Returns the specified path if it exists. Returns nullptr otherwise.
             * Returns the current path if exists.
             */
            IHMC_VOI::NodePath * getPath (const char *pszPathId);
            IHMC_VOI::NodePath * getPath (void);

            /*
             * Return true if the value was updated, false otherwise
             */
            bool addPath (IHMC_VOI::NodePath *pNodePath);
            bool setCurrentPath (const char *pszPathId);
            // probability in [0.0, 1.0]
            bool setPathProbability (const char *pszPathId, float probability);
            bool deletePath (const char *pszPathId);

            void reset (void);

            /*
             * Serialization
             */
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

        private:
            NOMADSUtil::String _currPathId;
            NOMADSUtil::StringHashtable<IHMC_VOI::NodePath> _paths;
    };
}

#endif  /* INCL_PATH_INFO_H */
