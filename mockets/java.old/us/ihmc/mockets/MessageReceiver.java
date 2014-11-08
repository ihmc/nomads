/**
 * Receiver class that implements the functionality of receiving packets and
 * managing the sliding window for the receiving side.
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
import java.util.Vector;


class MessageReceiver extends Thread implements ACKInformationListener
{
    MessageReceiver (MessageMocket mocket, long validation, int maxWindowSize)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _wndMutex = new Object();
        
        _mocket = mocket;
        _dgSocket = _mocket.getSocket();
        _status = _mocket.getStateMachine();

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
        _validation = validation;
        assert maxWindowSize > 0;
        _maxWindowSize = maxWindowSize;
        _currWindowSize = _maxWindowSize;

        _packetQueue = _mocket.getReceivedPacketQueue();
        _controlPacketQueue = _mocket.getControlPacketQueue();
        _relSeqPacketQueue = _mocket.getRelSeqPacketQueue();
        _unrelSeqPacketQueue = _mocket.getUnrelSeqPacketQueue();
        _relUnseqPacketQueue = _mocket.getRelUnseqPacketQueue();
        _unrelUnseqPacketQueue = _mocket.getUnrelUnseqPacketQueue();
        _outstandingPacketQueue = _mocket.getOutstandingPacketQueue();
        _ackManager = _mocket.getACKManager();

        _lastRecvTime = 0;
        _transmitter = null;
        _packetProcessor = null;

        setName ("MessageReceiver Thread for Mocket " + _mocket.getUID());
        setPriority (8);
    }

    // no need to synchronize this method...
    void setTransmitter (MessageTransmitter transmitter)
    {
        _transmitter = transmitter;
    }

    // no need to synchronize this method...
    void setPacketProcessor (MessagePacketProcessor packetProcessor)
    {
        _packetProcessor = packetProcessor;
    }

    public void run()
    {
        boolean done = false;
        boolean closeConn = false;
        long closeWaitTime = 0;
        DatagramPacket dpIn = new DatagramPacket (new byte [MessageMocket.MAXIMUM_MTU], 
                                                  MessageMocket.MAXIMUM_MTU);

        _logger.info ("MessageReceiver Thread for Mocket " + _mocket.getUID() + " starting.");
        
        while (!done) {
            boolean receiveError = false;
            int packetLen = 0;
            MessagePacket pIn = null;

            try {
                _dgSocket.receive (dpIn);

                packetLen = dpIn.getLength();
                if (packetLen < MessagePacket.HEADER_SIZE) {
                    // discard packet if it is too small to be valid
                    _logger.info ("Discarding a short packet of size: " + dpIn.getLength());
                    // update statistics
                    ++_mocket.getStatistics()._discardedPackets_Invalid;
                    receiveError = true;
                } else if (!_remoteAddress.equals (dpIn.getAddress()) || 
                           _remotePort != dpIn.getPort()) {
                    // discard packet if it was received from some other endpoint
                    _logger.info ("Discarding a packet received from some other endpoint");
                    // update statistics
                    ++_mocket.getStatistics()._discardedPackets_Invalid;
                    receiveError = true;
                } else {
                    // discard packet if validation tag is invalid
                    pIn = new MessagePacket (dpIn.getData(), packetLen);
                    if (_validation != pIn.getValidation()) {
                        _logger.info ("Discarding packet with invalid validation tag");
                        // update statistics
                        ++_mocket.getStatistics()._discardedPackets_Invalid;
                        receiveError = true;
                    }
                }
            }
            catch (SocketTimeoutException e) {
                // socket timeout
                receiveError = true;
            }
            catch (IOException e) {
                // I/O error
                receiveError = true;
            }

            if (receiveError) {
                if (!closeConn) {
                    long elapsedTime = System.currentTimeMillis() - _lastRecvTime;
                    
                    // maybe a factor 2 is too small?
                    if (elapsedTime > _mocket.getKeepaliveTimeout() * 2) {
                        for (Enumeration en = _mocket.getStatusListeners(); en.hasMoreElements(); ) {
                            MocketStatusListener msl = (MocketStatusListener) en.nextElement();
                            if (msl.peerUnreachableWarning (elapsedTime)) {
                                // Application has request that the connection be closed
                                closeConn = true;
                                // TODO: set linger time to 0
                                _mocket.close();
                            }
                        }
                    }
                    /*
                    if (_status.currentState() == MessageStateMachine.CLOSED)
                        done = true;
                    */
                }
            } else {
                synchronized (this) {
                    /*
                    _logger.info ("Received a packet of size: " + packetLen);
                    String flags = "";
                    if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT)) 
                        flags += "FF ";
                    if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_MORE_FRAGMENTS)) 
                        flags += "MF ";
                    if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_LAST_FRAGMENT)) 
                        flags += "LF ";
                    _logger.info ("Packet frag. flags: " + flags);
                    */
                    
                    // update _lastRecvTime
                    _lastRecvTime = System.currentTimeMillis();
                    
                    // notify the transmitter of new packet arrival
                    _transmitter.packetReceived (pIn.getWindowSize());
                    
                    /*
                    _logger.info ("Received packet # " + pIn.getSequenceNumber() + 
                                  (pIn.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL)? " C" : "") +
                                  (pIn.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)? " R" : "") +
                                  (pIn.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED)? " S" : "") +
                                  ", size: " + pIn.getSize() +
                                  ", local window: " + _currWindowSize +
                                  ", remote window: " + pIn.getWindowSize());
                    */
                    
                    // check for SACK or Cancelled Packets chunks and update Outstanding Packet Queue
                    for (Chunk chk = pIn.getFirstChunk(); chk != null; chk = pIn.getNextChunk()) {
                        int chkType = chk.getType();
                        
                        // dispatch chunk to the appropriate processing method
                        switch (chk.getType()) {
                            case Chunk.CHUNK_TYPE_INIT:
                            case Chunk.CHUNK_TYPE_INIT_ACK:
                            case Chunk.CHUNK_TYPE_COOKIE_ECHO:
                            case Chunk.CHUNK_TYPE_COOKIE_ACK:
                                _logger.warning ("Received unexpected chunk.");
                                break;
                            case Chunk.CHUNK_TYPE_SACK:
                                assert (chk instanceof SACKChunk);
                                // process SACK chunk 
                                receivedSACKChunk ((SACKChunk) chk);
                                break;
                            case Chunk.CHUNK_TYPE_CANCELLED_PACKETS:
                                assert (chk instanceof CancelledPacketsChunk);
                                // process Cancelled Packets chunk
                                receivedCancelledPacketsChunk ((CancelledPacketsChunk) chk);
                                break;
                        }
                    }

                    int packetLenWithoutSACKChunk = pIn.getSizeWithoutSACKChunk();

                    if (packetLenWithoutSACKChunk > _currWindowSize) {
                        // discard packet as it is too large to fit in the receiver window
                        _logger.info ("Discarding a packet too large to fit in the receiver window");
                        // update statistics
                        ++_mocket.getStatistics()._discardedPackets_NoRoom;
                    } else {
                        /*
                        _logger.warning ("Received " + _mocket.getStatistics()._receivedPackets + 
                                         " packets and " + _mocket.getStatistics()._receivedBytes + 
                                         " bytes. Local window: " + _currWindowSize +
                                         " remote window: " + pIn.getWindowSize());
                        */

                        boolean delivered = false;
                        boolean duplicated = false;
                        
                        MessagePacketInfo pi = new MessagePacketInfo (pIn, _lastRecvTime);
                        
                        if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_CONTROL)) {
                            // put the packet into the control queue
                            if (_controlPacketQueue.insert(pi)) {
                                _logger.fine ("Inserted packet " + pIn.getSequenceNumber() + 
                                              " in control packet queue (win: "+
                                              _currWindowSize + ")");
                                _ackManager.receivedControlPacket (pIn.getSequenceNumber());
                                delivered = true;
                            } else {
                                duplicated = true;
                            }
                        } else if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_SEQUENCED)) {
                            if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)) {
                                // put the packet into the reliable sequenced queue
                                if (_relSeqPacketQueue.insert(pi)) {
                                    _logger.info ("Inserted packet " + pIn.getSequenceNumber() + 
                                                  " in reliable sequenced packet queue (win: " + 
                                                  _currWindowSize + ")");
                                    _ackManager.receivedReliableSequencedPacket (pIn.getSequenceNumber());
                                    delivered = true;
                                } else {
                                    duplicated = true;
                                }
                            } else {
                                // put the packet into the unreliable sequenced queue
                                if (_unrelSeqPacketQueue.insert(pi)) {
                                    _logger.fine ("Inserted packet " + pIn.getSequenceNumber() + 
                                                  " in unreliable sequenced packet queue (win: " +
                                                  _currWindowSize + ")");
                                    delivered = true;
                                } else {
                                    duplicated = true;
                                }
                            }
                        } else {
                            if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_FIRST_FRAGMENT |
                                              MessagePacket.HEADER_FLAG_MORE_FRAGMENTS |
                                              MessagePacket.HEADER_FLAG_LAST_FRAGMENT)) {
                                // don't deliver reliable packets more than once
                                if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE)) {
                                    if (_ackManager.receivedReliableUnsequencedPacket(pIn.getSequenceNumber()) &&
                                        _relUnseqPacketQueue.insert(pi)) {
                                        _logger.fine ("Inserted fragment packet " + pIn.getSequenceNumber() + 
                                                      " in reliable sequenced packet queue");
                                        delivered = true;
                                    } else {
                                        duplicated = true;
                                    }
                                } else {
                                    if (_unrelUnseqPacketQueue.insert(pi)) {
                                        delivered = true;
                                    } else {
                                        duplicated = true;
                                    }
                                }
                            } else {
                                // pass the packet directly to the packet processor
                                // but don't deliver reliable packets more than once
                                if (pIn.isFlagSet(MessagePacket.HEADER_FLAG_RELIABLE) &&
                                    !_ackManager.receivedReliableUnsequencedPacket(pIn.getSequenceNumber())) {
                                    duplicated = true;
                                } else {
                                    _packetProcessor.processPacket (pIn);
                                    delivered = true;
                                }
                            }
                        }
                        
                        if (delivered) {
                            // update statistics
                            ++_mocket.getStatistics()._receivedPackets;
                            _mocket.getStatistics()._receivedBytes += packetLen;
                            
                            // update window size
                            decreaseCurrentWindowSize (packetLenWithoutSACKChunk);
                            
                            // notify packet processor
                            _packetProcessor.packetArrived();
                        } else {
                            // update statistics
                            if (duplicated) {
                                ++_mocket.getStatistics()._discardedPackets_Duplicates;
                            } else {
                                ++_mocket.getStatistics()._discardedPackets_NoRoom;
                            }
                            // trigger SACK transmission
                            _transmitter.requestSACKTransmission();
                        }
                    }
                }
            }

            if (_status.currentState() == MessageStateMachine.CLOSED)
                done = true;
        }

        try {
            _packetProcessor.join();
        }
        catch (InterruptedException e) {
            throw new RuntimeException ("Unexpected InterruptedException: " + e.getMessage());
        }

        _logger.info ("MessageReceiver Thread for Mocket " + _mocket.getUID() + " terminating.");
    }

    void receivedSACKChunk (SACKChunk chk) 
    {
        _logger.fine ("Processing SACKChunk");
        SACKInformation sackInfo = chk.getSACKInformation();
        sackInfo.acknowledge (_outstandingPacketQueue);
        _logger.fine ("Processed SACKChunk");
    }

    void receivedCancelledPacketsChunk (CancelledPacketsChunk chk) 
    {        
        _logger.fine ("Processing CancelledPacketsChunk");
        ACKInformation ackInfo = chk.getACKInformation();
        ackInfo.notify (this);
        _logger.fine ("Processed CancelledPacketsChunk");
    }

    public boolean processPacket (long tsn, boolean control, boolean sequenced)
    {
        assert !control || !sequenced : "Cannot set both control and sequenced";
        
        // put dummy packet in appropriate sequenced packet queue
        if (control) {
            // put the packet into the control queue
            MessagePacketInfo pi = new MessagePacketInfo (tsn, true, false, false, _lastRecvTime);
            _logger.info ("Inserting dummy packet in control packet queue...");
            _controlPacketQueue.insert (pi);
            _logger.info ("done.");
            _ackManager.receivedControlPacket (tsn);
            _packetProcessor.packetArrived();
        } else if (sequenced) {
            // put the packet into the reliable sequenced queue
            MessagePacketInfo pi = new MessagePacketInfo (tsn, false, true, true, _lastRecvTime);
            _logger.info ("Inserting dummy packet in reliable sequenced packet queue...");
            _relSeqPacketQueue.insert (pi);
            _logger.info ("done.");
            _ackManager.receivedReliableSequencedPacket (tsn);
            _packetProcessor.packetArrived();
        }
        
        return true;
    }
    
    public int processPacketsRange (long begin, long end, boolean control, boolean sequenced) 
    {
        assert SequentialArithmetic.lessThan (begin, end);
        assert !control || !sequenced : "Cannot set both control and sequenced";
        
        int ret = 0;

        for (long i = begin; SequentialArithmetic.lessThanOrEqual (i, end); 
             i = SequentialArithmetic.add (i, 1)) {
            if (processPacket (i, control, sequenced)) 
                ++ret;
        }
        
        return ret;
    }

    void setMaximumWindowSize (int size) 
    {
        assert size > 0;
        assert _maxWindowSize > 0;
        assert size < _maxWindowSize;
        
        synchronized (_wndMutex) {
            _maxWindowSize = size;
        }
    }
    
    int getCurrentWindowSize() 
    {
        return _currWindowSize;
    }
                                
    void increaseCurrentWindowSize (int size) 
    {
        assert size > 0;
        assert size <= _maxWindowSize - _currWindowSize;

        synchronized (_wndMutex) {
            _logger.info ("increasing current window size (" + _currWindowSize + 
                          ") by " + size + " bytes");
            _currWindowSize += size;
            if (_currWindowSize > _maxWindowSize)
               _currWindowSize = _maxWindowSize; 
        }
        
        assert _currWindowSize >= 0;
        assert _currWindowSize <= _maxWindowSize;
    }
    
    private void decreaseCurrentWindowSize (int size) 
    {
        assert size > 0;
        assert size <= _currWindowSize;
        
        synchronized (_wndMutex) {
            _logger.info ("decreasing current window size (" + _currWindowSize + 
                          ") by " + size + " bytes");
            _currWindowSize -= size;
            if (_currWindowSize < 0)
               _currWindowSize = 0; 
        }
        
        assert _currWindowSize >= 0;
        assert _currWindowSize <= _maxWindowSize;
    }
    
    private MessageMocket _mocket;
    private DatagramSocketWrapper _dgSocket;
    private InetAddress _remoteAddress;
    private int _remotePort;
    private Vector _statusListeners;
    private PacketQueue _packetQueue;
    private MessageStateMachine _status;
    private SequencedPacketQueue _controlPacketQueue;
    private SequencedPacketQueue _relSeqPacketQueue;
    private SequencedPacketQueue _unrelSeqPacketQueue;
    private UnsequencedPacketQueue _relUnseqPacketQueue;
    private UnsequencedPacketQueue _unrelUnseqPacketQueue;
    private OutstandingPacketQueue _outstandingPacketQueue;
    private MessagePacketProcessor _packetProcessor;
    private MessageTransmitter _transmitter;
    private ACKManager _ackManager;
    private long _lastRecvTime;
    private long _validation;
    private volatile int _currWindowSize;
    private int _maxWindowSize;
    private Object _wndMutex;

    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */

