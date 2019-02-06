#ifndef INCL_CSR_ADAPTER_H
#define INCL_CSR_ADAPTER_H

/*
 * CSRAdapter.h
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
 * The CSRAdapter class is an adapter that complies to the
 * ConnectorAdapter interface and implements its methods in
 * terms of the Mocket class interface to be used over CSR.
 */

#include <memory>

#include "InetAddr.h"
#include "Mocket.h"
#include "ProxyCommInterface.h"

#include "ConfigurationParameters.h"
#include "ConnectorAdapter.h"


namespace ACMNetProxy
{
    class CSRAdapter : public ConnectorAdapter
    {
    public:
        CSRAdapter (Mocket * const pMocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr);
        CSRAdapter (const NOMADSUtil::InetAddr & iaLocalProxyAddr, const NOMADSUtil::InetAddr & iaRemoteProxyAddr,
                    EncryptionType encryptionType);
        ~CSRAdapter (void);

        virtual int bufferingMode (int iMode);
        virtual int readConfigFile (const char * const pszConfigFile);
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        virtual EncryptionType getEncryptionType (void) const;
        virtual uint32 getOutgoingBufferSize (void) const;
        virtual uint16 getLocalPort (void) const;

        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                            bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr, bool bReliable,
                            bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);


    private:
        std::unique_ptr<Mocket> _upMocket;
    };


    inline CSRAdapter::CSRAdapter (Mocket * const pMocket, const NOMADSUtil::InetAddr & iaLocalProxyAddr) :
        ConnectorAdapter{CT_CSR, iaLocalProxyAddr, {pMocket->getRemoteAddress(), pMocket->getRemotePort()}, nullptr, 0},
        _upMocket{pMocket}
    { }

    inline CSRAdapter::CSRAdapter (const NOMADSUtil::InetAddr & iaLocalProxyAddr, const NOMADSUtil::InetAddr & iaRemoteProxyAddr,
                                   EncryptionType encryptionType) :
        ConnectorAdapter{CT_CSR, iaLocalProxyAddr, iaRemoteProxyAddr, nullptr, 0}, _upMocket{(encryptionType == ET_DTLS) ?
            new Mocket{nullptr, new ProxyCommInterface{"127.0.0.1", NetProxyApplicationParameters::CSR_PROXY_SERVER_PORT}, false, true} :
            new Mocket{nullptr, new ProxyCommInterface{"127.0.0.1", NetProxyApplicationParameters::CSR_PROXY_SERVER_PORT}}}
    { }

    inline CSRAdapter::~CSRAdapter (void) { }

    inline int CSRAdapter::bufferingMode (int iMode)
    {
        (void) iMode;
        return -1;
    }

    inline int CSRAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return _upMocket->readConfigFile (pszConfigFile);
    }

    inline int CSRAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
    {
        return _upMocket->registerPeerUnreachableWarningCallback (pCallbackFn, pCallbackArg);
    }

    inline bool CSRAdapter::isConnected (void) const
    {
        return _upMocket->isConnected();
    }

    inline EncryptionType CSRAdapter::getEncryptionType (void) const
    {
        return _upMocket->isEncrypted() ? ET_DTLS : ET_PLAIN;
    }

    inline uint32 CSRAdapter::getOutgoingBufferSize (void) const
    {
        return _upMocket->getOutgoingBufferSize();
    }

    inline uint16 CSRAdapter::getLocalPort (void) const
    {
        return _upMocket->getLocalPort();
    }

    inline int CSRAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        if (_iaLocalProxyAddr != 0) {
            if (_upMocket->bind (_iaLocalProxyAddr.getIPAsString(), _iaLocalProxyAddr.getPort()) < 0) {
                return -100;
            }
        }

        return _upMocket->connect (pcRemoteProxyIP, ui16RemoteProxyPort);
    }

    inline int CSRAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                                 bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        (void) pInetAddr;
        (void) ui32SourceIPAddr;
        (void) ui32DestinationIPAddr;

        return _upMocket->send (bReliable, bSequenced, pBuf, ui32BufSize, 0, 0, 0, 0);
    }

    inline int CSRAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        return _upMocket->receive (pBuf, ui32BufSize);
    }

    inline int CSRAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        if (bReadMode || bWriteMode) {
            return _upMocket->applicationAbort();
        }

        return 0;
    }

    inline int CSRAdapter::close (void)
    {
        return _upMocket->close();
    }

    inline int CSRAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32SourceIPAddr, uint32 ui32DestinationIPAddr,
                                  bool bReliable, bool bSequenced, const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
    {
        (void) pInetAddr;
        (void) ui32SourceIPAddr;
        (void) ui32DestinationIPAddr;

        return _upMocket->gsend (bReliable, bSequenced, 0, 0, 0, 0, pBuf1, ui32BufSize1, valist1, valist2);
    }

    inline int CSRAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        return _upMocket->receive (pBuf, ui32BufSize, i64Timeout);
    }
}

#endif   //INCL_CSR_ADAPTER_H
