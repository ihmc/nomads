/*
 * LoggingMutex.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#include "LoggingMutex.h"
#include "FileWriter.h"
#include "SocketWriter.h"
#include "TCPSocket.h"

using namespace NOMADSUtil;

Mutex LoggingMutex::_mWriter;
Writer * LoggingMutex::_pWriter;
bool LoggingMutex::_bIsConnected;

LoggingMutex::LoggingMutex (uint16 ui16Id, bool bDisableLogging)
{
    #ifdef ENABLE_MUTEX_LOGGING
        _statusBuf.status.ui16MutexId = ui16Id;
        _statusBuf.status.ui16CurrentLocationId = 0;
        _statusBuf.status.ui16LastLockLocationId = 0;
        _statusBuf.status.ui64CurrentThreadId = 0;
        _statusBuf.status.ui64OwnerThreadId = 0;
        _statusBuf.status.ui18Status = UNDEFINED;

        _ui8Type = FILE_TYPE;
        _mWriter.lock();
        if (_pWriter == NULL) {
            FILE *pRawMutexFile = NULL;
            #ifdef ANDROID
                pRawMutexFile = fopen ("/sdcard/mutex.raw", "wb");
            #else
                pRawMutexFile = fopen ("mutex.raw", "wb");
            #endif
            if (pRawMutexFile != NULL) {
                _pWriter = new FileWriter (pRawMutexFile);
                _bIsConnected = true;
            }
        }
        _mWriter.unlock();

        _bDisableLogging = bDisableLogging;
    #endif
}

LoggingMutex::LoggingMutex (uint16 ui16Id, SocketType type, const char *pszAddr, bool bDisableLogging)
{
    #ifdef ENABLE_MUTEX_LOGGING
        _statusBuf.status.ui16MutexId = ui16Id;
        _statusBuf.status.ui16CurrentLocationId = 0;
        _statusBuf.status.ui16LastLockLocationId = 0;
        _statusBuf.status.ui64CurrentThreadId = 0;
        _statusBuf.status.ui64OwnerThreadId = 0;
        _statusBuf.status.ui18Status = UNDEFINED;
        _localHostAddr.setIPAddress (pszAddr);
        _localHostAddr.setPort (MUTEX_STATUS_BROACAST_PORT);

        _mWriter.lock();
        if (_pWriter != NULL) {

            _ui8Type = (uint8) type;
            Socket *pSocket = NULL;
            switch (type) {
                case UDP_SOCKET:
                
                case TCP_SOCKET:
                    pSocket = new TCPSocket();
                    if (pSocket != NULL) {
                        _bIsConnected = (((TCPSocket *) pSocket)->connect (_localHostAddr.getIPAsString(), _localHostAddr.getPort()) == 0);
                    }
                    break;
            }

            if (_bIsConnected) {
                _pWriter = new SocketWriter (pSocket, true);
            }
        }
        _mWriter.unlock();

        _bDisableLogging = bDisableLogging;
    #endif
}

LoggingMutex::~LoggingMutex (void)
{
    #ifdef ENABLE_MUTEX_LOGGING
    if (_pWriter != NULL) {
        _pWriter->flush();
        // Since _pWriter is a shared, static variable, do not close it here
    }
    #endif
}

uint16 LoggingMutex::getId (void)
{
    #ifdef ENABLE_MUTEX_LOGGING
        return _statusBuf.status.ui16MutexId;
    #else
        return 0;
    #endif
}

int LoggingMutex::lock (uint16 ui16LocationId)
{
    #ifdef ENABLE_MUTEX_LOGGING
        if (!_bDisableLogging && _bIsConnected) {
            // Broadcast packet indicating lock has been called
            _statusBuf.status.ui16CurrentLocationId = ui16LocationId;
            _statusBuf.status.ui64CurrentThreadId = getCurrentThreadId();
            _statusBuf.status.ui18Status = TRYING_TO_LOCK;
            _pWriter->writeBytes (&_statusBuf, sizeof (_statusBuf));
            _pWriter->flush();
        }
    #endif

    int rc = Mutex::lock();

    #ifdef ENABLE_MUTEX_LOGGING
        if (!_bDisableLogging && _bIsConnected) {
            // Broadcast packet indicating lock has succeeded
            _statusBuf.status.ui64OwnerThreadId = getCurrentThreadId();
            _statusBuf.status.ui18Status = LOCK_SUCCEEDED;
            _statusBuf.status.ui16LastLockLocationId = ui16LocationId;
            _pWriter->writeBytes (&_statusBuf, sizeof (_statusBuf));
            _pWriter->flush();
        }
    #endif

    return rc;
}

int LoggingMutex::tryLock (uint16 ui16LocationId)
{
    int rc = Mutex::tryLock();

    #ifdef ENABLE_MUTEX_LOGGING
        if (!_bDisableLogging && rc == Mutex::RC_Ok && _bIsConnected) {
            // Broadcast packet indicating lock has succeeded
            _statusBuf.status.ui64CurrentThreadId = getCurrentThreadId();
            _statusBuf.status.ui64OwnerThreadId = getCurrentThreadId();
            _statusBuf.status.ui18Status = LOCK_SUCCEEDED;
            _statusBuf.status.ui16CurrentLocationId = ui16LocationId;
            _statusBuf.status.ui16LastLockLocationId = ui16LocationId;
            _pWriter->writeBytes (&_statusBuf, sizeof (_statusBuf));
            _pWriter->flush();
        }
    #endif

    return rc;
}

int LoggingMutex::unlock (uint16 ui16LocationId)
{
    int rc = Mutex::unlock();

    #ifdef ENABLE_MUTEX_LOGGING
        if (!_bDisableLogging && _bIsConnected) {
            // Broadcast packet indicating lock has been released
            _statusBuf.status.ui64OwnerThreadId = 0;
            _statusBuf.status.ui64CurrentThreadId = getCurrentThreadId();
            _statusBuf.status.ui16CurrentLocationId = ui16LocationId;
            _statusBuf.status.ui16LastLockLocationId = 0;
            _statusBuf.status.ui18Status = UNLOCKED;
            _pWriter->writeBytes (&_statusBuf, sizeof (_statusBuf));
            _pWriter->flush();
        }
    #endif

    return rc;
}
