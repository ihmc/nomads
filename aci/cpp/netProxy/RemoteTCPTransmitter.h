#ifndef INCL_REMOTE_TCP_TRANSMITTER_H
#define INCL_REMOTE_TCP_TRANSMITTER_H

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
 * The RemoteTCPTransmitter thread is responsible for sending data received
 * from the local nodes and applications to remote NetProxy instances
 */

#include <mutex>
#include <condition_variable>

#include "ManageableThread.h"


namespace ACMNetProxy
{
    class TCPConnTable;
    class ConnectionManager;
    class TCPManager;


    class RemoteTCPTransmitterThread : public NOMADSUtil::ManageableThread
    {
    public:
        RemoteTCPTransmitterThread (TCPConnTable & rTCPConnTable, ConnectionManager & rConnectionManager, TCPManager & rTCPManager);

        void run (void);
        void notify (void);
        void wakeUpThread (void);


    private:
        std::atomic<bool> _bNotified;

        TCPManager & _rTCPManager;
        TCPConnTable & _rTCPConnTable;
        ConnectionManager & _rConnectionManager;

        mutable std::mutex _mtx;
        mutable std::condition_variable _cv;

        static const int64 I64_RTT_TIME_BETWEEN_ITERATIONS = 250;               // Time between each iteration of the RTTT
    };


    inline RemoteTCPTransmitterThread::RemoteTCPTransmitterThread (TCPConnTable & rTCPConnTable, ConnectionManager & rConnectionManager,
                                                                   TCPManager & rTCPManager) :
        _bNotified{false}, _rTCPConnTable{rTCPConnTable}, _rConnectionManager{rConnectionManager}, _rTCPManager{rTCPManager}
    { }

    inline void RemoteTCPTransmitterThread::notify (void)
    {
        _cv.notify_one();
    }

    inline void RemoteTCPTransmitterThread::wakeUpThread (void)
    {
        _bNotified = true;
        notify();
    }
}

#endif  // INCL_REMOTE_TCP_TRANSMITTER_H
