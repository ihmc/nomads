#ifndef INCL_UDP_SOCKET_ADAPTER_H
#define INCL_UDP_SOCKET_ADAPTER_H

/*
 * UDPSocketAdapter.h
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
 * methods in terms of the UDPConnector class interface.
 */

#include <chrono>
#include <deque>
#include <memory>
#include <condition_variable>

#include "UDPDatagramSocket.h"

#include "ConfigurationParameters.h"
#include "ProxyNetworkMessage.h"
#include "ConnectorAdapter.h"


namespace ACMNetProxy
{
    class UDPSocketAdapter : public ConnectorAdapter
    {
    public:
        UDPSocketAdapter (std::unique_ptr<NOMADSUtil::UDPDatagramSocket> && upUDPSocket, const NOMADSUtil::InetAddr & iaLocalAddr);
        ~UDPSocketAdapter (void);

        int initUDPSocket (uint16 ui16Port, uint32 ui32ListenAddr);
        int setUDPSocketTransmissionRateLimit (uint32 ui32RateLimitInBps);
        virtual int bufferingMode (int iMode);

        virtual int readConfigFile (const char * const readConfigFile);
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        bool isEnqueueingAllowed (void) const;
        virtual EncryptionType getEncryptionType (void) const;
        virtual uint32 getOutgoingBufferSize (void) const;
        virtual uint16 getLocalPort (void) const;

        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                          bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);
        int receiveMessage (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr & iaRemoteAddress);
        int receiveMessage (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr & iaLocalAddress, NOMADSUtil::InetAddr & iaRemoteAddress);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);

        bool areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const;
        unsigned int removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID);

        int startUDPConnectionThread (void);
        void requestUDPConnectionThreadTermination (void);
        void requestUDPConnectionThreadTerminationAndWait (void);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                           bool bReliable, bool bSequenced, const void * pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);
        int receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pRemoteInetAddr, int64 i64Timeout = 0);
        int receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pLocalInetAddr,
                     NOMADSUtil::InetAddr * const pRemoteInetAddr, int64 i64Timeout = 0);


    private:
        class UDPConnectionThread : public NOMADSUtil::ManageableThread
        {
        public:
            UDPConnectionThread (NOMADSUtil::UDPDatagramSocket * const pSocket, EncryptionType encryptionType);
            ~UDPConnectionThread (void);

            void run (void);

            int addProxyNetworkMessage (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32SourceIPAddr,
                                        uint32 ui32DestinationIPAddr, const uint8 * const pui8MessagePayload = nullptr, const uint16 ui16MessagePayloadLen = 0);
            unsigned int getTransmissionQueueSize (void) const;
            unsigned int getAmountOfBufferedBytes (void) const;

            bool areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const;
            unsigned int removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID);


        private:
            int sendProxyNetworkMessageToRemoteProxy (const ProxyNetworkMessage & rProxyNetworkMessage);
            int sendProxyNetworkMessageToRemoteProxy (const NOMADSUtil::InetAddr * const pProxyAddr, const ProxyMessage * const pProxyMessage, uint32 ui32SourceIPAddr,
                                                      uint32 ui32DestinationIPAddr, const uint8 *pui8MessagePayload, uint16 ui16MessagePayloadLen);


            EncryptionType _encryptionType;
            uint32 _ui32TotalBufferedBytes;
            std::deque<ProxyNetworkMessage> _deqProxyNetworkMessages;

            // The UDPDatagramSocket instance referenced by _pUDPSocket is owned by UDPSocketAdapter::_upUDPSocket, which outlives this object
            NOMADSUtil::UDPDatagramSocket * const _pUDPSocket;
            unsigned char _pucOutBuf[NetworkConfigurationSettings::PROXY_MESSAGE_MTU];

            mutable std::mutex _mtxProxyMessagesDeq;
            mutable std::mutex _mUDPConnectionThread;
            std::condition_variable _cvUDPConnectionThread;

            constexpr static const std::chrono::milliseconds UC_TIME_BETWEEN_ITERATIONS{1000};          // Time between each iterations for UCT
        };


        int verifyMessageSizeMatching (void * const pBuf, uint32 ui32BufSize);

        std::unique_ptr<NOMADSUtil::UDPDatagramSocket> const _upUDPSocket;
        UDPConnectionThread _udpConnectionThread;
    };


    inline UDPSocketAdapter::UDPSocketAdapter (std::unique_ptr<NOMADSUtil::UDPDatagramSocket> && upUDPSocket, const NOMADSUtil::InetAddr & iaLocalAddr) :
        ConnectorAdapter{CT_UDPSOCKET, iaLocalAddr, {0UL, 0}, new uint8[NetworkConfigurationSettings::PROXY_MESSAGE_MTU],
                         NetworkConfigurationSettings::PROXY_MESSAGE_MTU},
        _upUDPSocket{std::move (upUDPSocket)}, _udpConnectionThread{_upUDPSocket.get(), getEncryptionType()}
    { }

    inline UDPSocketAdapter::~UDPSocketAdapter (void) { }

    inline int UDPSocketAdapter::initUDPSocket (uint16 ui16Port, uint32 ui32ListenAddr)
    {
        return _upUDPSocket->init (ui16Port, ui32ListenAddr);
    }

    inline int UDPSocketAdapter::setUDPSocketTransmissionRateLimit (uint32 ui32RateLimitInBps)
    {
        return _upUDPSocket->setTransmitRateLimit (ui32RateLimitInBps);
    }

    inline int UDPSocketAdapter::bufferingMode (int iMode)
    {
        (void) iMode;
        return -1;
    }

    inline int UDPSocketAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return ConnectorAdapter::readConfigFile (pszConfigFile);
    }

    inline int UDPSocketAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void * pCallbackArg)
    {
        (void) pCallbackFn;
        (void) pCallbackArg;

        return -1;
    }

    inline bool UDPSocketAdapter::isConnected (void) const
    {
        return true;
    }

    inline bool UDPSocketAdapter::isEnqueueingAllowed (void) const
    {
        return _udpConnectionThread.getAmountOfBufferedBytes() < NetworkConfigurationSettings::UDP_CONNECTION_BUFFER_SIZE;
    }

    // TO-DO: for the moment, encryption is always set to PLAIN
    inline EncryptionType UDPSocketAdapter::getEncryptionType (void) const
    {
        return ET_PLAIN;
    }

    inline uint32 UDPSocketAdapter::getOutgoingBufferSize (void) const
    {
        return _upUDPSocket->getSendBufferSize();
    }

    inline uint16 UDPSocketAdapter::getLocalPort (void) const
    {
        return _upUDPSocket->getLocalPort();
    }

    inline int UDPSocketAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        (void) pcRemoteProxyIP;
        (void) ui16RemoteProxyPort;

        return 0;
    }

    inline int UDPSocketAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                                       bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        (void) bReliable;
        (void) bSequenced;
        (void) ui32BufSize;
        if (!pInetAddr) {
            return -1;
        }

        if (0 > _udpConnectionThread.addProxyNetworkMessage (pInetAddr, (ProxyMessage * const) pBuf, ui32SourceIPAddr, ui32DestinationIPAddr)) {
            return -2;
        }

        return 0;
    }

    inline int UDPSocketAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        return _upUDPSocket->shutdown (bReadMode, bWriteMode);
    }

    inline int UDPSocketAdapter::close (void)
    {
        return _upUDPSocket->close();
    }

    inline bool UDPSocketAdapter::areThereTCPTypePacketsInUDPTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID) const
    {
        return _udpConnectionThread.areThereTCPTypePacketsInUDPTransmissionQueue (uiLocalID, uiRemoteID);
    }

    inline unsigned int UDPSocketAdapter::removeTCPTypePacketsFromTransmissionQueue (uint16 uiLocalID, uint16 uiRemoteID)
    {
        return _udpConnectionThread.removeTCPTypePacketsFromTransmissionQueue (uiLocalID, uiRemoteID);
    }

    inline int UDPSocketAdapter::startUDPConnectionThread (void)
    {
        return _udpConnectionThread.start();
    }

    inline void UDPSocketAdapter::requestUDPConnectionThreadTermination (void)
    {
        _udpConnectionThread.requestTermination();
    }

    inline void UDPSocketAdapter::requestUDPConnectionThreadTerminationAndWait (void)
    {
        _udpConnectionThread.requestTerminationAndWait();
    }

    inline int UDPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        (void) i64Timeout;
        return _upUDPSocket->receive (pBuf, ui32BufSize);
    }

    inline int UDPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pRemoteInetAddr, int64 i64Timeout)
    {
        (void) i64Timeout;
        return _upUDPSocket->receive (pBuf, ui32BufSize, pRemoteInetAddr);
    }

    inline int UDPSocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, NOMADSUtil::InetAddr * const pLocalInetAddr,
                                          NOMADSUtil::InetAddr * const pRemoteInetAddr, int64 i64Timeout)
    {
        (void) i64Timeout;
        return _upUDPSocket->receive (pBuf, ui32BufSize, pLocalInetAddr, pRemoteInetAddr);
    }

    inline UDPSocketAdapter::UDPConnectionThread::UDPConnectionThread (NOMADSUtil::UDPDatagramSocket * const pUDPSocket, EncryptionType encryptionType) :
        _encryptionType{encryptionType}, _ui32TotalBufferedBytes{0}, _pUDPSocket{pUDPSocket}
    { }

    inline UDPSocketAdapter::UDPConnectionThread::~UDPConnectionThread (void) { }

    inline unsigned int UDPSocketAdapter::UDPConnectionThread::getTransmissionQueueSize (void) const
    {
        // Locking is necessary in case a transmission or removeTCPTypePacketsFromTransmissionQueue() have been called, so that only a consistent result will be returned
        std::lock_guard<std::mutex> lg{_mtxProxyMessagesDeq};
        return _deqProxyNetworkMessages.size();
    }

    inline unsigned int UDPSocketAdapter::UDPConnectionThread::getAmountOfBufferedBytes (void) const
    {
        // Locking is necessary in case a transmission or removeTCPTypePacketsFromTransmissionQueue() have been called, so that only a consistent result will be returned
        std::lock_guard<std::mutex> lg{_mtxProxyMessagesDeq};
        return _ui32TotalBufferedBytes;
    }

    inline int UDPSocketAdapter::UDPConnectionThread::sendProxyNetworkMessageToRemoteProxy (const ProxyNetworkMessage & rProxyNetworkMessage)
    {
        if (0 >= sendProxyNetworkMessageToRemoteProxy (rProxyNetworkMessage.getDestInetAddr(), rProxyNetworkMessage.getProxyMessage(),
                                                       rProxyNetworkMessage.getSourceIPAddr(), rProxyNetworkMessage.getDestVirtualIPAddr(),
                                                       rProxyNetworkMessage.getPointerToPayload(), rProxyNetworkMessage.getMessagePayloadLength())) {
            return -1;
        }

        return 0;
    }

}

#endif  // INCL_UDP_SOCKET_ADAPTER_H
