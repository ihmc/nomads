package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

import java.util.Enumeration;

import java.util.logging.Level;
import java.util.logging.Logger;

import us.ihmc.algos.IntegerMovingAverage;
import us.ihmc.ds.CircularQueue;

/**
 * Transmitter class that implements the functionality of sending packets and
 * managing the sliding window for the transmission side.
 */
class Transmitter extends Thread
{
    Transmitter (StreamMocket mocket, DatagramSocketWrapper dgSocket, InetAddress remoteAddress, int remotePort)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;
        _dgSocket = dgSocket;
        _remoteAddress = remoteAddress;
        _remotePort = remotePort;
        _transmitBuf = new byte [StreamMocket.MTU];
        _transmitBufCount = 0;
        _lastTransmitTime = 0;
        _packetQueue = new CircularQueue (StreamMocket.SLIDING_WINDOW_SIZE);
        _terminate = false;
        _finReceived = false;
        _outgoingPacket = new DatagramPacket (new byte[0], 0);
        _seqNum = 0;
        _ackNum = 0;
        _ctrlSeqNum = 0;
        _ctrlAckNum = 0;
        _unAckDataSize = 0;
        _advertisedWindowSize = StreamMocket.RCVR_BUFFER_SIZE;    // Default
        _rtt = new IntegerMovingAverage (20);
        setName ("StreamMocket Transmitter Thread");
    }

    void setReceiver (Receiver receiver)
    {
        _receiver = receiver;
    }

    synchronized void send (byte[] buf, int off, int len)
        throws IOException
    {
        if ((_mocket.getDataBufferingTime() > 0) && (len < (_transmitBuf.length - _transmitBufCount))) {
            // We are buffering data and there is room in the outgoing buffer - just append
            if (_transmitBufCount == 0) {
                _lastTransmitTime = System.currentTimeMillis();
            }
            System.arraycopy (buf, off, _transmitBuf, _transmitBufCount, len);
            _transmitBufCount += len;
        }
        else {
            // Not enough room or not buffering - copy as much as will fit, send the packet, and handle the rest
            int bytesLeft = len;
            while (true) {
                int spaceAvail = _transmitBuf.length - _transmitBufCount;
                int bytesToSend = bytesLeft;
                if (bytesToSend > spaceAvail) {
                    bytesToSend = spaceAvail;
                }
                System.arraycopy (buf, off + (len - bytesLeft),
                                  _transmitBuf, _transmitBufCount, bytesToSend);
                _transmitBufCount += bytesToSend;
                bytesLeft -= bytesToSend;
                if (_transmitBufCount == _transmitBuf.length) {
                    StreamPacket p = createAndInitPacket();
                    p.setPayload (_transmitBuf, 0, _transmitBufCount);
                    _seqNum += _transmitBufCount;
                    _transmitBufCount = 0;
                    sendPacket (p, -1);
                    _lastTransmitTime = System.currentTimeMillis();
                }
                if (bytesLeft == 0) {
                    break;
                }
            }
        }
    }

    /**
     * Flush any buffered data
     *
     * @return true if data was flushed, false if no data was buffered
     */
    synchronized int flush (long timeout)
        throws IOException
    {
        if (_transmitBufCount > 0) {
            StreamPacket p = createAndInitPacket();
            p.setPayload (_transmitBuf, 0, _transmitBufCount);

            if (sendPacket (p, timeout)) {
                _seqNum += _transmitBufCount;
                _transmitBufCount = 0;
                _lastTransmitTime = System.currentTimeMillis();
                return 1;
            }
            else {
                return -1;
            }
        }
        else {
            return 0;
        }
    }

    /**
     * Send a packet to the remote end. The packet is enqueued into the
     * packet queue and then transmitted over the wire. The method will block
     * if the packet queue is full or the receiver's advertized window size
     * is too small to accept the outgoing data.
     *
     * If a timeout is specified, the method will return false if the packet
     * could not be transmitted within the timeout period.
     *
     * @param packet the packet to send
     * @param timeOut the timeout period in milliseconds; negative indicates wait indefinitely
     *
     * @return true if the packet was sent successfully, false otherwise
     */
    public synchronized boolean sendPacket (StreamPacket packet, long timeOut)
        throws IOException
    {
        long startTime = System.currentTimeMillis();
        while (!_finReceived) {
            try {
                if (_packetQueue.isFull()) {
                    if (timeOut >= 0) {
                        wait (timeOut);
                        long currTime = System.currentTimeMillis();
                        if ((currTime - startTime) > timeOut) {
                            return false;
                        }
                    }
                    else {
                        wait();
                    }
                    continue;
                }
                if (_advertisedWindowSize < (packet.getPayloadSize() + _unAckDataSize)) {
                    if (timeOut > 0) {
                        wait (timeOut);
                        long currTime = System.currentTimeMillis();
                        if ((currTime - startTime) > timeOut) {
                            return false;
                        }
                    }
                    else {
                        wait();
                    }
                    continue;
                }
                break;
            }
            catch (Exception e) {
                throw new RuntimeException ("nested exception - " + e);
            }
        }
        //if (_finReceived) {
        //    throw new System.IO.IOException("other end closed connection");
        //}

        transmitPacket (packet);

        packet.setLastTransmitTime (System.currentTimeMillis());
        _packetQueue.enqueue (packet);

        // update the size of the data that has not been acknoledge yet.
        _unAckDataSize += packet.getPayloadSize();

        return true;
    }

    synchronized void transmitPacket (StreamPacket packet)
        throws IOException
    {
        packet.setRemoteAddress (_remoteAddress);
        packet.setRemotePort (_remotePort);

        _outgoingPacket.setData (packet.getPacket());
        _outgoingPacket.setAddress (packet.getRemoteAddress());
        _outgoingPacket.setPort (packet.getRemotePort());
        try {
            _dgSocket.send (_outgoingPacket);
            _logger.fine ("transmitted a packet with a seqnum of " + packet.getSeqNum() + " and a payload of " + packet.getPayloadSize());
        }
        catch (IOException e) {
            throw e;
        }
    }

    // Build the ack packet that acknowledges the packet's sequence number
    synchronized void sendAck (long seqNum, short ctrlSeqNum)
        throws IOException
    {
        _ackNum = seqNum;
        _ctrlAckNum = ctrlSeqNum;
        StreamPacket packet = createAndInitPacket();
        _outgoingPacket.setData (packet.getPacket());
        _outgoingPacket.setAddress (_remoteAddress);
        _outgoingPacket.setPort (_remotePort);
        _dgSocket.send (_outgoingPacket);
        _logger.fine ("sent an ack with seq number " + _ackNum + " and ctrl seq number " + _ctrlAckNum);
    }

    synchronized void finReceived()
    {
        _finReceived = true;
        notifyAll();
    }

    synchronized void sendFin()
        throws IOException
    {
        StreamPacket packet = createAndInitCtrlPacket();
        packet.setFINBit();
        sendPacket (packet, StreamMocket.CLOSE_WAIT_TIME);
    }

    // Build the fin ack packet and send it
    synchronized void sendFinAck()
        throws IOException
    {
        StreamPacket packet = createAndInitCtrlPacket();
        packet.setFINAckBit();
        _outgoingPacket.setData (packet.getPacket());
        _outgoingPacket.setAddress (_remoteAddress);
        _outgoingPacket.setPort (_remotePort);
        _dgSocket.send (_outgoingPacket);
    }

    // Dequeue a packet after the ack is received.
    synchronized void ackReceived (long ackNum, short ctrlAckNum)
    {
        while (true) {
            StreamPacket packet = (StreamPacket) _packetQueue.peek();
            if (packet == null) {
                break;
            }
            if (((packet.getSeqNum() + packet.getPayloadSize()) <= ackNum) &&
                (packet.getCtrlSeqNum() < ctrlAckNum)) {
                _packetQueue.dequeue();

                // update the size of the data that has not been acknoledge yet.
                _unAckDataSize -= packet.getPayloadSize();

                // Compute RTT
                // Note that using the last transmitted time may result in an incorrect result - the
                // ack may be from a previously transmitted copy of the packet
                int rtt = (int) (System.currentTimeMillis() - packet.getLastTransmitTime());
                _rtt.add (rtt);

                // Update statistics
                _mocket.getStatistics()._sentPackets++;
                _mocket.getStatistics()._sentBytes += packet.getPayloadSize();

                _logger.fine ("received an ack for seq num " + ackNum + " and ctrl seq num " + ctrlAckNum + " and dequeued a packet");
            }
            else {
                _logger.fine ("received an ack for seq num " + ackNum + " and ctrl seq num " + ctrlAckNum + " but did not dequeue a packet. " +
                              "packet seq num is " + (packet.getSeqNum() + packet.getPayloadSize()) + " and packet ctrl seq num is " + packet.getCtrlSeqNum());
                break;
            }
        }
        notifyAll();
    }

    synchronized void setAdvertisedWindowSize (int windowSize)
    {
        if (_advertisedWindowSize != windowSize) {
            _advertisedWindowSize = windowSize;
            _logger.fine ("advertised window size = " + _advertisedWindowSize);
            notifyAll();
        }
    }

    public void terminate()
    {
        _terminate = true;
    }

    public void run()
    {
        boolean done = false;
        long closeWaitTime = 0;
        boolean retransmitted = false;
        while (!done) {
            synchronized (this) {
                try {
                    long currTime = System.currentTimeMillis();
                    if ((_lastTransmitTime + _mocket.getDataBufferingTime()) < currTime) {
                        if (!_packetQueue.isFull()) {
                            flush(StreamMocket.CLOSE_WAIT_TIME);
                        }
                        else {
                            _logger.fine ("time to flush but output buffer is full");
                        }
                    }
                    if ((_lastTransmitTime + StreamMocket.KEEPALIVE_TIME) < currTime) {
                        _logger.fine ("transmitting an empty packet due to inactivity");
                        StreamPacket p = createAndInitPacket();
                        transmitPacket (p);
                        _lastTransmitTime = currTime;
                    }
                    if (!_packetQueue.isEmpty()) {
                        Enumeration en = _packetQueue.elements();
                        while (en.hasMoreElements()) {
                            StreamPacket packet = (StreamPacket) en.nextElement();
                            if ((currTime - packet.getLastTransmitTime()) > StreamMocket.ACK_TIMEOUT) {
                                packet.setAdvertizedWindowSize (_receiver.getFreeBufferSpace());
                                packet.setAckNum (_ackNum);
                                packet.setCtrlAckNum (_ctrlAckNum);
                                _outgoingPacket.setData (packet.getPacket());
                                _outgoingPacket.setAddress (_remoteAddress);
                                _outgoingPacket.setPort (_remotePort);
                                try {
                                    _dgSocket.send (_outgoingPacket);
                                }
                                catch (IOException e) {
                                    e.printStackTrace();
                                }
                                _lastTransmitTime = currTime;

                                // Update statistics
                                _mocket.getStatistics()._retransmits++;

                                _logger.fine ("retransmitted a packet with seq num " + packet.getSeqNum() + " and ctrl seq num " + packet.getCtrlSeqNum());
                                packet.setLastTransmitTime (currTime);
                                retransmitted = true;

				// Sleep just a little bit in order to not flood the network
				try {
					sleep(StreamMocket.PACKET_TRANSMIT_INTERVAL);
				}
				catch (Exception ex) {} //intentionally left blank.
                            }
                        }
                    }
                    if ((_lastStatSendTime + StreamMocket.STATS_SEND_INTERVAL) < currTime) {
                        _mocket.getMocketStatusNotifier().sendStats();
                        _lastStatSendTime = currTime;
                    }
                }
                catch (IOException e) {
                    e.printStackTrace();
                }
            }
            try {
                if (retransmitted) {
                    retransmitted = false;
                    Thread.sleep (10);
                }
                else {
                    Thread.sleep (100);
                }
            }
            catch (InterruptedException e) {
                // Ignore this exception
            }
            if (_finReceived || _terminate) {
                if (closeWaitTime == 0) {
                    closeWaitTime = System.currentTimeMillis();
                }
                else if ((System.currentTimeMillis() - closeWaitTime) > StreamMocket.CLOSE_WAIT_TIME) {
                    done = true;
                }
            }
        }
        _mocket.transmitterTerminating();
        _logger.fine ("transmitter terminating");
    }

    boolean isBufferEmpty()
    {
        return _packetQueue.isEmpty();
    }

    /**
     * Wait for the buffer to be empty so it can then close the datagram socket
     */
    synchronized boolean waitForBufferToBeEmpty(long timeOut)
    {
        long startTime = System.currentTimeMillis();
        while (!_packetQueue.isEmpty()) {
            try {
                if (timeOut > 0) {
                    wait (timeOut);
                    long currTime = System.currentTimeMillis();
                    if ((currTime - startTime) > timeOut) {
                        return false;
                    }
                }
                else {
                    wait();
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return true;
    }

    StreamPacket createAndInitPacket()
    {
        StreamPacket packet = new StreamPacket();
        packet.setSeqNum (_seqNum);
        packet.setAckNum (_ackNum);
        packet.setCtrlSeqNum (_ctrlSeqNum);
        packet.setCtrlAckNum (_ctrlAckNum);
        packet.setAdvertizedWindowSize (_receiver.getFreeBufferSpace());
        return packet;
    }

    StreamPacket createAndInitCtrlPacket()
    {
        StreamPacket packet = new StreamPacket();
        packet.setSeqNum (_seqNum);
        packet.setAckNum (_ackNum);
        _ctrlSeqNum++;
        packet.setCtrlSeqNum (_ctrlSeqNum);
        packet.setCtrlAckNum (_ctrlAckNum);
        packet.setAdvertizedWindowSize (_receiver.getFreeBufferSpace());
        return packet;
    }

    private StreamMocket _mocket;
    private DatagramSocketWrapper _dgSocket;
    private InetAddress _remoteAddress;
    private int _remotePort;
    private Receiver _receiver;
    private byte[] _transmitBuf;
    private int _transmitBufCount;
    private long _lastTransmitTime;
    private CircularQueue _packetQueue;
    private boolean _terminate;
    private boolean _finReceived;
    private DatagramPacket _outgoingPacket;
    private long _seqNum;
    private long _ackNum;
    private short _ctrlSeqNum;
    private short _ctrlAckNum;
    private int _advertisedWindowSize;
    private int _unAckDataSize;

    private IntegerMovingAverage _rtt;

    private Logger _logger;

    private long _lastStatSendTime;
}
