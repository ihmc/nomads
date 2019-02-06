#ifndef INCL_CONNECTOR_ADAPTER_H
#define INCL_CONNECTOR_ADAPTER_H

/*
 * ConnectorAdapter.h
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Interface that wraps the Mocket, TCPSocket, and UDPDatagramSocket
 * classes to provide a single, uniform interface.
 */

#include <stdarg.h>
#include <mutex>

#include "FTypes.h"
#include "InetAddr.h"
#include "PeerStatusCallbacks.h"

#include "Utilities.h"


class Mocket;


namespace NOMADSUtil
{
    class TCPSocket;
    class UDPDatagramSocket;
}


namespace ACMNetProxy
{
    class ConnectionManager;


    class ConnectorAdapter
    {
    public:
        virtual ~ConnectorAdapter (void);

        virtual int bufferingMode (int iMode) = 0;
        virtual int readConfigFile (const char * const pszConfigFile) = 0;
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg) = 0;

        virtual bool isConnected (void) const = 0;
        virtual EncryptionType getEncryptionType (void) const = 0;
        virtual uint32 getOutgoingBufferSize (void) const = 0;

        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort) = 0;
        // Single buffer version of send
        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                          bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize) = 0;
        // Variable argument version of send (to handle a gather write)
        int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                   bool bReliable, bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, ...);
        // Retrieves the data from next message that is ready to be delivered to the application
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize) = 0;
        virtual int shutdown (bool bReadMode, bool bWriteMode) = 0;
        virtual int close (void) = 0;

        ConnectorType getConnectorType (void) const;
        const char * const getConnectorTypeAsString (void) const;
        const NOMADSUtil::InetAddr & getLocalInetAddr (void) const;
        virtual uint16 getLocalPort (void) const = 0;
        const NOMADSUtil::InetAddr & getRemoteInetAddr (void) const;


        static ConnectorAdapter * const ConnectorAdapterFactory (ConnectionManager & rConnectionManager, ConnectorType connectorType,
                                                                 EncryptionType encryptionType, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                 const NOMADSUtil::InetAddr & iaRemoteProxyAddr);
        static ConnectorAdapter * const ConnectorAdapterFactory (Mocket * const pMocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                 ConnectorType connectorType);
        static ConnectorAdapter * const ConnectorAdapterFactory (NOMADSUtil::TCPSocket * const pTCPSocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                 ConnectorType connectorType);
        static ConnectorAdapter * const ConnectorAdapterFactory (NOMADSUtil::UDPDatagramSocket * const pUDPSocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                                                 ConnectorType connectorType);


    protected:
        ConnectorAdapter (ConnectorType connectorType, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                          const NOMADSUtil::InetAddr & iaRemoteProxyAddr, uint8 * const pui8Buffer, uint16 ui16BufferSize);

        bool checkCorrectHeader (const void * const pBuf, int32 i32BufSize) const;

        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr, bool bReliable,
                           bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2) = 0;
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0) = 0;


        NOMADSUtil::InetAddr _iaLocalProxyAddr;
        NOMADSUtil::InetAddr _iaRemoteProxyAddr;
        std::unique_ptr<uint8> _upui8Buffer;
        const uint16 _ui16BufferSize;

        std::mutex _mtx;


    private:
        explicit ConnectorAdapter (const ConnectorAdapter & rConnectorAdapter);


        const ConnectorType _connectorType;
    };


    inline ConnectorAdapter::ConnectorAdapter (ConnectorType connectorType, const NOMADSUtil::InetAddr & iaLocalProxyAddr,
                                               const NOMADSUtil::InetAddr & iaRemoteProxyAddr, uint8 * const pui8Buffer, uint16 ui16BufferSize) :
        _iaLocalProxyAddr{iaLocalProxyAddr}, _iaRemoteProxyAddr{iaRemoteProxyAddr}, _upui8Buffer{pui8Buffer},
        _ui16BufferSize{ui16BufferSize}, _connectorType{connectorType}
    { }

    inline ConnectorAdapter::~ConnectorAdapter (void) { }

    // Default implementation of a pure virtual method
    inline int ConnectorAdapter::readConfigFile (const char * const pszConfigFile)
    {
        (void) pszConfigFile;
        return 0;
    }

    inline int ConnectorAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestIPAddr,
                                        bool bReliable, bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, ...)
    {
        int rc;
        va_list valist1, valist2;

        va_start (valist1, ui32BufSize1);
        va_start (valist2, ui32BufSize1);
        rc = gsend (pInetAddr, ui32SourceIPAddr, ui32DestIPAddr, bReliable, bSequenced, pBuf1, ui32BufSize1, valist1, valist2);
        va_end (valist1);
        va_end (valist2);

        return rc;
    }

    inline ConnectorType ConnectorAdapter::getConnectorType (void) const
    {
        return _connectorType;
    }

    inline const char * const ConnectorAdapter::getConnectorTypeAsString (void) const
    {
        return connectorTypeToString (_connectorType);
    }

    inline const NOMADSUtil::InetAddr & ConnectorAdapter::getLocalInetAddr (void) const
    {
        return _iaLocalProxyAddr;
    }

    inline uint16 ConnectorAdapter::getLocalPort (void) const
    {
        return _iaLocalProxyAddr.getPort();
    }

    inline const NOMADSUtil::InetAddr & ConnectorAdapter::getRemoteInetAddr (void) const
    {
        return _iaRemoteProxyAddr;
    }

}

#endif //INCL_CONNECTOR_ADAPTER_H
