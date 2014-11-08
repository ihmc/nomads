/**
 * The UnsequencedPacketQueue class represents one of the two packet queues 
 * (for reliable unsequenced and unreliable sequenced flows respectively) 
 * in which the MessageReceiver stores fragments waiting to be recomposed 
 * by the MessagePacketProcessor
 * 
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.Iterator;
import java.util.NoSuchElementException;

import java.util.logging.Logger;


class UnsequencedPacketQueue
{
    UnsequencedPacketQueue() 
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _timeStamp = System.currentTimeMillis();
        
        _firstNode = null;
        _lastNode = _firstNode;

        _packetsInQueue = 0;
        _nextNodeID = 1;
    }

    long getTimestamp()
    {
        return _timeStamp;
    }
    
    synchronized boolean insert (MessagePacketInfo pi) 
    {
        assert !pi.isSequenced() && !pi.isControl();

        long seqNum = pi.getPacket().getSequenceNumber();

        if (_firstNode == null) {
            // if queue is empty just perform a front insertion
            insertFront (pi);
        } else {
            // queue not empty - perform sorted insertion
            Node ptr = null;
            for (ptr = _firstNode; ptr != null; ptr = ptr._next) {
                long sn = ptr._data.getPacket().getSequenceNumber();
                if (sn == seqNum) {
                    // ignore packet since it is already in the queue 
                    return false;
                } else if (SequentialArithmetic.greaterThan(sn, seqNum)) {
                    break;
                }
            }

            if (ptr == null) {
                insertBack (pi);
            } else {
                insertBefore (pi, ptr);
            }
        }
            
        ++_packetsInQueue;
        
        return true;
    }

    synchronized MessagePacketInfo peek() 
    {
        // if queue is empty return null
        if (_firstNode == null)
            return null;
        
        return _firstNode._data; 
    }

    int packetsInQueue()
    {
        return _packetsInQueue;
    }
    
    synchronized void remove (MessagePacketInfo pi) 
    {
        Node ptr = null;
        for (ptr = _firstNode; ptr != null && ptr._data != pi; ptr = ptr._next);

        if (ptr == null) {
            // throw an exception as this could be a dangerous bug, 
            // potentially leading to memory leak
            throw new RuntimeException ("Packet not found in the queue.");
        } else {
            removeNode (ptr);
            --_packetsInQueue;
        }
    }

    void removeAll() 
    {
        _timeStamp = System.currentTimeMillis();
        
        _firstNode = null;
        _lastNode = _firstNode;

        _packetsInQueue = 0;
        _nextNodeID = 1;
    } 
    
    Iterator iterator()
    {
        return new UnsequencedPacketQueueIterator (this);
    }

    private void insertBefore (MessagePacketInfo pi, Node nextNode) 
    {
        assert nextNode != null : "nextNode must not be null.";        
        Node prevNode = nextNode._prev;
        
        Node n = new Node (prevNode, nextNode, pi);
        nextNode._prev = n;
        if (prevNode == null) {
            _firstNode = n;
        } else {
            prevNode._next = n;
        }
    }
    
    private void insertFront (MessagePacketInfo pi) 
    {
        Node n = new Node (null, _firstNode, pi);
        
        if (_firstNode != null) {
            _firstNode._prev = n;
        } else {
            assert _lastNode == null;
            _lastNode = n;
        }
        _firstNode = n;
    }

    private void insertBack (MessagePacketInfo pi) 
    {
        Node n = new Node (_lastNode, null, pi);
        
        if (_lastNode != null) {
            _lastNode._next = n;
        } else {
            assert _firstNode == null;
            _firstNode = n;
        }
        _lastNode = n;
    }

    private void removeNode (Node n) 
    {
        assert n != null : "n must not be null.";
        
        Node prevNode = n._prev;
        Node nextNode = n._next;

        if (prevNode == null) {
            _firstNode = nextNode;
        } else {
            prevNode._next = nextNode;
        }
        
        if (nextNode == null) {
            _lastNode = prevNode;
        } else {
            nextNode._prev = prevNode;
        }
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
        Node (Node prev, Node next, MessagePacketInfo data) 
        {
            _prev = prev;
            _next = next;
            _data = data;
            _id = getNextNodeID();
        }

        void _dump (PrintStream os)
        {
            String next = (_next != null ? new Integer(_next._id).toString() : "null");
            String prev = (_prev != null ? new Integer(_prev._id).toString() : "null");
            MessagePacketInfo mpi = _data;
            os.print ("id = " + _id + " (prev = " + prev + ", next = " + next + ") prio " +
                      mpi.getPriority() + " ");
            MessagePacket p = mpi.getPacket();
            if (p != null && p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT)) {
                os.print ("FF ");
            }
            if (p != null && p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS)) {
                os.print ("MF ");
            }
            if (p != null && p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT)) {
                os.print ("LF ");
            }
            os.println ("");
        }

        int _id;
        Node _prev;
        Node _next;
        MessagePacketInfo _data;
    }
    
    class UnsequencedPacketQueueIterator implements Iterator
    {
        UnsequencedPacketQueueIterator (UnsequencedPacketQueue upq)
        {
            _upq = upq;
            _ptr = _upq._firstNode;
        }

        public boolean hasNext()
        {
            return _ptr != null;
        }

        public Object next() throws NoSuchElementException
        {
            if (_ptr == null)
                throw new NoSuchElementException ("Invalid element!");
            
            Object ret = _ptr._data;
            assert ret != null;
            
            _ptr = _ptr._next;
            
            return ret;
        }

        public void remove()
        {
            if (_ptr != null && _ptr._data != null) {
                MessagePacketInfo tmp = _ptr._data;
                _ptr = _ptr._next;
                _upq.remove (tmp);
            }
        }
        
        private Node _ptr;
        private UnsequencedPacketQueue _upq;
    }
    
    private Logger _logger;
    private Node _firstNode;
    private Node _lastNode;
    private int _packetsInQueue;
    private long _timeStamp;
    private int _nextNodeID;
}

/*
 * vim: et ts=4 sw=4
 */

