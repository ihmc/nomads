/*
 * StreamMocket.cpp
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

#include "StreamMocket.h"

#include "Mocket.h"
#include "MessageSender.h"

#include "Logger.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

StreamMocket::StreamMocket (void)
    : _cv (&_m)
{
    _bTerminateThread = false;
    _bThreadTerminated = true;
    _ui32DataBufferingTime = DEFAULT_DATA_BUFFERING_TIME;
    _i64LastTransmitTime = 0;
    _pMMocket = new Mocket();
    _pTransmitBuf = (char*) malloc (_pMMocket->getMaximumMTU());
    _ui16TransmitBufCount = 0;
    _pReceiveBuf = (char*) malloc (_pMMocket->getMaximumMTU());
    _ui16ReceiveBufCount = 0;
    _ui16ReceiveBufOffset = 0;
}

StreamMocket::StreamMocket (Mocket *pMMocket)
    : _cv (&_m)
{
    _bTerminateThread = false;
    _bThreadTerminated = true;
    _ui32DataBufferingTime = DEFAULT_DATA_BUFFERING_TIME;
    _i64LastTransmitTime = 0;
    _pMMocket = pMMocket;
    _pTransmitBuf = (char*) malloc (_pMMocket->getMaximumMTU());
    _ui16TransmitBufCount = 0;
    _pReceiveBuf = (char*) malloc (_pMMocket->getMaximumMTU());
    _ui16ReceiveBufCount = 0;
    _ui16ReceiveBufOffset = 0;
    start();
}

StreamMocket::~StreamMocket (void)
{
    close();
    delete _pMMocket;
    _pMMocket = nullptr;
    while (!_bThreadTerminated) {
        sleepForMilliseconds (50);
    }
    free (_pTransmitBuf);
    _pTransmitBuf = nullptr;
    free (_pReceiveBuf);
    _pReceiveBuf = nullptr;
}

void StreamMocket::setIdentifier (const char *pszIdentifier)
{
    _pMMocket->setIdentifier (pszIdentifier);
}

const char * StreamMocket::getIdentifier (void)
{
    return _pMMocket->getIdentifier();
}

int StreamMocket::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
{
    return _pMMocket->registerPeerUnreachableWarningCallback (pCallbackFn, pCallbackArg);
}

MocketStats * StreamMocket::getStatistics (void)
{
    return _pMMocket->getStatistics();
}

int StreamMocket::bind (const char* pszBindAddr, uint16 ui16BindPort)
{
    return _pMMocket->bind (pszBindAddr, ui16BindPort);
}

int StreamMocket::connect (const char *pszRemoteHost, uint16 ui16RemotePort)
{
    int rc;
    if (0 == (rc = _pMMocket->connect (pszRemoteHost, ui16RemotePort))) {
        start();
    }
    return rc;
}

int StreamMocket::connect (const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout)
{
    int rc;
    if (0 == (rc = _pMMocket->connect (pszRemoteHost, ui16RemotePort, i64Timeout))) {
        start();
    }
    return rc;
}

uint32 StreamMocket::getRemoteAddress (void)
{
    return _pMMocket->getRemoteAddress();
}

uint16 StreamMocket::getRemotePort (void)
{
    return _pMMocket->getRemotePort();
}

uint32 StreamMocket::getLocalAddress (void)
{
    return _pMMocket->getLocalAddress();
}

uint16 StreamMocket::getLocalPort (void)
{
    return _pMMocket->getLocalPort();
}

int StreamMocket::close (void)
{
    _m.lock();
    flush();
    int rc = _pMMocket->close();
    _bTerminateThread = true;
    _cv.notifyAll();
    _m.unlock();
    return rc;
}

int StreamMocket::setConnectionLingerTime (uint32 ui32LingerTime)
{
    return _pMMocket->setConnectionLingerTime (ui32LingerTime);
}

uint32 StreamMocket::getConnectionLingerTime (void)
{
    return _pMMocket->getConnectionLingerTime();
}

int StreamMocket::send (const void *pBuf, unsigned long ulBufSize)
{
    _m.lock();

    if ((_ui32DataBufferingTime > 0) && (ulBufSize < (unsigned long) (_pMMocket->getMTU() - _ui16TransmitBufCount))) {
        // We are buffering data and there is room in the outgoing buffer - just append
        if (_ui16TransmitBufCount == 0) {
            _i64LastTransmitTime = getTimeInMilliseconds();
        }
        memcpy (_pTransmitBuf+_ui16TransmitBufCount, pBuf, ulBufSize);
        _ui16TransmitBufCount += (unsigned short) ulBufSize;      // Value will be small enough for an unsigned short
        checkAndLogMsg ("StreamMocket::send", Logger::L_MediumDetailDebug,
                        "%d total bytes in buffer\n", (int) _ui16TransmitBufCount);
    }
    else {
        // Not enough room or not buffering - copy as much as will fit, send the packet, and handle the rest
        unsigned long ulBytesLeft = ulBufSize;
        while (true) {
            unsigned short usSpaceAvail = _pMMocket->getMTU() - _ui16TransmitBufCount;
            unsigned long ulBytesToSend = ulBytesLeft;
            if (ulBytesToSend > usSpaceAvail) {
                ulBytesToSend = usSpaceAvail;
            }
            memcpy (_pTransmitBuf+_ui16TransmitBufCount, ((char*)pBuf)+(ulBufSize-ulBytesLeft), ulBytesToSend);
            _ui16TransmitBufCount += (unsigned short) ulBytesToSend;   // Value will be small enough for an unsigned short
            ulBytesLeft -= ulBytesToSend;
            if (_ui16TransmitBufCount == _pMMocket->getMTU()) {
                int rc;
                if (0 != (rc = _pMMocket->getSender (true, true).send (_pTransmitBuf, _ui16TransmitBufCount))) {
                    checkAndLogMsg ("StreamMocket::send", Logger::L_MildError,
                                    "send on Mocket failed with rc = %d\n", rc);
                    _m.unlock();
                    return -1;
                }
                _ui16TransmitBufCount = 0;
                _i64LastTransmitTime = getTimeInMilliseconds();
            }
            if (ulBytesLeft == 0) {
                break;
            }
        }
    }
    _m.unlock();
    return 0;
}

int StreamMocket::flush (void)
{
    _m.lock();
    if (_ui16TransmitBufCount > 0) {
        int rc;
        if (0 != (rc = _pMMocket->getSender (true, true).send (_pTransmitBuf, _ui16TransmitBufCount))) {
            checkAndLogMsg ("StreamMocket::flush", Logger::L_MildError,
                            "send on Mocket failed with rc = %d\n", rc);
            _m.unlock();
            return -1;
        }
        _ui16TransmitBufCount = 0;
        _i64LastTransmitTime = getTimeInMilliseconds();
    }
    _m.unlock();
    return 0;
}

int StreamMocket::receive (void *pBuf, unsigned long ulBufSize, unsigned long ulTimeOut)
{
    if (_ui16ReceiveBufCount == 0) {
        int rc = _pMMocket->receive (_pReceiveBuf, _pMMocket->getMaximumMTU(), ulTimeOut);
        if (rc <= 0) {
            return rc;
        }
        else {
            _ui16ReceiveBufCount = (uint16) rc;
            _ui16ReceiveBufOffset = 0;
        }
    }
    uint16 ui16BytesToCopy = _ui16ReceiveBufCount;
    if (ui16BytesToCopy > ulBufSize) {
        ui16BytesToCopy = (uint16) ulBufSize;
    }
    memcpy (pBuf, _pReceiveBuf+_ui16ReceiveBufOffset, ui16BytesToCopy);
    _ui16ReceiveBufCount -= ui16BytesToCopy;
    _ui16ReceiveBufOffset += ui16BytesToCopy;

    return ui16BytesToCopy;
}

uint32 StreamMocket::getBytesAvailable (void)
{
    return _pMMocket->getCumulativeSizeOfAvailableMessages();
}

void StreamMocket::run (void)
{
    _bTerminateThread = false;
    while (!_bTerminateThread) {
        _m.lock();
        if ((_i64LastTransmitTime + _ui32DataBufferingTime) < getTimeInMilliseconds()) {
            int rc;
            if (0 != (rc = flush())) {
                checkAndLogMsg ("StreamMocket::run", Logger::L_MildError,
                                "flush() failed with rc = %d\n", rc);
                _bTerminateThread = true;
            }
        }
        else {
            _cv.wait (_ui32DataBufferingTime < 10 ? 10 : _ui32DataBufferingTime);
        }
        _m.unlock();
    }
    _bThreadTerminated = true;
}
