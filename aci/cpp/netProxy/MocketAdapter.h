#ifndef INCL_MOCKET_ADAPTER_H
#define INCL_MOCKET_ADAPTER_H

/*
 * MocketAdapter.h
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
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * The MocketAdapter class is an adapter that complies to the
 * ConnectorAdapter interface and implements its methods in
 * terms of the Mocket class interface.
 */

#include "InetAddr.h"
#include "Mocket.h"
#include "ProxyCommInterface.h"

#include "ConnectorAdapter.h"


namespace ACMNetProxy
{
    class MocketAdapter : public ConnectorAdapter
    {
    public:
        MocketAdapter (Mocket * const pMocket);
        MocketAdapter (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr);
        ~MocketAdapter (void);

        virtual int bufferingMode (int iMode);
        virtual int readConfigFile (const char * const pszConfigFile);
        virtual int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        virtual bool isConnected (void) const;
        virtual int connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        virtual int close (void);

        virtual uint32 getOutgoingBufferSize (void);

        virtual int send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable,
                          bool bSequenced, const void *pBuf, uint32 ui32BufSize);
        virtual int receiveMessage (void * const pBuf, uint32 ui32BufSize);


    protected:
        virtual int gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                           const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);
        virtual int receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);


    private:
        Mocket * const _pMocket;
    };


    inline MocketAdapter::MocketAdapter (Mocket * const pMocket) :
        ConnectorAdapter (CT_MOCKETS, pMocket->getRemoteAddress(), pMocket->getRemotePort(), NULL, 0), _pMocket (pMocket) { }

    inline MocketAdapter::MocketAdapter (const NOMADSUtil::InetAddr * const pRemoteProxyInetAddr) :
        ConnectorAdapter (CT_MOCKETS, pRemoteProxyInetAddr->getIPAddress(), pRemoteProxyInetAddr->getPort(), NULL, 0), _pMocket (new Mocket()) { }

    inline MocketAdapter::~MocketAdapter (void)
    {
        delete _pMocket;
    }

    inline int MocketAdapter::bufferingMode (int iMode)
    {
        return -1;
    }

    inline int MocketAdapter::readConfigFile (const char * const pszConfigFile)
    {
        return _pMocket->readConfigFile (pszConfigFile);
    }

    inline int MocketAdapter::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
    {
        return _pMocket->registerPeerUnreachableWarningCallback (pCallbackFn, pCallbackArg);
    }

    inline bool MocketAdapter::isConnected (void) const
    {
        return _pMocket->isConnected();
    }

    inline int MocketAdapter::connect (const char * const pcRemoteProxyIP, uint16 ui16RemoteProxyPort)
    {
        return _pMocket->connect (pcRemoteProxyIP, ui16RemoteProxyPort);
    }

    inline int MocketAdapter::shutdown (bool bReadMode, bool bWriteMode)
    {
        return _pMocket->close();
    }

    inline int MocketAdapter::close (void)
    {
        return _pMocket->close();
    }

    inline uint32 MocketAdapter::getOutgoingBufferSize (void)
    {
        return _pMocket->getOutgoingBufferSize();
    }

    inline int MocketAdapter::send (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable,
                                    bool bSequenced, const void *pBuf, uint32 ui32BufSize)
    {
        return _pMocket->send (bReliable, bSequenced, pBuf, ui32BufSize, 0, 0, 0, 0);
    }

    inline int MocketAdapter::receiveMessage (void * const pBuf, uint32 ui32BufSize)
    {
        return _pMocket->receive (pBuf, ui32BufSize);
    }

    inline int MocketAdapter::receive (void * const pBuf, uint32 ui32BufSize, int64 i64Timeout)
    {
        return _pMocket->receive (pBuf, ui32BufSize, i64Timeout);
    }

    inline int MocketAdapter::gsend (const NOMADSUtil::InetAddr * const pInetAddr, uint32 ui32DestVirtualIPAddr, bool bReliable, bool bSequenced,
                                     const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
    {
        return _pMocket->gsend (bReliable, bSequenced, 0, 0, 0, 0, pBuf1, ui32BufSize1, valist1, valist2);
    }
}

#endif //INCL_MOCKET_ADAPTER_H
