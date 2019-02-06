/*
 * ServerMocket.cpp
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
 */

#include "ServerMocket.h"

#include "Mocket.h"
#include "UDPCommInterface.h"
#include "DTLSCommInterface.h"
#include "UDPDatagramSocket.h"
#include "InetAddr.h"
#include "Logger.h"
#include "net/NetUtils.h"

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

ServerMocket::ServerMocket(
    const char *pszConfigFile,
    CommInterface *pCI,
    bool bDeleteCIWhenDone,
    bool enableDtls,
    const char* pathToCertificate,
    const char* pathToPrivateKey)
    : _cvInAccept (&_mInAccept)
{
    _configFile = pszConfigFile;
    _bAccepting = false;
    _bInAccept = false;
    _ui16Port = 0;
    _ui32ListenAddr = 0;
    _bEnableDtls = enableDtls;

    if (pCI == nullptr) {
        _pCommInterface = nullptr;
        _bLocallyCreatedCI = true;
        _bDeleteCIWhenDone = true;
    }
    else {
        _pCommInterface = pCI;
        _bLocallyCreatedCI = false;
        _bDeleteCIWhenDone = bDeleteCIWhenDone;
    }

    pathToCertificate ? (_cpPathToCertificate = strdup(pathToCertificate)) : (_cpPathToCertificate = nullptr);
    pathToPrivateKey ? (_cpPathToPrivateKey = strdup(pathToPrivateKey)) : (_cpPathToPrivateKey = nullptr);

}

ServerMocket::~ServerMocket (void)
{
    close();
    _mInAccept.lock();
    while (_bInAccept) {
        _cvInAccept.wait();
    }
    _mInAccept.unlock();
    if (_bDeleteCIWhenDone) {
        delete _pCommInterface;
    }
    _pCommInterface = nullptr;
    free(_cpPathToCertificate);
    free(_cpPathToPrivateKey);
}

int ServerMocket::listen (uint16 ui16Port)
{
    if (_bEnableDtls) {
        if ((_bLocallyCreatedCI) && (_pCommInterface == nullptr)) {
            DTLSCommInterface *pSupport = new DTLSCommInterface(new UDPCommInterface(new UDPDatagramSocket(), true), true, 500, _cpPathToCertificate, _cpPathToPrivateKey);
            _pCommInterface = pSupport;
            _bLocallyCreatedCI = true;
            _bDeleteCIWhenDone = true;
        }
        else if (_pCommInterface == nullptr) {
            return -1;
        }
    }
    else {
        if ((_bLocallyCreatedCI) && (_pCommInterface == nullptr)) {
            _pCommInterface = new UDPCommInterface(new UDPDatagramSocket(), true);
            _bLocallyCreatedCI = true;
            _bDeleteCIWhenDone = true;
        }
        else if (_pCommInterface == nullptr) {
            return -1;
        }
    }

    if (_pCommInterface->bind(ui16Port)) {
        checkAndLogMsg("ServerMocket::listen", Logger::L_HighDetailDebug,
            "failed to bind to port %d\n", ui16Port);
        return -2;
    }
    _bAccepting = true;
    if (ui16Port == 0) {
        ui16Port = _pCommInterface->getLocalPort();
    }
    _ui16Port = ui16Port;
    return ui16Port;
}

//todo add support to dtls
int ServerMocket::listen (uint16 ui16Port, const char *pszListenAddr)
{
    if (pszListenAddr != nullptr) {
        checkAndLogMsg("ServerMocket::listen", Logger::L_Info, "Stored listen addr: %s\n", pszListenAddr);
        _ui32ListenAddr = InetAddr(pszListenAddr).getIPAddress();
    }

    if (_bEnableDtls) {
        if ((_bLocallyCreatedCI) && (_pCommInterface == nullptr)) {
            DTLSCommInterface *pSupport = new DTLSCommInterface(
                new UDPCommInterface(new UDPDatagramSocket(), true),
                true,
                500,
                _cpPathToCertificate, _cpPathToPrivateKey);
            _pCommInterface = pSupport;
            _bLocallyCreatedCI = true;
            _bDeleteCIWhenDone = true;
        }
        else if (_pCommInterface == nullptr) {
            return -1;
        }
        InetAddr addr(NetUtils::getHostByName(pszListenAddr), ui16Port);
        if (_pCommInterface->bind(&addr)) {
            checkAndLogMsg("ServerMocket::listen2", Logger::L_MildError,
                "failed to bind to port %d and address %s\n", (int)ui16Port, pszListenAddr);
            return -2;
        }
        _bAccepting = true;
        if (ui16Port == 0) {
            ui16Port = _pCommInterface->getLocalPort();
        }
        _ui16Port = ui16Port;
        return ui16Port;
    }
    else {
        if ((_bLocallyCreatedCI) && (_pCommInterface == nullptr)) {
            _pCommInterface = new UDPCommInterface(new UDPDatagramSocket(), true);
            _bLocallyCreatedCI = true;
            _bDeleteCIWhenDone = true;
        }
        else if (_pCommInterface == nullptr) {
            return -1;
        }

        InetAddr addr(NetUtils::getHostByName(pszListenAddr), ui16Port);
        if (_pCommInterface->bind(&addr)) {
            checkAndLogMsg("ServerMocket::listen2", Logger::L_MildError,
                "failed to bind to port %d and address %s\n", (int)ui16Port, pszListenAddr);
            return -2;
        }
        _bAccepting = true;
        if (ui16Port == 0) {
            ui16Port = _pCommInterface->getLocalPort();
        }
        _ui16Port = ui16Port;
        return ui16Port;
    }
}

Mocket *ServerMocket::accept (uint16 ui16PortForNewConnection)
{
    const char * const pszMethodName = "ServerMocket::accept";
    char buf [Mocket::MAXIMUM_MTU];
    bool bHandShakeFinished = false;
    int rc = 0;

    if (_pCommInterface == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "ServerMocket has not been initialized by calling listen()\n");
        return nullptr;
    }

    if ((!_bLocallyCreatedCI) && (ui16PortForNewConnection != 0)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "cannot specify a local port when using an external CommInterface\n");
        return nullptr;
    }

    #if defined (LINUX)
        // Linux seems to have different behavior than Solaris or MacOSX - when a thread is blocked
        // in a receive on a socket, closing the socket from a different thread does not unblock
        // the original thread
        // Hence - set a timeout which basically makes the accept() poll for one second intervals
        _pCommInterface->setReceiveTimeout (1000);
    #endif

    while (true) {
        _mInAccept.lock();
        if (!_bAccepting) {
            _bInAccept = false;
            _cvInAccept.notifyAll();
            _mInAccept.unlock();
            return nullptr;
        }
        _bInAccept = true;
        _mInAccept.unlock();

        InetAddr remoteAddr;
        if (_bEnableDtls) {
            if (!bHandShakeFinished) {
                checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Mocket Server waiting for DTLS connect\n");
                //During the DTLS handshake the two peers will exchange the certificate which will generally be bigger than the MTU
                if ((rc = _pCommInterface->receive(buf, Mocket::MAXIMUM_MTU, &remoteAddr)) == 0) {
                    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Dtls HandShake completed successfully\n");
                    bHandShakeFinished = true;
                }
                else {
                    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Retry DTLS handshake\n");
                    DTLSCommInterface *pI = static_cast<DTLSCommInterface *> (_pCommInterface);
                    pI->deleteDtlsObject();
                    pI->resetDtlsObject();
                    continue;
                }
            }
        }

        rc = _pCommInterface->receive (buf, Mocket::MAXIMUM_MTU, &remoteAddr);
        if (rc < 0) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "failed to receive a packet; rc = %d; OS error = %d\n",  rc, _pCommInterface->getLastError());
            // Originally, this was returning nullptr since the assumption was that this
            // should not have failed
            // However, on Windows, this does seem to fail under some circumstances
            // Therefore, sleep for a short duration and try again
            if (false) {
                // We got here but the remote mockets maybe died, need to restart dtls process
                // Should do this only if it is dtls that returns an error!! +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Accept receive failed, resetting DTLS state\n");
                DTLSCommInterface *pI = static_cast<DTLSCommInterface *> (_pCommInterface);
                bHandShakeFinished = false;
                pI->deleteDtlsObject();
                pI->resetDtlsObject();
            }
            continue;
        }
        else if (rc == 0) {
            // The receive call timed out - just continue to the top of the loop
            // and check if accept() should continue
            continue;
        }
        else if (rc < Packet::HEADER_SIZE) {
            checkAndLogMsg (pszMethodName, Logger::L_MildError, "received a short packet; rc = %d; Packet::HEADER_SIZE = %d\n", rc, Packet::HEADER_SIZE);
            continue;
        }
        checkAndLogMsg (pszMethodName, Logger::L_MediumDetailDebug, "got a packet\n");

        Packet incomingPacket (buf, rc);
        Mocket *pMocket = processIncomingPacket (&incomingPacket, &remoteAddr, ui16PortForNewConnection);
        if (pMocket) {
            _mInAccept.lock();
            _bInAccept = false;
            _cvInAccept.notifyAll();
            _mInAccept.unlock();

            pMocket->setIdentifier (getIdentifier());

            if (_bEnableDtls) {
                DTLSCommInterface *support = (DTLSCommInterface*)_pCommInterface;
                support->resetDtlsObject();
            }

            return pMocket;
        }
    }
}

int ServerMocket::close (void)
{
    _bAccepting = false;
    if (_pCommInterface == nullptr) {
        return -1;
    }
    return _pCommInterface->close();
}

Mocket * ServerMocket::processIncomingPacket (Packet *pPacket, InetAddr *pRemoteAddr, uint16 ui16PortForNewConnection)
{
    switch (pPacket->getChunkType()) {
        case Packet::CT_Init:
        {
            int rc;
            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MediumDetailDebug,
                            "received an init from %s:%d\n",
                            pRemoteAddr->getIPAsString(), pRemoteAddr->getPort());
            uint32 ui32OutgoingValidation = (uint32) rand();
            uint32 ui32InitialControlTSN = 0;
            uint32 ui32InitialReliableSequencedTSN = 0;
            uint32 ui32InitialUnreliableSequencedTSN = 0;
            uint32 ui32InitialReliableUnsequencedId = 0;
            uint32 ui32InitialUnreliableUnsequencedId = 0;
            InitChunkAccessor accessor = pPacket->getInitChunk();
            Packet replyPacket (Mocket::MAXIMUM_MTU);
            replyPacket.setValidation (ui32OutgoingValidation);
            replyPacket.setWindowSize (Mocket::DEFAULT_MAXIMUM_WINDOW_SIZE);
            replyPacket.setSequenceNum (0);
            StateCookie cookie (getTimeInMilliseconds(), Mocket::DEFAULT_COOKIE_LIFESPAN,
                                accessor.getValidation(), ui32OutgoingValidation,
                                accessor.getControlTSN(), ui32InitialControlTSN,
                                accessor.getReliableSequencedTSN(), ui32InitialReliableSequencedTSN,
                                accessor.getUnreliableSequencedTSN(), ui32InitialUnreliableSequencedTSN,
                                accessor.getReliableUnsequencedId(), ui32InitialReliableUnsequencedId,
                                accessor.getUnreliableUnsequencedId(), ui32InitialUnreliableUnsequencedId,
                                pRemoteAddr->getPort(), _ui16Port);
            replyPacket.addInitAckChunk (ui32OutgoingValidation, ui32InitialControlTSN, ui32InitialReliableSequencedTSN, ui32InitialUnreliableSequencedTSN, ui32InitialReliableUnsequencedId, ui32InitialUnreliableUnsequencedId, &cookie);

            if ((rc = _pCommInterface->sendTo (pRemoteAddr, replyPacket.getPacket(), replyPacket.getPacketSize())) < 0) {
                checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "failed to send InitAck packet to client to %s:%d; rc = %d\n",
                                pRemoteAddr->getIPAsString(), (int) pRemoteAddr->getPort(), rc);
            }

            // Commented out the code below because it causes a problem if an Init packet is received again
            // after already receiving the CookieEcho packet (Observed with CSR) - Niranjan Suri - 22-Jan-2014
            //----
            // Clear out any existing cookie record for the remote endpoint that has sent the init packet
            // This is to fix a bug that occurs if a remote side closes a connection and opens a new one
            // that reuses the same port
            //clearCookieRec (*pRemoteAddr);

            return nullptr; // No connection has been established at this point
        }

        case Packet::CT_CookieEcho:
        {
            int rc;
            CookieEchoChunkAccessor accessor = pPacket->getCookieEchoChunk();
            StateCookie cookie = accessor.getStateCookie();
            /*!!*/ // Need to validate cookie here
            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MediumDetailDebug,
                            "received a cookie echo from %s:%d\n",
                            pRemoteAddr->getIPAsString(), pRemoteAddr->getPort());
            Mocket *pMocket = nullptr;
            CookieRec *pCookieRec = getCookieRec (*pRemoteAddr);
            if (pCookieRec->ui16Count == 0) {
                // This is the first CookieEcho - create a local endpoint
                if (_bLocallyCreatedCI) {
                    if (_bEnableDtls) {
                        // We are not using a CommInterface that was passed into ServerMocket, so create a new one for this connection
                        UDPDatagramSocket *pDGSocket = new UDPDatagramSocket();
                        if (_ui32ListenAddr != 0) {
                            // A specific address was passed into the call to listen - bind the socket that is
                            // being created for the new connection to the same port
                            if (pDGSocket->init(ui16PortForNewConnection, _ui32ListenAddr) < 0) {
                                checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "error initializing the socket\n");
                                delete pDGSocket;
                                return nullptr;
                            }
                        }
                        else {
                            if (pDGSocket->init(ui16PortForNewConnection) < 0) {
                                checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "error initializing the socket\n");
                                delete pDGSocket;
                                return nullptr;
                            }
                        }
                        pCookieRec->ui16LocalPort = pDGSocket->getLocalPort();
                        if (pCookieRec->ui16LocalPort == 0) {
                            checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "could not get local port\n");
                            delete pDGSocket;
                            return nullptr;
                        }
                        CommInterface *pSupportUDPCommInterface = new UDPCommInterface(pDGSocket, true);
                        DTLSCommInterface *pSupport = (DTLSCommInterface*)_pCommInterface;

                        IHMC_DTLS::DtlsEndpoint *newDtls;
                        pSupport->getDtlsObject(&newDtls);
                        //Dtls will not be able to handle defragmented packets after this point


                        //should set new mocket max mtu here
                        //->setMTU(Mocket::MAXIMUM_MTU);
                        DTLSCommInterface *pNewCommInterface = new DTLSCommInterface(pSupportUDPCommInterface, true, &newDtls);
                        pMocket = new Mocket(cookie, pRemoteAddr, _configFile, pNewCommInterface, true, true);
                        pSupport = nullptr;
                    }
                    else {
                        // We are not using a CommInterface that was passed into ServerMocket, so create a new one for this connection
                        UDPDatagramSocket *pDGSocket = new UDPDatagramSocket();
                        if (_ui32ListenAddr != 0) {
                            // A specific address was passed into the call to listen - bind the socket that is
                            // being created for the new connection to the same port
                            if (pDGSocket->init(ui16PortForNewConnection, _ui32ListenAddr) < 0) {
                                checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "error initializing the socket\n");
                                delete pDGSocket;
                                return nullptr;
                            }
                        }
                        else {
                            if (pDGSocket->init(ui16PortForNewConnection) < 0) {
                                checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "error initializing the socket\n");
                                delete pDGSocket;
                                return nullptr;
                            }
                        }
                        pCookieRec->ui16LocalPort = pDGSocket->getLocalPort();
                        if (pCookieRec->ui16LocalPort == 0) {
                            checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "could not get local port\n");
                            delete pDGSocket;
                            return nullptr;
                        }
                        CommInterface *pNewCommInterface = new UDPCommInterface(pDGSocket, true);
                        pMocket = new Mocket(cookie, pRemoteAddr, _configFile, pNewCommInterface, true);
                    }
                }
                else {
                    if (_bEnableDtls) {
                        // We are using a CommInterface that was passed into ServerMocket
                        CommInterface *pNewCommInterface = _pCommInterface->newInstance();
                        if (_ui32ListenAddr != 0) {
                            // A specific address was passed into the call to listen - use the same with
                            // the CommInterface
                            InetAddr bindAddr(_ui32ListenAddr);
                            pNewCommInterface->bind(&bindAddr);
                        }
                        else {
                            InetAddr bindAddr;
                            pNewCommInterface->bind(&bindAddr);
                        }
                        pCookieRec->ui16LocalPort = pNewCommInterface->getLocalPort();
                        if (pCookieRec->ui16LocalPort == 0) {
                            checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "could not get local port\n");
                            return nullptr;
                        }

                        DTLSCommInterface *pSupport = (DTLSCommInterface*)_pCommInterface;
                        IHMC_DTLS::DtlsEndpoint *newDtls;
                        pSupport->getDtlsObject(&newDtls);

                        //should set new mocket max mtu here
                        //->setMTU(Mocket::MAXIMUM_MTU);
                        DTLSCommInterface *pNewDtlsCommInterface = new DTLSCommInterface(pNewCommInterface, true, &newDtls);
                        pMocket = new Mocket(cookie, pRemoteAddr, _configFile, pNewDtlsCommInterface, false, true);
                        pSupport = nullptr;
                    }
                    else {
                        // We are using a CommInterface that was passed into ServerMocket
                        CommInterface *pNewCommInterface = _pCommInterface->newInstance();
                        if (_ui32ListenAddr != 0) {
                            // A specific address was passed into the call to listen - use the same with
                            // the CommInterface
                            InetAddr bindAddr(_ui32ListenAddr);
                            pNewCommInterface->bind(&bindAddr);
                        }
                        else {
                            InetAddr bindAddr;
                            pNewCommInterface->bind(&bindAddr);
                        }
                        pCookieRec->ui16LocalPort = pNewCommInterface->getLocalPort();
                        if (pCookieRec->ui16LocalPort == 0) {
                            checkAndLogMsg("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "could not get local port\n");
                            return nullptr;
                        }
                        pMocket = new Mocket(cookie, pRemoteAddr, _configFile, pNewCommInterface, false);
                    }
                }
                checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MediumDetailDebug,
                                "allocated port %d for new mocket connection\n", (int) pCookieRec->ui16LocalPort);

                // Check if the remote side sent its public key, which indicates that it wishes to pre-exchange keys
                // to support reconnection after an abrupt disconnection
                if (accessor.getKeyLength() > 0) {
                    #ifdef MOCKETS_NO_CRYPTO
                        checkAndLogMsg ("Mocket::processIncomingPacket", Logger::L_SevereError,
                                        "crypto disabled at build time\n");
                    #else
                        int rc;
                        // Add to the packet Ka[Ks, UUID] if the connection supports abrupt disconnections
                        Key::KeyData *pKeyData = new Key::KeyData (Key::KeyData::X509, accessor.getKeyData(), accessor.getKeyLength());
                        PublicKey *pPubKey = new PublicKey();
                        if (0 != (rc = pPubKey->setKeyFromDEREncodedX509Data (pKeyData))) {
                            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                            "unable to create the public key; rc = %d\n", rc);
                            delete pMocket;
                            return nullptr;
                        }
                        delete pKeyData;
                        pKeyData = nullptr;
                        // Create and save in pMocket connection UUID and Ks
                        pMocket->newMocketUUID();
                        //printf ("UUID %lu\n", pMocket->getMocketUUID());
                        pMocket->newSecretKey();

                        // Set that Ks and the UUID have been set so this side of the mocket supports abrupt disconnections
                        pMocket->setSupportReEstablish (true);

                        // Insert in a buffer the UUID and the password to initialize Ks
                        uint8 aui8Buf[MAX_RESUME_TOKEN_LEN];
                        uint16 ui16BufSize = 0;

                        // Insert password
                        memset (aui8Buf, 0, MAX_RESUME_TOKEN_LEN);
                        strncpy ((char*)aui8Buf, pMocket->getPassword(), pMocket->getPasswordLength());
                        ui16BufSize += pMocket->getPasswordLength()+1;

                        // Insert the UUID
                        *((uint32*)(aui8Buf+ui16BufSize)) = pMocket->getMocketUUID();
                        ui16BufSize += 4;

                        // Encrypt the buffer with the received Ka
                        uint32 ui32EncryptedParamLen = 0;
                        void *pEncryptedParam = CryptoUtils::encryptDataUsingPublicKey (pPubKey , aui8Buf, ui16BufSize, &ui32EncryptedParamLen);
                        delete pPubKey;
                        pPubKey = nullptr;

                        // Save the encrypted UUID and Ks in the CookieRec
                        if (ui32EncryptedParamLen < MAX_RESUME_TOKEN_LEN) {
                            memcpy (pCookieRec->aui8ResumeToken, pEncryptedParam, ui32EncryptedParamLen);
                            pCookieRec->ui16ResumeTokenLen = (uint16) ui32EncryptedParamLen;
                        }
                        else {
                            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                            "encrypted cookie is longer (%lu bytes) than buffer (%d bytes)\n",
                                            ui32EncryptedParamLen, (int) MAX_RESUME_TOKEN_LEN);
                        }
                        free (pEncryptedParam);
                    #endif
                }

                // Start the threads now!
                pMocket->startThreads();
            }

            // Create the COOKIE_ACK packet
            Packet replyPacket (Mocket::MAXIMUM_MTU);
            replyPacket.setValidation (cookie.getValidationZ());
            replyPacket.setWindowSize (Mocket::DEFAULT_MAXIMUM_WINDOW_SIZE);
            replyPacket.setSequenceNum (0);
            replyPacket.addCookieAckChunk (pCookieRec->ui16LocalPort);
            if (pCookieRec->ui16ResumeTokenLen > 0) {
                replyPacket.addConnectionId (pCookieRec->aui8ResumeToken, pCookieRec->ui16ResumeTokenLen);
            }

            if ((rc = _pCommInterface->sendTo (pRemoteAddr, replyPacket.getPacket(), replyPacket.getPacketSize())) < 0) {
                checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "failed to send CookieAck packet to client to %s:%d; rc = %d\n",
                                pRemoteAddr->getIPAsString(), (int) pRemoteAddr->getPort(), rc);
            }
            pCookieRec->ui16Count++;
            return pMocket;
        }

        case Packet::CT_SimpleConnect:
        {
            int rc;
            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MediumDetailDebug,
                            "received a simpleConnect from %s:%d\n",
                            pRemoteAddr->getIPAsString(), pRemoteAddr->getPort());
            uint32 ui32OutgoingValidation = (uint32) rand();
            uint32 ui32InitialControlTSN = 0;
            uint32 ui32InitialReliableSequencedTSN = 0;
            uint32 ui32InitialUnreliableSequencedTSN = 0;
            uint32 ui32InitialReliableUnsequencedId = 0;
            uint32 ui32InitialUnreliableUnsequencedId = 0;
            SimpleConnectChunkAccessor accessor = pPacket->getSimpleConnectChunk();
            StateCookie cookie (getTimeInMilliseconds(), Mocket::DEFAULT_COOKIE_LIFESPAN,
                                accessor.getValidation(), ui32OutgoingValidation,
                                accessor.getControlTSN(), ui32InitialControlTSN,
                                accessor.getReliableSequencedTSN(), ui32InitialReliableSequencedTSN,
                                accessor.getUnreliableSequencedTSN(), ui32InitialUnreliableSequencedTSN,
                                accessor.getReliableUnsequencedId(), ui32InitialReliableUnsequencedId,
                                accessor.getUnreliableUnsequencedId(), ui32InitialUnreliableUnsequencedId,
                                pRemoteAddr->getPort(), _ui16Port);

            Mocket *pMocket = nullptr;
            uint16 ui16LocalPort = 0;
            // Create a local endpoint
            if (_bLocallyCreatedCI) {
                // We are not using a CommInterface that was passed into ServerMocket, so create a new one for this connection
                UDPDatagramSocket *pDGSocket = new UDPDatagramSocket();
                if (_ui32ListenAddr != 0) {
                    // A specific address was passed into the call to listen - bind the socket that is
                    // being created for the new connection to the same port
                    if (pDGSocket->init (ui16PortForNewConnection, _ui32ListenAddr) < 0) {
                        checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                        "error initializing the socket\n");
                        delete pDGSocket;
                        return nullptr;
                    }
                }
                else {
                    if (pDGSocket->init (ui16PortForNewConnection) < 0) {
                        checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                        "error initializing the socket\n");
                        delete pDGSocket;
                        return nullptr;
                    }
                }
                ui16LocalPort = pDGSocket->getLocalPort();
                if (ui16LocalPort == 0) {
                    checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "could not get local port\n");
                    delete pDGSocket;
                    return nullptr;
                }
                CommInterface *pNewCommInterface = new UDPCommInterface (pDGSocket, true);
                pMocket = new Mocket (cookie, pRemoteAddr, _configFile, pNewCommInterface, true);
            }
            else {
                // We are using a CommInterface that was passed into ServerMocket
                CommInterface *pNewCommInterface = _pCommInterface->newInstance();
                if (_ui32ListenAddr != 0) {
                    // A specific address was passed into the call to listen - use the same with
                    // the CommInterface
                    InetAddr bindAddr (_ui32ListenAddr);
                    pNewCommInterface->bind (&bindAddr);
                }
                else {
                    InetAddr bindAddr;
                    pNewCommInterface->bind (&bindAddr);
                }
                ui16LocalPort = pNewCommInterface->getLocalPort();
                if (ui16LocalPort == 0) {
                    checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                    "could not get local port\n");
                    return nullptr;
                }
                pMocket = new Mocket (cookie, pRemoteAddr, _configFile, pNewCommInterface, false);
            }
            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MediumDetailDebug,
                            "allocated port %d for new mocket connection\n", (int) ui16LocalPort);
            // Start the threads now!
            pMocket->startThreads();

            // Create the SIMPLE_CONNECT_ACK packet
            Packet replyPacket (Mocket::MAXIMUM_MTU);
            replyPacket.setValidation (cookie.getValidationZ());
            replyPacket.setWindowSize (Mocket::DEFAULT_MAXIMUM_WINDOW_SIZE);
            replyPacket.setSequenceNum (0);
            replyPacket.addSimpleConnectAckChunk (ui32OutgoingValidation, ui32InitialControlTSN, ui32InitialReliableSequencedTSN, ui32InitialUnreliableSequencedTSN, ui32InitialReliableUnsequencedId, ui32InitialUnreliableUnsequencedId, ui16LocalPort, &cookie);

            if ((rc = _pCommInterface->sendTo (pRemoteAddr, replyPacket.getPacket(), replyPacket.getPacketSize())) < 0) {
                checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_MildError,
                                "failed to send SimpleConnectAck packet to client to %s:%d; rc = %d\n",
                                pRemoteAddr->getIPAsString(), (int) pRemoteAddr->getPort(), rc);
                return nullptr;
            }
            return pMocket;
        }

        default:
            checkAndLogMsg ("ServerMocket::processIncomingPacket", Logger::L_Warning,
                            "received a packet with an unexpected chunk type %d from %s:%d\n",
                            (int) pPacket->getChunkType(), pRemoteAddr->getIPAsString(), (int) pRemoteAddr->getPort());
            return nullptr;
    }
}

int ServerMocket::clearCookieRec (InetAddr remoteAddr)
{
    // See if there is an active record for the specified remote address
    unsigned short us;
    int64 i64CurrTime = getTimeInMilliseconds();
    for (us = 0; us < _cookieHistory.size(); us++) {
        if (_cookieHistory[us].bInUse) {
            if (i64CurrTime > _cookieHistory[us].i64CookieExpirationTime) {
                // This history record is too old - delete
                _cookieHistory[us].bInUse = false;
                _cookieHistory[us].i64CookieExpirationTime = 0;
                _cookieHistory[us].ui16Count = 0;
                _cookieHistory[us].ui16LocalPort = 0;
                _cookieHistory[us].ui16ResumeTokenLen = 0;
            }
            else if (_cookieHistory[us].remoteAddr == remoteAddr) {
                _cookieHistory[us].bInUse = false;
                _cookieHistory[us].i64CookieExpirationTime = 0;
                _cookieHistory[us].ui16Count = 0;
                _cookieHistory[us].ui16LocalPort = 0;
                _cookieHistory[us].ui16ResumeTokenLen = 0;
            }
        }
    }
    return 0;
}

ServerMocket::CookieRec * ServerMocket::getCookieRec (InetAddr remoteAddr)
{
    // See if there is an active record for the specified remote address
    unsigned short us;
    int64 i64CurrTime = getTimeInMilliseconds();
    for (us = 0; us < _cookieHistory.size(); us++) {
        if (_cookieHistory[us].bInUse) {
            if (i64CurrTime > _cookieHistory[us].i64CookieExpirationTime) {
                // This history record is too old - delete
                _cookieHistory[us].bInUse = false;
                _cookieHistory[us].i64CookieExpirationTime = 0;
                _cookieHistory[us].ui16Count = 0;
                _cookieHistory[us].ui16LocalPort = 0;
                _cookieHistory[us].ui16ResumeTokenLen = 0;
            }
            else if (_cookieHistory[us].remoteAddr == remoteAddr) {
                return &_cookieHistory[us];
            }
        }
    }

    // There is no active record - return a new blank one
    for (us = 0; us < _cookieHistory.size(); us++) {
        if (!_cookieHistory[us].bInUse) {
            // Found an empty record
            _cookieHistory[us].bInUse = true;
            _cookieHistory[us].i64CookieExpirationTime = i64CurrTime + Mocket::DEFAULT_COOKIE_LIFESPAN;
            _cookieHistory[us].remoteAddr = remoteAddr;
            _cookieHistory[us].ui16Count = 0;
            _cookieHistory[us].ui16LocalPort = 0;
            _cookieHistory[us].ui16ResumeTokenLen = 0;
            return &_cookieHistory[us];
        }
    }

    // Allocate a new entry
    CookieRec *pRec = &_cookieHistory[_cookieHistory.size()];
    pRec->bInUse = true;
    pRec->i64CookieExpirationTime = i64CurrTime + Mocket::DEFAULT_COOKIE_LIFESPAN;
    pRec->remoteAddr = remoteAddr;
    pRec->ui16Count = 0;
    pRec->ui16LocalPort = 0;
    pRec->ui16ResumeTokenLen = 0;
    return pRec;
}
