/* 
 * NetworkInterfaceFactory.cpp
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

#include "NetworkInterfaceFactory.h"

#include "LocalNetworkInterface.h"
#include "ManycastForwardingNetworkInterface.h"
#include "NormNetworkInterface.h"
#include "ProxyNetworkInterface.h"

#if !defined (ANDROID)
    #include "ProxyDatagramSocket.h"
#endif

#include <stdlib.h>
#include <assert.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

namespace IHMC_NMS
{
   NetworkInterface * getNetworkInterface (const String &addr, PROPAGATION_MODE mode, bool bAsyncTransmission)
   {
    #if !defined (ANDROID)
        if (addr.startsWith (ProxyDatagramSocket::ADDRESS_PREFIX)) {
            return new ProxyNetworkInterface (mode, bAsyncTransmission);
        }
    #endif
        switch (mode) {
            #if defined (LINUX)
                #if !defined (ANDROID)
                case NORM:
                    return new NormNetworkInterface();
                #endif
            #endif
            case BROADCAST:
            case MULTICAST:
            default: {
                if (addr == NetworkInterface::IN_ADDR_ANY_STR) {   // "0.0.0.0"
                    return new WildcardNetworkInterface(mode, bAsyncTransmission);
                }
                else {
                    return new LocalNetworkInterface (mode, bAsyncTransmission);
                }
            }
        }
    }
}

NetworkInterface * NetworkInterfaceFactory::getNetworkInterface (const String &addr, PROPAGATION_MODE mode,
                                                                 bool bAsyncTransmission, bool bReplyViaUnicast)
{
    NetworkInterface *pNetInt = IHMC_NMS::getNetworkInterface (addr, mode, bAsyncTransmission);
    assert (pNetInt != NULL);
    if (pNetInt == NULL) {
        return NULL;
    }
    if (bReplyViaUnicast) {
        pNetInt = new ManycastForwardingNetworkInterface (pNetInt);
    }
    return pNetInt;
}

