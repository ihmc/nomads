/**
 * Back-end receiver class that processes sequenced packets waiting in the 
 * sequenced packet queues.
 * The MessagePacketProcessor class performs control operations and delivers 
 * data packets to the received data queue, where they will be read by the 
 * application.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketAddress;
import java.net.InetSocketAddress;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.net.SocketException;

import java.util.logging.Logger;
import java.util.Enumeration;
import java.util.NoSuchElementException;
import java.util.Vector;
import java.util.Iterator;


class MessagePacketProcessor extends Thread
{ 
    // *TSNs parameters are the sequence numbers of the next expected packets
    MessagePacketProcessor (MessageMocket mocket, long nextControlTSN,
                            long nextRelSeqTSN, long nextUnrelSeqTSN)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;

        _ackManager = _mocket.getACKManager();
        _packetQueue = _mocket.getReceivedPacketQueue();
        _controlPacketQueue = _mocket.getControlPacketQueue();
        _relSeqPacketQueue = _mocket.getRelSeqPacketQueue();
        _relUnseqPacketQueue = _mocket.getRelUnseqPacketQueue();
        _unrelSeqPacketQueue = _mocket.getUnrelSeqPacketQueue();
        _unrelUnseqPacketQueue = _mocket.getUnrelUnseqPacketQueue();
        _status = _mocket.getStateMachine();
        _receiver = null;

        // next expected control packet TSN
        _nextControlTSN = nextControlTSN;
        // next expected reliable sequenced packet TSN
        _nextRelSeqTSN = nextRelSeqTSN;
        // next expected unreliable sequenced packet TSN
        _nextUnrelSeqTSN = nextUnrelSeqTSN;
    
        setName ("MessagePacketProcessor Thread for Mocket " + _mocket.getUID());
        setPriority (8);
    }

    // no need to synchronize this method...
    void setReceiver (MessageReceiver receiver) 
    {
        _receiver = receiver;
    }

    void processPacket (MessagePacket pIn)
    {
        boolean foundStateChangeChunk = false;
        boolean foundDataChunk = false;
        DataChunk dataChk = null;
        
        _logger.fine ("PROCESSPACKET CALLED");
        
        for (Chunk chk = pIn.getFirstChunk(); chk != null; chk = pIn.getNextChunk()) {
            int chkType = chk.getType();
            
            // quick check to verify that there is no more than one state change 
            // chunk in the same packet. maybe we'll have to add more self consistency 
            // checks later...
            if ((chkType & Chunk.CHUNK_CLASS_STATECHANGE) != 0) {
                if (foundStateChangeChunk)
                    // ok, throwing an exception here is maybe too much, but we want 
                    // to catch errors quickly during the first development phases
                    throw new RuntimeException ("Can't have more than one state " + 
                                                "change chunk in the same packet!");
                foundStateChangeChunk = true;
            }
            
            if ((chkType & Chunk.CHUNK_CLASS_DATA) != 0) {
                if (foundDataChunk)
                    // ok, throwing an exception here is maybe too much, but we want 
                    // to catch errors quickly during the first development phases
                    throw new RuntimeException ("Can't have more than one data " + 
                                                "chunk in the same packet!");
                foundDataChunk = true;
            }
            
            // dispatch chunk to the appropriate processing method
            switch (chk.getType()) {
                case Chunk.CHUNK_TYPE_SHUTDOWN:
                    assert (chk instanceof ShutdownChunk);
                    //_logger.fine ("RECEIVED SHUTDOWN IN STATE " + _status.currentStateAsString());
                    _status.receivedShutdown();
                    break;
                case Chunk.CHUNK_TYPE_SHUTDOWN_ACK:
                    assert (chk instanceof ShutdownAckChunk);
                    //_logger.fine ("RECEIVED SHUTDOWN-ACK IN STATE " + _status.currentStateAsString());
                    _status.receivedShutdownAck();
                    break;
                case Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE:
                    //_logger.fine ("RECEIVED SHUTDOWN-COMPLETE IN STATE " + _status.currentStateAsString());
                    assert (chk instanceof ShutdownCompleteChunk);
                    _status.receivedShutdownComplete();
                    break;
                case Chunk.CHUNK_TYPE_ABORT:
                    assert (chk instanceof AbortChunk);
                    // abort is not supported at the moment
                    break;
                case Chunk.CHUNK_TYPE_DATA:
                    assert (chk instanceof DataChunk);
                    _logger.fine ("RECEIVED DATA");
                    dataChk = (DataChunk) chk;
                    break;
                case Chunk.CHUNK_TYPE_HEARTBEAT:
                    assert (chk instanceof HeartbeatChunk);
                    // do nothing
                    break;
                case Chunk.CHUNK_TYPE_SACK:
                    assert (chk instanceof SACKChunk);
                    // do nothing
                    break;
                case Chunk.CHUNK_TYPE_CANCELLED_PACKETS:
                    assert (chk instanceof CancelledPacketsChunk);
                    // do nothing
                    break;
                /*
                // these chunk types are handled by MessageReceiver
                case Chunk.CHUNK_TYPE_INIT:
                case Chunk.CHUNK_TYPE_INIT_ACK:
                case Chunk.CHUNK_TYPE_COOKIE_ECHO:
                case Chunk.CHUNK_TYPE_COOKIE_ACK:
                */
                default:
                    _logger.fine ("Received unexpected chunk.");
            }
        }

        // this must be done at last to avoid race conditions on buffer
        // release, because in the received data message queue we put 
        // the buffer containing ALL of the message packet 
        if (foundDataChunk) {
            //_logger.fine ("ABOUT TO INSERT DATA IN RECEIVED PACKET QUEUE - 1");
            DataBuffer p = dataChk.getDataBuffer();
            //_logger.fine ("ABOUT TO INSERT DATA IN RECEIVED PACKET QUEUE - 1");
            _packetQueue.insert (p);
            //_logger.fine ("INSERTED DATA IN RECEIVED PACKET QUEUE - 1");
            _receiver.increaseCurrentWindowSize (pIn.getSizeWithoutSACKChunk() - p.getSize());
            //_logger.fine ("INCREASED CURRENT WINDOW SIZE");
        } else {
            _receiver.increaseCurrentWindowSize (pIn.getSizeWithoutSACKChunk());
        }
    }
    
    void processFragments (Enumeration e)
    {
        Vector dataBuffers = new Vector();

        while (e.hasMoreElements()) {
            MessagePacket pIn = (MessagePacket) e.nextElement();
            boolean foundStateChangeChunk = false;
            boolean foundDataChunk = false;
            DataChunk dataChk = null;
            
            for (Chunk chk = pIn.getFirstChunk(); chk != null; chk = pIn.getNextChunk()) {
                int chkType = chk.getType();
                
                // quick check to verify that there is no more than one state change 
                // chunk in the same packet. maybe we'll have to add more self consistency 
                // checks later...
                if ((chkType & Chunk.CHUNK_CLASS_STATECHANGE) != 0) {
                    if (foundStateChangeChunk)
                        // ok, throwing an exception here is maybe too much, but we want 
                        // to catch errors quickly during the first development phases
                        throw new RuntimeException ("Can't have more than one state " + 
                                                    "change chunk in the same packet!");
                    foundStateChangeChunk = true;
                }
                
                if ((chkType & Chunk.CHUNK_CLASS_DATA) != 0) {
                    if (foundDataChunk)
                        // ok, throwing an exception here is maybe too much, but we want 
                        // to catch errors quickly during the first development phases
                        throw new RuntimeException ("Can't have more than one data " + 
                                                    "chunk in the same packet!");
                    foundDataChunk = true;
                }
                
                // dispatch chunk to the appropriate processing method
                switch (chk.getType()) {
                    case Chunk.CHUNK_TYPE_SHUTDOWN:
                        assert (chk instanceof ShutdownChunk);
                        //_logger.fine ("RECEIVED SHUTDOWN IN STATE " + _status.currentStateAsString());
                        _status.receivedShutdown();
                        break;
                    case Chunk.CHUNK_TYPE_SHUTDOWN_ACK:
                        assert (chk instanceof ShutdownAckChunk);
                        //_logger.fine ("RECEIVED SHUTDOWN-ACK IN STATE " + _status.currentStateAsString());
                        _status.receivedShutdownAck();
                        break;
                    case Chunk.CHUNK_TYPE_SHUTDOWN_COMPLETE:
                        //_logger.fine ("RECEIVED SHUTDOWN-COMPLETE IN STATE " + _status.currentStateAsString());
                        assert (chk instanceof ShutdownCompleteChunk);
                        _status.receivedShutdownComplete();
                        break;
                    case Chunk.CHUNK_TYPE_ABORT:
                        assert (chk instanceof AbortChunk);
                        // abort is not supported at the moment
                        break;
                    case Chunk.CHUNK_TYPE_DATA:
                        assert (chk instanceof DataChunk);
                        _logger.fine ("RECEIVED DATA");
                        dataChk = (DataChunk) chk;
                        break;
                    case Chunk.CHUNK_TYPE_HEARTBEAT:
                        assert (chk instanceof HeartbeatChunk);
                        // do nothing
                        break;
                    case Chunk.CHUNK_TYPE_SACK:
                        assert (chk instanceof SACKChunk);
                        // do nothing
                        break;
                    case Chunk.CHUNK_TYPE_CANCELLED_PACKETS:
                        assert (chk instanceof CancelledPacketsChunk);
                        // do nothing
                        break;
                    /*
                    // these chunk types are handled by MessageReceiver
                    case Chunk.CHUNK_TYPE_INIT:
                    case Chunk.CHUNK_TYPE_INIT_ACK:
                    case Chunk.CHUNK_TYPE_COOKIE_ECHO:
                    case Chunk.CHUNK_TYPE_COOKIE_ACK:
                    */
                    default:
                        _logger.fine ("Received unexpected chunk.");
                }
            }

            // this must be done at last to avoid race conditions on buffer
            // release, because in the received data message queue we put 
            // the buffer containing ALL of the message packet 
            if (foundDataChunk) {
                _logger.fine ("INSERT DATA IN RECEIVED PACKET QUEUE");
                DataBuffer p = dataChk.getDataBuffer();
                dataBuffers.add (p);
                _receiver.increaseCurrentWindowSize (pIn.getSizeWithoutSACKChunk() - p.getSize());
            } else {
                _receiver.increaseCurrentWindowSize (pIn.getSizeWithoutSACKChunk());
            }
        }

        if (dataBuffers.size() > 0)
            _packetQueue.insert (dataBuffers);
    }
    
    /*
    private long tryToProcessFirstPacketFromSequencedQueue (SequencedPacketQueue queue, 
                                                            long nextExpectedTSN)
    {
        MessagePacketInfo pi = queue.peek();
        // if the queue is empty, bail out
        if (pi == null)
            return nextExpectedTSN;

        long sn = pi.getSequenceNumber();

        _logger.fine ("PEEKING PACKET QUEUE, sn = " + sn + " nextExpectedTSN = " + 
                         nextExpectedTSN);
        
        // process next packet to be delivered or the first unreliable packet 
        // with an expired timeout 
        if (sn == nextExpectedTSN) {
            MessagePacket p = pi.getPacket();
            if (p == null || 
                (p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT |
                             MessagePacket.HEADER_FLAG_MORE_FRAGMENTS) &&
                 !p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT))) {
                // this is a cancelled packet 
                // or a fragment from packet whose first fragment was cancelled
                _logger.fine (p == null ? "null packet" : "cancelled/invalid packet");
                queue.remove (pi);
                nextExpectedTSN = SequentialArithmetic.add (sn, 1);
                queue.resetNextExpectedTSN (nextExpectedTSN);
            } else {
                // try to see if we satisfy delivery prerequisites
                DeliveryPrerequisites dp = p.getDeliveryPrerequisites();
                if (dp != null && !dp.isSatisfied(_nextControlTSN, 
                                                  _nextRelSeqTSN, 
                                                  _nextUnrelSeqTSN)) {
                    _logger.fine ("unsatisfied delivery prerequisites");
                    return nextExpectedTSN;
                }
                    
                if (p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT)) {
                    assert !p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                    assert p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
                    
                    Vector pVec = new Vector();
                    Vector mpiVec = new Vector();
                    long neTSN = nextExpectedTSN;
                    long first = nextExpectedTSN;
                    int packetsRead = 0;
                    
                    synchronized (queue) {
                        Iterator it = queue.iterator();

                        while (true) {
                            MessagePacketInfo tmpi = null;
                            try {
                                tmpi = (MessagePacketInfo)it.next();
                            }
                            catch (NoSuchElementException e) {} // intentionally left blank
                            assert packetsRead != 0 || (tmpi != null);
                            if (tmpi == null) {
                                // just exit, since we haven't received LF yet
                                _logger.fine ("tmpi == null");
                                break;
                            }
                
                            MessagePacket tmp = tmpi.getPacket();
                            assert packetsRead != 0 || (tmp == p);
                            if (tmp == null) {
                                // cancelled packet, delete everything we have received, 
                                // update nextExpectedTSN and exit
                                _logger.fine ("tmp == null");
                                for (Enumeration e = mpiVec.elements(); e.hasMoreElements();) {
                                    queue.remove ((MessagePacketInfo)e.nextElement());
                                }
                                nextExpectedTSN = SequentialArithmetic.add (
                                                      tmpi.getSequenceNumber(), 1);
                                queue.resetNextExpectedTSN (nextExpectedTSN);
                                break;
                            }
                            
                            if (tmp.getSequenceNumber() == neTSN) {
                                _logger.fine ("next fragment found");
                                assert tmp.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                                     MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                                mpiVec.add (tmpi);
                                pVec.add (tmp);
                                
                                if (tmp.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT))  {
                                    _logger.fine ("recomposing packet from TSN " + first +
                                                  " to TSN " + neTSN);
                                    // add packets to buffer queue and exit
                                    for (Enumeration e = mpiVec.elements(); e.hasMoreElements();) {
                                        queue.remove ((MessagePacketInfo)e.nextElement());
                                    }
                                    processFragments (pVec.elements());
                                    nextExpectedTSN = SequentialArithmetic.add (neTSN, 1);
                                    queue.resetNextExpectedTSN (nextExpectedTSN);
                                    break;
                                }
                                
                                ++packetsRead;
                                neTSN = SequentialArithmetic.add (neTSN, 1);
                            } else {
                                // just exit, since we haven't received LF yet
                                _logger.fine ("tmp.getSequenceNumber() != neTSN");
                                break;
                            }
                        }
                    }
                } else {
                    _logger.fine ("full packet found");
                    queue.remove (pi);
                    if (!p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                     MessagePacket.HEADER_FLAG_LAST_FRAGMENT)) 
                        processPacket (p);
                    nextExpectedTSN = SequentialArithmetic.add (sn, 1);
                    queue.resetNextExpectedTSN (nextExpectedTSN);
                }
            }
        }

        // returns original nextExpectedTSN if no packet was processed 
        // or last processed TSN + 1 if one or more packets have been processed
        return nextExpectedTSN;
    }
    */
    
    // private access motifier removed and performProcessing parameter added 
    // for testing purposes only
    long tryToProcessFirstPacketFromSequencedQueue (SequencedPacketQueue queue, 
                                                    long nextExpectedTSN, 
                                                    boolean performProcessing)
    {
        assert queue.getNextExpectedTSN() == nextExpectedTSN;

        MessagePacketInfo pi = queue.peek();
        // if the queue is empty, bail out
        if (pi == null)
            return nextExpectedTSN;

        assert pi.isSequenced() || pi.isControl();

        boolean isReliable = pi.isReliable() || pi.isControl();
        boolean ready = false;
        boolean unreliableTimeout = false;
        long sn = pi.getPacket().getSequenceNumber();

        /*
        _logger.fine ("PEEKING PACKET QUEUE, sn = " + sn + " nextExpectedTSN = " + 
                      nextExpectedTSN);
        if (!isReliable) {
            _logger.fine ("_nextUnrelSeqTSN: " + _nextUnrelSeqTSN);
            _logger.fine ("min ((sn - _nextUnrelSeqTSN), 5): " + 
                    Math.min(SequentialArithmetic.subtract(sn, _nextUnrelSeqTSN), 5));
            _logger.fine ("now - lastIOOperationTime: " + 
                    (System.currentTimeMillis() - pi.getLastIOOperationTime()));
        }
        */
        
        if (sn == nextExpectedTSN ||
            (!isReliable &&
             System.currentTimeMillis() - pi.getLastIOOperationTime() >= 
               UNRELIABLE_DELIVERY_TIMEOUT * 
                 Math.min(SequentialArithmetic.subtract(sn, nextExpectedTSN), 5))) {
            
            /*
            // TODO: if unreliable, update nextExpectedTSN 
            // and call queue.resetNextExpectedTSN(nextExpectedTSN)
            if (!isReliable) {
                nextExpectedTSN = sn;
                queue.resetNextExpectedTSN (sn);
            }
            */
            
            _logger.fine ("processing entry " + pi.getPacket().getSequenceNumber());
            
            assert sn >= nextExpectedTSN;
            
            MessagePacket p = pi.getPacket();
            if (p == null ||
                (p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT |
                             MessagePacket.HEADER_FLAG_MORE_FRAGMENTS) &
                 !p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT))) {
                // this is a cancelled packet 
                // or a fragment from packet whose first fragment was cancelled
                _logger.fine ((p == null ? "null packet " : "cancelled/invalid packet ") + sn);
                queue.remove (pi);
                nextExpectedTSN = SequentialArithmetic.add (sn, 1);
                queue.resetNextExpectedTSN (nextExpectedTSN);
            } else {
                // try to see if we satisfy delivery prerequisites
                DeliveryPrerequisites dp = p.getDeliveryPrerequisites();
                if (dp != null) {
                    long TSN1 = 0;
                    long TSN2 = 0;
                    
                    if (pi.isControl()) {
                        TSN1 = _nextRelSeqTSN;
                        TSN2 = _nextUnrelSeqTSN;
                    } else if (pi.isReliable()) {
                        TSN1 = _nextControlTSN;
                        TSN2 = _nextUnrelSeqTSN;
                    } else {
                        TSN1 = _nextControlTSN;
                        TSN2 = _nextRelSeqTSN;
                    }
                    
                    if (!dp.isSatisfied(TSN1, TSN2)) {
                        _logger.fine ("unsatisfied delivery prerequisites " + sn);
                        return nextExpectedTSN;
                    }
                }

                if (p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT)) {
                    assert !p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                    assert p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);

                    Vector pVec = new Vector();
                    Vector mpiVec = new Vector();
                    long neTSN = sn;
                    long first = nextExpectedTSN;
                    int packetsRead = 0;
                    long lastReceivedTime = System.currentTimeMillis();
                    long lastReceivedTSN = sn;
                    
                    synchronized (queue) {
                        _logger.fine ("processing entry - " + neTSN);
                        Iterator it = queue.iterator();

                        while (true) {
                            _logger.fine ("1");
                            MessagePacketInfo tmpi = null;
                            try {
                                tmpi = (MessagePacketInfo)it.next();
                            }
                            catch (NoSuchElementException e) {} // intentionally left blank
                            assert packetsRead != 0 || (tmpi != null);
                            if (tmpi == null) {
                                _logger.fine ("tmpi == null " + neTSN);
                                
                                if (!isReliable &&
                                    System.currentTimeMillis() - lastReceivedTime >
                                    (neTSN - lastReceivedTSN) * UNRELIABLE_DELIVERY_TIMEOUT) {
                                    
                                    _logger.fine ("timeout on " + neTSN);
                                    
                                    // timeout - delete everything we have received and exit
                                    for (Enumeration e = mpiVec.elements(); 
                                         e.hasMoreElements(); ) {
                                        queue.remove ((MessagePacketInfo)e.nextElement());
                                    }
                                    
                                    nextExpectedTSN = neTSN;
                                    queue.resetNextExpectedTSN (nextExpectedTSN);
                                }
                                
                                // do nothing, we haven't received LF yet
                                break;
                            }
                
                            _logger.fine ("2");
                            MessagePacket tmp = tmpi.getPacket();
                            if (tmp == null) {
                                // cancelled fragment, delete everything we have received 
                                // and exit
                                for (Enumeration e = mpiVec.elements(); e.hasMoreElements();) {
                                    queue.remove ((MessagePacketInfo)e.nextElement());
                                }
                                
                                nextExpectedTSN = SequentialArithmetic.add
                                                      (tmpi.getPacket().getSequenceNumber(), 1);
                                queue.resetNextExpectedTSN (nextExpectedTSN);
                                
                                break;
                            }
                            
                            assert packetsRead != 0 || (tmp == p);
                            
                            _logger.fine ("3");
                            if (tmp.getSequenceNumber() == neTSN) {
                                _logger.fine ("4");
                                assert tmp.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                                     MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                                mpiVec.add (tmpi);
                                pVec.add (tmp);
                                ++packetsRead;
                                neTSN = SequentialArithmetic.add (neTSN, 1);
                                lastReceivedTime = tmpi.getLastIOOperationTime();
                                lastReceivedTSN = tmpi.getPacket().getSequenceNumber();

                                if (tmp.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT))  {
                                    _logger.fine ("4a");
                                    // add packets to buffer queue and exit                                        
                                    for (Enumeration e = mpiVec.elements(); 
                                         e.hasMoreElements(); ) {
                                        queue.remove ((MessagePacketInfo)e.nextElement());
                                    }
                                    
                                    if (performProcessing)
                                        processFragments (pVec.elements());
                                    
                                    nextExpectedTSN = neTSN;
                                    queue.resetNextExpectedTSN (nextExpectedTSN);

                                    break;
                                }
                            } else {
                                _logger.fine ("5");
                                _logger.fine ("neTSN: " + neTSN + " lastReceivedTSN: " +
                                                 lastReceivedTSN);
                                _logger.fine ("System.currentTimeMillis() - " + 
                                                 "lastReceivedTime: " + 
                                                 (System.currentTimeMillis() -
                                                  lastReceivedTime));
                                
                                if (!isReliable &&
                                    System.currentTimeMillis() - lastReceivedTime >
                                    (neTSN - lastReceivedTSN) * UNRELIABLE_DELIVERY_TIMEOUT) {
                                    // timeout - delete everything we have received and exit
                                    _logger.fine ("5a");
                                    for (Enumeration e = mpiVec.elements(); 
                                         e.hasMoreElements(); ) {
                                        queue.remove ((MessagePacketInfo)e.nextElement());
                                    }
                                    nextExpectedTSN = neTSN;
                                    queue.resetNextExpectedTSN (nextExpectedTSN);
                                }

                                // do nothing, we haven't received LF yet
                                break;
                            }
                        }
                    }
                } else {
                    _logger.fine ("processing packet " + pi.getPacket().getSequenceNumber());
                    
                    queue.remove (pi);
                    
                    _logger.fine ("packet " + pi.getPacket().getSequenceNumber() + " removed");
                    
                    if (performProcessing &&
                        !p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                     MessagePacket.HEADER_FLAG_LAST_FRAGMENT)) 
                        processPacket (p);
                    
                    _logger.fine ("Resetting Next Expected TSN...");
                    nextExpectedTSN = SequentialArithmetic.add (sn, 1);
                    queue.resetNextExpectedTSN (nextExpectedTSN);
                    _logger.fine ("Next Expected TSN Reset!!!");
                }
            }
        }
        
        // returns original nextExpectedTSN if no packet was processed 
        // or last processed TSN + 1 if one or more packets have been processed
        return nextExpectedTSN;
    }

    // private access motifier removed and performProcessing parameter added 
    // for testing purposes only
    /*
     * returns true if recomposition was successful, false otherwise
     */
    boolean tryToDefragmentPacketFromUnsequencedQueue (UnsequencedPacketQueue queue, 
                                                       boolean performProcessing)
    {
        MessagePacketInfo pi = queue.peek();
        // if the queue is empty, bail out
        if (pi == null)
            return false;

        _logger.fine ("0a");
        assert !pi.isSequenced() && !pi.isControl();

        boolean isReliable = pi.isReliable();
        
        if (!isReliable &&
            System.currentTimeMillis() - queue.getTimestamp() > 
                UNSEQUENCED_RECOMPOSITION_TIMEOUT) {
            queue.removeAll();
            return false;
        }
        _logger.fine ("0b");

        boolean ret = false;
        
        synchronized (queue) {            
            Iterator it = queue.iterator();
            
            while (it.hasNext()) {
                MessagePacketInfo mpi = (MessagePacketInfo)it.next();
                MessagePacket p = mpi.getPacket();
                
                _logger.fine ("0c");
                
                if (p.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT)) {
                    // try to recompose packet
                    assert !p.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                    assert p.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);

                    _logger.fine ("0d");
                
                    Vector mpiVec = new Vector();
                    mpiVec.add (mpi);
                    Vector pVec = new Vector();
                    pVec.add (p);
                    long first = p.getSequenceNumber();
                    long neTSN = SequentialArithmetic.add (first, 1);
                    int packetsRead = 1;
                    long lastReceivedTime = System.currentTimeMillis();
                    long lastReceivedTSN = first;

                    while (true) {
                        _logger.fine ("processing entry - " + neTSN + ", first: " + first);
                        _logger.fine ("1");
                        MessagePacketInfo tmpi = null;
                        try {
                            tmpi = (MessagePacketInfo)it.next();
                        }
                        catch (NoSuchElementException e) {} // intentionally left blank
                        assert packetsRead != 0 || (tmpi != null);
                        if (tmpi == null) {
                            _logger.fine ("tmpi == null " + neTSN);
                            
                            if (!isReliable &&
                                System.currentTimeMillis() - lastReceivedTime >
                                (neTSN - lastReceivedTSN) * UNRELIABLE_DELIVERY_TIMEOUT) {
                                
                                _logger.fine ("timeout on " + neTSN);
                                
                                // timeout - delete everything we have received and exit
                                for (Enumeration e = mpiVec.elements(); 
                                     e.hasMoreElements(); ) {
                                    queue.remove ((MessagePacketInfo)e.nextElement());
                                }
                            }
                            
                            // do nothing, we haven't received LF yet
                            break;
                        }
            
                        _logger.fine ("2");
                        MessagePacket tmp = tmpi.getPacket();
                        if (tmp == null) {
                            // cancelled fragment, delete everything we have received 
                            // and exit
                            for (Enumeration e = mpiVec.elements(); e.hasMoreElements();) {
                                queue.remove ((MessagePacketInfo)e.nextElement());
                            }
                            
                            break;
                        }
                        
                        assert packetsRead != 0 || (tmp == p);
                        
                        _logger.fine ("3");
                        _logger.fine ("tmp.getSequenceNumber() == " + tmp.getSequenceNumber());
                        if (tmp.getSequenceNumber() == neTSN) {
                            _logger.fine ("4");
                            assert tmp.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                                 MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                            mpiVec.add (tmpi);
                            pVec.add (tmp);
                            ++packetsRead;
                            neTSN = SequentialArithmetic.add (neTSN, 1);
                            lastReceivedTime = tmpi.getLastIOOperationTime();
                            lastReceivedTSN = tmpi.getPacket().getSequenceNumber();

                            if (tmp.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT))  {
                                _logger.fine ("4a");
                                // add packets to buffer queue and exit                                        
                                for (Enumeration e = mpiVec.elements(); 
                                     e.hasMoreElements(); ) {
                                    queue.remove ((MessagePacketInfo)e.nextElement());
                                }
                                
                                if (performProcessing)
                                    processFragments (pVec.elements());
                                
                                ret = true;
                                break;
                            }
                        } else {
                            _logger.fine ("5");
                            _logger.fine ("neTSN: " + neTSN + " lastReceivedTSN: " +
                                          lastReceivedTSN);
                            _logger.fine ("System.currentTimeMillis() - " + 
                                          "lastReceivedTime: " + 
                                          (System.currentTimeMillis() -
                                           lastReceivedTime));
                            
                            if (!isReliable &&
                                System.currentTimeMillis() - lastReceivedTime >
                                (neTSN - lastReceivedTSN) * UNRELIABLE_DELIVERY_TIMEOUT) {
                                // timeout - delete everything we have received and exit
                                _logger.fine ("5a");
                                for (Enumeration e = mpiVec.elements(); 
                                     e.hasMoreElements(); ) {
                                    queue.remove ((MessagePacketInfo)e.nextElement());
                                }
                            }

                            // do nothing, we haven't received LF yet
                            break;
                        }
                    }
                } else if (!isReliable && 
                           System.currentTimeMillis() - mpi.getLastIOOperationTime() >
                               UNRELIABLE_DELIVERY_TIMEOUT) {
                    _logger.fine ("removing packet " + p.getSequenceNumber());
                    queue.remove (mpi);
                }
            }            
        }
        
        return ret;
    }
    
    public void run()
    {
        boolean done = false;

        _logger.fine ("MessagePacketProcessor Thread for Mocket " + 
                      _mocket.getUID() + " starting.");
        
        synchronized (this) {
            while (!done) {
                boolean delivered = false;
                
                //_logger.fine ("PP1");
                long originalTSN = _nextControlTSN;
                _nextControlTSN = tryToProcessFirstPacketFromSequencedQueue 
                                      (_controlPacketQueue, _nextControlTSN, true);
                if (originalTSN != _nextControlTSN) {
                    _logger.fine ("_nextControlTSN " + originalTSN + 
                                  " -> " + _nextControlTSN);
                    delivered = true;
                }
                
                //_logger.fine ("PP2");
                originalTSN = _nextRelSeqTSN;
                _nextRelSeqTSN = tryToProcessFirstPacketFromSequencedQueue 
                                     (_relSeqPacketQueue, _nextRelSeqTSN, true);
                if (originalTSN != _nextRelSeqTSN) {
                    _logger.fine ("_nextRelSeqTSN " + originalTSN + 
                                  " -> " + _nextRelSeqTSN);
                    delivered = true;
                }
                
                //_logger.fine ("PP3");
                originalTSN = _nextUnrelSeqTSN;
                _nextUnrelSeqTSN = tryToProcessFirstPacketFromSequencedQueue 
                                       (_unrelSeqPacketQueue, _nextUnrelSeqTSN, true);
                if (originalTSN != _nextUnrelSeqTSN) {
                    _logger.fine ("_nextUnrelSeqTSN " + originalTSN + 
                                  " -> " + _nextUnrelSeqTSN);
                    delivered = true;
                }

                //_logger.fine ("PP4");
                if (!delivered) {
                    try {
                        wait (PACKET_PROCESSOR_SLEEP_TIMEOUT);
                    }
                    catch (InterruptedException e) {} // intentionally left blank
                }
                if (_status.currentState() == MessageStateMachine.CLOSED)
                    done = true;

                //_logger.fine ("PP5");
            }
            notifyAll();
        }

        _logger.fine ("MessagePacketProcessor Thread for Mocket " + 
                      _mocket.getUID() + " terminating.");
    }
    
    synchronized void packetArrived() 
    {
        notifyAll();
    }

    
    private static int UNRELIABLE_DELIVERY_TIMEOUT = 3000; // delay is 3 seconds
    private static int PACKET_PROCESSOR_SLEEP_TIMEOUT = 3000;
    private static final long UNSEQUENCED_RECOMPOSITION_TIMEOUT = 15000L; // 15 seconds

    private MessageMocket _mocket;
    private PacketQueue _packetQueue;
    private SequencedPacketQueue _controlPacketQueue;
    private SequencedPacketQueue _relSeqPacketQueue;
    private SequencedPacketQueue _unrelSeqPacketQueue;
    private UnsequencedPacketQueue _relUnseqPacketQueue;
    private UnsequencedPacketQueue _unrelUnseqPacketQueue;
    private ACKManager _ackManager;
    private MessageStateMachine _status;
    private MessageReceiver _receiver;
    private long _nextControlTSN;
    private long _nextRelSeqTSN;
    private long _nextUnrelSeqTSN;

    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */

