/**
 * The MessageMocket class represents a message-oriented mocket.
 *
 * @author Mauro Tortonesi 
 */
package us.ihmc.mockets;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.lang.System;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;

import java.util.Random;

import java.util.logging.Logger;

public class MessageMocket extends Mocket
{
    /**
     * This constructor creates a new MessageMocket mocket. 
     * The mocket cannot be used until it is turned into an active 
     * message mocket by calling the connect method.
     */
    public MessageMocket()
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _msm = new MessageStateMachine();

        _connectTimeout = DEFAULT_CONNECT_TIMEOUT;
        _pendingPacketQueueSize = DEFAULT_PENDING_PACKET_QUEUE_SIZE;
        _keepaliveTimeout = DEFAULT_KEEPALIVE_TIMEOUT;
        _mtu = DEFAULT_MTU;
        _useCrossSequencing = DEFAULT_CROSS_SEQUENCING_SETTING;
        _rto = DEFAULT_RETRANSMISSION_TIMEOUT;
        _maxWindowSize = DEFAULT_MAX_WINDOW_SIZE;
        _receiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
        
        _outgoingPacket = new DatagramPacket (new byte[0], 0);
        _remoteAddress = null;
        _remotePort = 0;
        _receivedPacketQueue = new PacketQueue();
        _pendingPacketQueue = new PendingPacketQueue (_pendingPacketQueueSize, false);
        _outstandingPacketQueue = new OutstandingPacketQueue();
        
        // TODO: after debugging, use random values to initialize outgoing validation
        // by using something like:
        /*
        Random generator = new Random (System.currentTimeMillis());
        while (_outgoingValidation == 0) {
            _outgoingValidation = generator.nextLong();
        }
        */
        _outgoingValidation = 1;

        // incoming validation is undefined until the connection is estabished
        _incomingValidation = 0;
        
        // TODO: after debugging, use random values to initialize outgoing TSNs
        // by using something like:
        /*
        _initialRelSeqTSN = generator.nextLong();
        _initialUnrelSeqTSN = generator.nextLong();
        _initialControlTSN = generator.nextLong();
        _initialRelUnseqID = generator.nextLong();
        _initialUnrelUnseqID = generator.nextLong();
        */
        _initialRelSeqTSN = 0;
        _initialUnrelSeqTSN = 0;
        _initialControlTSN = 0;
        _initialRelUnseqID = 0;
        _initialUnrelUnseqID = 0;
        
        // undefined until the connection is estabished
        _ackManager = null;
        _receiver = null;
        _transmitter = null;
        _controlPacketQueue = null;
        _relSeqPacketQueue = null;
        _relUnseqPacketQueue = null;
        _unrelSeqPacketQueue = null;
        _unrelUnseqPacketQueue = null;
        
        try {
            // allocate socket here so that bind will succeed
            _dgSocket = new DatagramSocketWrapper();
        }
        catch (SocketException ex) {
            throw ex;
        }
    }
    
    MessageMocket (StateCookie cookie, int remoteWindowSize, InetAddress remoteAddress, 
                   DatagramSocketWrapper dsw)
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _msm = new MessageStateMachine (MessageStateMachine.ESTABLISHED);

        _connectTimeout = DEFAULT_CONNECT_TIMEOUT;
        _pendingPacketQueueSize = DEFAULT_PENDING_PACKET_QUEUE_SIZE;
        _keepaliveTimeout = DEFAULT_KEEPALIVE_TIMEOUT;
        _mtu = DEFAULT_MTU;
        _useCrossSequencing = DEFAULT_CROSS_SEQUENCING_SETTING;
        _rto = DEFAULT_RETRANSMISSION_TIMEOUT;
        _maxWindowSize = DEFAULT_MAX_WINDOW_SIZE;
        _receiveTimeout = DEFAULT_RECEIVE_TIMEOUT;
        
        _outgoingPacket = new DatagramPacket (new byte[0], 0);
        _remoteAddress = remoteAddress;
        _remotePort = cookie.getAPort();
        _outgoingValidation = cookie.getZValidation();
        _incomingValidation = cookie.getAValidation();
        _initialRelSeqTSN = cookie.getZInitialRelSeqTSN();
        _initialUnrelSeqTSN = cookie.getZInitialUnrelSeqTSN();
        _initialControlTSN = cookie.getZInitialControlTSN();
        _initialRelUnseqID = cookie.getZInitialRelUnseqID();
        _initialUnrelUnseqID = cookie.getZInitialUnrelUnseqID();
        _receivedPacketQueue = new PacketQueue();
        _pendingPacketQueue = new PendingPacketQueue (_pendingPacketQueueSize, false);
        _outstandingPacketQueue = new OutstandingPacketQueue();
        _ackManager = new ACKManager (cookie.getZInitialRelSeqTSN(),
                                      cookie.getZInitialControlTSN(),
                                      cookie.getZInitialRelUnseqID());
        _controlPacketQueue = new SequencedPacketQueue (cookie.getAInitialControlTSN());
        _relSeqPacketQueue = new SequencedPacketQueue (cookie.getAInitialRelSeqTSN());
        _unrelSeqPacketQueue = new SequencedPacketQueue (cookie.getAInitialUnrelSeqTSN());
        _relUnseqPacketQueue = new UnsequencedPacketQueue();
        _unrelUnseqPacketQueue = new UnsequencedPacketQueue();
        
        _dgSocket = dsw;
        _dgSocket.setReceiveBufferSize (DEFAULT_UDP_BUFFER_SIZE);
        _dgSocket.setSoTimeout ((int)Math.max(_receiveTimeout, 0));
        
        _logger.fine ("_dgSocket.getReceiveBufferSize() : " + _dgSocket.getReceiveBufferSize());
        _logger.fine ("_dgSocket.getSendBufferSize() : " + _dgSocket.getSendBufferSize());

        _transmitter = new MessageTransmitter (this, remoteWindowSize);
        _receiver = new MessageReceiver (this, _incomingValidation, _maxWindowSize);
        _packetProcessor = new MessagePacketProcessor (this, 
                                                       cookie.getAInitialControlTSN(), 
                                                       cookie.getAInitialRelSeqTSN(),
                                                       cookie.getAInitialUnrelSeqTSN());
        _transmitter.setReceiver (_receiver);
        _receiver.setTransmitter (_transmitter);
        _packetProcessor.setReceiver (_receiver);
        _receiver.setPacketProcessor (_packetProcessor);
        _transmitter.start();
        _receiver.start();
        _packetProcessor.start();
    }

    /**
     * This function returns the local address to which the mocket is bound, or 
     * null if the mocket is unbound. It is the equivalent of the getsockname(2) 
     * function for UDP sockets in the BSD socket api.
     */
    public SocketAddress getLocalName() 
        throws SocketException
    {
        SocketAddress local = _dgSocket.getLocalSocketAddress();
        
        // return exception if socket is not bound yet
        if (local == null) {
            throw new SocketException ("Mocket is not bound.");
        }
        
        return local;
    }
    
    /**
     * This function returns the address of the peer the mocket is communicating with, 
     * or null if the mocket has no specified peer. It is the equivalent of the 
     * getpeername(2) function for UDP sockets in the BSD socket api.
     */
    public SocketAddress getPeerName()
        throws SocketException
    {
        // perhaps we should check that the datagram socket is actually connected?
        if (_remoteAddress == null) {
            return null;
        }

        return new InetSocketAddress (_remoteAddress, _remotePort);
    }
    
    /**
     * This function sets the default receive timeout (in milliseconds)
     * for this mocket.
     * For timeout:
     * -1 means wait forever
     *  0 return immediately (unsupported)
     */
    public void setReceiveTimeout (long timeout)
        throws SocketException 
    {
        if (timeout < -1)
            throw new IllegalArgumentException ("timeout should be >= -1!!!");
        _receiveTimeout = timeout; 
    }
    
    public long getReceiveTimeout()
    {
        return _receiveTimeout; 
    }
    
    public void setConnectTimeout (long timeout)
        throws SocketException 
    {
        if (timeout <= 0)
            throw new IllegalArgumentException ("timeout should be a positive value!!!");
        _connectTimeout = timeout; 
    }
    
    public long getConnectTimeout()
    {
        return _connectTimeout; 
    }

    public int getPendingPacketQueueSize()
    {
        return _pendingPacketQueueSize;
    }

    public void setKeepaliveTimeout (int timeout) 
    {
        if (timeout <= 0)
            throw new IllegalArgumentException ("timeout should be a positive value!!!");
        _keepaliveTimeout = timeout;
    }
    
    public int getKeepaliveTimeout() 
    {
        return _keepaliveTimeout;
    }
    
    public void setRetransmissionTimeout (long timeout)
        throws SocketException 
    {
        if (timeout <= 0)
            throw new IllegalArgumentException ("timeout should be a positive value!!!");
        _rto = timeout; 
    }
    
    public long getRetransmissionTimeout()
    {
        return _rto; 
    }

    public void setMaximumWindowSize (int size) 
    {
        if (size <= 0)
            throw new IllegalArgumentException ("size should be a positive value!!!");
        _maxWindowSize = size;

        // update receiver if needed
        if (_receiver != null)
            _receiver.setMaximumWindowSize (size);
    }
    
    public int getMaximumWindowSize() 
    {
        return _maxWindowSize;
    }
    
    public void setMTU (int mtu)
    {
        if (mtu > MAXIMUM_MTU)
            throw new IllegalArgumentException ("Cannot set a MTU greater than " + 
                                                MAXIMUM_MTU + " bytes!!!");
        if (_msm.currentState() == MessageStateMachine.ESTABLISHED)
            throw new IllegalStateException ("Cannot change MTU after a connection " + 
                                             "has been established!!!");
        _mtu = mtu;
    }
    
    public int getMTU()
    {
        return _mtu;
    }
    
    public boolean isClosed()
        throws SocketException 
    {
        return (_msm.currentState() == MessageStateMachine.CLOSED);
    }

    public boolean isConnected()
        throws SocketException 
    {
        return (_msm.currentState() == MessageStateMachine.ESTABLISHED);
    }

    public void bind (SocketAddress localAddress)
        throws SocketException 
    {
        _dgSocket.bind (localAddress);
    }

    public void close()
    {
        // enter SHUTDOWN-PENDING state
        _msm.shutdown();

        // wait for the transmitter and the receiver to terminate
        try {
            _receiver.join();
            _transmitter.join();
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            System.exit (1);
        }
        
        _transmitter = null;
        _receiver = null;
        _packetProcessor = null;
    }
    
    DatagramSocketWrapper getSocket()
    {
        return _dgSocket;
    }

    PacketQueue getReceivedPacketQueue()
    {
        return _receivedPacketQueue;
    }

    PendingPacketQueue getPendingPacketQueue()
    {
        return _pendingPacketQueue;
    }

    OutstandingPacketQueue getOutstandingPacketQueue()
    {
        return _outstandingPacketQueue;
    }

    SequencedPacketQueue getControlPacketQueue()
    {
        return _controlPacketQueue;
    }

    SequencedPacketQueue getRelSeqPacketQueue()
    {
        return _relSeqPacketQueue;
    }
    
    SequencedPacketQueue getUnrelSeqPacketQueue()
    {
        return _unrelSeqPacketQueue;
    }
    
    UnsequencedPacketQueue getRelUnseqPacketQueue()
    {
        return _relUnseqPacketQueue;
    }
    
    UnsequencedPacketQueue getUnrelUnseqPacketQueue()
    {
        return _unrelUnseqPacketQueue;
    }
    
    MessageStateMachine getStateMachine()
    {
        return _msm;
    }
    
    MessageTransmitter getTransmitter()
    {
        return _transmitter;
    }
    
    MessageReceiver getReceiver()
    {
        return _receiver;
    }

    /**
     * Receives a datagram packet from this mocket and writes it in the 
     * specified buffer. This function uses the specified input timeout 
     * (in milliseconds).
     * If timeout == 0, then the default timeout is used.
     * If timeout == -1, then wait forever.
     */
    public int receive (byte[] buf, int off, int len, long timeout) 
        throws IOException 
    {
        if (timeout == 0)
            timeout = _receiveTimeout;
        
        // change timeout format, as PacketQueue.extract expects
        // a non-negative timeout value, for which 0 means wait forever
        if (timeout < 0) {
            timeout = 0;
        } else if (timeout == 0) {
            // use the minimum timeout we can (1 msec)
            timeout = 1;
        } 

        _logger.fine ("trying to receive packet");
        ReceivedMessage p = _receivedPacketQueue.extract (timeout);
        if (p == null) {
            throw new IOException ("Input timeout expired!");
        }

        _logger.fine ("received packet");
        int copied = 0;
        try {
            copied = p.getData (buf, off, len);
        }
        catch (NotEnoughSizeException e) {
            _logger.fine ("not enough size in destination buffer");
        }

        _receiver.increaseCurrentWindowSize (p.getSize());

        _logger.fine ("received packet - 2");
        return copied;
    }
    
    /**
     * Receives a datagram packet from this mocket and writes it in the 
     * specified buffer. This function uses the default input timeout 
     * (which could be wait forever).
     */
    public int receive (byte[] buf, int off, int len)
        throws IOException 
    {
        return receive (buf, off, len, 0);
    }
    
    /**
     * This function returns a Sender with user-defined parameters. 
     */
    public Sender getSender (boolean sequenced, boolean reliable)
        throws SocketException 
    {
        return new SenderImpl (this, sequenced, reliable);
    }
    
    public void connect (SocketAddress ma)
        throws SocketException
    {
        InetAddress addr = ((InetSocketAddress)ma).getAddress();
        int port = ((InetSocketAddress)ma).getPort();

        // -1 is: wait forever
        connect (addr, port, -1);
    }

    public void connect (SocketAddress ma, long timeout)
        throws SocketException
    {
        InetAddress addr = ((InetSocketAddress)ma).getAddress();
        int port = ((InetSocketAddress)ma).getPort();

        connect (addr, port, timeout);
    }
    
    /**
     * Open a connection to the specified remote address and port.
     *
     * @param addr  the remote address to connect to
     * @param port  the remote port to connect to
     *
     * @exception IOException   if there was a problem in opening the connection
     */
    void connect (InetAddress addr, int port, long timeout)
        throws SocketException
    {
        // these will not be known until the user calls connect 
        _remoteAddress = addr;
        _remotePort = port;

        _dgSocket.setReceiveBufferSize (DEFAULT_UDP_BUFFER_SIZE);
        _dgSocket.setSoTimeout (DEFAULT_CONNECT_RECEIVE_TIMEOUT);

        _logger.fine ("_dgSocket.getReceiveBufferSize() : " + _dgSocket.getReceiveBufferSize());
        _logger.fine ("_dgSocket.getSendBufferSize() : " + _dgSocket.getSendBufferSize());

        // prepare INIT packet 
        // (for connection setup we use unsequenced, unreliable packets)
        DatagramPacket dpInit = new DatagramPacket (new byte [0], 0); 
        InitChunk iChk = new InitChunk (_outgoingValidation,
                                        _initialRelSeqTSN,
                                        _initialUnrelSeqTSN,
                                        _initialControlTSN,
                                        _initialRelUnseqID,
                                        _initialUnrelUnseqID);
        MessagePacket init = new MessagePacket();
        init.setWindowSize (DEFAULT_MAX_WINDOW_SIZE);
        init.setValidation (_outgoingValidation);
        init.setSequenceNumber (0);
        
        if (!init.addChunk(iChk)) {
            // this should never happen
            _logger.severe ("INIT chunk size bigger than packet size.");
            System.exit (1);
        }

        init.writeToDatagramPacket (dpInit);
        //init.release();
        
        DatagramPacket dpCookieEcho = new DatagramPacket (new byte [0], 0); 
        
        boolean done = false;
        DatagramPacket dpIn = new DatagramPacket (new byte [MessageMocket.MAXIMUM_MTU], 
                                                  MessageMocket.MAXIMUM_MTU);
        DatagramPacket dpOut = dpInit;
        
        _logger.fine ("dpOut initialized");

        StateCookie cookie = null;
            
        // change status to INIT-SENT
        _msm.associate();
       
        int remoteWindowSize = 0;
        int numTries = 0;
        long startTime = System.currentTimeMillis();

        while (!done && System.currentTimeMillis() - startTime < _connectTimeout) {
            _logger.fine ("starting loop");
            assert dpOut != null;
            try {
                dpOut.setAddress (_remoteAddress);
                dpOut.setPort (_remotePort);
                _dgSocket.send (dpOut);
            }
            catch (IOException e) {
                _logger.severe ("Problem while writing a packet to the " + 
                                "datagram socket: " + e.getMessage());
                _msm.abort ();
                throw new SocketException (e.getMessage());
            }

            try {
                _dgSocket.receive (dpIn);
            }
            catch (SocketTimeoutException e) {
                numTries++;
                continue;
            }
            catch (IOException e) {
                _logger.severe ("Problem while reading a packet from the " + 
                                "datagram socket: " + e.getMessage());
                _msm.abort ();
                throw new SocketException (e.getMessage());
            }

            if (dpIn.getLength() < MessagePacket.HEADER_SIZE) {
                // discard packet as it is too small to be valid
                _logger.fine ("Received a short packet of size " + 
                              dpIn.getLength() + ": discarding it...");
                continue;
            } else {
                _logger.fine ("Received a packet of size " + dpIn.getLength());
            }
            
            if (dpIn.getAddress().equals (_remoteAddress)) {
                MessagePacket pIn = new MessagePacket (dpIn.getData(), dpIn.getLength());

                // update window size
                remoteWindowSize = pIn.getWindowSize();

                int chunksProcessed = 0;
                boolean foundStateChangeChunk = false;
                
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
                    
                    // dispatch chunk to the appropriate processing method
                    switch (chk.getType()) {
                        case Chunk.CHUNK_TYPE_INIT_ACK:
                            assert (chk instanceof InitAckChunk);
                            // check state
                            if (_msm.currentState() == MessageStateMachine.COOKIE_ECHOED) {
                                // if we receive an INIT-ACK message when expecting a COOKIE-ACK
                                // instead, then we send a new COOKIE-ECHO message and keep going
                                // (that is, we restart the loop without decreasing numTries)
                                continue;
                            } else {
                                assert _msm.currentState() == MessageStateMachine.COOKIE_WAIT;
                                InitAckChunk iaChk = (InitAckChunk) chk;
                                cookie = iaChk.getCookie();
                                // create COOKIE-ECHO packet if needed
                                if (dpCookieEcho != null) {
                                    dpCookieEcho = new DatagramPacket (new byte [0], 0);
                                    dpCookieEcho.setAddress (_remoteAddress);
                                    dpCookieEcho.setPort (_remotePort);
                                    CookieEchoChunk ceChk = new CookieEchoChunk (cookie);
                                    MessagePacket cookieEcho = new MessagePacket();
                                    cookieEcho.setWindowSize (DEFAULT_MAX_WINDOW_SIZE);
                                    cookieEcho.setValidation (_outgoingValidation);
                                    cookieEcho.setSequenceNumber (0);
                                    if (!cookieEcho.addChunk(ceChk)) {
                                        // this should never happen
                                        _logger.severe ("COOKIE-ECHO chunk size bigger than packet size.");
                                        System.exit(1);
                                    }
                                    cookieEcho.writeToDatagramPacket (dpCookieEcho);
                                    //cookieEcho.release();
                                }
                                // reset loop counter
                                numTries = 0;
                                // now we start sending COOKIE-ECHO
                                dpOut = dpCookieEcho;
                                // change status to COOKIE-ECHOED
                                _msm.receivedInitAck();
                            }
                            ++chunksProcessed;
                            _logger.fine ("remoteWindowSize = " + remoteWindowSize);
                            break;
                        case Chunk.CHUNK_TYPE_COOKIE_ACK:
                            assert (chk instanceof CookieAckChunk);
                            ++chunksProcessed;
                            _msm.receivedCookieAck();
                            CookieAckChunk caChk = (CookieAckChunk) chk;
                            // update remote port
                            _remotePort = caChk.getPort();
                            _logger.fine ("remote port is now " + _remotePort);
                            done = true;
                            break;
                        default:
                            _logger.fine ("Packet has an unknown/unexpected chunk type!");
                    }
                }

                assert chunksProcessed > 0;
            }
        }
        if (!done) {
            // Failed to open a connection
            _msm.abort ();
            throw new SocketException ("Failed to open a connection to " + 
                                       _remoteAddress + ":" + _remotePort);
        }
        
        _dgSocket.setSoTimeout ((int)Math.max(_receiveTimeout, 0));

        // initialize mocket status from cookie 
        _incomingValidation = cookie.getZValidation();
        _ackManager = new ACKManager (cookie.getAInitialRelSeqTSN(),
                                      cookie.getAInitialControlTSN(),
                                      cookie.getAInitialRelUnseqID());
        _controlPacketQueue = new SequencedPacketQueue (cookie.getZInitialControlTSN());
        _relSeqPacketQueue = new SequencedPacketQueue (cookie.getZInitialRelSeqTSN());
        _unrelSeqPacketQueue = new SequencedPacketQueue (cookie.getZInitialUnrelSeqTSN());
        _relUnseqPacketQueue = new UnsequencedPacketQueue();
        _unrelUnseqPacketQueue = new UnsequencedPacketQueue();

        _transmitter = new MessageTransmitter (this, remoteWindowSize);
        _receiver = new MessageReceiver (this, _incomingValidation, DEFAULT_MAX_WINDOW_SIZE);
        _packetProcessor = new MessagePacketProcessor (this,
                                                       cookie.getZInitialControlTSN(),
                                                       cookie.getZInitialRelSeqTSN(),
                                                       cookie.getZInitialUnrelSeqTSN());
        _transmitter.setReceiver (_receiver);
        _receiver.setTransmitter (_transmitter);
        _packetProcessor.setReceiver (_receiver);
        _receiver.setPacketProcessor (_packetProcessor);
        _transmitter.start();
        _receiver.start();
        _packetProcessor.start();

        _connected = true;
    }

    /**
     * Internal method to query the number of bytes available (invoked by MocketInputStream)
     */
    public int bytesAvailable()
    {
        return _receivedPacketQueue.getSize();
    }

    public void enableCrossSequencing (boolean value)
    {
        _useCrossSequencing = value;
    }
    
    public boolean isCrossSequencingEnabled()
    {
        return _useCrossSequencing;
    }
    
    ACKManager getACKManager()
    {
        return _ackManager;
    }

    long getOutgoingValidation()
    {
        return _outgoingValidation;
    }

    long getInitialRelSeqTSN()
    {
        return _initialRelSeqTSN;
    }

    long getInitialUnrelSeqTSN()
    {
        return _initialUnrelSeqTSN;
    }
    
    long getInitialControlTSN()
    {
        return _initialControlTSN;
    }
        
    long getInitialRelUnseqID()
    {
        return _initialRelUnseqID;
    }
    
    long getInitialUnrelUnseqID()
    {
        return _initialUnrelUnseqID;
    }
    
    public static int getMaximumMTU()
    {
        return MAXIMUM_MTU;
    }
    
    // /////////////////////////////////////////////////////////////////////////
    // CLASS CONSTANTS /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    static final int MAXIMUM_MTU = 2048;               // Max Maximum Transmission Unit for message mockets packet
                                                       // (MTU includes header) 
    static final int DEFAULT_CONNECT_RECEIVE_TIMEOUT = 250; // Timeout to resend a SYN packet (in milliseconds)
    static final int DEFAULT_UDP_BUFFER_SIZE = 32768;  // Default UDP socket buffer size
    static final int DEFAULT_UDP_RECEIVE_TIMEOUT = 1200; // Default timeout placed on the underlying datagram socket
    static final long DEFAULT_COOKIE_LIFESPAN = 60000; // Default state cookie lifespan (in milliseconds)

    // changeable 
    static final int DEFAULT_CONNECT_TIMEOUT = 30000;  // Default timeout for connection establishment
    static final int DEFAULT_PENDING_PACKET_QUEUE_SIZE = 32768; // Default pending packet queue size (in bytes)
    static final int DEFAULT_KEEPALIVE_TIMEOUT = 1000; // Default maximum inactivity time after which 
                                                       // a Heartbeat packet is transmitted (in milliseconds)
    static final int DEFAULT_MTU = 1400;               // Default Maximum Transmission Unit for message mockets packet 
                                                       // (MTU includes header)
    static final boolean DEFAULT_CROSS_SEQUENCING_SETTING = true; // Default cross sequencing setting
    static final int DEFAULT_RETRANSMISSION_TIMEOUT = 1000; // Default RTO (in milliseconds)
    static final int DEFAULT_MAX_WINDOW_SIZE = 65535;  // Default maximum window size (in bytes)
    static final int DEFAULT_RECEIVE_TIMEOUT = -1;     // Default timeout on input operations performed 
                                                       // by the receive method (-1 means no timeout)
    /*
    static final int DEFAULT_SEND_TIMEOUT = -1;        // Default timeout on output operations performed 
                                                       // by the send method (-1 means no timeout)
    */

    static final int DELAYED_ACK_TIMEOUT = 200;        // Delayed ACK timeout (in milliseconds)
    
    // /////////////////////////////////////////////////////////////////////////
    // CLASS VARIABLES /////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////
    private MessageStateMachine _msm;                 // Maintains the state of this mocket
    private DatagramSocketWrapper _dgSocket;          // Underlying Datagram socket

    private InetAddress _remoteAddress;               // Remote address
    private int _remotePort;                          // Remote port

    private MessageReceiver _receiver;                // Receiver
    private MessageTransmitter _transmitter;          // Transmitter
    private MessagePacketProcessor _packetProcessor;  // PacketProcessor

    private long _connectTimeout;                     // Timeout for the connect method
    private int _pendingPacketQueueSize;              // Pending packet queue size
    private int _keepaliveTimeout;                    // Maximum inactivity time
    private int _mtu;                                 // Maximum Transmission Unit
    private boolean _useCrossSequencing;              // Cross sequencing setting
    private long _rto;                                // RTO
    private int _maxWindowSize;                       // Maximum window size
    private long _receiveTimeout;                     // Timeout for input operations (in milliseconds)

    private Logger _logger;                           // Logger
    private DatagramPacket _outgoingPacket;
    private PacketQueue _receivedPacketQueue;
    private PendingPacketQueue _pendingPacketQueue;
    private OutstandingPacketQueue _outstandingPacketQueue;
    private SequencedPacketQueue _controlPacketQueue;
    private SequencedPacketQueue _relSeqPacketQueue;
    private SequencedPacketQueue _unrelSeqPacketQueue;
    private UnsequencedPacketQueue _relUnseqPacketQueue;
    private UnsequencedPacketQueue _unrelUnseqPacketQueue;
    private boolean _connected;

    private long _incomingValidation;
    private long _outgoingValidation;

    // initial outgoing sequence numbers
    private long _initialRelSeqTSN;
    private long _initialUnrelSeqTSN;
    private long _initialControlTSN;
    private long _initialRelUnseqID;
    private long _initialUnrelUnseqID;
    
    private ACKManager _ackManager;

}
/*
 * vim: et ts=4 sw=4
 */

