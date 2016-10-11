/* 
 * NMSProperties.h
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
 * Created on June 24, 2015, 3:30 PM
 */

#ifndef INCL_NMS_PROPERTIES_H
#define	INCL_NMS_PROPERTIES_H

#include "StrClass.h"

namespace NOMADSUtil
{
    class NMSProperties
    {
        public:
            static const uint16 DEFAULT_NMS_PROXY_PORT;
            static const uint16 DEFAULT_CMD_PROCESSOR_PORT;

            static const String NMS_PROXY_PORT;
            static const String NMS_PROXY_ADDRESS;
            static const String NMS_CMD_PROCESSOR;
            static const String NMS_CMD_PROCESSOR_PORT;

            static const String NMS_TRANSMISSION_MODE;
            static const String NMS_TRANSMISSION_ASYNC;
            static const String NMS_TRANSMISSION_UNICAST_REPLY;

            static const String NMS_REQUIRED_INTERFACES;
            static const String NMS_IGNORED_INTERFACES;
            static const String NMS_OPTIONAL_INTERFACES;
            static const String NMS_PRIMARY_INTERFACE;
            static const String NMS_USE_MANAGED_INTERFACES;
            static const String NMS_PERIODIC_MULTICAST_GROUP_REJOIN;

            static const String NMS_PORT;
            static const String NMS_OUTGOING_ADDR;
            static const String NMS_TTL;
            static const String NMS_MTU;

            static const String NMS_MSG_VERSION;

            static const String NMS_DELIVERY_ASYNC;
    };
}

#endif	/* INCL_NMS_PROPERTIES_H */

