/*
 * ThreadPool.h
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

#ifndef INCL_THREAD_POOL_H
#define INCL_THREAD_POOL_H

#include "ConditionVariable.h"
#include "Mutex.h"
#include "Queue.h"
#include "Runnable.h"
#include "Thread.h"

namespace NOMADSUtil
{

    class ThreadPoolMonitor;

    class ThreadPool
    {
        public:
            ThreadPool (int iMaxNumWorkers = 10);
            ~ThreadPool (void);

            int enqueue (Runnable *pRunnable, bool bDeleteWhenFinished = true, ThreadPoolMonitor *pTPMon = NULL);

        private:
            class ThreadPoolTask
            {
                public:
                    ThreadPoolTask (Runnable *pRunnable, bool bDeleteWhenFinished, ThreadPoolMonitor *pTPMon);
                    ThreadPoolTask (void);

                public:
                    Runnable *pRunnable;
                    bool bDeleteWhenFinished;
                    ThreadPoolMonitor *pThreadPoolMon;
            }; // class ThreadPoolTask

            class ThreadPoolWorker : public Thread
            {
                public:
                    ThreadPoolWorker (ThreadPool *pThreadPool);
                    void run();

                private:
                    ThreadPool * _pThreadPool;
            }; // class ThreadPoolWorker

        private:
            int dequeueTask (ThreadPoolTask &tpt);
            void checkAndActivateThreads (void);
            void threadBusyStatusChanged (bool bThreadBusy);

        private:
    	    //friend class ThreadPoolWorker;

            Queue<ThreadPoolTask> _taskQueue;
            Mutex _mTaskQueue;
            ConditionVariable _cvTaskQueue;

            ThreadPoolWorker ** _pTPWorkers;

            Mutex _mBusyWorkerCount;

            // total number of Worker Threads that have been activated.
            int _iNumActiveWorkers;

            // number of Active worker threads that are executing a task.
            int _iNumBusyWorkers;

            // maximun number of WorkerThreads that will ever be activated.
            int _iMaxNumWorkers;
    };

    class ThreadPoolMonitor
    {
        public:
            virtual void runFinished (Runnable *pRunnable, int iRunRC) = 0;
    };

}

#endif //INCL_THREAD_POOL_H

