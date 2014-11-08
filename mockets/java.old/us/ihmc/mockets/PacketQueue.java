/**
 * The PacketQueue class represents the queue for received data buffers 
 * waiting to be fetched by the application.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.Vector;
import java.util.logging.Logger;


class PacketQueue
{
    PacketQueue() 
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _firstNode = null;
        _lastNode = _firstNode;

        _nodesInQueue = 0;
        _packetsInQueue = 0;
        _bytesInQueue = 0;
        _nextNodeID = 1;
        _mutex = new Object();
    }

    boolean isEmpty() 
    {
        boolean ret = false;
        
        // perhaps synchronized is a bit too much here?
        synchronized (_mutex) {
            if (_firstNode == null) {
                assert _nodesInQueue == 0 : "_firstNode and _nodesInQueue are misaligned!!!";
                assert _bytesInQueue == 0 : "_firstNode and _bytesInQueue are misaligned!!!";
                assert _packetsInQueue == 0 : "_firstNode and _packetsInQueue are misaligned!!!";
                assert _lastNode == null : "_firstNode and _lastNode are misaligned!!!";
                ret = true;
            }
            _mutex.notifyAll();
        }
        
        return ret;
    }
    
    int getSize() 
    {
        return _bytesInQueue;
    }
    
    int getPacketsInQueue() 
    {
        return _packetsInQueue;
    }
    
    private void insertNode (Node n)
    {
        assert n != null;

        if (_lastNode == null) {
            _firstNode = n;
        } else {
            _lastNode._next = n;
        }
        _lastNode = n;

        ++_nodesInQueue;
    }
    
    private void removeFirstNode()
    {
        if (_firstNode != null) {
            if (_firstNode == _lastNode) {
                _firstNode = null;
                _lastNode = null;
            } else {
                _firstNode = _firstNode._next;
            }
            --_nodesInQueue;
        }
    }
    
    void insert (DataBuffer p) 
    {
        _logger.fine ("Entering PacketQueue::insert with a packet of size = " +
                      p.getSize());

        synchronized (_mutex) {
            insertNode (new Node (p));
            ++_packetsInQueue;
            _mutex.notifyAll();
        }

        _logger.fine ("Exiting PacketQueue::insert - packets in queue: " + 
                      _packetsInQueue);
    }

    void insert (Vector v) // v is a Vector of data buffers
    {        
        _logger.fine ("Entering PacketQueue::insert with a vector of size = " + 
                      v.size());
        
        assert v.size() > 0;

        _logger.fine ("v.size() IN is " + v.size());

        synchronized (_mutex) {
            if (v.size() == 1) {
                insertNode (new Node ((DataBuffer)v.get(0)));
            } else {
                insertNode (new Node ((DataBuffer)v.get(0), Node.FIRST_FRAGMENT));
                
                for (int i = 1; i < v.size() - 1; ++i) {
                    insertNode (new Node ((DataBuffer)v.get(i), Node.MORE_FRAGMENTS));
                }

                insertNode (new Node ((DataBuffer)v.get(v.size() - 1), Node.LAST_FRAGMENT));
            }
            ++_packetsInQueue;
            _mutex.notifyAll();
        }
        
        _logger.fine ("Exiting PacketQueue::insert - packets in queue: " + 
                      _packetsInQueue);
    }
    
    /*
     * Extract a packet from the queue. This function uses the specified 
     * input timeout (in milliseconds). If timeout == 0, then wait forever.
     * 
     *                    ! ! ! ! B E W A R E ! ! ! !
     *                    
     * If the timeout expires and the list is still empty, this function 
     * may throw an InterruptedException or return a null value.
     */
    ReceivedMessage extract (long timeout) 
    {
        // no need to worry about the timeout < 0 case here, as wait will raise 
        // an IllegalArgumentException if that happens
        
        _logger.fine ("Entering PacketQueue::extract with timeout = " + timeout + 
                      ", packets in queue: " + _packetsInQueue);
        
        long start = System.currentTimeMillis();
        long tmout = timeout;
        long curr_time = start;
        Node n = null;
       
        synchronized (_mutex) {
            do {
                assert _packetsInQueue >= 0;
               
                // while queue is empty and the given timeout is not elapsed 
                // wait for an incoming packet
                while ((_packetsInQueue == 0) &&
                       (timeout == 0 || curr_time < start + timeout)) {
                    _logger.fine ("PacketQueue::extract WAITING...");
                    
                    try {
                        _mutex.wait (tmout);
                    }
                    catch (InterruptedException e) {} // intentionally left blank

                    curr_time = System.currentTimeMillis();
                    tmout = (timeout == 0 ? 0 : Math.max (timeout - (curr_time - start), 1));
                }
                    
                // if queue is not empty and the given timeout is not elapsed 
                // return first message
                if ((_packetsInQueue > 0) &&
                    (timeout == 0 || curr_time < start + timeout)) {
                    _logger.fine ("PacketQueue::extract found packet");
                    
                    assert (_firstNode._flags & (Node.LAST_FRAGMENT | Node.MORE_FRAGMENTS)) == 0;
                    
                    if ((_firstNode._flags & Node.FIRST_FRAGMENT) != 0) {
                        int i = 1;
                        int size = 0;
                        Vector v = new Vector();

                        while (_firstNode != null) {
                            _logger.fine ("found fragment # " + i++);
                            boolean lastFragment = false;
                            if ((_firstNode._flags & Node.LAST_FRAGMENT) != 0) {
                                lastFragment = true;
                                _logger.fine ("last fragment");
                            }
                            size += _firstNode._data.getSize();
                            
                            v.add (_firstNode._data);
                            removeFirstNode();
                            if (lastFragment) break;
                        }

                        --_packetsInQueue;
                        _logger.fine ("v.size() is " + v.size());
                        
                        _logger.fine ("PacketQueue::extract returning packet - 1");
                        _mutex.notifyAll();
                        return new ReceivedMessage (v);
                    } else {
                        ReceivedMessage ret = new ReceivedMessage (_firstNode._data);
                        removeFirstNode();                    
                        --_packetsInQueue;
                        _logger.fine ("PacketQueue::extract returning packet - 2");
                        _mutex.notifyAll();
                        return ret;
                    }
                }
                
                curr_time = System.currentTimeMillis();
                tmout = (timeout == 0 ? 0 : Math.max (timeout - (curr_time - start), 1));
            } while (timeout == 0 || curr_time < start + timeout);
            _mutex.notifyAll();
        }

        /*
        _logger.fine ("Exiting PacketQueue::extract with timeout = " + timeout + 
                      ", packets in queue: " + _packetsInQueue);
        */
        _logger.fine ("PacketQueue::extract returning packet - 1");

        return null;
    }

    private int getNextNodeID()
    {
        return _nextNodeID++;
    }
    
    void _dump (PrintStream os)
    {
        os.println ("--------------------------------------------------");
        for (Node ptr = _firstNode; ptr != null; ptr = ptr._next)
            ptr._dump (os);
        os.println ("--------------------------------------------------");
    }
    

    class Node
    {
        Node (DataBuffer data) 
        {
            _next = null;
            _data = data;
            _flags = 0;
            _id = getNextNodeID();
        }

        Node (DataBuffer data, int flags) 
        {            
            assert (flags & ~FLAGS_MASK) == 0;
            
            _next = null;
            _data = data;
            _flags = flags;
            _id = getNextNodeID();
        }

        Node (Node next, DataBuffer data) 
        {
            _next = next;
            _data = data;
            _flags = 0;
            _id = getNextNodeID();
        }

        Node (Node next, DataBuffer data, int flags) 
        {
            assert (flags & ~FLAGS_MASK) == 0;

            _next = next;
            _data = data;
            _flags = flags;
            _id = getNextNodeID();
        }

        void _dump (PrintStream os) 
        {
            String next = (_next != null ? new Integer(_next._id).toString() : "null");
            os.print ("id = " + _id + " (next = " + next + ") ");
            if ((_flags & FIRST_FRAGMENT) != 0) {
                os.print ("FF ");
            }
            if ((_flags & MORE_FRAGMENTS) != 0) {
                os.print ("MF ");
            }
            if ((_flags & LAST_FRAGMENT) != 0) {
                os.print ("LF ");
            }
            os.println ("");
        }

        int _id;
        int _flags;
        Node _next;
        DataBuffer _data;

        static final int FIRST_FRAGMENT = 0x01;
        static final int MORE_FRAGMENTS = 0x02;
        static final int LAST_FRAGMENT  = 0x04;
        static final int FLAGS_MASK = FIRST_FRAGMENT |
                                      MORE_FRAGMENTS |
                                      LAST_FRAGMENT;
    }
    
    private Logger _logger;
    private Node _firstNode;
    private Node _lastNode;
    private int _nodesInQueue;
    private int _packetsInQueue;
    private int _bytesInQueue;
    private int _nextNodeID;
    private Object _mutex;
}

/*
 * vim: et ts=4 sw=4
 */

