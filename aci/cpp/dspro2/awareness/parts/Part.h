/**
 * Part.h
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
 * Created on January 24, 2016, 8:23 PM
 */

#ifndef INCL_NODE_CONTEXT_IMPL_PART_H
#define INCL_NODE_CONTEXT_IMPL_PART_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_ACI
{
    class Part
    {
        public:
            Part (void);
            ~Part (void);

            virtual void reset (void) = 0;

            uint16 getVersion (void) const;
            void incrementVersion (void);
            void setVersion (uint16 ui16NewVersion);

            /*
             * Serialization
             */
            virtual int fromJson (const NOMADSUtil::JsonObject *pJson) = 0;
            virtual NOMADSUtil::JsonObject * toJson (void) const = 0;

    protected:
            uint16 _ui16Version;
    };
}

#endif  /* INCL_NODE_CONTEXT_IMPL_PART_H */
