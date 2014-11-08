/*
 * MsgQueue.java
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

package us.ihmc.util;

import java.util.LinkedList;

/**
 * This class construct a queue to buffer message
 * Borrowed from gnutella.
 */
@SuppressWarnings({ "rawtypes", "unchecked" })
public class MsgQueue
{
    public MsgQueue (int maxSize)
    {
        _maxSize = maxSize;
        _queue = new LinkedList();
    }

    public MsgQueue ()
    {
        _queue = new LinkedList();
    }

    /**
     * Enqueue an object
     *
     * @return true if enqueue worked, false if queue is full
     */
    public synchronized boolean enqueue (Object o)
    {
        _queue.add (o);
        return true;
    }

    /**
     * Remove an object from the queue
     */
    public synchronized Object dequeue()
    {
        return _queue.remove(0);
    }

    /**
     * Check if the queue is empty
     *
     * @return true if empty, false if not empty
     */
    public synchronized boolean empty()
    {
        return _queue.size() == 0;
    }

    /**
     * Get the size of message queue
     *
     * @return int  the size of the queue
     */
    public synchronized int getSize()
    {
        return _queue.size();
    }

    @SuppressWarnings("unused")
    private int _maxSize;
    private LinkedList _queue;
}