/**
 * The OutstandingPacketQueue class represents the outstanding packet queue
 * (the queue of packets already sent and waiting for an acknowledgement) 
 * of a message mocket.
 * 
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.PrintStream;

import java.util.logging.Logger;


class OutstandingPacketQueue implements ACKInformationListener
{
    OutstandingPacketQueue() 
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");

        _firstNode = null;
        _lastNode = null;

        _packetsInQueue = 0;
        _bytesInQueue = 0;
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

    synchronized int getSize() 
    {
        return _bytesInQueue;
    }
    
    synchronized void insert (MessagePacketInfo pi) 
    {
        MessagePacket p = pi.getPacket();
        
        long timeout = pi.getLastIOOperationTime() + pi.getRetransmissionTimeout();
        
        assert timeout > 0 : "timeout must be a positive value.";
        
        if (isEmpty()) {
            insertFront (pi);
        } else {
            /* sorted insertion */
            Node ptr = null;
            for (ptr = _lastNode; ptr != null; ptr = ptr._prev) {
                long tmp = ptr._data.getLastIOOperationTime() + 
                           ptr._data.getRetransmissionTimeout();
                if (tmp < timeout) 
                    break;
            }

            if (ptr == null) {
                insertFront (pi);
            } else {
                insertAfter (pi, ptr);
            }
        }
        
        _bytesInQueue += p.getSize();
        ++_packetsInQueue;
    }

    synchronized long timeToNextRetransmission() 
    {
        if (_firstNode == null) 
            return -1;

        MessagePacketInfo pi = _firstNode._data;
        return pi.getLastIOOperationTime() + 
               pi.getRetransmissionTimeout() - 
               System.currentTimeMillis();
    }
    
    synchronized MessagePacketInfo extractIfTimeout () 
    {
        /* return null if queue is empty */
        if (_firstNode == null)
            return null;        
        
        MessagePacketInfo pi = _firstNode._data;        
        MessagePacket p = pi.getPacket();        

        //_logger.info ("next timeout = " + (pi.getLastIOOperationTime() + pi.getRetransmissionTimeout()));
        //_logger.info ("current time = " + System.currentTimeMillis());
        
        if (pi.getLastIOOperationTime() + pi.getRetransmissionTimeout() > 
                System.currentTimeMillis())
            return null;
        
        removeNode (_firstNode);
        _bytesInQueue -= p.getSize();
        --_packetsInQueue;

        return pi;
    }

    synchronized int acknowledgePacketsTo (long tsn, boolean control, boolean sequenced) 
    {
        if (control && sequenced)
            throw new IllegalArgumentException ("Cannot set both control and sequenced.");
       
        String pktType = (control ? "control" : (sequenced ? "reliable sequenced": "reliable unsequenced"));
        _logger.info ("Acknowlegding TSN " + tsn + " for " + pktType + " flow"); 

        int ret = 0;
        
        Node ptr = _firstNode;
        while (ptr != null) {
            Node n = ptr;
            MessagePacket pkt = n._data.getPacket();
            long sn = pkt.getSequenceNumber();

            ptr = ptr._next;
            
            if (SequentialArithmetic.greaterThanOrEqual(tsn, sn) &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL) == control &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED) == sequenced) {
                // remove node
                removeNode (n);
                // decrement number of packets and bytes in the queue
                _bytesInQueue -= pkt.getSize();
                --_packetsInQueue;
                // update removed packets count
                ++ret;
            }
        }

        return ret;
    }
    
    public synchronized boolean processPacket (long sequenceNumber, boolean control, boolean sequenced) 
    {
        if (control && sequenced)
            throw new IllegalArgumentException ("Cannot set both control and sequenced.");

        Node ptr = _firstNode;
        while (ptr != null) {
            Node n = ptr;
            MessagePacket pkt = n._data.getPacket();
            long sn = pkt.getSequenceNumber();

            ptr = ptr._next;
            
            if (pkt.getSequenceNumber() == sequenceNumber &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL) == control &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED) == sequenced) {
                // remove node
                removeNode (n);
                // decrement number of packets and bytes in the queue
                _bytesInQueue -= pkt.getSize();
                --_packetsInQueue;
                return true;
            }
        }

        return false;
    }

    public synchronized int processPacketsRange (long startTSN, long endTSN, boolean control, boolean sequenced) 
    {
        if (control && sequenced)
            throw new IllegalArgumentException ("Cannot set both control and sequenced.");
        
        int ret = 0;
        
        Node ptr = _firstNode;
        while (ptr != null) {
            Node n = ptr;
            MessagePacket pkt = n._data.getPacket();
            long sn = pkt.getSequenceNumber();

            ptr = ptr._next;
            
            if (SequentialArithmetic.greaterThanOrEqual(sn, startTSN) && 
                SequentialArithmetic.greaterThanOrEqual(endTSN, sn) &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL) == control &&
                pkt.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED) == sequenced) {
                // remove node
                removeNode (n);
                // decrement number of packets and bytes in the queue
                _bytesInQueue -= pkt.getSize();
                --_packetsInQueue;
                // update removed packets count
                ++ret;
            }
        }

        return ret;
    }

    synchronized void cancel (boolean sequenced, int tag, PacketOperationListener pol) 
    {
        Node ptr = _firstNode;
        while (ptr != null) {
            Node next = ptr._next;
            MessagePacket p = ptr._data.getPacket();
            if (!p.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL) && // cannot override control packets
                p.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED) == sequenced &&
                p.getTagID() == tag) {
                pol.packetProcessed (p.getSequenceNumber());
                removeNode (ptr);
                _bytesInQueue -= p.getSize();
                --_packetsInQueue;
            }
            ptr = next;
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
    
    void _dump (PrintStream os)
    {
        for (Node ptr = _firstNode; ptr != null; ptr = ptr._next) {
            MessagePacket p = ptr._data.getPacket();
            String flags = "";
            if (p.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)) flags += "R";
            if (p.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED)) flags += "S";
            //os.println ("Packet # " + p.getSequenceNumber() + flags);
            os.print (p.getSequenceNumber() + flags + " ");
        }
        os.print ("\n");
    }
    

    class Node
    {
        Node (Node prev, Node next, MessagePacketInfo data) {
            _prev = prev;
            _next = next;
            _data = data;
        }

        Node _prev;
        Node _next;
        MessagePacketInfo _data;
    }
    
    private Logger _logger;
    private Node _firstNode;
    private Node _lastNode;
    
    private int _bytesInQueue;
    private int _packetsInQueue;
}

/*
 * vim: et ts=4 sw=4
 */

