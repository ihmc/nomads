/*
 * ManageableThread.h
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

#ifndef INCL_MANAGEABLE_THREAD_H
#define INCL_MANAGEABLE_THREAD_H

#include "ConditionVariable.h"
#include "Mutex.h"
#include "Thread.h"

namespace NOMADSUtil
{

    class ManageableThread;

    typedef void (*TerminationCallbackFnPtr) (void *pCallbackArg, ManageableThread *pThread);

    class ManageableThread : public Thread
    {
        public:
            ManageableThread (void);
            virtual ~ManageableThread (void);

            bool isRunning (void);
            virtual void requestTermination (void);
            virtual void requestTerminationAndWait (void);
            bool hasTerminated (void);
            int getTerminatingResultCode (void);

            void setTerminationCallback (TerminationCallbackFnPtr pCallbackFn, void *pCallbackArg);

        protected:
            // Should be called from the run() method of the thread before doing anything else
            void started (void);

            // Should be called from the run() method periodically to check to see if someone has requested termination
            bool terminationRequested (void);

            // May be used to set a result code before termination
            void setTerminatingResultCode (int rc);

            // Should be called from the run() method when the method is about to terminate
            // NOTE: This should be the last method invoked before run() terminates!
            void terminating (void);

        private:
            bool _bRunning;
            bool _bTerminateRequested;
            int _iTerminatingResultCode;
            bool _bTerminated;
            TerminationCallbackFnPtr _pTerminationCallbackFn;
            void *_pTerminationCallbackArg;
            Mutex _m;
            ConditionVariable _cv;
    };

    inline bool ManageableThread::isRunning (void)
    {
        return _bRunning;
    }

    inline void ManageableThread::requestTermination (void)
    {
        _bTerminateRequested = true;
    }

    inline bool ManageableThread::hasTerminated (void)
    {
        return _bTerminated;
    }

    inline int ManageableThread::getTerminatingResultCode (void)
    {
        return _iTerminatingResultCode;
    }

    inline void ManageableThread::setTerminationCallback (TerminationCallbackFnPtr pCallbackFn, void *pCallbackArg)
    {
        _pTerminationCallbackFn = pCallbackFn;
        _pTerminationCallbackArg = pCallbackArg;
    }

    inline void ManageableThread::started (void)
    {
        _bRunning = true;
    }

    inline bool ManageableThread::terminationRequested (void)
    {
        return _bTerminateRequested;
    }

    inline void ManageableThread::setTerminatingResultCode (int rc)
    {
        _iTerminatingResultCode = rc;
    }

    inline void ManageableThread::terminating (void)
    {
        if (_pTerminationCallbackFn) {
            (*_pTerminationCallbackFn) (_pTerminationCallbackArg, this);
        }
        _bTerminated = true;
        _cv.notifyAll();
    }
}

#endif   // #ifndef INCL_MANAGEABLE_THREAD_H
