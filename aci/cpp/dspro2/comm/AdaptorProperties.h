/* 
 * AdaptorProperties.h
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
 * Created on April 24, 2013, 2:56 AM
 */

#ifndef INCL_ADAPTOR_PROPERTIES_H
#define	INCL_ADAPTOR_PROPERTIES_H

#include "Defs.h"

namespace IHMC_ACI
{
    enum AdaptorType
    {
        UNKNOWN    = 0x00,

        DISSERVICE = 0x01,
        MOCKETS    = 0x02,
        TCP        = 0x03
    };

    const char *getAdaptorTypeAsString (AdaptorType type);

    struct AdaptorProperties
    {
        AdaptorProperties (AdaptorId uiAdaptId, AdaptorType type, bool bSuppCaching,
                           bool bSuppDirectConn);
        AdaptorProperties (const AdaptorProperties &adptProp);
        ~AdaptorProperties (void);

        const AdaptorId uiAdaptorId;
        const AdaptorType uiAdaptorType;
        const bool bSupportsCaching;
        const bool bSupportsDirectConnection;
    };

    inline AdaptorProperties::AdaptorProperties (AdaptorId uiAdaptId, AdaptorType type,
                                                 bool bSuppCaching, bool bSuppDirectConn)
        : uiAdaptorId (uiAdaptId),
          uiAdaptorType (type),
          bSupportsCaching (bSuppCaching),
          bSupportsDirectConnection (bSuppDirectConn)
    {
    }

    inline AdaptorProperties::AdaptorProperties (const AdaptorProperties &adaptProp)
        : uiAdaptorId (adaptProp.uiAdaptorId),
          uiAdaptorType (adaptProp.uiAdaptorType),
          bSupportsCaching (adaptProp.bSupportsCaching),
          bSupportsDirectConnection (adaptProp.bSupportsDirectConnection)
    {
    }

    inline AdaptorProperties::~AdaptorProperties (void)
    {
    }
}

#endif	/* INCL_ADAPTOR_PROPERTIES_H */

