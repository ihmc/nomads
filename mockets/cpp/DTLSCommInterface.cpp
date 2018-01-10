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
*/

#include <string.h>
#include <stdlib.h>

#include "DTLSCommInterface.h"
#include "DTLSConstants.h"
#include "Dtls.h"

using namespace NOMADSUtil;
using namespace IHMC_DTLS;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

DTLSCommInterface::DTLSCommInterface(CommInterface *pCI, bool isServer, DtlsEndpoint** dtls)
{
	_pDTLS                          = *dtls;
	_bIsServer                      = isServer;
	_mtu                            = DEFAULT_MTU;
    _pCI                            = pCI;
	_pcLastHanshakeMessage          = NULL;
	_pLastRemoteAddr                = NULL;
	_lastMessageSize                = 0;
	_bLastHanshakeMessageHasBeenSet = false;
    _ui32RcvTimeout                 = 240;
    _int64Timeout                   = getTimeInMilliseconds() + 30000;
}

DTLSCommInterface::DTLSCommInterface(
    CommInterface *pCI, 
    bool bIsServer, 
    uint32 ui32RcvTimeout,
    const char *pszPathToCertificate, 
    const char *pszPathToPrivateKey)
{
    const char *pszMethodName = "DTLSCommInterface::DTLSCommInterface";
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Recreating the DTLS object\n");
    _bIsServer = bIsServer;
    setCertificatePaths(pszPathToCertificate, pszPathToPrivateKey);
    if ((_pDTLS = new DtlsEndpoint(_sPathToCertificate.c_str(), _sPathToPrivateKey.c_str())) == NULL) {
        checkAndLogMsg(pszMethodName, Logger::L_MildError, "End point creation failed\n");
    }
    _mtu                            = DEFAULT_MTU;
    _pCI                            = pCI;
    _pDTLS->init(bIsServer);
    _pcLastHanshakeMessage          = NULL;
    _pLastRemoteAddr                = NULL;
    _lastMessageSize                = 0;
    _bLastHanshakeMessageHasBeenSet = false;
    _numberOfAttempts               = HANDSHAKE_ATTEMPTS;
    _intervalBetweenAttempts        = HANDSHAKE_INTERVAL_BEFORE_RESEND;
    _ui32RcvTimeout                 = ui32RcvTimeout;
    _int64Timeout                   = getTimeInMilliseconds() + 30000;
}

DTLSCommInterface::~DTLSCommInterface(void)
{
    //need to handle destruction of embedded pCI only when requested!! should add a flag or something
    if (_pDTLS != NULL) {
        delete _pDTLS;
        _pDTLS = NULL;
    }

    if (_pcLastHanshakeMessage != NULL) {
        delete _pcLastHanshakeMessage;
        _pcLastHanshakeMessage = NULL;
    }

    if (_pLastRemoteAddr != NULL) {
        delete _pLastRemoteAddr;
        _pLastRemoteAddr = NULL;
    }
}

bool DTLSCommInterface::freeLastHanshakeMessage()
{
    const char *pszMethodName = "DTLSCommInterface::freeLastHanshakeMessage";
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Freeing the last handshake message\n");

    _lastMessageSize = 0;
    _bLastHanshakeMessageHasBeenSet = false;

    if (_pLastRemoteAddr != NULL) {
        delete _pLastRemoteAddr;
        _pLastRemoteAddr = NULL;
    }

    if (_pcLastHanshakeMessage != NULL) {
        delete[] _pcLastHanshakeMessage;
        _pcLastHanshakeMessage = NULL;
    }
    return true;
}

int DTLSCommInterface::getDtlsObject(IHMC_DTLS::DtlsEndpoint ** dtls)
{
    *dtls = _pDTLS;
    return 0;
}

void DTLSCommInterface::handleInitializationError(const int errorCode, const bool isServer)
{
    const char *pszMethodName = "DTLSCommInterface::handleInitializationError";
	if (errorCode != 0) {
        if (isServer) {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Error during Server initialization: \n");
        }
        else {
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Error during Client initialization: \n");
        }
    
        String sMessage;
        if (errorCode > -8) {
            sMessage = "Error in context object initialization: \n";
        }
        else {
            sMessage = "Error in SSL object initialization: \n";
        }
        checkAndLogMsg(pszMethodName, Logger::L_SevereError, sMessage.c_str());

        switch (errorCode){
        case -1:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Mode not correctly specified?\n");
            break;
        case -2:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Impossible to find certificate file in executable directory\n");
            break;
        case -3:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Impossible to find key file in executable directory\n");
            break;
        case -4:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Context object not initialized correctly... not enough memory?\n");
            break;
        case -5:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Impossible to set cypher list\n");
            break;
        case -6:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Impossible to load the selected certificate please check its integrity\n");
            break;
        case -7:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Impossible to load the selected key certificate please check its integrity\n");
            break;
        case -8:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, 
                "Key certificate loaded correctly but non deemed valid, please check that it was created correctly\n");   
            break;
        case -9:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Mode not correctly selected?\n");
            break;
        case -10:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "CTX object is null?\n");
            break;
        case -11:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "SSL object creation failed, not enough memory?\n");           
            break;
        case -12:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Creation of read memory BIO failed, not enough memory?\n");
            break;
        case -13:
            checkAndLogMsg(pszMethodName, Logger::L_SevereError, "Creation of write memory BIO failed, not enough memory?\n");
            break;
        }
	}
}

void DTLSCommInterface::handleNoRcv(void)
{
    const char *pszMethodName = "DTLSCommInterface::handleNoRcv";
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Rcv timeout (%d), trying again...\n", _ui32RcvTimeout); 
    if (lastHandshakeMessageHasBeenSet()) {
        if (int rc = sendHSAgain() <= 0) {
            checkAndLogMsg(pszMethodName, Logger::L_Warning, 
                "sendHSAgain() send error (rc = %d), trying again...\n", rc);
        }
    }
}

void DTLSCommInterface::handleRcvError(const int rc)
{
    const char *pszMethodName = "DTLSCommInterface::handleRcvError";
    checkAndLogMsg(pszMethodName, Logger::L_Warning, "Rcv failed (rc = %d), trying again...\n", rc);
}

int DTLSCommInterface::handleRcv(const int bufSize, char *pBuf, const int handshakeStep, InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "DTLSCommInterface::handleRcv";
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Received <%dB>\n", bufSize);

    int recoverDataRc  = _pDTLS->recoverData(pBuf, bufSize, &pBuf);
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Recover data rc is %d\n", recoverDataRc);
    int handshakeRc = _pDTLS->handShake(&pBuf);
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "handshake rc is %d\n", handshakeRc);  
    if (handshakeRc == 0) {
        return 0;
    }
    if (handshakeRc < 0) {
        checkAndLogMsg(pszMethodName, Logger::L_Warning, "HandShake failed with rc: %d\n", handshakeRc);
        if (pBuf != NULL) {
            delete(pBuf);
            pBuf = NULL;
        }
        return -1;
    }
    if (_bIsServer || !(handshakeStep == 1)) {
        for (bool bSend = true; bSend; ) {
            int sendToRc;
            if ((sendToRc = _pCI->sendTo(pRemoteAddr, pBuf, handshakeRc, 0)) < 0) {
                checkAndLogMsg(pszMethodName, Logger::L_MildError, 
                    "send to <%s>:<%d> failed; rc = %d\n", pRemoteAddr->getIPAsString(), (int)pRemoteAddr->getPort(), sendToRc);
                checkAndLogMsg(pszMethodName, Logger::L_MildError, "Trying again...\n");
                continue;
            }
            checkAndLogMsg(pszMethodName, Logger::L_MediumDetailDebug, "Sent: %d bytes to %s\n", 
                sendToRc, pRemoteAddr->getIPAsString());
            bSend = false;
        }

        freeLastHanshakeMessage();
        setLastHanshakeMessage(pBuf, handshakeRc, pRemoteAddr);

    }
    return 1;
}

bool DTLSCommInterface::doHandshake(InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "DTLSCommInterface::doHandshake";
    int rc;
    int read = 0;
    int counter = 0;
    _bLastHanshakeMessageHasBeenSet = !_bIsServer;
    //should be constants!
    //Dtls is able to take care of this defragmentation
    char *pBuf = new char[3000]; 
    int handshakeStep = 0;

    //for (bool bKeepTrying = true; bKeepTrying; ) {
    while(!_pDTLS->isHandshakeOver()) {     
        if (!_bIsServer && (getTimeInMilliseconds() > _int64Timeout)) {
            return false;
        }

        read = _pCI->receive(pBuf, 3000, pRemoteAddr);    
        if (read == NO_RCV) {
            handleNoRcv();
            continue;
        }

        if (read < 0) {
            handleRcvError(read);
            continue;
        }

        if (read > 0) {
            rc = handleRcv(read, pBuf, handshakeStep, pRemoteAddr);
            
            if (rc < 0) {
                checkAndLogMsg(pszMethodName, Logger::L_Warning, "handleRcv failed, rc: %d\n", rc);
                return false;
            }

            if (rc == 1) {
                handshakeStep++;
                checkAndLogMsg(pszMethodName, Logger::L_MediumDetailDebug, "Hand shake step: %d\n", handshakeStep);
            }

        }
    }

    if (pBuf != NULL) {
        delete(pBuf);
        pBuf = NULL;
    }

    checkAndLogMsg(pszMethodName, Logger::L_Info, "DTLS Handshake successful\n", read);
    return true;
    /*
    for (int handshakeStep = 0; handshakeStep < 2; handshakeStep++) {   
        if (_pCI->setReceiveTimeout(_ui32RcvTimeout) != 0) {
            checkAndLogMsg(pszMethodName, Logger::L_MildError, "could not set UDP timeout\n"); 
        }

        while (read == 0) {
            read = _pCI->receive(pBuf, _ui32RcvTimeout, pRemoteAddr);
            if (read == 0) {
                checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug,  "Timeout for handshake message expired, attempt: %d out of %d, trying again\n",  counter + 1, numberOfAttempts);

                if (lastHandshakeMessageHasBeenSet()) {
                    if (sendHSAgain() <= 0) {
                        freeLastHanshakeMessage();
                        return false;
                    }
                }

                counter++;

                if (counter == numberOfAttempts) {
                    freeLastHanshakeMessage();
                    return false;
                }
            }
        }

        if (lastHandshakeMessageHasBeenSet()) {
            freeLastHanshakeMessage();
        }

        checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Received data: %d\n", read);
        read = _pDTLS->recoverData(pBuf, read, &pBuf);
        read = _pDTLS->handShake(&pBuf);
        if (read < 0) {
            checkAndLogMsg(pszMethodName, Logger::L_Warning, "HandShake failed with rc: %d\n", read);
            if (pBuf != NULL) {
                delete(pBuf);
                pBuf = NULL;
            }           
            return false;
        }

        //Client will not have anything to send
        if (_bIsServer || !(handshakeStep == 1)) {
            checkAndLogMsg(pszMethodName, Logger::L_MediumDetailDebug, "About to send: %d bytes to %s\n", read, pRemoteAddr->getIPAsString());
            if ((rc = _pCI->sendTo(pRemoteAddr, pBuf, read, 0) < 0)) {
                checkAndLogMsg(pszMethodName, Logger::L_MildError, "failed to send HS packet to %s:%d; rc = %d\n",
                    pRemoteAddr->getIPAsString(), (int)pRemoteAddr->getPort(), rc);
                if (pBuf != NULL) {
                    delete(pBuf);
                }
                pBuf = NULL;
                return false;
            }
            freeLastHanshakeMessage();
            setLastHanshakeMessage(pBuf, read, pRemoteAddr);
        }
        read = 0;
    }
    if (pBuf != NULL) {
        delete(pBuf);
        pBuf = NULL;
    }
    checkAndLogMsg(pszMethodName, Logger::L_NetDetailDebug, "DTLS Handshake successful\n", read);
    return true;
    */
}

void DTLSCommInterface::deleteDtlsObject(void)
{
    delete _pDTLS;
    _pDTLS = NULL;
}

int DTLSCommInterface::receive(void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "DTLSCommInterface::receive";
    if (!_pDTLS->isHandshakeOver()) {
        if (!doHandshake(pRemoteAddr)) {
            checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "HandShake failed\n");
            return -2;
        }
        else {
            if (!_pDTLS->isHandshakeOver()) {
                checkAndLogMsg(pszMethodName, Logger::L_Warning, "Something is wrong with the internal state of the DTLS object\n");
                return -3;
            }
            return 0;
        }
    }

    int size;
    char encryptedMessage[3000];
    size = _pCI->receive(encryptedMessage, iBufSize, pRemoteAddr);
    char *pDMessage = NULL;
    // decrypt the message
    size = _pDTLS->recoverData(encryptedMessage, size, &pDMessage);
    if ((pBuf == NULL) || (pDMessage == NULL)) {
        return 0;
    }
    //Copy the message in the original buffer and free the support buffer
    if (size > 0) {
        memcpy(pBuf, pDMessage, size);
        if (pDMessage != NULL) {
            delete(pDMessage);
        }
        return size;
    }
    else {
        //something went wrong!
        return -1;
    }
}

int DTLSCommInterface::resetDtlsObject(void)
{
    const char *pszMethodName = "DTLSCommInterface::resetDtlsObject";
	checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "Resetting the dtls object of the server mocket\n");
	_pDTLS = new DtlsEndpoint(_sPathToCertificate.c_str(), _sPathToPrivateKey.c_str());
	_mtu = DEFAULT_MTU;
	_pDTLS->init(_bIsServer);
    return 0;
}

bool DTLSCommInterface::sendHSAgain(void)
{
    const char *pszMethodName = "DTLSCommInterface::sendHSAgain";
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, 
        "Sending again the last handshake message to %s\n", _pLastRemoteAddr->getIPAsString());
    return _pCI->sendTo(_pLastRemoteAddr, _pcLastHanshakeMessage, _lastMessageSize, NULL);
}

int DTLSCommInterface::sendTo(InetAddr *pRemoteAddr, const void *pBuf, int iBufSize, const char *pszHints)
{
    static const char *pszMethodName = "DTLSCommInterface::sendTo";
	int rc = 0;

	if (!(rc = _pDTLS->isHandshakeOver())) {
        if (startHandshake(pRemoteAddr)) {
			if (!doHandshake(pRemoteAddr)) {
                static const char *pszMessage = "doHandshake failed, resetting dtls object\n";
                checkAndLogMsg(pszMethodName, Logger::L_Warning, pszMessage);
				delete(_pDTLS);
				resetDtlsObject();
                return -1;
            }
            else {
                static const char *pszMessage = "Dtls HandShake completed\n";
                checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, pszMessage);
                return 0;
            }
        }
        else {
            static const char *pszMessage = "Client start handshake failed\n";
            checkAndLogMsg(pszMethodName, Logger::L_MildError, pszMessage);
            return -2;
        }
    }
    else {
        static const char *pszMessage = "About to encrypt and send a message\n";
		checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, pszMessage);
        int ui32Size;
        char *pEMsg = NULL;
        //if iBufSize is > MTU - 38 DTLS will not be able to handle the message unless it's the handshake
        ui32Size = _pDTLS->prepareDataForSending((char *)pBuf, iBufSize, &pEMsg);
        static const char *pszMessage2 = "Original size: %d; Send message size: %d\n";
		checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, pszMessage2, iBufSize, ui32Size);
        if (pEMsg == NULL) {
            static const char *pszMessage3 = "Message enctyption failed\n";
            checkAndLogMsg(pszMethodName, Logger::L_MildError, pszMessage3);
        }
        ui32Size = _pCI->sendTo(pRemoteAddr, pEMsg, ui32Size, pszHints);
        delete[] pEMsg;
        return ui32Size;
    }
}

void DTLSCommInterface::setCertificatePaths(const char *pszPathToCertificate, const char *pszPathToPrivateKey)
{
    if (pszPathToCertificate != NULL) {
        _sPathToCertificate = strdup(pszPathToCertificate);
    }
    else {
        _bIsServer ? (_sPathToCertificate = pszServerDefaultCertPath) : _sPathToCertificate = pszClientDefaultCertPath;
    }

    if (pszPathToPrivateKey != NULL) {
        _sPathToPrivateKey = pszPathToPrivateKey;
    }
    else {
        _bIsServer ? (_sPathToPrivateKey = pszServerDefaultKeyPath) : _sPathToPrivateKey = pszClientDefaultKeytPath;
    }
}

bool DTLSCommInterface::setLastHanshakeMessage(
    const char* buffer, 
    const int size, 
    const InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "DTLSCommInterface::setLastHanshakeMessage";
	checkAndLogMsg(pszMethodName, Logger::L_LowDetailDebug, "Storing current handshake message\n");
	_bLastHanshakeMessageHasBeenSet = true;
	_lastMessageSize = size;
	_pLastRemoteAddr = new InetAddr(pRemoteAddr->getIPAddress(), pRemoteAddr->getPort());
	_pcLastHanshakeMessage = new char[size];

	if ((_pcLastHanshakeMessage != NULL) && (_pLastRemoteAddr != NULL)) {
		memcpy(_pcLastHanshakeMessage, buffer, size);
		return true;
	}
	else {
		return false;
	}
}

int DTLSCommInterface::setMTU(const int newMTU)
{
    _mtu = newMTU;
    return 0;
}

bool DTLSCommInterface::startHandshake(InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "DTLSCommInterface::startHandshake";
    //Dtls is able to take care of this defragmentation, the buf has to be at least as big as the certificate
    char *pBuf = new char[2000];
    int read;
    int rc = 0;
    //Start Dtls Handshake process
    if (((read = _pDTLS->handShake(&pBuf)) < 0)) {
        checkAndLogMsg(pszMethodName, Logger::L_MediumDetailDebug, "Client HS failed with rc: %d\n", rc);
        if (pBuf != NULL) {
            delete(pBuf);
            pBuf = NULL;
        }
        return false;
    }

    //save the message in case we need to resend it
    setLastHanshakeMessage(pBuf, read, pRemoteAddr);
    if ((rc = _pCI->sendTo(pRemoteAddr, pBuf, read, 0) < 0)) {
        checkAndLogMsg(pszMethodName, Logger::L_MildError, "Client failed to send HS packet to %s:%d; rc = %d\n", 
            pRemoteAddr->getIPAsString(), (int)pRemoteAddr->getPort(), rc);

        if (pBuf != NULL) {
            delete(pBuf);
        }
        pBuf = NULL;
        return false;
    }
    if (pBuf != NULL) {
        delete(pBuf);
    }
    pBuf = NULL;
    checkAndLogMsg(pszMethodName, Logger::L_HighDetailDebug, "First message sent <%B>\n", read);
    return true;
}
