package us.ihmc.mockets;

import java.io.IOException;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;
//import java.net.SocketTimeoutException;
//import java.util.Random;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.logging.Logger;

public class MessageServerMocket
{
    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified SocketAddress.
     *
     * @param port The port number to use.
     *
     * @exception SocketException If the specified port is in use.
     */
    public MessageServerMocket (SocketAddress sockAddress)
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _connections = new Hashtable();
        _lastConnectionsCheck = System.currentTimeMillis();

        try {
            _serverSocket = new DatagramSocketWrapper (sockAddress);
            _port = _serverSocket.getLocalPort();
        }
        catch (IOException ex) {
            throw new SocketException (ex.getMessage());
        }
    }

    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified port.
     *
     * @param port The port number to use.
     *
     * @exception SocketException If the specified port is in use.
     */
    public MessageServerMocket (int port)
        throws SocketException
    {
        _logger = Logger.getLogger ("us.ihmc.mockets");
        _connections = new Hashtable();
        _lastConnectionsCheck = System.currentTimeMillis();
        
        try {
            _serverSocket = new DatagramSocketWrapper (port);

            // read the port number out of the Socket once it has been initialized
            // because if the port given by whoever call the constructor was 0,
            // the Socket chose a random port number.
            _port = _serverSocket.getLocalPort();
        }
        catch (IOException ex) {
            throw new SocketException (ex.getMessage());
        }
    }

    /**
     * Accept a new connection and return an instance of Mocket that represents
     * the local endpoint for the new connection. The method will block until a
     * new connection is received.
     *
     * @return A new Mocket instance that represents the new connection.
     *
     * @exception SocketException If there was a problem in accepting the connection.
     */
    public MessageMocket accept()
        throws SocketException
    {
        boolean done = false;
        long start = System.currentTimeMillis();
        MessageMocket mocket = null;
        DatagramPacket dpIn = new DatagramPacket (new byte [MessageMocket.MAXIMUM_MTU], 
                                                  MessageMocket.MAXIMUM_MTU);

        _serverSocket.setReceiveBufferSize (MessageMocket.DEFAULT_UDP_BUFFER_SIZE);
        // in case we want to implement a timeout on accept
        /*
        _serverSocket.setSoTimeout (timeout);
        */

        _logger.fine ("_serverSocket.getReceiveBufferSize() : " + _serverSocket.getReceiveBufferSize());
        _logger.fine ("_serverSocket.getSendBufferSize() : " + _serverSocket.getSendBufferSize());

        while (!done) {
            if (_lastConnectionsCheck + CONNECTIONS_CHECK_TIMEOUT >  System.currentTimeMillis()) {
                /* check connections to see if they are still valids and remove closed connections */
                for (Enumeration e = _connections.elements(); e.hasMoreElements(); ) {
                    ConnectionInfo ci = (ConnectionInfo) e.nextElement();
                    MessageMocket m = ci.getMocket();
                    if (m.isClosed()) 
                        _connections.remove (ci);
                }
                _lastConnectionsCheck = System.currentTimeMillis();
            }
            
            try {
                _serverSocket.receive (dpIn);
            }            
            // in case we want to implement a timeout on accept
            /*
            catch (SocketTimeoutException e) {
                long currTime = System.currentTimeMillis();
                if (currTime - start >= timeout) {
                    throw new SocketException ("Timeout exceedeed.");
                } else {
                    _serverSocket.setSoTimeout (timeout - (currTime - start));
                    continue;
                }
            }
            */
            catch (IOException e) {
                _logger.severe ("Problem while reading a packet from the " + 
                                "datagram socket: " + e.getMessage());
                throw new SocketException (e.getMessage());
            }

            if (dpIn.getLength() < MessagePacket.HEADER_SIZE) {
                // discard packet as it is too small to be valid
                _logger.info ("Received a short packet of size " + 
                              dpIn.getLength() + ": discarding it...");
                continue;
            } else {
                _logger.fine ("Received a packet of size " + dpIn.getLength());
            }

            MessagePacket pIn = new MessagePacket (dpIn.getData(), dpIn.getLength());

            int chunksProcessed = 0;
            boolean foundStateChangeChunk = false;
            InetSocketAddress endpoint = new InetSocketAddress (dpIn.getAddress(), dpIn.getPort());
            
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
                    case Chunk.CHUNK_TYPE_INIT:
                        assert (chk instanceof InitChunk);
                        InitChunk initChk = (InitChunk) chk;
                        receivedInitChunk (initChk, endpoint);
                        ++chunksProcessed;
                        break;
                    case Chunk.CHUNK_TYPE_COOKIE_ECHO:
                        assert (chk instanceof CookieEchoChunk);
                        CookieEchoChunk ceChk = (CookieEchoChunk) chk;
                        StateCookie cookie = ceChk.getCookie();
                        ConnectionInfo connInfo = (ConnectionInfo) _connections.get (endpoint);
                        MessageMocket mk = null;
                        if (connInfo != null &&
                            cookie.isEqual(connInfo.getCookie())) {
                            /* just send the COOKIE-ACK message again */
                            mk = connInfo.getMocket(); 
                        } else {
                            /* create new mocket and send the COOKIE-ACK message */
                            mk = receivedCookieEchoChunk (cookie, pIn.getWindowSize(), endpoint);
                            mocket = mk;
                            _connections.put (endpoint, new ConnectionInfo(mocket, cookie));
                            done = true;
                        }
                        InetSocketAddress local = (InetSocketAddress) mk.getLocalName();
                        CookieAckChunk ackChk = new CookieAckChunk (local.getPort());
                        sendMessage (ackChk, cookie.getZValidation(), endpoint);
                        ++chunksProcessed;
                        break;
                    default:
                        _logger.warning ("Packet has an unknown/unexpected chunk type!");
                }

                assert chunksProcessed > 0;
            }
        }

        assert mocket != null;
        
        return mocket;
    }    

    private void sendMessage (Chunk chk,
                              long outgoingValidation,
                              InetSocketAddress remoteEndpoint) 
    {
        DatagramPacket outgoingPacket = new DatagramPacket (new byte[0], 0);
        MessagePacket p = new MessagePacket();
        p.setWindowSize (MessageMocket.DEFAULT_MAX_WINDOW_SIZE);
        p.setValidation (outgoingValidation);
        p.setSequenceNumber (0);
        if (!p.addChunk(chk)) {
            // this should never happen
            _logger.severe ("Chunk size bigger than packet size.");
            System.exit (1);
        }
        p.writeToDatagramPacket (outgoingPacket);

        outgoingPacket.setAddress (remoteEndpoint.getAddress());
        outgoingPacket.setPort (remoteEndpoint.getPort());
                                            
        // send packet over the socket
        try {
            _serverSocket.send (outgoingPacket);
        }
        catch (IOException e) {        
            e.printStackTrace();
            System.exit (1);
        }
    }
        
    private void receivedInitChunk (InitChunk chk, 
                                    InetSocketAddress remoteEndpoint)
    {
        // TODO: after debugging, use random values to initialize outgoing validation
        // and TSNs by using something like:
        /*
        long outgoingValidation;
        Random generator = new Random (System.currentTimeMillis());
        while (outgoingValidation == 0) {
            outgoingValidation = generator.nextLong();
        }
        long initialRelSeqTSN = generator.nextLong();
        long initialUnrelSeqTSN = generator.nextLong();
        long initialControlTSN = generator.nextLong();
        long initialRelUnseqID = generator.nextLong();
        long initialUnrelUnseqID = generator.nextLong();
        */
        long outgoingValidation = 2;
        long initialRelSeqTSN = 0;
        long initialUnrelSeqTSN = 0;
        long initialControlTSN = 0;
        long initialRelUnseqID = 0;
        long initialUnrelUnseqID = 0;
        
        StateCookie cookie = new StateCookie (System.currentTimeMillis(), 
                                              MessageMocket.DEFAULT_COOKIE_LIFESPAN,
                                              chk.getValidation(), outgoingValidation,
                                              chk.getInitialRelSeqTSN(), initialRelSeqTSN,
                                              chk.getInitialUnrelSeqTSN(), initialUnrelSeqTSN,
                                              chk.getInitialControlTSN(), initialControlTSN,
                                              chk.getInitialRelUnseqID(), initialRelUnseqID,
                                              chk.getInitialUnrelUnseqID(), initialUnrelUnseqID,
                                              remoteEndpoint.getPort(), _port);
        InitAckChunk ackChk = new InitAckChunk (outgoingValidation, initialRelSeqTSN, initialUnrelSeqTSN,
                                                initialControlTSN, initialRelUnseqID, initialUnrelUnseqID,
                                                cookie);
        
        sendMessage (ackChk, outgoingValidation, remoteEndpoint);
    }
    
    private MessageMocket receivedCookieEchoChunk (StateCookie cookie, 
                                                   int windowSize,
                                                   InetSocketAddress remoteEndpoint)
        throws SocketException
    {
        // create new socket
        InetSocketAddress addr = new InetSocketAddress (_serverSocket.getLocalAddress(), 0);
        DatagramSocketWrapper socket = new DatagramSocketWrapper (addr);
        
        MessageMocket mocket = null;
        try {
            mocket = new MessageMocket (cookie, windowSize, remoteEndpoint.getAddress(), socket);
        }
        catch (SocketException e) {
            e.printStackTrace();
            System.exit (1);
        }
        return mocket;
    }
    
    /**
     * Returns the port on which this socket is listening.
     */
    public int getLocalPort()
    {
        return _port;
    }

    /**
     *  Close the server mocket and no longer accept new incoming connections.
     */
    public void close()
    {
        _serverSocket.close();
    }

    private class ConnectionInfo 
    {
        ConnectionInfo (MessageMocket mocket, StateCookie cookie) 
        {
            _mocket = mocket;
            _cookie = cookie;
        }

        MessageMocket getMocket() 
        {
            return _mocket;
        }

        StateCookie getCookie() 
        {
            return _cookie;
        }
        
        private MessageMocket _mocket;
        private StateCookie _cookie;
    }

    // accept timeout is 60 seconds
    private static long ACCEPT_TIMEOUT = 60000;
    // connections check timeout is 30 seconds
    private static long CONNECTIONS_CHECK_TIMEOUT = 30000;

    private DatagramSocketWrapper _serverSocket;
    private int _port;
    private long _lastConnectionsCheck;
    private Hashtable _connections;
    private Logger _logger;
}

/*
 * vim: et ts=4 sw=4
 */
