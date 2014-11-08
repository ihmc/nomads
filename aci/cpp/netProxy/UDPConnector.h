#ifndef INCL_UDP_CONNECTOR_H
#define INCL_UDP_CONNECTOR_H

/*
 * UDPConnector.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).

 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Handles incoming and outgoing UDP datagrams to/from remote NetProxies.
 */

#if defined (LINUX)
    #include <algorithm>
#endif

#include "ManageableThread.h"

#include "Connector.h"
#include "Connection.h"
#include "UDPSocketAdapter.h"


namespace ACMNetProxy
{
    class NetProxyConfigManager;

    class UDPConnector : public NOMADSUtil::ManageableThread, public virtual Connector
    {
    public:
        UDPConnector (void);
        virtual ~UDPConnector (void);

        static Connection * getUDPConnection (void);

        int init (uint16 ui16SocketPort);
        virtual void terminateExecution (void);
        void run (void);

        unsigned int removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID);


    protected:
        virtual bool isEnqueueingAllowed (void) const;


    private:
        unsigned char _pucInBuf[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];
        int32 _i32BytesInBuffer;
        
        static UDPSocketAdapter * const _pUDPSocketAdapter;
    };


    inline Connection * UDPConnector::getUDPConnection (void)
    {
        static Connection udpConnection (_pUDPSocketAdapter, NULL);

        return &udpConnection;
    }

    inline bool UDPConnector::isEnqueueingAllowed (void) const
    {
        return _pUDPSocketAdapter->_udpConnectionThread.getBufferedBytesAmount() < NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE;
    }

    inline unsigned int UDPConnector::removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID)
    {
        return _pUDPSocketAdapter->_udpConnectionThread.removeTCPTypePacketsFromTransmissionQueue (uiLocalID, uiRemoteID);
    }
}

#endif   // #ifndef INCL_UDP_CONNECTOR_H
