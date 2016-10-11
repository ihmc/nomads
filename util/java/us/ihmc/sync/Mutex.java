/*
 * Mutex.java
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

package us.ihmc.sync;

/**
 * Mutex.java
 *
 * Created on April 17, 2007, 6:19 PM
 *
 * @author nsuri
 */
public class Mutex
{
    public synchronized void lock()
    {
        while (true) {
            if (_lockOwner == null) {
                _lockOwner = Thread.currentThread();
                _lockCount = 1;
                return;
            }
            else if (_lockOwner == Thread.currentThread()) {
                _lockCount++;
                return;
            }
            else {
                try {
                    wait();
                }
                catch (InterruptedException e) {}
            }
        }
    }

    public synchronized void unlock()
    {
        if (_lockOwner == null) {
            throw new RuntimeException ("mutex is not locked");
        }
        else if (_lockOwner != Thread.currentThread()) {
            throw new RuntimeException ("calling thread is not owner of lock");
        }
        else {
            _lockCount--;
            if (_lockCount == 0) {
                _lockOwner = null;
                notifyAll();
            }
        }
    }

    private int _lockCount;
    private Thread _lockOwner;
}
