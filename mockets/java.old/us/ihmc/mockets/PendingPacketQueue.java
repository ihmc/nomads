/**
 * The PendingPacketQueue class represents a the pending packet queue
 * (the queue of packets waiting to be sent) for a message mocket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.logging.Logger;

class PendingPacketQueue
{
    PendingPacketQueue (long size_in_bytes, boolean enableCrossSequencing) 
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _firstNode = null;
        _lastNode = null;

        _size = size_in_bytes;
        _packetsInQueue = 0;
        _bytesInQueue = 0;
        _crossSequencing = enableCrossSequencing;
        _nextNodeID = 1;
    }

    synchronized boolean isEmpty() 
    {
        if (_firstNode == null) {
            assert _bytesInQueue == 0 : "_firstNode and _bytesInQueue are misaligned!!!";
            assert _packetsInQueue == 0 : "_firstNode and _packetsInQueue are misaligned!!!";
            assert _lastNode == null : "_firstNode and _lastNode are misaligned!!!";
            return true;
        }
        return false;
    }

    /**
     * Insert a packet in the queue. This function uses the specified timeout
     * (in milliseconds). If timeout == 0, then wait forever.
     *
     *                    ! ! ! ! B E W A R E ! ! ! !
     *
     * If the timeout expires and the packet could not be added to the
     * list, this function may throw an InterruptedException or return false.
     */
    synchronized boolean insert (MessagePacketInfo pi, long timeout) 
    {
        // no need to worry about the timeout < 0 case here, as wait will raise
        // an IllegalArgumentException in that situation

        long start = System.currentTimeMillis();
        long tmout = (timeout == 0 ? 100 : timeout);
        long curr_time;
        boolean added = false;

        MessagePacket p = pi.getPacket();

        do {
            // check that queue is not full
            if (_bytesInQueue + p.getSize() <= _size) {
                doInsert (pi);
                _bytesInQueue += p.getSize();
                ++_packetsInQueue;
                added = true;
            }
            else {
                try {
                    wait (tmout);
                }
                catch (InterruptedException e) {} // intentionally left blank
            }

            curr_time = System.currentTimeMillis();
            tmout = (timeout == 0 ? 100 : timeout - (curr_time - start));
        }
        while ((!added) && (timeout == 0 || curr_time < start + timeout));

        /*
        if (added) {
            _logger.fine ("PPQ::INSERTION OK / PACKET size: " + p.getSize() +
                          " PPQ size: " + _bytesInQueue + " (max: " + _size +
                          ") packets: " + _packetsInQueue);
        }
        else {
            _logger.fine ("PPQ::INSERTION FAILED!!!");
        }
        */

        return added;
    }

    synchronized void cancel (boolean sequenced, int tag) 
    {
        Node ptr = _firstNode;
        while (ptr != null) {
            Node next = ptr._next;
            MessagePacket p = ptr._data.getPacket();
            if (p.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED) == sequenced &&
                p.getTagID() == tag) {
                removeNode (ptr);
                _bytesInQueue -= p.getSize();
                --_packetsInQueue;
            }
            ptr = next;
        }
    }
    
    synchronized MessagePacketInfo peek() 
    {
        // if queue is empty return null
        if (_firstNode == null)
            return null;

        return _firstNode._data;
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
            MessagePacket p = ptr._data.getPacket();
            _bytesInQueue -= p.getSize();
            --_packetsInQueue;
            /*
            _logger.fine ("PPQ::REMOVAL OK / PACKET size: " + p.getSize() +
                          " PPQ size: " + _bytesInQueue + " (max: " + _size +
                          ") packets: " + _packetsInQueue);
            */
           notifyAll();
        }
    }

    private void doInsert (MessagePacketInfo pi) 
    {
        int pPriority = pi.getPriority();
        MessagePacket p = pi.getPacket();
        boolean pIsSequenced = p.isFlagSet (MessagePacket.HEADER_FLAG_SEQUENCED);
        boolean pIsReliable = p.isFlagSet (MessagePacket.HEADER_FLAG_RELIABLE);

        assert pPriority >= TxParams.MIN_PRIORITY && pPriority <= TxParams.MAX_PRIORITY;

        Node ptr = _lastNode;

        while (ptr != null) {
            MessagePacketInfo qi = ptr._data;
            MessagePacket q = qi.getPacket();
            int qPriority = qi.getPriority();
            boolean qIsSequenced = q.isFlagSet (MessagePacket.HEADER_FLAG_SEQUENCED);
            boolean qIsReliable = q.isFlagSet (MessagePacket.HEADER_FLAG_RELIABLE);

            if ((pPriority <= qPriority) ||
                ((pIsSequenced && qIsSequenced) &&
                 (_crossSequencing || pIsReliable == qIsReliable))) {
                break;
            } else if (q.isFlagSet (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                                    MessagePacket.HEADER_FLAG_LAST_FRAGMENT |
                                    MessagePacket.HEADER_FLAG_MORE_FRAGMENTS)) {
                Node tmp = ptr._prev;

                while (tmp != null && 
                       !tmp._data.getPacket().isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT))
                    tmp = tmp._prev;
                
                if (tmp == null)
                    break;
                
                ptr = tmp;
            }
            
            ptr = ptr._prev;
        }

        if (ptr == null) {
            insertFront (pi);
        } else {
            insertAfter (pi, ptr);
        }
    }

    private void insertAfter (MessagePacketInfo pi, Node prevNode) 
    {
        assert prevNode != null : "prevNode must not be null.";

        Node nextNode = prevNode._next;
        Node n = new Node (prevNode, nextNode, pi);
        prevNode._next = n;
        if (nextNode == null) {
            _lastNode = n;
        } else {
            nextNode._prev = n;
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
        os.println ("\n--------------------------------------------------");
    }


    class Node
    {
        Node (MessagePacketInfo data) 
        {
            _next = null;
            _prev = null;
            _data = data;
            _id = getNextNodeID();
        }

        Node (Node prev, Node next, MessagePacketInfo data) 
        {
            _next = next;
            _prev = prev;
            _data = data;
            _id = getNextNodeID();
        }

        void _dump (PrintStream os) 
        {
            String next = (_next != null ? new Integer(_next._id).toString() : "null");
            String prev = (_prev != null ? new Integer(_prev._id).toString() : "null");
            MessagePacketInfo mpi = _data;
            os.print (mpi.getPacket().getSequenceNumber() + " ");
            /*
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
            */
        }

        int _id;
        Node _next;
        Node _prev;
        MessagePacketInfo _data;
    }

    private Logger _logger;
    private Node _firstNode;
    private Node _lastNode;
    private long _size;
    private long _packetsInQueue;
    private long _bytesInQueue;
    private boolean _crossSequencing;
    private int _nextNodeID;
}

/*
 * vim: et ts=4 sw=4
 */

