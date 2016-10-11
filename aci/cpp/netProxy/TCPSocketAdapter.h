#ifndef INCL_TCP_SOCKET_ADAPTER_H
#define INCL_TCP_SOCKET_ADAPTER_H

/*
 * TCPSocketAdapter.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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

#include "TCPSocket.h"

#include "ConnectorAdapter.h"
#include "ConfigurationParameters.h"


namespace ACMNetProxy
{
    class TCPSocketAdapter : public ConnectorAdapter
    {
    public:
        TCPSocketAdapter (NOMADSUtil::TCPSocket * const pTCPSocket);
        TCPSocketAdapter (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr);
        ~TCPSocketAdapter (void);

        virtual int bufferingMode (int iMode);
        virtual int readConfigFile (const char * const pszConfigFile);
        int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);

        virtual uint32 getOutgoingBufferSize (void);

        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr,
                          bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                           const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);


    private:
        template <class T> int receiveProxyHeader (T * const pMess, int &iReceived);

        NOMADSUtil::TCPSocket * const _pTCPSocket;
    };


    inline TCPSocketAdapter::TCPSocketAdapter (NOMADSUtil::TCPSocket * const pTCPSocket) :
        ConnectorAdapter (CT_SOCKET, NOMADSUtil::InetAddr(pTCPSocket->getRemoteHostAddr()).getIPAddress(), pTCPSocket->getRemotePort(),
        new uint8[NetProxyApplicationParameters::PROXY_MESSAGE_MTU], NetProxyApplicationParameters::PROXY_MESSAGE_MTU), _pTCPSocket (pTCPSocket) { }

    inline TCPSocketAdapter::TCPSocketAdapter (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr) :
        ConnectorAdapter (CT_SOCKET, pRemoteProxyInetAddr->getIPAddress(), pRemoteProxyInetAddr->getPort(),
        new uint8[NetProxyApplicationParameters::PROXY_MESSAGE_MTU], NetProxyApplicationParameters::PROXY_MESSAGE_MTU), _pTCPSocket (new NOMADSUtil::TCPSocket()) { }

    inline TCPSocketAdapter::~TCPSocketAdapter (void)
    {
        delete _pTCPSocket;
    }

    inline int TCPSocketAdapter::bufferingMode (int iMode)
    {
        return _pTCPSocket->bufferingMode (iMode);
    }

    inline int TCPSocketAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return ConnectorAdapter::readConfigFile (pszConfigFile);
    }

    inline int TCPSocketAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
    {
        (void) pCallbackFn;
        (void) pCallbackArg;
        
        return -1;
    }

    inline bool TCPSocketAdapter::isConnected (void) const
    {
        return _pTCPSocket->isConnected() == 1;
    }

    inline int TCPSocketAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        return _pTCPSocket->connect (pcRemoteProxyIP, ui16RemoteProxyPort);
    }

    inline int TCPSocketAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        return _pTCPSocket->shutdown (bReadMode, bWriteMode);
    }

    inline int TCPSocketAdapter::close (void)
    {
        return _pTCPSocket->disconnect();
    }

    inline uint32 TCPSocketAdapter::getOutgoingBufferSize (void)
    {
        /*!!*/ // Fix this code to return the actual space available
        return 4096;
    }

    inline int TCPSocketAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable,
                                       bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        (void) pInetAddr;
        (void) ui32DestVirtualIPAddr;
        (void) bReliable;
        (void) bSequenced;
        
        int rc = _pTCPSocket->send (pBuf, ui32BufSize);
        if (rc == ui32BufSize) {
            return 0;
        }

        return rc;
    }

    inline int TCPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        (void) i64Timeout;
        
        return _pTCPSocket->receive (pBuf, ui32BufSize);
    }

}

#endif  // INCL_TCP_SOCKET_ADAPTER_H
