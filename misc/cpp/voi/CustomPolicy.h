/*
 * CustomPolicy.h
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
 * Created on February 12, 2015, 7:09 PM
 */

#ifndef INCL_CUSTUM_POLICY_H
#define	INCL_CUSTUM_POLICY_H

#include "Match.h"

namespace IHMC_VOI
{
    class MetadataInterface;

    class CustomPolicy
    {
        public:
            virtual float getRankWeight (void) const = 0;
            virtual Match rank (MetadataInterface *pMetadata) = 0;
    };
}

#endif  /* INCL_CUSTUM_POLICY_H */

