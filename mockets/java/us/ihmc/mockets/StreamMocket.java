/*
 * StreamMocket.java
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 * 
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 * 
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * The Mocket class represents an endpoint of a mobile socket connection. The class is designed to be
 * a drop-in replacement for the Socket class in the Java Platform API.
 *
 * @author Niranjan Suri
 * @author Maggie Breedy
 * @author Enrico Casini (ecasini@ihmc.us)
 *
 * @version $Revision$
 */

package us.ihmc.mockets;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.net.*;

public class StreamMocket implements Serializable
{
    /**
     * Create a new, unconnected, endpoint for a mocket connection.
     */
    public StreamMocket ()
            throws IOException
    {
        this(null, null);
    }

    /**
     * Create a new <code>StreamMocket</code> and connects it to the specified endpoint address.
     *
     * @param host the host name, or <code>null</code> for the loopback address.
     * @param port the port number.
     */
    public StreamMocket (String host, int port) throws IOException
    {
        this(host != null ? new InetSocketAddress(host, port) : new InetSocketAddress(InetAddress.getByName(null),
                port), null);
    }

    /**
     * Create a new <code>StreamMocket</code> and connects it to the specified <code>InetAddress</code> endpoint
     * address.
     *
     * @param address the IP address.
     * @param port    the port number.
     */
    public StreamMocket (InetAddress address, int port) throws IOException
    {
        this(address != null ? new InetSocketAddress(address, port) : null, null);
    }

    /**
     * Creates a new <code>StreamMocket</code> and connects it to the specified remote host on
     * the specified remote port. The <code>StreamMocket</code> will also bind() to the local
     * address and port supplied.
     *
     * @param host      the name of the remote host, or <code>null</code> for the loopback address.
     * @param port      the remote port
     * @param localAddr the local address the socket is bound to
     * @param localPort the local port the socket is bound to
     * @throws IOException
     */
    public StreamMocket (String host, int port, InetAddress localAddr, int localPort) throws IOException
    {
        this(host != null ? new InetSocketAddress(host, port) :
                        new InetSocketAddress(InetAddress.getByName(null), port),
                new InetSocketAddress(localAddr, localPort));
    }

    /**
     * Creates a new <code>StreamMocket</code> and connects it to the specified remote host on
     * the specified remote port. The <code>StreamMocket</code> will also bind() to the local
     * address and port supplied.
     *
     * @param address   @param address the remote address
     * @param port      the remote port
     * @param localAddr the local address the socket is bound to
     * @param localPort the local port the socket is bound to
     * @throws IOException
     */
    public StreamMocket (InetAddress address, int port, InetAddress localAddr, int localPort) throws IOException
    {
        this(address != null ? new InetSocketAddress(address, port) : null,
                new InetSocketAddress(localAddr, localPort));

    }

    private StreamMocket (SocketAddress address, SocketAddress localAddr) throws IOException
    {
        init();

        try {
            if (localAddr != null)
                bind(localAddr);
            if (address != null)
                connect(address);
        }
        catch (IOException e) {
            closeSync();
            throw e;
        }
    }

    /**
     * Register a callback function to be invoked when no data (or keepalive) has
     * been received from the peer mocket.
     * The callback will indicate the time (in milliseconds) since last contact.
     * If the callback returns true, the mocket connection will be closed.
     * The peer is declared unreachable and the callback is invoked if no messages
     * are received from the peer for more than 2 seconds.
     * This waiting time is twice the default interval between keep-alive messages.
     *
     * @param msl callback function to register.
     */
    public void setStatusListener (MocketStatusListener msl)
    {
        if (msl == null) {
            return;
        }

        registerStatusListener(msl);
    }

    /**
     * Opens a connection to the specified remote <code>SocketAddress</code>
     *
     * @param remoteAddress the remote <code>SocketAddress</code> to connect to.
     * @param timeout       the timeout value to be used in milliseconds.
     * @throws java.io.IOException
     */
    public void connect (SocketAddress remoteAddress, int timeout)
            throws IOException
    {
        if (remoteAddress == null)
            throw new IllegalArgumentException("connect: The address can't be null");

        if (!(remoteAddress instanceof InetSocketAddress))
            throw new IllegalArgumentException("Unsupported address type");

        this.connect(((InetSocketAddress) remoteAddress).getAddress(), ((InetSocketAddress) remoteAddress).getPort(),
                timeout);
    }


    /**
     * Opens a connection to the specified remote <code>SocketAddress</code>
     *
     * @param remoteAddress the remote <code>SocketAddress</code> to connect to.
     * @throws java.io.IOException
     */
    public void connect (SocketAddress remoteAddress)
            throws IOException
    {
        if (remoteAddress == null)
            throw new IllegalArgumentException("connect: The address can't be null");

        if (!(remoteAddress instanceof InetSocketAddress))
            throw new IllegalArgumentException("Unsupported address type");

        this.connect(((InetSocketAddress) remoteAddress).getAddress(), ((InetSocketAddress) remoteAddress).getPort(),
                0);
    }

    /**
     * Opens a connection to the specified <code>InetAddress</code> remote address and port.
     *
     * @param remoteAddress the remote address to connect to. Specified as an IP address.
     * @param remotePort    the remote port to connect to.
     * @throws java.io.IOException
     */
    public void connect (InetAddress remoteAddress, int remotePort)
            throws IOException
    {
        this.connect(remoteAddress, remotePort, 0);
    }

    /**
     * Opens a connection to the specified remote address and port.
     * Allows to choose the connection attempt timeout.
     *
     * @param remoteAddress the remote address to connect to. Specified as an IP address.
     * @param remotePort    the remote port to connect to.
     * @param timeout       the timeout value to be used in milliseconds.
     * @throws IOException if there was a problem in opening the connection
     */
    public void connect (InetAddress remoteAddress, int remotePort, int timeout)
            throws IOException
    {
        if (remoteAddress == null)
            throw new IllegalArgumentException("connect: The address can't be null");

        if (remotePort < 0) {
            throw new IllegalArgumentException("port cannot be negative: " + remotePort);
        }
        if (timeout < 0) {
            throw new IllegalArgumentException("timeout cannot be negative: " + timeout);
        }

        if (isClosed())
            throw new IOException("StreamMocket is closed");

        if (isConnected())
            throw new IOException("StreamMocket already connected");

        connect(remoteAddress.getHostAddress(), remotePort, timeout);

        _connected = true;
        /*
         * If the StreamMocket was not bound before the connect, it is now because
         * the kernel will have picked an ephemeral port & a local address
         */
        _bound = true;
    }

    /**
     * Returns the local address as a socket address.
     *
     * @return local address.
     */
    public SocketAddress getLocalSocketAddress ()
    {
        return new InetSocketAddress(getLocalAddress(), getLocalPort());
    }

    /**
     * Returns the address of the remote peer as a socket address.
     *
     * @return address of the remote peer.
     */
    public SocketAddress getRemoteSocketAddress ()
    {
//        if (!isConnected())
//            return null;

        return new InetSocketAddress(getRemoteAddress(), getRemotePort());
    }

    /**
     * Binds a specific socket address (IP and port) to the local endpoint.
     * NOTE: This address will not be retained if the mocket is serialized.
     *
     * @param localSocketAddress the socket address to which the local endpoint should be bound
     * @throws IOException in case there is a problem with binding to the specified address
     */
    public void bind (SocketAddress localSocketAddress)
            throws IOException
    {
        if (isClosed())
            throw new IOException("StreamMocket is closed");
        if (isBound())
            throw new SocketException("Already bound");
        if (!(localSocketAddress instanceof InetSocketAddress))
            throw new IllegalArgumentException("Unsupported address type");

        InetSocketAddress ina = (InetSocketAddress) localSocketAddress;
        bind(ina.getAddress().getHostAddress(), ina.getPort());

        _bound = true;
    }

    /**
     * Returns the input stream for this connection.
     *
     * @return the input stream for the connection.
     */
    public InputStream getInputStream ()
    {
        if (_is == null) {
            _is = new MocketInputStream(this);
        }
        return _is;
    }

    /**
     * Returns the output stream for this connection.
     *
     * @return the output stream for the connection.
     */
    public OutputStream getOutputStream ()
    {
        if (_os == null) {
            _os = new MocketOutputStream(this);
        }

        return _os;
    }

    /**
     * Closes this <code>StreamMocket</code> in a synchronized way
     * <p/>
     * Once a <code>StreamMocket</code> has been closed, it is not available for further networking
     * use (i.e. can't be reconnected or rebound). A new <code>StreamMocket</code> needs to be
     * created.
     * <p/>
     * <p> Closing this socket will also close the <code>StreamMocket</code>'s
     * {@link java.io.InputStream InputStream} and
     * {@link java.io.OutputStream OutputStream}.
     * <p/>
     *
     * @throws IOException if an I/O error occurs when closing this socket.
     * @see #isClosed
     */
    @SuppressWarnings("deprecated")
    public synchronized void closeSync () throws IOException
    {

        synchronized (closeLock) {

            if (isClosed())
                return;

            close();
            _closed = true;
        }
    }

    /**
     * Returns the statistics associated with this mocket.
     *
     * @return the <code>Statistics</code> object for this mocket connection.
     */
    public Statistics getStatistics ()
    {
        return null;
    }

    /**
     * Sets the maximum time for buffering outgoing data before transmitting data.
     * Equivalent to TCP_NDELAY socket option.
     * Note that setting this to 0 generates a packet each time send() is called.
     * Accuracy of this setting depends on the period of the run() loop in the transmitter - currently 100ms.
     */
    /**
     * Sets the maximum time for buffering outgoing data before transmitting data.
     * Equivalent to TCP_NDELAY socket option.
     * Note that setting this to 0 generates a packet each time send() is called.
     * Accuracy of this setting depends on the period of the run() loop in the transmitter - currently 100ms.
     *
     * @param ms milliseconds data will wait in the buffer before transmission.
     */
    public native void setDataBufferingTime (int ms);


    /**
     * @return
     */
    public boolean isConnected ()
    {
        return _connected;
    }

    /**
     * @return
     */
    public boolean isClosed ()
    {
        synchronized (closeLock) {
            return _closed;
        }
    }

    public boolean isBound ()
    {
        return _bound;
    }

//    /**
//     * Removes <code>StreamMocket</code> object.
//     */
//    protected void finalize ()
//    {
//        dispose();
//    }

    /**
     * Returns the maximum time for buffering outgoing data before transmitting data.
     *
     * @return data buffering time in milliseconds.
     * @see StreamMocket#setDataBufferingTime(int)
     */
    public native int getDataBufferingTime ();

    /**
     * Closes the connection.
     *
     * @deprecated This method is deprecated in favor of closeSync()
     */
    public native void close ();

    private native void connect (String hostAddress, int portNumber, int timeout)
            throws IOException;

    private native String getLocalAddress ();

    private native int getLocalPort ();

    private native String getRemoteAddress ();

    private native int getRemotePort ();

    private native void init ();

    private native void dispose ();

    private native void bind (String address, int port)
            throws IOException;

    private native void registerStatusListener (MocketStatusListener msl);


    // /////////////////////////////////////////////////////////////////////////
    // INTERNAL CLASSES ////////////////////////////////////////////////////////
    // /////////////////////////////////////////////////////////////////////////

    /**
     * The Statistics class is used to retrieve statistics about the
     * current mocket connection.
     * Obtained by calling <code>getStatistics</code> on an instance of ServerMocket.
     */
    public class Statistics implements Serializable
    {
        public class Discards
        {
            public int getTotal ()
            {
                return _belowWindow + _noRoom + _overlap;
            }

            public int _belowWindow;
            public int _noRoom;
            public int _overlap;
        }

        public class Waits
        {
            public int _packetQueueFull;
            public int _remoteWindowFull;
        }

        public int getRetransmitCount ()
        {
            return _retransmits;
        }

        public int getSentPacketCount ()
        {
            return _sentPackets;
        }

        public int getSentByteCount ()
        {
            return _sentBytes;
        }

        public int getReceivedPacketCount ()
        {
            return _receivedPackets;
        }

        public int getReceivedByteCount ()
        {
            return _receivedBytes;
        }

        public Discards getDiscardedPacketCounts ()
        {
            return _discardedPackets;
        }

        public Waits getTransmitterWaitCounts ()
        {
            return _waits;
        }

        int _retransmits;
        int _sentPackets;
        int _sentBytes;
        int _receivedPackets;
        int _receivedBytes;

        Discards _discardedPackets = new Discards();
        Waits _waits = new Waits();
    }


    /**
     * Various states of this StreamMocket.
     */
    private boolean created = false;
    private boolean _bound = false;
    private boolean _connected = false;
    private boolean _closed = false;
    private final Object closeLock = new Object();

    private InputStream _is = null;
    private OutputStream _os = null;

    // /////////////////////////////////////////////////////////////////////////
    // field used by the JNI code. DO NOT rename or delete it.
    private long _mocket;

    // /////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary("mocketsjavawrapper");
    }
}
