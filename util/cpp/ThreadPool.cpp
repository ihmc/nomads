/*
 * ThreadPool.cpp
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

#include "ThreadPool.h"

using namespace NOMADSUtil;

ThreadPool::ThreadPool (int iMaxNumWorkers)
    : _cvTaskQueue (&_mTaskQueue)
{
    if (iMaxNumWorkers <= 0) {
        _pTPWorkers = NULL;
        return;
    }

    _iMaxNumWorkers = iMaxNumWorkers;
    _pTPWorkers = new ThreadPoolWorker*[iMaxNumWorkers];

    _iNumActiveWorkers = 0;
    _iNumBusyWorkers = 0;
}

ThreadPool::~ThreadPool()
{
    if (_pTPWorkers) {
        while (_iNumActiveWorkers > 0) {
            delete _pTPWorkers[_iNumActiveWorkers];
            _iNumActiveWorkers--;
        }
    }
}

int ThreadPool::enqueue (Runnable *pRunnable, bool bDeleteWhenFinished, ThreadPoolMonitor *pTPMon)
{
    checkAndActivateThreads();

    ThreadPoolTask tpt (pRunnable, bDeleteWhenFinished, pTPMon);
    _mTaskQueue.lock();

    _taskQueue.enqueue (tpt);

    // Raffaele: I think that doing simply a notify() here will not cause the "lost signal"
    // problem, because the worker will check the _cvTaskQueue.isEmpty() before waiting again.
    // So it's like using _cvTaskQueue's current size as an indicator of how many notify()
    // have been issued.

    _cvTaskQueue.notify();
    //_cvTaskQueue.notifyAll();
    _mTaskQueue.unlock();
    return 0;
}

int ThreadPool::dequeueTask (ThreadPool::ThreadPoolTask &tpt)
{
    _mTaskQueue.lock();
    while (_taskQueue.isEmpty()) {
        _cvTaskQueue.wait();
    }
    _taskQueue.dequeue(tpt);
    _mTaskQueue.unlock();

    return 0;
}

void ThreadPool::threadBusyStatusChanged (bool bThreadBusy)
{
    _mBusyWorkerCount.lock();

    if (bThreadBusy) {
        _iNumBusyWorkers++;
    }
    else {
        _iNumBusyWorkers--;
    }

    _mBusyWorkerCount.unlock();
}

void ThreadPool::checkAndActivateThreads()
{
    if (_iNumActiveWorkers >= _iMaxNumWorkers) {
        return;
    }

    _mBusyWorkerCount.lock();

    int iNumFreeWorkers = _iNumActiveWorkers - _iNumBusyWorkers;
    if (iNumFreeWorkers == 0) {
        ThreadPoolWorker *pTPWAux = new ThreadPoolWorker (this);
        _pTPWorkers[_iNumActiveWorkers++] = pTPWAux;
        pTPWAux->start();
    }

    _mBusyWorkerCount.unlock();
}

// ============================================
// ThreadPoolWorker
// ============================================
ThreadPool::ThreadPoolWorker::ThreadPoolWorker (ThreadPool *pThreadPool)
{
    _pThreadPool = pThreadPool;
}

void ThreadPool::ThreadPoolWorker::run()
{
    ThreadPool::ThreadPoolTask tpt;

    while (true) {
        _pThreadPool->dequeueTask (tpt);
        _pThreadPool->threadBusyStatusChanged (true);

        if (tpt.pRunnable != NULL) {
            int rc = tpt.pRunnable->run();

            if (tpt.bDeleteWhenFinished) {
                delete tpt.pRunnable;
            }
            if (tpt.pThreadPoolMon != NULL) {
                tpt.pThreadPoolMon->runFinished (tpt.pRunnable, rc);
            }
        }

           _pThreadPool->threadBusyStatusChanged (false);
    }
}

// ============================================
// ThreadPoolTask
// ============================================
ThreadPool::ThreadPoolTask::ThreadPoolTask()
{
    this->pRunnable = NULL;
    this->bDeleteWhenFinished = false;
    this->pThreadPoolMon = NULL;
}

ThreadPool::ThreadPoolTask::ThreadPoolTask (Runnable *pRunnable, bool bDeleteWhenFinished, ThreadPoolMonitor *pTPMon)
{
    this->pRunnable = pRunnable;
    this->bDeleteWhenFinished = bDeleteWhenFinished;
    this->pThreadPoolMon = pTPMon;
}

