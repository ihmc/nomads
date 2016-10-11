/*
 * NormNetworkInterface.h
 *
 * This file is part of the IHMC Util Library
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
 */

#ifndef INCL_NORM_NETWORK_INTERFACE_H
#define INCL_NORM_NETWORK_INTERFACE_H

#include "AbstractNetworkInterface.h"

namespace IHMC_MISC
{
     class Nocket;
}

namespace NOMADSUtil
{
    class NormNetworkInterface : public AbstractNetworkInterface
    {
        public:
            NormNetworkInterface (void);
            ~NormNetworkInterface (void);

            int init (uint16 ui16Port, const char *pszBindingInterfaceSpec,
                      NetworkInterfaceManagerListener *pNMSParent,
                      bool bReceiveOnly, bool bSendOnly,
                      const char *pszPropagationAddr, uint8 ui8McastTTL);

            int rebind (void);

            uint8 getMode (void);
            uint16 getMTU (void);
            const char * getNetworkAddr (void);

            void setDisconnected (void);
            bool isAvailable (void);

            int getReceiveBufferSize (void);
            int setReceiveBufferSize (int iBufSize);

            uint8 getType (void);

            uint32 getTransmitRateLimit (const char *pszDestinationAddr);
            uint32 getTransmitRateLimit (void);

            int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

            int setTTL (uint8 ui8TTL);

            int receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr);

            int sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints);
            bool clearToSend (void);

        private:
            IHMC_MISC::Nocket *_pNock;
    };
}

#endif /* INCL_NORM_NETWORK_INTERFACE_H */

