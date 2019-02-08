/*
 * DatagramBasedAbstractNetworkInterface.h
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

#ifndef INCL_DATAGRAM_BASED_ABSTRACT_NETWORK_INTERFACE_H
#define INCL_DATAGRAM_BASED_ABSTRACT_NETWORK_INTERFACE_H

#include "AbstractNetworkInterface.h"

#include "NetworkInterface.h"
#include "NetworkInterfaceManager.h"

#include "MessageFactory.h"
#include "NetUtils.h"

#include "DatagramSocket.h"

namespace NOMADSUtil
{
    class AbstractDatagramNetworkInterface : public AbstractNetworkInterface
    {
        public:
            AbstractDatagramNetworkInterface (PROPAGATION_MODE mode, bool bAsyncTransmission);
            virtual ~AbstractDatagramNetworkInterface (void);

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

            virtual uint8 getType (void);

            uint32 getTransmitRateLimit (const char *pszDestinationAddr);
            uint32 getTransmitRateLimit (void);

            int setTransmitRateLimit (const char *pszDestinationAddr, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

            int receive (void *pBuf, int iBufSize, InetAddr *pIncomingIfaceByAddr, InetAddr *pRemoteAddr);

            int sendMessageNoBuffering (const NetworkMessage *pNetMsg, uint32 ui32IPAddr, const char *pszHints);

            bool clearToSend (void);

    protected:
        virtual int bind (void) = 0;

    protected:
        const PROPAGATION_MODE _mode;

        bool _bIsAvailable;
        String _networkAddr;
        DatagramSocket *_pDatagramSocket;

    private:
        uint8 _ui8Type;
        int _iBufSize;
        Mutex _mBind;
    };
}

#endif  /* INCL_DATAGRAM_BASED_ABSTRACT_NETWORK_INTERFACE_H */

