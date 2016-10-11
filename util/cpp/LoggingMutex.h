/*
 * LoggingMutex.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_LOGGING_MUTEX_H
#define INCL_LOGGING_MUTEX_H

#include "InetAddr.h"
#include "FTypes.h"
#include "Mutex.h"

namespace NOMADSUtil
{
    class Writer;

    class LoggingMutex : private Mutex
    {
        public:
            enum SocketType {
               UDP_SOCKET = 0x00,
               TCP_SOCKET = 0x01
            };

            LoggingMutex (uint16 ui16Id, bool bDisableLogging = false);
            LoggingMutex (uint16 ui16Id, SocketType type, const char *pszAddr = "127.0.0.1",
                          bool bDisableLogging = false);
            ~LoggingMutex (void);

            static const uint16 MUTEX_STATUS_BROACAST_PORT = 9753;

            uint16 getId (void);

            int lock (uint16 ui16LocationId);

            // Returns RC_Busy if mutex is already locked and RC_Ok if lock was obtained
            int tryLock (uint16 ui16LocationId);

            int unlock (uint16 ui16LocationId);

        private:
            friend class LoggingConditionVariable;

            static const uint8 FILE_TYPE = 0x02;

            enum LockStatus {
                UNDEFINED      = 0x00,
                TRYING_TO_LOCK = 0x01,
                LOCK_SUCCEEDED = 0x02,
                UNLOCKED       = 0x03
            };

            #pragma pack (1)
            union {
                struct {
                    uint16 ui16MutexId;
                    uint16 ui16CurrentLocationId;
                    uint16 ui16LastLockLocationId;
                    uint64 ui64OwnerThreadId;
                    uint64 ui64CurrentThreadId;
                    uint8 ui18Status;       // Status = 1 for trying to lock, 2 for lock succeeded, 3 for unlocked
                } status;
                char buf [23];
            } _statusBuf;
            #pragma pack()

        private:
            uint64 getCurrentThreadId (void);

        private:
            static Mutex _mWriter;
            static Writer *_pWriter;
            static bool _bIsConnected;
            bool _bDisableLogging;
            uint8 _ui8Type;
            uint32 _ui16LastLockLocationId;
            InetAddr _localHostAddr;
    };

    inline uint64 LoggingMutex::getCurrentThreadId (void)
    {
        #if defined (WIN32)
            return (uint64) GetCurrentThreadId();
        #elif defined (UNIX)
            return (uint64) pthread_self();
        #endif
    }

}

#endif   // #ifndef INCL_LOGGING_MUTEX_H
