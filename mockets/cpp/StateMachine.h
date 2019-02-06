#ifndef INCL_STATE_MACHINE_H
#define INCL_STATE_MACHINE_H

/*
 * StateMachine.h
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

#include "Mutex.h"


namespace NOMADSUtil
{
    class Mutex;
}

//#include <stdio.h>

class StateMachine
{
    public:
        StateMachine (void);
        ~StateMachine (void);

        enum State {
            S_UNDEFINED = 0,
            S_CLOSED = 1,
            S_COOKIE_WAIT = 2,
            S_COOKIE_ECHOED = 3,
            S_ESTABLISHED = 4,
            S_SHUTDOWN_PENDING = 5,
            S_SHUTDOWN_SENT = 6,
            S_SHUTDOWN_RECEIVED = 7,
            S_SHUTDOWN_ACK_SENT = 8,
            S_SUSPEND_PENDING = 9,              // Enter this state when application has asked for suspension and mocket is waiting to flush data
            S_SUSPEND_SENT = 10,                // Enter this state after mocket is done waiting to flush and sends SUSPEND message
            S_SUSPENDED = 11,                   // Enter this state when mocket has received SUSPENDACK message
            S_SUSPEND_RECEIVED = 12,            // Enter this state when mocket has received SUSPEND message: from S_ESTABLISHED, S_SUSPEND_PENDING
                                                //      and from S_SUSPEND_SENT in case of simultaneous suspension if the local node is not selected for suspension
            S_RESUME_SENT = 13,                 // Enter this state when application has called resume
            S_SIMPLE_CONNECT_ACK_WAIT = 14,

            S_APPLICATION_ABORT = 15            // Enter this state when the application has requested the abort
        };

        bool receivedInit (void);
        bool abort (void);
        bool associate (void);
        bool receivedCookieEcho (void);
        bool receivedInitAck (void);
        bool receivedCookieAck (void);
        bool shutdown (void);
        bool receivedShutdown (void);
        bool outstandingQueueFlushed (void);
        bool receivedShutdownAck (void);
        bool receivedShutdownComplete (void);
        bool suspend (void);
        bool queueFlushedOrTimeout (void);
        bool receivedSuspend (void);
        bool receivedSuspendAck (void);
        bool resumeFromSuspension (void);
        bool resume (void);
        bool receivedResume (void);
        bool receivedResumeAck (void);
        bool suspendTimeoutExpired (void);
        bool simpleConnect (void);
        bool receivedSimpleConnectAck (void);

        void setClosed (void);

        bool applicationAbort (void);

        State getCurrentState (void);
        const char * getCurrentStateAsString (void);

    private:
        State _state;
        NOMADSUtil::Mutex _m;
};


inline StateMachine::StateMachine (void)
{
    _state = S_CLOSED;
}

inline StateMachine::~StateMachine (void)
{ }

inline void StateMachine::setClosed (void)
{
    _state = S_CLOSED;
}

inline StateMachine::State StateMachine::getCurrentState (void)
{
    return _state;
}

#endif   // #ifndef INCL_STATE_MACHINE_H
