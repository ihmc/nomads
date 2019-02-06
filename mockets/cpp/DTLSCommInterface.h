#ifndef INCL_DTLS_COMM_INTERFACE_H
#define INCL_DTLS_COMM_INTERFACE_H

/*
* DTLSommInterface.h
*
* This file is part of the IHMC Mockets Library/Component
* Copyright (c) 2002-2014 IHMC.
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
*
* Author:                   Roberto Fronteddu (rfronteddu@ihmc.us)
* Year of creation:         2015/2016
* Last Revision by:
* Year of last Revision:
* Description:
*
*
*/
#include "CommInterface.h"
#include "UDPCommInterface.h"
#include "Dtls.h"
#include "InetAddr.h"
#include "StrClass.h"
//#include "dtlsInfrastructure.h"
class DTLSCommInterface : public CommInterface
{
    public:
        DTLSCommInterface (CommInterface * pCI, bool server, IHMC_DTLS::DtlsEndpoint ** dtls);
        DTLSCommInterface (CommInterface * pCI, bool server, uint32 ui32RcvTimeout,
                           const char * pathToCertificate, const char * pathToPrivateKey);
        virtual ~DTLSCommInterface(void);

        int handleRcv (const int read, char * pBuf, const int handshakeStep, NOMADSUtil::InetAddr * pRemoteAddr);
        void handleNoRcv (void);
        void handleRcvError (const int rc);
        virtual int bind (uint16 ui16Port);
        virtual int bind (NOMADSUtil::InetAddr * pLocalAddr);

        virtual int close (void);
        virtual int shutdown (bool bReadMode, bool bWriteMode);
        void deleteDtlsObject (void);
        virtual NOMADSUtil::InetAddr getLocalAddr (void);
        int getDtlsObject (IHMC_DTLS::DtlsEndpoint ** dtls);
        virtual int getLastError (void);
        virtual int isRecoverableSocketError (void);

        virtual int getLocalPort (void);

        virtual CommInterface * newInstance (void);

        /*
        * When no certificate/key are specified, use defaults
        */
        int resetDtlsObject (void);

        virtual int setReceiveTimeout (uint32 ui32TimeoutInMS);
        virtual int setReceiveBufferSize (uint32 ui32BufferSize);
        virtual int sendTo (NOMADSUtil::InetAddr * pRemoteAddr, const void * pBuf, int iBufSize, const char * pszHints = nullptr);

        virtual int receive (void * pBuf, int iBufSize, NOMADSUtil::InetAddr * pRemoteAddr);
        void setCertificatePaths (const char * pathToCertificate, const char * pathToPrivateKey);

        virtual int setMTU (const int newMTU);
        void setTimeout (const int64 ui32Timeout);


    private:
        virtual bool doHandshake (NOMADSUtil::InetAddr * pRemoteAddr);
        virtual bool startHandshake (NOMADSUtil::InetAddr * pRemoteAddr);

        void handleInitializationError (const int errorCode, const bool isServer);

        bool setLastHanshakeMessage (const char * buffer, const int size, const NOMADSUtil::InetAddr * pRemoteAddr);
        bool lastHandshakeMessageHasBeenSet();
        bool sendHSAgain (void);
        bool freeLastHanshakeMessage (void);


        bool _bLastHanshakeMessageHasBeenSet;
        char* _pcLastHanshakeMessage;
        int _lastMessageSize;
        NOMADSUtil::InetAddr *_pLastRemoteAddr;

        CommInterface *_pCI;
        bool _bDeleteDGSocketWhenDone;
        bool _bIsServer;
        uint32 _mtu;
        IHMC_DTLS::DtlsEndpoint *_pDTLS;

        uint32 _numberOfAttempts;
        uint32 _intervalBetweenAttempts;
        uint32 _ui32RcvTimeout;
        int64  _int64Timeout;
        NOMADSUtil::String _sPathToCertificate;
        NOMADSUtil::String _sPathToPrivateKey;

        const char * pszServerDefaultKeyPath    = "server-key.pem";
        const char * pszServerDefaultCertPath   = "server-cert.pem";
        const char * pszClientDefaultKeytPath   = "client-key.pem";
        const char * pszClientDefaultCertPath   = "client-cert.pem";
};


inline int DTLSCommInterface::bind (uint16 ui16Port)
{
    return _pCI->bind (ui16Port);
}

inline int DTLSCommInterface::bind (NOMADSUtil::InetAddr *pLocalAddr)
{
    return _pCI->bind (pLocalAddr);
}

inline int DTLSCommInterface::close (void)
{
    return _pCI->close();
}
inline int DTLSCommInterface::shutdown (bool bReadMode, bool bWriteMode)
{
    return _pCI->shutdown (bReadMode, bWriteMode);
}
inline int DTLSCommInterface::getLastError (void)
{
    return _pCI->getLastError();
}

inline NOMADSUtil::InetAddr DTLSCommInterface::getLocalAddr (void)
{
    return _pCI->getLocalAddr();
}

inline int DTLSCommInterface::getLocalPort (void)
{
    return _pCI->getLocalPort();
}

inline bool DTLSCommInterface::lastHandshakeMessageHasBeenSet (void)
{
    return _bLastHanshakeMessageHasBeenSet;
}

inline CommInterface *DTLSCommInterface::newInstance (void)
{
    return _pCI->newInstance();
}

inline void DTLSCommInterface::setTimeout (const int64 i64Timeout)
{
    _int64Timeout = i64Timeout;
}

inline int DTLSCommInterface::setReceiveBufferSize (uint32 ui32BufferSize)
{
    return _pCI->setReceiveBufferSize (ui32BufferSize);
}

inline int DTLSCommInterface::setReceiveTimeout (uint32 ui32TimeoutInMS)
{
    _ui32RcvTimeout = ui32TimeoutInMS;
    return _pCI->setReceiveTimeout (ui32TimeoutInMS);
}

#endif   // #ifndef INCL_DTLS_COMM_INTERFACE_H