#ifndef INCL_UDP_SOCKET_ADAPTER_H
#define INCL_UDP_SOCKET_ADAPTER_H

/*
 * UDPSocketAdapter.h
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
 * The UDPSocketAdapter class is an adapter that complies
 * to the ConnectorAdapter interface and implements its
 * methods in terms of the UDPConnector class interface.
 */

#include "PtrQueue.h"
#include "UDPDatagramSocket.h"

#include "ConfigurationParameters.h"
#include "ProxyNetworkMessage.h"
#include "ConnectorAdapter.h"


namespace ACMNetProxy
{
    class ConnectionManager;

    class UDPSocketAdapter : public ConnectorAdapter
    {
    public:
        UDPSocketAdapter (NOMADSUtil::UDPDatagramSocket * const pUDPSocket);
        ~UDPSocketAdapter (void);

        virtual int bufferingMode (int iMode);
        virtual int readConfigFile (const char * const readConfigFile);
        int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);

        virtual uint32 getOutgoingBufferSize (void);

        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable,
                          bool bSequenced, const void *pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);
        int receiveMessage (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pInetAddr);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                            const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);
        int receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pInetAddr, int64 i64Timeout = 0);


    private:
        friend class UDPConnector;

        class UDPConnectionThread : public NOMADSUtil::ManageableThread
        {
        public:
            UDPConnectionThread (NOMADSUtil::UDPDatagramSocket * const _pSocket);
            ~UDPConnectionThread (void);

            void run (void);

            int addProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32DestVirtualIPAddr,
                                        const uint8 * const pui8MessagePayload = NULL, const uint16 ui16MessagePayloadLen = 0);
            unsigned int getTransmissionQueueSize (void);
            unsigned int getBufferedBytesAmount (void);
            unsigned int removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID);


        private:
            int sendProxyNetworkMessageToRemoteProxy (const ProxyNetworkMessage * const pProxyNetworkMessage);
            int sendProxyNetworkMessageToRemoteProxy (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage,
                                                      uint32 ui32DestVirtualIPAddr, const uint8 *pui8MessagePayload, uint16 ui16MessagePayloadLen);

            NOMADSUtil::UDPDatagramSocket * const _pSocket;
            NOMADSUtil::PtrQueue<const ProxyNetworkMessage> _ProxyNetworkMessagesPtrQueue;
            uint32 _ui32TotalBufferedBytes;

            unsigned char _pucOutBuf[NetProxyApplicationParameters::PROXY_MESSAGE_MTU];

            NOMADSUtil::Mutex _mPtrQueue;
            NOMADSUtil::Mutex _mUDPConnectionThread;
            NOMADSUtil::ConditionVariable _cvUDPConnectionThread;

            static const uint32 UC_TIME_BETWEEN_ITERATIONS = 1000;          // Time between each iterations for UCT
        };


        int verifyMessageSizeMatching (void * const pBuf, uint32 ui32BufSize);

        NOMADSUtil::UDPDatagramSocket * const _pUDPSocket;
        UDPConnectionThread _udpConnectionThread;

        static ConnectionManager * const P_CONNECTION_MANAGER;
    };


    inline UDPSocketAdapter::UDPConnectionThread::UDPConnectionThread (NOMADSUtil::UDPDatagramSocket * const pSocket) :
        _pSocket (pSocket), _cvUDPConnectionThread (&_mUDPConnectionThread)
    {
        _ui32TotalBufferedBytes = 0;
        memset (_pucOutBuf, 0, NetProxyApplicationParameters::PROXY_MESSAGE_MTU);
    }

    inline UDPSocketAdapter::UDPConnectionThread::~UDPConnectionThread (void)
    {
        _ProxyNetworkMessagesPtrQueue.removeAll (true);
    }

    inline unsigned int UDPSocketAdapter::UDPConnectionThread::getTransmissionQueueSize (void)
    {
        // Locking is necessary in case a transmission or removeTCPTypePacketsFromTransmissionQueue() have been called, so that only a consistent result will be returned
        _mPtrQueue.lock();
        unsigned int uiQueueSize = _ProxyNetworkMessagesPtrQueue.size();
        _mPtrQueue.unlock();

        return uiQueueSize;
    }

    inline unsigned int UDPSocketAdapter::UDPConnectionThread::getBufferedBytesAmount (void)
    {
        // Locking is necessary in case a transmission or removeTCPTypePacketsFromTransmissionQueue() have been called, so that only a consistent result will be returned
        _mPtrQueue.lock();
        unsigned int uiBufferedBytes = _ui32TotalBufferedBytes;
        _mPtrQueue.unlock();

        return uiBufferedBytes;
    }

    inline int UDPSocketAdapter::UDPConnectionThread::sendProxyNetworkMessageToRemoteProxy (const ProxyNetworkMessage * const pProxyNetworkMessage)
    {
        if (!pProxyNetworkMessage) {
            return -1;
        }

        else if (0 >= sendProxyNetworkMessageToRemoteProxy (pProxyNetworkMessage->getDestInetAddr(), pProxyNetworkMessage->getProxyMessage(),
                                                            pProxyNetworkMessage->getDestVirtualIPAddr(), pProxyNetworkMessage->getMessagePayload(),
                                                            pProxyNetworkMessage->getMessagePayloadLength())) {
            return -2;
        }

        return 0;
    }

    inline UDPSocketAdapter::UDPSocketAdapter (NOMADSUtil::UDPDatagramSocket * const pUDPSocket) :
        ConnectorAdapter (CT_UDP, 0, 0, new uint8[NetProxyApplicationParameters::PROXY_MESSAGE_MTU], NetProxyApplicationParameters::PROXY_MESSAGE_MTU),
        _pUDPSocket (pUDPSocket), _udpConnectionThread (pUDPSocket) { }

    inline UDPSocketAdapter::~UDPSocketAdapter (void) { }

    inline int UDPSocketAdapter::bufferingMode (int iMode)
    {
        return -1;
    }

    inline int UDPSocketAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return ConnectorAdapter::readConfigFile (pszConfigFile);
    }

    inline int UDPSocketAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
    {
        return -1;
    }

    inline bool UDPSocketAdapter::isConnected (void) const
    {
        return true;
    }

    inline int UDPSocketAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        return 0;
    }

    inline int UDPSocketAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        return _pUDPSocket->shutdown (bReadMode, bWriteMode);
    }

    inline int UDPSocketAdapter::close (void)
    {
        return _pUDPSocket->close();
    }

    inline uint32 UDPSocketAdapter::getOutgoingBufferSize (void)
    {
        return _pUDPSocket->getSendBufferSize();
    }

    inline int UDPSocketAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable,
                                       bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        if (!pInetAddr) {
            return -1;
        }

        if (0 > _udpConnectionThread.addProxyNetworkMessage (pInetAddr, (ProxyMessage * const) pBuf, ui32DestVirtualIPAddr)) {
            return -2;
        }

        return 0;
    }

    inline int UDPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        return _pUDPSocket->receive (pBuf, ui32BufSize);
    }

    inline int UDPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pInetAddr, int64 i64Timeout)
    {
        return _pUDPSocket->receive (pBuf, ui32BufSize, pInetAddr);
    }

}

#endif  // INCL_UDP_SOCKET_ADAPTER_H
