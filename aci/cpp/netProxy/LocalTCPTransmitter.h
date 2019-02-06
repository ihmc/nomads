#ifndef INCL_LOCAL_TCP_TRANSMITTER_H
#define INCL_LOCAL_TCP_TRANSMITTER_H

/*
 * LocalUDPDatagramsManager.h
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
 * The LocalTCPTransmitter thread is responsible for sending data received
 * from remote NetProxy instances to the local nodes and applications via TCP
 */

#include <mutex>
#include <condition_variable>

#include "ManageableThread.h"


namespace ACMNetProxy
{
    class TCPConnTable;
    class TCPManager;


    class LocalTCPTransmitterThread : public NOMADSUtil::ManageableThread
    {
    public:
        LocalTCPTransmitterThread (TCPConnTable & rTCPConnTable, TCPManager & rTCPManager);

        void run (void);
        void notify (void);
        void wakeUpThread (void);


    private:
        std::atomic<bool> _bNotified;

        TCPConnTable & _rTCPConnTable;
        TCPManager & _rTCPManager;

        mutable std::mutex _mtx;
        mutable std::condition_variable _cv;

        static const int64 I64_LTT_TIME_BETWEEN_ITERATIONS = 250;           // Time between each iteration of the LTTT
    };


    inline LocalTCPTransmitterThread::LocalTCPTransmitterThread (TCPConnTable & rTCPConnTable, TCPManager & rTCPManager) :
        _rTCPConnTable{rTCPConnTable}, _rTCPManager{rTCPManager}
    { }

    inline void LocalTCPTransmitterThread::notify (void)
    {
        _cv.notify_one();
    }

    inline void LocalTCPTransmitterThread::wakeUpThread (void)
    {
        _bNotified = true;
        notify();
    }
}

#endif  // INCL_LOCAL_TCP_TRANSMITTER_H
