/*
* Dtls.h
*
* This file is part of the IHMC Mockets Library/Component
* Copyright (c) 2002-2016 IHMC.
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
*/
#ifndef INCL_DTLS
#define INCL_DTLS

#include "FTypes.h"

#ifdef WIN32
    #include <winsock2.h>
#endif

#include <stdio.h>
#include "DTLSConstants.h"
#include "openssl/ssl.h"
#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "openssl/dh.h"
#include "openssl/conf.h"
#include "openssl/engine.h"
#include "string.h"
#include "Logger.h"

namespace IHMC_DTLS
{
    enum Mode { CLIENT, SERVER };
    struct dtls_fields{
        SSL_CTX* ctx;   //main ssl context
        SSL* ssl;        //the SSL* which represents a connection
        BIO* in_bio;    //memory read bios
        BIO* out_bio;    //memory write bios
        char name[512];
    };

    class DtlsEndpoint
    {
    public:
        DtlsEndpoint(const char* pathToCertificate, const char* pathToPrivateKey);
        ~DtlsEndpoint();
        int handShake(char **msg);
        void init(bool isServer);
        bool isHandshakeOver();
        int prepareDataForSending(const char* msg, int msgSize, char** encrypted);
        int recoverData(const char *encrypted, int enSize, char **decrypted);

    private:
        int commonInitializationCTX(void);
        int commonInitializationSSL(void);
        bool fileExists(const char *pszFilePath);
        int getEncryptedBuffer(dtls_fields* connection, char *buf);
        bool getIsServer(void) const;
        void handlerCleanup(dtls_fields* handler);
        bool handshakeIsStarted() const;
        int loadCertificateAndPrivateKey(
            dtls_fields *pCommonHandler,
            const char  *pszCertificate,
            const char  *pszKey);

        static int noverify(int ok, X509_STORE_CTX* ctx);
        void setIsServer(bool isServer);
        //<---------------------------------------------------------------------------------------------------------------------->
    private:
        char            _cCertfile[1024];
        char            _cKeyfile[1024];
        bool            _bHandShakeStarted;
        bool            _bHandShakeFinished;
        uint32          _iMtu;
        bool            _bIsServer;
        dtls_fields*    _pServer;
        dtls_fields*    _pClient;
        char *          _cpPathToCertificate;
        char*           _cpPathToPrivateKey;
    };

    inline bool DtlsEndpoint::getIsServer(void) const { return _bIsServer; }

    inline bool DtlsEndpoint::handshakeIsStarted(void) const { return _bHandShakeStarted; }

    inline void DtlsEndpoint::setIsServer(bool isServer) { _bIsServer = isServer; }
}
#endif