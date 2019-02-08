/*
 * NetworkInterfaceFactory.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Created on May 19, 2015, 3:56 PM
 */

#ifndef INCL_NETWORK_INTERFACE_FACTORY_H
#define    INCL_NETWORK_INTERFACE_FACTORY_H

#include "StrClass.h"
#include "NetworkMessageService.h"

namespace NOMADSUtil
{
    class NetworkInterface;

    class NetworkInterfaceFactory
    {
        public:
            static NetworkInterface * getNetworkInterface (const String &addr, PROPAGATION_MODE mode,
                                                           bool bAsyncTransmission, bool bReplyViaUnicast);
    };
}

#endif    /* INCL_NETWORK_INTERFACE_FACTORY_H */

