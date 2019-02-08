/*
 * AdaptorProperties.cpp
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
 * Created on August 18, 2016, 6:36 PM
 */

#include  "AdaptorProperties.h"

const char * IHMC_ACI::getAdaptorTypeAsString (AdaptorType type)
{
    switch (type)
    {
        case DISSERVICE:
            return "DISSERVICE";
        case MOCKETS:
            return "MOCKETS";
        case TCP:
            return "TCP";
        case UDP:
            return "UDP";
        default:
            return "UNKNOWN";
    }
}

