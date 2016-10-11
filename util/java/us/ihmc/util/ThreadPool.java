/*
 * ThreadPool.java
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

/**
 * ThreadPool
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision: 1.9 $
 *              $Date: 2016/06/09 20:02:46 $
 */

package us.ihmc.util;

import java.util.ArrayList;
import java.util.List;

/**
 *
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class ThreadPool
{
    public ThreadPool (int maxNumWorkers)
    {
        if (maxNumWorkers <= 0) {
            throw new IllegalArgumentException ("maxNumWorker cannot be <= 0");
        }

        _maxNumWorkers = maxNumWorkers;
        _workers = new ArrayList<ThreadPoolWorker>();
        _tasks = new ArrayList<ThreadPoolTask>();
    }

    /**
     *
     */
    public void enqueue (Runnable runnable, ThreadPoolMonitor tpm)
    {
        ThreadPoolTask tpt = new ThreadPoolTask (runnable, tpm);

        this.checkAndActivateThreads();

        synchronized (_tasks) {
            _tasks.add (tpt);
            _tasks.notifyAll();
        }
    } //enqueue()

    /**
     *
     */
    public void enqueue (Runnable runnable)
    {
        this.enqueue (runnable, null);
    }

    // /////////////////////////////////////////////////////////////////////////
    // PRIVATE METHODS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private ThreadPoolTask dequeueTask()
    {
        ThreadPoolTask task = null;

        synchronized (_tasks) {
            while (_tasks.isEmpty()) {
                try {
                    _tasks.wait();
                }
                catch (Exception ex) {
                    ex.printStackTrace();
                }
            } //while()

            task = _tasks.remove (0);
        }

        return task;
    } //dequeueTask()

    /**
     *
     */
    private void threadBusyStatusChanged (boolean threadIsBusy)
    {
        synchronized (this) {
            if (threadIsBusy) {
                _numBusyWorkers++;
            }
            else {
                _numBusyWorkers--;
            }
        }
    } //threadBusyStatusChanged()

    /**
     *
     */
    private void checkAndActivateThreads()
    {
        int numWorkers = _workers.size();
        if (numWorkers >= _maxNumWorkers) {
            return;
        }

        synchronized (this) {
            int numIdleWorkers = numWorkers - _numBusyWorkers;
            if (numIdleWorkers == 0) {
                ThreadPoolWorker tpt = new ThreadPoolWorker (this);
                _workers.add (tpt);
                tpt.start();
            }
        }
    } //checkAndActivateThreads()

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     *
     */
    private class ThreadPoolTask
    {
        public ThreadPoolTask (Runnable r, ThreadPoolMonitor tpm)
        {
            runnable = r;
            tpMon = tpm;
        }

        Runnable runnable       = null;
        ThreadPoolMonitor tpMon = null;
    } //class ThreadPoolTask

    /**
     *
     */
    private static class ThreadPoolWorker extends Thread
    {
        ThreadPoolWorker (ThreadPool tp)
        {
            setName ("ThreadPoolWorker" + (_tpwInstanceCounter++) + "-[" + getName() + "]");
            _threadPool = tp;
        }

        /**
         *
         */
        public void run()
        {
            while (true) {
                ThreadPoolTask tpt = _threadPool.dequeueTask();
                Exception runnableEx = null;

                _threadPool.threadBusyStatusChanged (true);

                try {
                    tpt.runnable.run();
                }
                catch (Exception ex) {
                    runnableEx = ex;
                }

                if (tpt.tpMon != null) {
                    try {
                        tpt.tpMon.runFinished (tpt.runnable, runnableEx);
                    }
                    catch (Exception ex) {
                    } //intentionally left blank.
                }

                _threadPool.threadBusyStatusChanged (false);
            }
        } //run()

        private ThreadPool _threadPool = null;
        private static int _tpwInstanceCounter = 0;
    } // class ThreadPoolWorker

    /**
     *
     */
    public interface ThreadPoolMonitor
    {
        public void runFinished (Runnable runnable, Exception exception);
    } //interface ThreadPoolMonitor

    // /////////////////////////////////////////////////////////////////////////
    private int _maxNumWorkers = 0;
    private int _numBusyWorkers = 0;

    private final List<ThreadPoolWorker> _workers;
    private final List<ThreadPoolTask> _tasks;
} //class ThreadPool
