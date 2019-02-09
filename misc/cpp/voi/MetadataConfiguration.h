/*
 * MetadataConfiguration.h
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
 * Created on June 22, 2012, 1:34 PM
 */

#ifndef INCL_METADATA_CONFIGURATION_H
#define	INCL_METADATA_CONFIGURATION_H

#include "StrClass.h"

namespace IHMC_VOI
{
    class MetadataConfiguration
    {
        public:
            virtual unsigned int getNumberOfLearningFields (void) const = 0;
            virtual unsigned int getNumberOfFields (void) const = 0;
            virtual NOMADSUtil::String getFieldName (unsigned int uiIdx) const = 0;
            virtual bool isLearningField (unsigned int uiIdx) const = 0;
    };
}

#endif	/* INCL_METADATA_CONFIGURATION_H */

