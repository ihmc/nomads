#ifndef INCL_CONNECTOR_ADAPTER_H
#define INCL_CONNECTOR_ADAPTER_H

/*
 * ConnectorAdapter.h
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Interface that wraps the Mocket, TCPSocket, and UDPDatagramSocket
 * classes to provide a single, uniform interface.
 */

#include <stdarg.h>

#include "FTypes.h"
#include "InetAddr.h"
#include "UInt64Hashtable.h"
#include "Mutex.h"
#include "Mocket.h"
#include "TCPSocket.h"
#include "UDPDatagramSocket.h"

#include "Utilities.h"


namespace ACMNetProxy
{
    class CompressionSetting;

    class ConnectorAdapter
    {
    public:
        ConnectorAdapter (ConnectorType connectorType, uint8 * const pui8MemBuf, uint16 ui16MemBufSize);
        ConnectorAdapter (ConnectorType connectorType, uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort, uint8 * const pui8MemBuf, uint16 ui16MemBufSize);
        virtual ~ConnectorAdapter (void);

        virtual int bufferingMode (int iMode) = 0;
        virtual int readConfigFile (const char * const pszConfigFile) = 0;
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg) = 0;

        virtual bool isConnected (void) const = 0;
        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort) = 0;
        virtual int shutdown (bool bReadMode, bool bWriteMode) = 0;
        virtual int close (void) = 0;

        virtual uint32 getOutgoingBufferSize (void) = 0;

        // Single buffer version of send
        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                          const void *pBuf, uint32 ui32BufSize) = 0;
        // Variable argument version of send (to handle a gather write)
        int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                   const void *pBuf1, uint32 ui32BufSize1, ...);
        // Retrieves the data from next message that is ready to be delivered to the application
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize) = 0;

        ConnectorType getConnectorType (void) const;
        const char * const getConnectorTypeAsString (void) const;
        NOMADSUtil::InetAddr * const getRemoteInetAddr (void) const;
        uint32 getRemoteIPAddr (void) const;

        static ConnectorAdapter * const ConnectorAdapterFactory (ConnectorType connectorType, const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr);
        static ConnectorAdapter * const ConnectorAdapterFactory (Mocket * const pMocket, ConnectorType connectorType);
        static ConnectorAdapter * const ConnectorAdapterFactory (NOMADSUtil::TCPSocket * const pTCPSocket);
        static ConnectorAdapter * const ConnectorAdapterFactory (NOMADSUtil::UDPDatagramSocket * const pUDPSocket);


    protected:
        bool checkCorrectHeader (const void * const pBuf, int32 i32BufSize) const;
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                           const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2) = 0;
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0) = 0;

        NOMADSUtil::InetAddr _remoteProxyInetAddr;
        uint8 * const _pui8MemBuf;
        const uint16 _ui16MemBufSize;
        NOMADSUtil::Mutex _m;


    private:
        explicit ConnectorAdapter (const ConnectorAdapter& rConnectorAdapter);

        const ConnectorType _connectorType;
        //NOMADSUtil::UInt64Hashtable<Connection> _pConnections;
        NOMADSUtil::Mutex _mConnections;
    };


    inline ConnectorAdapter::ConnectorAdapter (ConnectorType connectorType, uint8 * const pui8MemBuf, uint16 ui16MemBufSize) :
        _pui8MemBuf (pui8MemBuf), _ui16MemBufSize (ui16MemBufSize), _connectorType (connectorType) { }

    inline ConnectorAdapter::ConnectorAdapter (ConnectorType connectorType, uint32 ui32RemoteProxyIP, uint16 ui16RemoteProxyPort,
                                               uint8 * const pui8MemBuf, uint16 ui16MemBufSize) :
        _pui8MemBuf (pui8MemBuf), _ui16MemBufSize (ui16MemBufSize), _connectorType (connectorType)
    {
        _remoteProxyInetAddr = NOMADSUtil::InetAddr (ui32RemoteProxyIP, ui16RemoteProxyPort);
    }

    // Default implementation of a pure virtual method
    inline int ConnectorAdapter::readConfigFile (const char * const pszConfigFile)
    {
        (void) pszConfigFile;
        return 0;
    }

    inline int ConnectorAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                                        const void *pBuf1, uint32 ui32BufSize1, ...)
    {
        int rc;
        va_list valist1, valist2;

        va_start (valist1, ui32BufSize1);
        va_start (valist2, ui32BufSize1);
        rc = gsend (pInetAddr, ui32DestVirtualIPAddr, bReliable, bSequenced, pBuf1, ui32BufSize1, valist1, valist2);
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

    inline NOMADSUtil::InetAddr * const ConnectorAdapter::getRemoteInetAddr (void) const
    {
        return const_cast<NOMADSUtil::InetAddr * const> (&_remoteProxyInetAddr);
    }

    inline uint32 ConnectorAdapter::getRemoteIPAddr (void) const
    {
        return _remoteProxyInetAddr.getIPAddress();
    }
}

#endif //INCL_CONNECTOR_ADAPTER_H
