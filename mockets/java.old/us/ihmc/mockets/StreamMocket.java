package us.ihmc.mockets;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.net.DatagramPacket;
//import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;

import java.util.Vector;
import java.util.logging.Logger;

import us.ihmc.util.ByteConverter;

/**
 * The Mocket class represents an endpoint of a mobile socket connection. The class is designed to be
 * a drop-in replacement for the Socket class in the Java Platform API.
 *
 * @author Niranjan Suri
 * @author Maggie Breedy
 *
 * @version $Revision: 1.2 $
 */
public class StreamMocket
{
    /**
     * Create a new, unconnected, endpoint for a mocket connection.
     */
    public StreamMocket()
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _msn = new MocketStatusNotifier (this);

        _msm = new MocketStateMachine();
        _stats = new Statistics();
        _statusListeners = new Vector();
        _dataBufferingTime = BUFFERING_TIME;

        try {
            // Allocate socket here so that bind will succeed.
            _dgSocket = new DatagramSocketWrapper();
            _dgSocket.setSoTimeout (RECEIVE_TIMEOUT);
        }
        catch (SocketException ex) {
            throw ex;
        }
    }

    /**
     * Create a Mocket to encapsulate an accepted incoming datagram socket
     * connection.
     * This constructor is only invoked by ServerMocket
     *
     * @param datagram socket    the datagram socket that is the endpoint of
     *                           the newly accepted incoming connection
     */
    StreamMocket (DatagramSocketWrapper dataSocket, InetAddress remoteAddress, int remotePort)
        throws IOException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _msn = new MocketStatusNotifier (this);

        _msm = new MocketStateMachine();
        _stats = new Statistics();
        _statusListeners = new Vector();
        _dataBufferingTime = BUFFERING_TIME;

        _msm.passiveOpen();
        _msm.synReceived();
        _msm.synAckReceived();

        _dgSocket = dataSocket;
        _dgSocket.setReceiveBufferSize (UDP_BUFFER_SIZE);
        _dgSocket.setSoTimeout (RECEIVE_TIMEOUT);

        _logger.fine("DatagramSocket.getReceiveBufferSize() : " + _dgSocket.getReceiveBufferSize());
        _logger.fine("DatagramSocket.getSendBufferSize() : " + _dgSocket.getSendBufferSize());
        if (_dgSocket.getReceiveBufferSize() < RCVR_BUFFER_SIZE) {
            _logger.fine("Warning: RCVR_BUFFER_SIZE (" + RCVR_BUFFER_SIZE + 
                         ") is larger than DatagramSocket size (" + _dgSocket.getReceiveBufferSize() + ")");
        }

        _remoteAddress = remoteAddress;
        _remotePort = remotePort;
        _transmitter = new Transmitter (this, _dgSocket, _remoteAddress, _remotePort);
        _receiver = new Receiver (this, _dgSocket, _remoteAddress, _remotePort);
        _receiver.setStatusListeners (_statusListeners);
        _transmitter.setReceiver (_receiver);
        _receiver.setTransmitter (_transmitter);
        
        _msn.connected (false, _dgSocket.getLocalPort(), _remoteAddress, _remotePort);

        _transmitter.start();
        _receiver.start();
    }

    public void addMocketStatusListener (MocketStatusListener msl)
    {
        if (msl != null) {
            _statusListeners.add (msl);
        }
    }

    /**
     * Open a connection to the specified remote address and port
     *
     * @param remoteAddress  the remote address to connect to
     * @param remote port    the remote port to connect to
     *
     * @exception IOException     if there was a problem in opening the connection
     */
    public void connect (InetAddress remoteAddress, int remotePort)
        throws IOException
    {
        _remoteAddress = remoteAddress;
        _remotePort = 0;    // This will not be known until the connection is established

        _dgSocket = new DatagramSocketWrapper();
        _dgSocket.setReceiveBufferSize(UDP_BUFFER_SIZE);
        _dgSocket.setSoTimeout (CONNECT_TIMEOUT);

        _logger.fine("DatagramSocket.getReceiveBufferSize() : " + _dgSocket.getReceiveBufferSize());
        _logger.fine("DatagramSocket.getSendBufferSize() : " + _dgSocket.getSendBufferSize());

        StreamPacket packet = new StreamPacket();
        packet.setSYNBit();
        packet.setCtrlSeqNum ((short)0);
        packet.setCtrlAckNum ((short)0);
        packet.setSeqNum (0);
        packet.setAckNum (0);
        packet.setAdvertizedWindowSize (RCVR_BUFFER_SIZE);
        DatagramPacket dpOut = new DatagramPacket (packet.getPacket(), packet.getPacket().length,
                                                   remoteAddress, remotePort);
        DatagramPacket dpIn = new DatagramPacket (new byte [StreamPacket.getHeaderSize() + 2], StreamPacket.getHeaderSize() + 2);

        _msm.activeOpen();

        int numTries = 0; // In case the syn is lost.
        while (numTries < CONNECT_RETRIES) {
            _dgSocket.send (dpOut);
            try {
                _dgSocket.receive (dpIn);
                if (dpIn.getAddress().equals (remoteAddress) && (dpIn.getPort() == remotePort)) {
                    StreamPacket recPacket = new StreamPacket (dpIn.getData());
                    if (recPacket.getSYNAckBit()) {
                        _remotePort = ByteConverter.from2BytesToUnsignedShortInt (dpIn.getData(), StreamPacket.getHeaderSize());
                        break;
                    }
                }
            }
            catch (SocketTimeoutException e) {
                // Ignore this exception
            }
            numTries++;
        }
        if (numTries >= CONNECT_RETRIES) {
            // Failed to open a connection
            _msn.sendConnectionFailed (remoteAddress, remotePort);
            throw new IOException ("failed to open a connection to " + remoteAddress + ":" + remotePort);
        }

        _dgSocket.setSoTimeout (RECEIVE_TIMEOUT);

        _msm.synAckReceived();

        _transmitter = new Transmitter (this, _dgSocket, _remoteAddress, _remotePort);
        _receiver = new Receiver (this, _dgSocket, _remoteAddress, _remotePort);
        _receiver.setStatusListeners(_statusListeners);
        _transmitter.setReceiver (_receiver);
        _receiver.setTransmitter (_transmitter);

        _msn.connected (true, _dgSocket.getLocalPort(), _remoteAddress, _remotePort);

        _transmitter.start();
        _receiver.start();
    }

    public SocketAddress getLocalSocketAddress()
    {
        return _dgSocket.getLocalSocketAddress();
    }

    public DatagramSocketWrapper getRealSocket()
    {
        return _dgSocket;
    }

    public SocketAddress getRemoteSocketAddress()
    {
        if (_remoteAddress == null) {
            return null;
        }

        InetSocketAddress sa = new InetSocketAddress (_remoteAddress, _remotePort);
        return sa;
    }

    public void bind (SocketAddress localSocketAddress)
        throws IOException
    {
        _dgSocket.bind (localSocketAddress);
    }

    public boolean isConnected()
    {
        return (_msm.currentState() == MocketStateMachine.State.ESTABLISHED);
    }

    /**
     * Return the input stream for this connection
     *
     * @return     the input stream for the connection
     */
    public InputStream getInputStream()
    {
        if (_inputStream == null) {
            _inputStream = new MocketInputStream (this);
        }
        return _inputStream;
    }

    /**
     * Return the output stream for this connection
     *
     * @return     the output stream for the connection
     */
    public OutputStream getOutputStream()
    {
        if (_outputStream == null) {
            _outputStream = new MocketOutputStream (this);
        }
        return _outputStream;
    }


    /**
     *  Close the connection
     */
    public void close()
    {
        try {
            _msn.sendStats();
        }
        catch (IOException e) {}
        if (_msm.currentState() != MocketStateMachine.State.ESTABLISHED) {
            return;
        }
        synchronized (this) {
            try {
                _transmitter.flush (CLOSE_WAIT_TIME);
                _transmitter.waitForBufferToBeEmpty(CLOSE_WAIT_TIME);
                _msm.close();
                _transmitter.sendFin();
            }
            catch (Exception ex) {}

            try {
                _receiver.waitToReceiveFinAck();
            }
            catch (Exception ex) {}

            _transmitter.terminate();
            _receiver.terminate();
        }
    }

    /**
     * Set the maximum time for buffering outgoing data before transmitting data
     * Equivalent to TCP_NDELAY socket option
     * Note that setting this to 0 generates a packet each time send() is called
     * Accuracy of this setting depends on the period of the run() loop in the transmitter - currently 100ms
     */
    public void setDataBufferingTime (int ms)
    {
        if (ms < 0) {
            throw new IllegalArgumentException ("time (" + ms + ") cannot be less than 0");
        }

        _dataBufferingTime = ms;
    }

    public int getDataBufferingTime()
    {
        return _dataBufferingTime;
    }

    /**
     * Returns the statistics associated with this mocket
     *
     * @return the statistics
     **/
    public Statistics getStatistics()
    {
        return _stats;
    }

    /**
     * Internal method to send data (invoked by MocketOutputStream)
     */
    void send (byte[] buf, int off, int len)
        throws IOException
    {
        if (_msm.currentState() != MocketStateMachine.State.ESTABLISHED) {
            throw new IOException("The Socket is not connected");
        }

        _transmitter.send (buf, off, len);
    }

    /**
     * Internal method to query the number of bytes available (invoked by MocketInputStream)
     */
    int bytesAvailable()
    {
        return _receiver.bytesAvailable();
    }

    /**
     * Internal method to receive data (invoked by MocketInputStream)
     */
    int receive (byte[] buf, int off, int maxLen)
        throws IOException
    {
        return _receiver.receive (buf, off, maxLen, 0);
    }

    int receive (byte[] buf, int off, int maxLen, int timeout)
        throws IOException
    {
        return _receiver.receive (buf, off, maxLen, timeout);
    }

    /**
     * Internal method to retrieve the state machine for this mocket
     */
    MocketStateMachine getStateMachine()
    {
        return _msm;
    }

    /**
     * Internal method to retrieve the MocketStatusNotifier for this mocket
     */
    MocketStatusNotifier getMocketStatusNotifier()
    {
        return _msn;
    }

    /**
     * Internal callback method from the Transmitter to indicate that the
     * Transmitter is terminating
     */
    void transmitterTerminating()
    {
        synchronized (_termSync) {
            _termSync._transmitterTerminated = true;
            if (_termSync._receiverTerminated) {
                // Both transmitter and receiver are done
                _dgSocket.close();
            }
        }
    }

    /**
     * Internal callback method from the Receiver to indicate that the
     * Receiver is terminating
     */
    void receiverTerminating()
    {
        synchronized (_termSync) {
            _termSync._receiverTerminated = true;
            if (_termSync._transmitterTerminated) {
                // Both transmitter and receiver are done
                _dgSocket.close();
            }
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    /**
     * The Statistics class is used to retrieve statistics about the
     * current mocket connection
     */
    public class Statistics
    {
        public int getRetransmitCount()
        {
            return _retransmits;
        }

        public int getSentPacketCount()
        {
            return _sentPackets;
        }

        public int getSentByteCount()
        {
            return _sentBytes;
        }

        public int getReceivedPacketCount()
        {
            return _receivedPackets;
        }

        public int getReceivedByteCount()
        {
            return _receivedBytes;
        }

        public int getDiscardedPacketCount()
        {
            return _discardedPackets;
        }

        int _retransmits;
        int _sentPackets;
        int _sentBytes;
        int _receivedPackets;
        int _receivedBytes;
        int _discardedPackets;
    }

    /**
     * An internal class used to synchronize between the Mocket and the
     * Transmitter and Receiver threads during termination
     */
    private class TermSync
    {
        public boolean _transmitterTerminated;
        public boolean _receiverTerminated;
    }

    // /////////////////////////////////////////////////////////////////////////
    // CLASS CONSTANTS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static final int MTU = 1400;                // Maximum transmission unit for packet payload
    static final int BUFFERING_TIME = 100;      // Default maximum buffering time before transmitting data (in milliseconds)
    static final int KEEPALIVE_TIME = 1000;     // Maximum inactivity time after which an empty packet is transmitted
    static final int RECEIVE_TIMEOUT = 1200;    // Timeout placed on the underlying datagram socket
    static final int CONNECT_TIMEOUT = 250;     // Timeout to resend a SYN packet (in milliseconds)
    static final int CONNECT_RETRIES = 20;      // Max number of retransmissions of the SYN packet
    static final int ACK_TIMEOUT = 250;         // Timeout to retransmit a packet (in milliseconds)
    static final int FIN_ACK_TIMEOUT = 3000;    // Timeout to wait for the FINACK
    static final int CLOSE_WAIT_TIME = 2000;    // Time to wait to respond to FIN requests when doing a close
    static final int MAX_PACKET_SIZE = 2048;    // Max packet size (must be larger than MTU + Header size)
    static final int RCVR_BUFFER_SIZE = 49152;  // Receiver buffer size (in bytes)
    static final int SLIDING_WINDOW_SIZE = 32;  // transmitter sliding window size (in packets - should be less than RCVR_BUFFER_SIZE / MTU)
    static final int PACKET_TRANSMIT_INTERVAL = 10; //Time in millis that the transmitter will wait between trasmitting one packet and the next.
    static final int UDP_BUFFER_SIZE = MTU * SLIDING_WINDOW_SIZE;
    static final int SYN_HISTORY_SIZE = 64;
    static final int SYN_VALIDITY_WINDOW = 10000;
    static final int STATS_PORT = 9753;
    static final int STATS_SEND_INTERVAL = 1000; // The interval (in milliseconds) for sending connection statistics

    // /////////////////////////////////////////////////////////////////////////
    // CLASS VARIABLES /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    private MocketStateMachine _msm;                  // Maintains the state of this mocket
    private DatagramSocketWrapper _dgSocket;          // Underlying Datagram socket
    private Vector _statusListeners;                  // Vector of MocketStatusListener instances
    private MocketStatusNotifier _msn;                // Notifies MocketStatusMonitor about the status of this mocket

    private InetAddress _remoteAddress;               // Remote address
    private int _remotePort;                          // Remote port

    private Transmitter _transmitter;                 // Transmitter
    private Receiver _receiver;                       // Receiver
    private MocketInputStream _inputStream;           // Cached InputStream
    private MocketOutputStream _outputStream;         // Cached OutputStream

    private int _dataBufferingTime;                   // Maximum buffering time before transmitting data (in milliseconds)

    private Statistics _stats;                        // Statistics information

    private TermSync _termSync = new TermSync();      // Synchronization object for termination

    protected Logger _logger;                         // Logger
}
/*
 * vim: et ts=4 sw=4
 */
