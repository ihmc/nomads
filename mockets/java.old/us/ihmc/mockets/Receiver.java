package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;

import java.util.logging.Logger;
import java.util.Enumeration;
import java.util.Vector;

/**
 * Receiver class that implements the functionality of receiving packets and
 * managing the sliding window for the receiving side.
 */
class Receiver extends Thread
{
    Receiver (StreamMocket mocket, DatagramSocketWrapper dgSocket, InetAddress remoteAddress, int remotePort)
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _mocket = mocket;
        _dgSocket = dgSocket;
        _remoteAddress = remoteAddress;
        _remotePort = remotePort;
        _recBuffer = new ReceiverByteBuffer (StreamMocket.RCVR_BUFFER_SIZE);
        _terminate = false;
        _finReceived = false;
        _finAckReceived = false;
        _nextCtrlSeqNum = 1;
        _lastRecvTime = System.currentTimeMillis();

        setName ("StreamMocket Receiver Thread");
        setPriority(8);
    }

    void setTransmitter (Transmitter transmitter)
    {
        _transmitter = transmitter;
    }

    void setStatusListeners (Vector statusListeners)
    {
        _statusListeners = statusListeners;
    }

    synchronized int bytesAvailable()
    {
        return _recBuffer.getContiguousByteCount();
    }

    synchronized int receive (byte[] buf, int off, int maxLen)
        throws IOException
    {
        return this.receive (buf, off, maxLen, 0);
    }

    synchronized int receive (byte[] buf, int off, int maxLen, int timeout)
        throws IOException
    {
        long start = System.currentTimeMillis();

        while (_recBuffer.getContiguousByteCount() == 0) {
            if ( (timeout != 0) && ((System.currentTimeMillis() - start) > timeout) ) {
                throw new SocketTimeoutException ();
            }
            if (_finReceived) {
                return -1;
            }
            if (_mocket.getStateMachine().currentState() != MocketStateMachine.State.ESTABLISHED) {
                return -1;
            }
            try {
                wait(timeout);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
        return _recBuffer.dequeue (buf, off, maxLen);
    }

    void terminate()
    {
        _terminate = true;
    }

    public void run()
    {
        boolean done = false;
        boolean closeConn = false;
        long closeWaitTime = 0;
        DatagramPacket dpIn = new DatagramPacket (new byte [StreamMocket.MAX_PACKET_SIZE], StreamMocket.MAX_PACKET_SIZE);
        while (!done) {
            boolean timedOut = false;
            try {
                _dgSocket.receive (dpIn);
                _logger.fine ("received a packet of size " + dpIn.getLength());
                if (dpIn.getLength() < StreamPacket.getHeaderSize()) {
                    // Discard packet - too small
                    _logger.info ("received a short packet of size " + dpIn.getLength());
                    continue;
                }
            }
            catch (SocketTimeoutException e) {
                timedOut = true;
            }
            catch (IOException e) {
                e.printStackTrace();
                timedOut = true;
            }
            if ((!closeConn) && (timedOut)) {
                long currTime = System.currentTimeMillis();
                long elapsedTime = currTime - _lastRecvTime;
                _mocket.getMocketStatusNotifier().setTimeSinceLastContact (elapsedTime);
                if (elapsedTime > StreamMocket.KEEPALIVE_TIME * 2) {
                    if (_statusListeners != null) {
                        for (Enumeration en = _statusListeners.elements(); en.hasMoreElements(); ) {
                            MocketStatusListener msl = (MocketStatusListener) en.nextElement();
                            if (msl.peerUnreachableWarning (elapsedTime)) {
                                closeConn = true;
                            }
                        }
                    }
                }
                if (closeConn) {
                    try {
                        _transmitter.flush(StreamMocket.CLOSE_WAIT_TIME);
                    }
                    catch (IOException ex1) {
                        ex1.printStackTrace();
                    }
                    _transmitter.waitForBufferToBeEmpty (StreamMocket.CLOSE_WAIT_TIME);
                    _mocket.getStateMachine().close();
                    try {
                        _transmitter.sendFin();
                    }
                    catch (IOException ex2) {
                        ex2.printStackTrace();
                    }
                    _transmitter.terminate();
                    terminate();
                    synchronized (this) {
                        notifyAll();
                    }
                }
            }
            else if (!timedOut) {
                if ((!_remoteAddress.equals (dpIn.getAddress())) || (_remotePort != dpIn.getPort())) {
                    // Discard packet - received from some other endpoint
                    continue;
                }
                _lastRecvTime = System.currentTimeMillis();
                _mocket.getMocketStatusNotifier().setTimeSinceLastContact (0);
                StreamPacket pIn = new StreamPacket (dpIn.getData());
                synchronized (this) {
                    try {
                        _transmitter.ackReceived (pIn.getAckNum(), pIn.getCtrlAckNum());
                        _transmitter.setAdvertisedWindowSize (pIn.getAdvertizedWindowSize());
                        if (pIn.isCtrlPacket()) {
                            if (pIn.getCtrlSeqNum() != _nextCtrlSeqNum) {
                                _transmitter.sendAck (_recBuffer.getFirstByteIndex() + _recBuffer.getContiguousByteCount(),
                                                      _nextCtrlSeqNum);
                            }
                            else {
                                _nextCtrlSeqNum++;
                                _transmitter.sendAck (_recBuffer.getFirstByteIndex() + _recBuffer.getContiguousByteCount(),
                                                      _nextCtrlSeqNum);
                                // Check if packet is a FIN
                                // If so, send FINACK and set _finReceived to true
                                if (pIn.getFINBit()) {
                                    if (!_finReceived) {
                                        _mocket.getStateMachine().finReceived();
                                        // Send FIN from this end also
                                        _transmitter.flush (StreamMocket.CLOSE_WAIT_TIME);
                                        StreamPacket p = _transmitter.createAndInitCtrlPacket();
                                        p.setFINBit();
                                        _transmitter.sendPacket (p, StreamMocket.CLOSE_WAIT_TIME);
                                    }
                                    _finReceived = true;
                                    _transmitter.finReceived();
                                    _transmitter.sendFinAck ();
                                    notifyAll();
                                }
                                else if (pIn.getFINAckBit()) {
                                    _finAckReceived = true;
                                    notifyAll();
                                }
                            }
                        }
                        else {
                            if (pIn.getPayloadSize() > 0) {
                                byte[] data = new byte [pIn.getPayloadSize()];
                                System.arraycopy (pIn.getPacket(), StreamPacket.getHeaderSize(), data, 0, data.length);
                                if (!_recBuffer.enqueue (data, (int) pIn.getSeqNum())) {
                                    /*!!*/ // Fix the cast to int
                                    // Failed to enqueue
                                    _mocket.getStatistics()._discardedPackets++;
                                    //_logger.Log ("failed to enqueue data of size " + data.Length + " with sequence number " + pIn.getSeqNum());
                                }
                                else {
                                    _mocket.getStatistics()._receivedPackets++;
                                    _mocket.getStatistics()._receivedBytes += pIn.getPayloadSize();
                                }
                                _transmitter.sendAck (_recBuffer.getFirstByteIndex() + _recBuffer.getContiguousByteCount(),
                                                      _nextCtrlSeqNum);
                                notifyAll();
                            }
                        }
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }

            if (_finAckReceived || _terminate) {
                if (closeWaitTime == 0) {
                    closeWaitTime = System.currentTimeMillis();
                }
                else if ((System.currentTimeMillis() - closeWaitTime) > StreamMocket.CLOSE_WAIT_TIME) {
                    done = true;
                }
            }
        }
        _mocket.receiverTerminating();
        _logger.fine ("receiver terminating");
    }

    /**
     * Wait until a finack frame is received so it can close the connection.
     */
    synchronized void waitToReceiveFinAck()
    {
        while (!_finAckReceived) {
            try {
                wait (StreamMocket.FIN_ACK_TIMEOUT);
                _finAckReceived = true;
            }
            catch (Exception e) {
                System.out.println ("<>Receiver::waitToReceiveFinAck:Unable to wait");
                e.printStackTrace();
            }
        }
    }

    int getFreeBufferSpace()
    {
        // This method is not synchronized on purpose - it will lead to deadlock if it is
        // synchronized. Since getMaxSize() and getContiguousByteCount() simply return values
        // of variables, there should be no need to synchronize
        return _recBuffer.getMaxSize() - _recBuffer.getContiguousByteCount();
    }

    // Variables
    private StreamMocket _mocket;
    private DatagramSocketWrapper _dgSocket;
    private InetAddress _remoteAddress;
    private int _remotePort;
    private Vector _statusListeners;
    private Transmitter _transmitter;
    private ReceiverByteBuffer _recBuffer;
    private boolean _terminate;
    private boolean _finReceived;
    private boolean _finAckReceived;
    private short _nextCtrlSeqNum;
    private long _lastRecvTime;

    private Logger _logger;
}
