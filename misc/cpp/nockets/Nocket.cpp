/*
 * Nocket.cpp
 *
 * This file is part of the IHMC NORM Socket Library.
 * Copyright (c) 2016 IHMC.
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
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 */

#include "Nocket.h"

#include "NormUtil.h"

#include "InetAddr.h"
#include "Logger.h"
#include "NetUtils.h"
#include "NLFLib.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_MISC;
using namespace NOMADSUtil;
#ifdef USE_NORM
namespace IHMC_MISC
{
    void checkAndLogNormEventType (const char *pszMethodName, NormEventType evtType)
    {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,  "%s\n", getNormEventAsString (evtType));
    }

    void checkAndLogNormEvent (const char *pszMethodName, NormEvent evt)
    {
        checkAndLogNormEventType (pszMethodName, evt.type);
    }

    bool validateRecvdData (const char *pchData, unsigned int uiDataLen)
    {
        return true;
    }

    void initSession (NormSessionHandle pSessionHandle, bool bSend, bool bReceive)
    {
        const char *pszMethodName = "Nocket::initSession";
        if (bSend) {
            NormSetTxRate (pSessionHandle, 2.0e+06);  // in bits/secon
        }
        if (bReceive) {
            NormSetRxCacheLimit (pSessionHandle, 4096);
        }
        NormSessionId sessionId = static_cast<NormSessionId>(rand ());
        checkAndLogMsg (pszMethodName, Logger::L_Info, "starting NORM sender ...\n");
        if (bSend) {
            NormStartSender (pSessionHandle, sessionId, 1024 * 1024, 1400, 64, 16);
        }
        if (bReceive) {
            NormStartReceiver (pSessionHandle, 1024 * 1024);
        }
    }
}
#endif

Nocket::Nocket (bool bReceive, bool bSend)
    : _bReceive (bReceive),
      _bSend (bSend),
      _bSent (true),
      _pInstanceHandle (
#ifdef USE_NORM
        NormCreateInstance()
#else
        NULL
#endif
      ),
      _pSessionHandle (NULL),
      _cvRx (&_mRx),
      _cvTx (&_mTx)
{
}

Nocket::~Nocket (void)
{
    close();
}

void Nocket::run (void)
{
    #ifdef USE_NORM
    const char *pszMethodName = "Nocket::run";
    started();
    for (NormEvent evt; !terminationRequested();) {
        _m.lock();
        MessageWrapper *pWr = _outgoingMsgs.dequeue();
        _m.unlock();
        if (pWr != NULL) {
            char dataInfo[256];
            NormObjectHandle dataObj = NormDataEnqueue (pWr->pSessionHandle, static_cast<const char *>(pWr->bw.getBuffer()),
                                                        pWr->bw.getBufferLength(), dataInfo, strlen(dataInfo));
            //delete pWr;
            if (NORM_OBJECT_INVALID == dataObj) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "enqueuing of outgoig message failed");
            }
        }

        if (!NormGetNextEvent(_pInstanceHandle, &evt)) {
            continue;
        }
        switch (evt.type) {
            case NORM_RX_OBJECT_COMPLETED: {
                checkAndLogNormEvent (pszMethodName, evt);
                int64 objSize = NormObjectGetSize (evt.object);
                const char *pchDataPtr = NormDataAccessData (evt.object);
                // Check data info
                char dataInfo[8192];
                unsigned int dataInfoLen = NormObjectGetInfo (evt.object, dataInfo, 8191);
                dataInfo[dataInfoLen] = '\0';
                uint32 dataLen = 32U;
                if (validateRecvdData (pchDataPtr, dataLen)) {
                    BufferWriter *pWr = new BufferWriter (objSize, 1024);
                    if ((pWr != NULL) && (pWr->writeBytes (pchDataPtr, objSize) >= 0)) {
                        _m.lock();
                        _incomingMsgs.enqueue (pWr);
                        _m.unlock();
                    }
                }
                break;
            }

            case NORM_TX_FLUSH_COMPLETED: {
                _bSent = true;
                _cvRx.notify();
                break;
            }

            default:
                checkAndLogNormEvent(pszMethodName, evt);
                break;
        }
    }
    terminating();
    #endif
}

int Nocket::close (void)
{
    #ifdef USE_NORM
    if (_bReceive) {
        NormStopReceiver (_pSessionHandle);
    }
    if (_bSend) {
        NormStopSender (_pSessionHandle);
    }
    NormDestroySession (_pSessionHandle);
    NormDestroyInstance (_pInstanceHandle);
    #endif
    return 0;
}

uint16 Nocket::getLocalPort (void)
{
    return _addr.getPort();
}

InetAddr Nocket::getLocalAddr (void)
{
    return _addr;
}

int Nocket::init (uint16 ui16Port, uint32 ui32IPAddr)
{
    InetAddr addr (ui32IPAddr);
    return init (ui16Port, addr.getIPAsString());
}

int Nocket::getLocalSocket (void)
{
    assert (false);
    return 0;
}

int Nocket::getMTU (void)
{
    assert (false);
    return 1200;
}

int Nocket::getReceiveBufferSize (void)
{
    unsigned int uiBufSize = 2048;
    // NormGetRxSocketBuffer (_pInstanceHandle, uiBufSize);
    if (uiBufSize > INT_MAX) {
        uiBufSize = INT_MAX;
    }
    return uiBufSize;
}

int Nocket::setReceiveBufferSize (int iSize)
{
    if (_bReceive) {
        if (iSize > USHRT_MAX) {
            iSize = USHRT_MAX;
        }
#ifdef USE_NORM
        NormSetRxCacheLimit(_pSessionHandle, static_cast<unsigned short>(iSize));
#endif
        return 0;
    }
    return -1;
}

bool Nocket::pktInfoEnabled (void)
{
    assert (false);
    return false;
}

int Nocket::setSendBufferSize (int iSize)
{
    
    assert (false);
    return -1;
}

int Nocket::setTimeout (uint32 ui32TimeoutInMS)
{
    assert (false);
    return -1;
}

int Nocket::setTTL(uint8 ui8TTL)
{
#ifdef USE_NORM
    return (NormSetTTL(_pSessionHandle, ui8TTL) ? 0 : -1);
#else
    return 0;
#endif
}

int Nocket::init (uint16 ui16Port, const char *pszAddr)
{
#ifdef USE_NORM
    const char *pszMethodName = "Nocket::init";
    if (pszAddr == NULL) {
        return -1;
    }
    srand (getTimeInMilliseconds());
    char sessionAddr[256];
    strcpy (sessionAddr, pszAddr);
    _pSessionHandle = NormCreateSession (_pInstanceHandle, sessionAddr, ui16Port, NORM_NODE_ANY);
    NormSetDebugLevel(3);
    initSession (_pSessionHandle, _bSend, _bReceive);
    _addr.setIPAddress (pszAddr);
    _addr.setPort (ui16Port);
    if (!isRunning()) {
        start();
    }
#endif
    return 0;
}

int Nocket::receive (void *pBuf, int iBufSize)
{
    InetAddr addr;
    return receive (pBuf, iBufSize, &addr);
}

int Nocket::receive (void *pBuf, int iBufSize, InetAddr *pRemoteAddr)
{
    const char *pszMethodName = "Nocket::receive";
    if (!_bReceive) {
        return -1;
    }
    if ((pBuf == NULL) || (iBufSize <= 0)) {
        return -2;
    }

    _m.lock();
    BufferWriter *pWr = _incomingMsgs.dequeue();
    _m.unlock();
    if (pWr != NULL) {
        int iRcvd = pWr->getBufferLength();
        if (iRcvd <= iBufSize) {
            memcpy (pBuf, pWr->getBuffer(), iRcvd);
        }
        else {
            iRcvd = -3;
        }
        delete  pWr;
        return iRcvd;
    }
    sleepForMilliseconds (10);
    return 0;
}

int Nocket::sendTo (uint32 ui32IPv4Addr, uint16 ui16Port, const void *pBuf, int iBufSize, const char *pszHints)
{
#ifdef USE_NORM
    const char *pszMethodName = "Nocket::sendTo";
    if ((pBuf == NULL) || (iBufSize == 0) || (_pSessionHandle == NULL)) {
        return -1;
    }
    SessionWrapper *pSessionWr = NULL;
    if (NetUtils::isMulticastAddress (ui32IPv4Addr)) {
        pSessionWr = new SessionWrapper;
        pSessionWr->pHandle = _pSessionHandle;
    }
    else {
        // assume unicast
        pSessionWr = _unicastSessionsByDst.get (ui32IPv4Addr);
        if (pSessionWr == NULL) {
            pSessionWr = new SessionWrapper;
            InetAddr addr (ui32IPv4Addr, ui16Port);
            pSessionWr->pHandle = NormCreateSession (_pInstanceHandle, addr.getIPAsString(), ui16Port, NORM_NODE_ANY);
            initSession (pSessionWr->pHandle, _bSend, _bReceive);
            _unicastSessionsByDst.put (ui32IPv4Addr, pSessionWr);
        }
    }

    MessageWrapper *pmsgWr = new MessageWrapper (iBufSize, pSessionWr);
    if ((pmsgWr == NULL) || (pmsgWr->bw.writeBytes (pBuf, iBufSize) < 0)) {
        return -2;
    }
    _m.lock();
    _outgoingMsgs.enqueue (pmsgWr);
    _m.unlock();

    _mRx.lock();
    while (!_bSent) {
        _cvRx.wait (10);
    }
    _mRx.unlock();
    return iBufSize;
#else
    return 0;
#endif
    
}

int Nocket::setTransmissionBufferSize (unsigned int uiBufSize)
{
#ifdef USE_NORM
    NormSetTxSocketBuffer(_pSessionHandle, uiBufSize);
#endif
    return 0;
}

