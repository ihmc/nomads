/**
 * The MessageTransmitter class implements the functionality of sending 
 * packets and managing the sliding window for the transmission side
 * of a MessageMocket.
 *
 * @author Mauro Tortonesi
 */
package us.ihmc.mockets;

import java.io.IOException;

import java.lang.Long;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetSocketAddress;
import java.net.InetAddress;
import java.net.SocketException;

import java.util.logging.Level;
import java.util.logging.Logger;

class MessageTransmitter extends Thread
{
    MessageTransmitter (MessageMocket mocket, int remoteWindowSize)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;
        _dgSocket = _mocket.getSocket();
        
        InetSocketAddress tmp = null;
        try {
            tmp = (InetSocketAddress) _mocket.getPeerName();
        }
        catch (SocketException e) {
            e.printStackTrace();
            System.exit (1);
        }        
        _remoteAddress = tmp.getAddress();
        _remotePort = tmp.getPort();
        
        _receivedPacketQueue = _mocket.getReceivedPacketQueue();
        _pendingPacketQueue = _mocket.getPendingPacketQueue();
        _outstandingPacketQueue = _mocket.getOutstandingPacketQueue();
        _ackManager = _mocket.getACKManager();
        _remoteWindowSize = remoteWindowSize;

        _validation = _mocket.getOutgoingValidation();
        _relSeqTSN = _mocket.getInitialRelSeqTSN();
        _unrelSeqTSN = _mocket.getInitialUnrelSeqTSN();
        _controlTSN = _mocket.getInitialControlTSN();
        _relUnseqID = _mocket.getInitialRelUnseqID();
        _unrelUnseqID = _mocket.getInitialUnrelUnseqID();
        _useCrossSequencing = _mocket.isCrossSequencingEnabled();
        _status = _mocket.getStateMachine();
        
        _outgoingPacket = new DatagramPacket (new byte[0], 0);        

        _lastTransmitTime = 0;
        _lastSACKTransmitTime = 0;
        _sendSACKInformationNow = false;
        
        _shutdownSent = 0;
        _shutdownAckSent = 0;

        _unacknowledgedPackets = 0;

        setName ("MessageTransmitter Thread for Mocket " + _mocket.getUID());
        // mauro: why 8? is there any special reason to choose 8 instead of e.g. 7 or 9?
        setPriority (8);
    }

    // no need to synchronize this...
    void setReceiver (MessageReceiver receiver) 
    {
        _receiver = receiver;
    }

    // no need to synchronize this...
    void requestSACKTransmission()
    {
        _sendSACKInformationNow = true;
    }
    
    void packetReceived (int remoteWindowSize) 
    {
        //_logger.info ("packetReceived (" + remoteWindowSize + ")");
        _receivedPacket = true;
        _remoteWindowSize = remoteWindowSize;
    }
    
    void send (boolean reliable, boolean sequenced, 
               byte[] buf, int off, int len, int tag, int priority,
               long enqueue_timeout, long retry_timeout) 
        throws IOException 
    {
        /* we accept data only if the connection is not closed */
        if (_status.currentState() != MessageStateMachine.ESTABLISHED)
            throw new IOException ("Connection closed!");
        
        /* this is the space we can use for data in each packet 
         * (provided we have only a single DATA chunk in each packet) */
        int availSize = _mocket.getMTU() - (MessagePacket.HEADER_SIZE +
                                            DataChunk.DATA_CHUNK_HEADER_SIZE);

        if (_mocket.isCrossSequencingEnabled())
            availSize -= DeliveryPrerequisites.DELIVERY_PREREQUISITES_SIZE;
        
        _logger.fine ("MessageTransmitter::send " +
                      (reliable? "R ": "") +
                      (sequenced? "S ": "") +
                      "len = " + len + " tag = " + tag);

        int toSend = len;
        int sent = 0;
        int fragmentNum = 0;
        boolean fragmentationNeeded = len > availSize;

        if (fragmentationNeeded)
            _logger.info ("fragmentation needed!!!");
        
        // build and enqueue message packet(s)
        while (toSend > 0) {
            int cc = Math.min (toSend, availSize);
            
            MessagePacket p = new MessagePacket();
            if (reliable) 
                p.setFlags (MessagePacket.HEADER_FLAG_RELIABLE);
            if (sequenced)
                p.setFlags (MessagePacket.HEADER_FLAG_SEQUENCED);
            
            if (fragmentationNeeded)
                p.allocateSpaceForDeliveryPrerequisites();
            
            DataChunk chk = new DataChunk (tag, buf, off + sent, cc);

            if (!p.addChunk(chk)) {
                // this should never happen
                throw new IOException ("Buffer size too big");
            }
            
            toSend -= cc;

            if (fragmentationNeeded) {
                assert toSend >= 0;
                if (toSend == 0) {
                    p.setFlags (MessagePacket.HEADER_FLAG_LAST_FRAGMENT);
                }
                else {
                    if (fragmentNum == 0) {
                        p.setFlags (MessagePacket.HEADER_FLAG_FIRST_FRAGMENT);
                    }
                    p.setFlags (MessagePacket.HEADER_FLAG_MORE_FRAGMENTS);
                }
            }

            // enqueue message packet
            if (!enqueueMessagePacket (p, priority, enqueue_timeout, retry_timeout)) {
                throw new IOException ("Pending packet queue is full.");
            }
                
            ++fragmentNum;
            sent += cc;
        }

        transmitNextPacket();
                    
        // wake up transmitter thread
        synchronized (this) {
            notifyAll();
        }
    }
    
    void cancel (boolean sequenced, int oldtag)
    {
        // cancel stuff from the pending packet queue
        _pendingPacketQueue.cancel (sequenced, oldtag);
        
        // cancel stuff from the outstanding packet queue
        TSNRangeHandler rh = new TSNRangeHandler (false, sequenced);
        _outstandingPacketQueue.cancel (sequenced, oldtag, rh);
        ACKInformation ai = new ACKInformation();
        rh.fillACKInformation (ai);
        
        // send control packet containing cancelled packets chunk
        MessagePacket p = new MessagePacket();
        p.setFlags (MessagePacket.HEADER_FLAG_CONTROL);        
        CancelledPacketsChunk cpChk = new CancelledPacketsChunk (ai);
        p.addChunk (cpChk);
        MessagePacketInfo pi = new MessagePacketInfo (p, TxParams.MAX_PRIORITY, 0, 
                                                      0, MessageMocket.DEFAULT_RETRANSMISSION_TIMEOUT);
        transmitPacket (pi);
    }
    
    void replace (boolean sequenced, byte[] buf, int off, int len, 
                  int oldtag, int newtag, int priority,
                  long enqueue_timeout, long retry_timeout) 
        throws IOException 
    {
        cancel (sequenced, oldtag);
        send (true, sequenced, buf, off, len, newtag, priority,
              enqueue_timeout, retry_timeout);
    }
    
    private boolean enqueueMessagePacket (MessagePacket p, int priority,
                                          long enqueue_timeout, long retry_timeout) 
    {
        // last_tx_time = 0 means that the packet has not been sent yet
        MessagePacketInfo pi = new MessagePacketInfo (p, priority, retry_timeout, 
                                                      0, MessageMocket.DEFAULT_RETRANSMISSION_TIMEOUT);
        
        return _pendingPacketQueue.insert (pi, enqueue_timeout);
    }
   
    /* 
     * This method tries to append a SACK chunk to a MessagePacket. 
     * Returns true if the operation succeeds, false elsewhere.
     */
    private boolean appendSACKData (MessagePacket p, boolean force)
    {
        /*
        if (force ||
            System.currentTimeMillis() > _lastSACKTransmitTime + SACK_XMIT_MIN_INTERVAL) {
        */
            SACKInformation sackInfo = _ackManager.getSACKInformation();
            assert sackInfo != null;

            //sackInfo._dump (System.out);
            
            //_logger.warning ("Created SACKInfo size: " + sackInfo.getLength());
            SACKChunk chk = new SACKChunk (sackInfo);
            //_logger.warning ("Created SACKChunk size: " + chk.getLength());
            assert chk != null;
         
            return p.setSACKChunk (chk);
        /*
        }

        return false;
        */
    }
    
    private void transmitPacket (MessagePacketInfo mpi)
    {
        MessagePacket p = mpi.getPacket();
        transmitMessagePacket (p, true);
        mpi.setLastIOOperationTime (System.currentTimeMillis());
    }
    
    private void transmitMessagePacket (MessagePacket p, boolean tryToAppendACKInfo)
    {
        // set packet sequence number
        long seqNum = 0;
        if (p.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL)) {
            assert !p.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE) && 
                   !p.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED);
            seqNum = _controlTSN;
            _controlTSN = SequentialArithmetic.add (_controlTSN, 1);
        } else if (p.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED)) {
            if (p.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)) {
                seqNum = _relSeqTSN;
                _relSeqTSN = SequentialArithmetic.add (_relSeqTSN, 1);
            } else {
                seqNum = _unrelSeqTSN;
                _unrelSeqTSN = SequentialArithmetic.add (_unrelSeqTSN, 1);
            }    
        } else {
            if (p.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)) {
                seqNum = _relUnseqID;
                _relUnseqID = SequentialArithmetic.add (_relUnseqID, 1);
            } else {
                seqNum = _unrelUnseqID;
                _unrelUnseqID = SequentialArithmetic.add (_unrelUnseqID, 1);
            }    
        }        
        p.setSequenceNumber (seqNum);
        
        transmitDatagram (p, tryToAppendACKInfo);
    }

    private void transmitDatagram (MessagePacket p, boolean tryToAppendACKInfo)
    {
        // append SACK chunk if we can
        if (tryToAppendACKInfo && appendSACKData(p, false)) {
            _lastSACKTransmitTime = System.currentTimeMillis();
            _unacknowledgedPackets = 0;
        } 
        
        // fill in outgoing datagram packet
        p.setWindowSize (_receiver.getCurrentWindowSize());
        p.setValidation (_validation);
        p.writeToDatagramPacket (_outgoingPacket);

        _outgoingPacket.setAddress (_remoteAddress);
        _outgoingPacket.setPort (_remotePort);
        
        // send packet over the socket
        try {
            _dgSocket.send (_outgoingPacket);
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        
        // update last transmission time
        _lastTransmitTime = System.currentTimeMillis();
        
        Thread.yield();
    }
    
    private void transmitSACKPacket()
    {
        MessagePacket p = new MessagePacket();
        
        // append SACK chunk
        if (!appendSACKData (p, true))
            // this should never happen
            throw new RuntimeException ("Impossible to add SACK chunk to an empty packet.");
            
        _lastSACKTransmitTime = System.currentTimeMillis();
        _unacknowledgedPackets = 0;
        
        transmitMessagePacket (p, false);
    }
    
    private void transmitHeartbeatPacket()
    {
        MessagePacket p = new MessagePacket();
        
        HeartbeatInfo hbInfo = new HeartbeatInfo (System.currentTimeMillis());
        HeartbeatChunk hbChk = new HeartbeatChunk (hbInfo);

        // append HEARTBEAT chunk
        if (!p.addChunk(hbChk)) {
            // this should never happen
            throw new RuntimeException ("Impossible to add HEARTBEAT chunk to an empty packet.");
        }

        transmitMessagePacket (p, true);
    }

    private void transmitShutdownPacket()
    {
        MessagePacket p = new MessagePacket();

        ShutdownChunk shChk = new ShutdownChunk();

        // append SHUTDOWN chunk
        if (!p.addChunk(shChk)) {
            // this should never happen
            throw new RuntimeException ("Impossible to add SHUTDOWN chunk to an empty packet.");
        }
        
        transmitMessagePacket (p, true);
    }
    
    private void transmitShutdownAckPacket()
    {
        MessagePacket p = new MessagePacket();

        ShutdownAckChunk saChk = new ShutdownAckChunk();

        // append SHUTDOWN-ACK chunk
        if (!p.addChunk(saChk)) {
            // this should never happen
            throw new RuntimeException ("Impossible to add SHUTDOWN-ACK chunk to an empty packet.");
        }
        
        transmitMessagePacket (p, true);
    }
    
    private void transmitShutdownCompletePacket()
    {
        MessagePacket p = new MessagePacket();

        ShutdownCompleteChunk scChk = new ShutdownCompleteChunk();

        // append SHUTDOWN-COMPLETE chunk
        if (!p.addChunk(scChk)) {
            // this should never happen
            throw new RuntimeException ("Impossible to add SHUTDOWN-COMPLETE chunk to an empty packet.");
        }
        
        transmitMessagePacket (p, true);
    }
    
    private void transmitNextPacket()
    {
        // TODO: implement a serious bandwidth constraint mechanism
        
        synchronized (_pendingPacketQueue) {
            if (!_pendingPacketQueue.isEmpty()) {
                //_logger.info ("CHECKING PENDING PACKET QUEUE");
                MessagePacketInfo mpi = _pendingPacketQueue.peek();
                int available = Math.max (_remoteWindowSize - _outstandingPacketQueue.getSize(), 0);
                /*
                _logger.info ("2send from PPQ: " + mpi.getPacket().getSize() + 
                              " available: " + available +
                              " (win: " + _remoteWindowSize + 
                              " opq.size: " + _outstandingPacketQueue.getSize() + 
                              ")");
                */
                
                if (mpi != null && (mpi.getPacket().getSizeWithoutSACKChunk() < available)) {
                    //_logger.info ("XMIT FROM PENDING PACKET QUEUE");
                   
                    // transmit packet
                    transmitPacket (mpi);
                    MessagePacket p = mpi.getPacket();

                    // update remote window size
                    _remoteWindowSize -= p.getSizeWithoutSACKChunk();
                    
                    // update statistics
                    int sent = p.getSize();
                    
                    ++_mocket.getStatistics()._sentPackets; 
                    _mocket.getStatistics()._sentBytes += sent;
                    
                    _logger.info ("XMITTED PACKET " + p.getSequenceNumber() + " SIZE " + 
                                  p.getSize() + " FROM PENDING PACKET QUEUE");
                    _pendingPacketQueue._dump (System.err);
                   
                    // move packet from the pending packet queue...
                    _pendingPacketQueue.remove (mpi);
                   
                    _logger.info ("REMOVED PACKET " + p.getSequenceNumber() + " SIZE " +
                                  p.getSize() + " FROM PENDING PACKET QUEUE");
                    _pendingPacketQueue._dump (System.err);
                    
                    // ...to the outstanding queue
                    if (mpi.getPacket().isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE) ||
                        mpi.getPacket().isFlagSet(MessagePacket.HEADER_FLAG_CONTROL)) 
                    {
                        // remove SACK chunk from packet which is going to be inserted 
                        // in the outstanding packet queue
                        p.unsetSACKChunk();
                        _outstandingPacketQueue.insert (mpi);
                    }
                    
                    //_logger.info ("END XMIT FROM PENDING PACKET QUEUE");
                }
            }
        }

        synchronized (_outstandingPacketQueue) {
            if (!_outstandingPacketQueue.isEmpty()) {
                //_logger.info ("CHECKING OUTSTANDING PACKET QUEUE");
                MessagePacketInfo mpi = _outstandingPacketQueue.extractIfTimeout();
                if (mpi != null) { 
                    //_logger.info ("XMIT FROM OUTSTANDING PACKET QUEUE");
                    
                    // retransmit packet
                    MessagePacket p = mpi.getPacket();
                    transmitDatagram (p, true);
                    
                    _logger.info ("XMITTED PACKET " + p.getSequenceNumber() + " SIZE " + 
                                  p.getSize() + " FROM OUTSTANDING PACKET QUEUE");
                    _outstandingPacketQueue._dump (System.err);
                    
                    // update statistics
                    int sent = p.getSize();
                    ++_mocket.getStatistics()._retransmittedPackets;
                    _mocket.getStatistics()._sentBytes += sent;
                    
                    // remove SACK chunk from packet which is going to be reinserted 
                    // in the outstanding packet queue
                    p.unsetSACKChunk();
                    assert p.getSize() == p.getSizeWithoutSACKChunk();
                    
                    // reinsert mpi at the end of the outstanding packet queue
                    _outstandingPacketQueue.insert (mpi);
                    
                    //_logger.info ("END XMIT FROM OUTSTANDING PACKET QUEUE");
                }
            } 
        } 
    }
        
    public void run()
    {
        boolean done = false;
        
        assert _receiver != null;

        //_logger.info ("MessageTransmitter Thread for Mocket " + _mocket.getUID() + " starting.");
       
        _shutdownSent = 0;
        _shutdownAckSent = 0;

        // setup correct inital values for _lastTransmitTime and _lastSACKTransmitTime
        long currTime = System.currentTimeMillis();
        _lastTransmitTime = currTime;
        _lastSACKTransmitTime = currTime;
        
        while (!done) {
            synchronized (this) {
                final int MAX_ERRORS = 5;
                final int SHUTDOWN_TIMEOUT = 2000; // 2 sec.
                long toWait = 100;
                currTime = System.currentTimeMillis();
                
                /*
                if (_status.currentState() == MessageStateMachine.SHUTDOWN_PENDING || 
                    _status.currentState() == MessageStateMachine.SHUTDOWN_RECEIVED) {
                    _logger.warning ("In message transmitter: state is " + _status.currentStateAsString());
                    _logger.warning ("_outstandingPacketQueue.isEmpty(): " + _outstandingPacketQueue.isEmpty());
                    _logger.warning ("_pendingPacketQueue.isEmpty(): " + _pendingPacketQueue.isEmpty());
                }
                */

                if (_sendSACKInformationNow) {
                    _logger.info ("Transmitting an empty packet with SACK information due to an external request");
                    transmitSACKPacket();
                    _sendSACKInformationNow = false;
                }

                boolean havePacketsToTransmit = !_outstandingPacketQueue.isEmpty() || !_pendingPacketQueue.isEmpty();
                    
                if (_status.currentState() == MessageStateMachine.SHUTDOWN_SENT) {
                    _logger.info ("STATE SHUTDOWN_SENT");
                    if (_receivedPacket) {
                        // reset _receivedPacket
                        _receivedPacket = false;
                        // send SHUTDOWN 
                        transmitShutdownPacket();
                        _shutdownSent = 0;
                        // wait for SHUTDOWN_TIMEOUT
                        toWait = SHUTDOWN_TIMEOUT;
                    } else if (_shutdownSent >= MAX_ERRORS) {
                        // abort the connection
                        _status.close();
                    } else {
                        // send SHUTDOWN 
                        transmitShutdownPacket();
                        ++_shutdownSent;
                        // wait for SHUTDOWN_TIMEOUT
                        toWait = SHUTDOWN_TIMEOUT;
                    }
                } else if (_status.currentState() == MessageStateMachine.SHUTDOWN_ACK_SENT) {
                    _logger.info ("STATE SHUTDOWN_ACK_SENT");
                    if (_receivedPacket) {
                        // reset _receivedPacket
                        _receivedPacket = false;
                        // send SHUTDOWN 
                        transmitShutdownAckPacket();
                        _shutdownAckSent = 0;
                        // wait for SHUTDOWN_TIMEOUT
                        toWait = SHUTDOWN_TIMEOUT;
                    } else if (_shutdownAckSent >= MAX_ERRORS) {
                        // abort the connection
                        _status.close();
                    } else {
                        // send SHUTDOWN-ACK
                        transmitShutdownAckPacket();
                        ++_shutdownAckSent;
                        // wait for SHUTDOWN_TIMEOUT
                        toWait = SHUTDOWN_TIMEOUT;
                    }
                } else if (_status.currentState() == MessageStateMachine.CLOSED) {
                    _logger.info ("STATE CLOSED");
                    // mauro: should we handle _receivedPacket here?
                    // send SHUTDOWN-COMPLETE
                    transmitShutdownCompletePacket();
                    done = true;
                } else if (_status.currentState() == MessageStateMachine.ESTABLISHED) {
                    if (havePacketsToTransmit) {
                        transmitNextPacket();
                        toWait = Math.min (100, _outstandingPacketQueue.timeToNextRetransmission());
                    } 
                    if (_receivedPacket) {
                        _receivedPacket = false;
                        // Implement delayed ACK: send an ACK every other packet received 
                        // or after at most MessageMocket.DELAYED_ACK_TIMEOUT (notice that
                        // that this is intentionally less than SACK_XMIT_MIN_INTERVAL)
                        if (++_unacknowledgedPackets >= 2 ||
                            currTime - _lastSACKTransmitTime >= MessageMocket.DELAYED_ACK_TIMEOUT) {
                            _logger.info ("Sending SACK packet from packetReceived.");
                            transmitSACKPacket();
                        }
                    }
                } else if (_status.currentState() == MessageStateMachine.SHUTDOWN_PENDING || 
                           _status.currentState() == MessageStateMachine.SHUTDOWN_RECEIVED) {
                    _logger.info ("STATE " + _status.currentStateAsString());
                    // no more packets in the queues, proceed with connection close...
                    assert _shutdownSent == 0;
                    // enter SHUTDOWN_SENT state
                    _status.outstandingQueueFlushed();
                    // don't wait
                    toWait = -1;
                }
                
                if ((_ackManager.haveNewInformationSince (_lastSACKTransmitTime)) && 
                    (_lastSACKTransmitTime + SACK_XMIT_MAX_INTERVAL) < currTime) {
                    _logger.info ("Transmitting an empty packet with updated SACK information.");
                    // transmit an empty packet with a SACK chunk
                    transmitSACKPacket();
                    toWait = Math.min (100, _outstandingPacketQueue.timeToNextRetransmission());
                } else if ((_lastTransmitTime + _mocket.getKeepaliveTimeout()) < currTime) {
                    _logger.info ("Transmitting an empty packet due to inactivity."); 
                    /*
                    _logger.warning ("Transmitting an empty packet due to inactivity " + 
                                     "(currTime = " + currTime + ", " + 
                                     "_lastTransmitTime = " + _lastTransmitTime + ").");
                    */
                    // transmit a heartbeat packet
                    transmitHeartbeatPacket();
                    toWait = Math.min (100, _outstandingPacketQueue.timeToNextRetransmission());
                }

                if (toWait > 0) {
                    try {
                        wait (toWait);
                    }
                    catch (InterruptedException e) {} // intentionally left blank
                }
            }
        }
        
        _logger.info ("MessageTransmitter Thread for Mocket " + _mocket.getUID() + " terminating.");
    }

    /* transmit SACK information at most every second */
    private static long SACK_XMIT_MIN_INTERVAL = 1000;
    /* transmit SACK information at least every 30 seconds */
    private static long SACK_XMIT_MAX_INTERVAL = 30000;
        
    private DatagramPacket _outgoingPacket;
    private MessageMocket _mocket;
    private DatagramSocketWrapper _dgSocket;
    private InetAddress _remoteAddress;
    private PacketQueue _receivedPacketQueue;
    private PendingPacketQueue _pendingPacketQueue;
    private OutstandingPacketQueue _outstandingPacketQueue;
    private ACKManager _ackManager;
    private MessageStateMachine _status;
    private MessageReceiver _receiver;
    private int _remotePort;
    private int _shutdownSent;
    private int _shutdownAckSent;
    private long _validation;
    private long _lastTransmitTime;
    private long _lastSACKTransmitTime;
    private long _relSeqTSN;
    private long _unrelSeqTSN;
    private long _controlTSN;
    private long _relUnseqID;
    private long _unrelUnseqID;
    private boolean _useCrossSequencing;
    private volatile int _unacknowledgedPackets;
    private volatile int _remoteWindowSize;
    private volatile boolean _sendSACKInformationNow;
    private volatile boolean _receivedPacket;

    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */

