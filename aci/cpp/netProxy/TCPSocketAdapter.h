#ifndef INCL_TCP_SOCKET_ADAPTER_H
#define INCL_TCP_SOCKET_ADAPTER_H

/*
 * TCPSocketAdapter.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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
 * The UDPSocketAdapter class is an adapter that complies
 * to the ConnectorAdapter interface and implements its
 * methods in terms of the TCPSocket class interface.
 */

#include <memory>

#include "TCPSocket.h"

#include "ConfigurationParameters.h"
#include "ConnectorAdapter.h"


namespace ACMNetProxy
{
    class TCPSocketAdapter : public ConnectorAdapter
    {
    public:
        TCPSocketAdapter (NOMADSUtil::TCPSocket * const pTCPSocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr);
        TCPSocketAdapter (const NOMADSUtil::InetAddr & iaLocalProxyAddr, const NOMADSUtil::InetAddr & iaRemoteProxyAddr,
                          EncryptionType encryptionType);
        ~TCPSocketAdapter (void);

        virtual int bufferingMode (int iMode);
        virtual int readConfigFile (const char * const pszConfigFile);
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        virtual EncryptionType getEncryptionType (void) const;
        virtual uint32 getOutgoingBufferSize (void) const;
        virtual uint16 getLocalPort (void) const;

        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                          bool bReliable, bool bSequenced, const void * pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                           bool bReliable, bool bSequenced, const void * pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);


    private:
        template <class T> int receiveProxyHeader (T * const pMess, int & iReceived);

        std::unique_ptr<NOMADSUtil::TCPSocket> _upTCPSocket;
    };


    inline TCPSocketAdapter::TCPSocketAdapter (NOMADSUtil::TCPSocket * const pTCPSocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr) :
        ConnectorAdapter{CT_TCPSOCKET, iaLocalProxyAddr, NOMADSUtil::InetAddr{pTCPSocket->getRemoteHostAddr(), pTCPSocket->getRemotePort()},
                         new uint8[NetworkConfigurationSettings::PROXY_MESSAGE_MTU], NetworkConfigurationSettings::PROXY_MESSAGE_MTU},
        _upTCPSocket{pTCPSocket}
    { }

    inline TCPSocketAdapter::TCPSocketAdapter (const NOMADSUtil::InetAddr & iaLocalProxyAddr, const NOMADSUtil::InetAddr & iaRemoteProxyAddr,
                                               EncryptionType encryptionType) :
        ConnectorAdapter{CT_TCPSOCKET, iaLocalProxyAddr, iaRemoteProxyAddr, new uint8[NetworkConfigurationSettings::PROXY_MESSAGE_MTU],
        NetworkConfigurationSettings::PROXY_MESSAGE_MTU}, _upTCPSocket{new NOMADSUtil::TCPSocket()}
    {
        (void) encryptionType;
    }

    inline TCPSocketAdapter::~TCPSocketAdapter (void) { }

    inline int TCPSocketAdapter::bufferingMode (int iMode)
    {
        return _upTCPSocket->bufferingMode (iMode);
    }

    inline int TCPSocketAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return ConnectorAdapter::readConfigFile (pszConfigFile);
    }

    inline int TCPSocketAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void * pCallbackArg)
    {
        (void) pCallbackFn;
        (void) pCallbackArg;

        return -1;
    }

    inline bool TCPSocketAdapter::isConnected (void) const
    {
        return _upTCPSocket->isConnected() == 1;
    }

    // TO-DO: for the moment, TCP connections are never encrypted
    inline EncryptionType TCPSocketAdapter::getEncryptionType (void) const
    {
        return ET_PLAIN;
    }

    // TO-DO: fix this code to return the actual space available
    inline uint32 TCPSocketAdapter::getOutgoingBufferSize (void) const
    {
        return 4096;
    }

    inline uint16 TCPSocketAdapter::getLocalPort (void) const
    {
        return _upTCPSocket->getLocalPort();
    }

    inline int TCPSocketAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        if (_iaLocalProxyAddr != 0) {
            if (_upTCPSocket->bind (_iaLocalProxyAddr.getIPAddress(), _iaLocalProxyAddr.getPort()) < 0) {
                return -100;
            }
        }

        return _upTCPSocket->connect (pcRemoteProxyIP, ui16RemoteProxyPort);
    }

    inline int TCPSocketAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                                       bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        (void) pInetAddr;
        (void) ui32SourceIPAddr;
        (void) ui32DestinationIPAddr;
        (void) bReliable;
        (void) bSequenced;

        int rc = _upTCPSocket->send (pBuf, ui32BufSize);
        if (rc == ui32BufSize) {
            return 0;
        }

        return rc;
    }

    inline int TCPSocketAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        return _upTCPSocket->shutdown (bReadMode, bWriteMode);
    }

    inline int TCPSocketAdapter::close (void)
    {
        return _upTCPSocket->disconnect();
    }

    inline int TCPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        (void) i64Timeout;

        return _upTCPSocket->receive (pBuf, ui32BufSize);
    }

}

#endif  // INCL_TCP_SOCKET_ADAPTER_H
