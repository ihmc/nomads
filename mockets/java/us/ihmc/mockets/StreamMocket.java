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
 *
 * @version $Revision: 1.15 $
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
            throws SocketException
    {
        init();
    }

    /**
     * Create a new <code>StreamMocket</code> and connects it to the endpoint address.
     */
    public StreamMocket (String host, int port) throws IOException
    {
        init();
        connect(InetAddress.getByName(host), port);
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
     * Opens a connection to the specified remote address and port.
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
     * @throws IOException if there was a problem in opening the connection
     */
    public void connect (InetAddress remoteAddress, int remotePort, int connectTimeout)
            throws IOException
    {
        if (remoteAddress == null) {
            throw new NullPointerException("remoteAddress");
        }
        if (remotePort < 0) {
            throw new IllegalArgumentException("port cannot be negative: " + remotePort);
        }
        if (connectTimeout < 0) {
            throw new IllegalArgumentException("connectTimeout cannot be negative: " + connectTimeout);
        }

        connect(remoteAddress.getHostAddress(), remotePort, connectTimeout);
    }

    /**
     * Returns the local address as a socket address.
     *
     * @return local address.
     */
    public SocketAddress getLocalSocketAddress ()
    {
        SocketAddress sockAddr = new InetSocketAddress(getLocalAddress(), getLocalPort());
        return sockAddr;
    }

    /**
     * Returns the address of the remote peer as a socket address.
     *
     * @return address of the remote peer.
     */
    public SocketAddress getRemoteSocketAddress ()
    {
        SocketAddress sockAddr = new InetSocketAddress(getRemoteAddress(), getRemotePort());
        return sockAddr;
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
        InetSocketAddress ina = (InetSocketAddress) localSocketAddress;
        bind(ina.getAddress().getHostAddress(), ina.getPort());
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
     * Returns the maximum time for buffering outgoing data before transmitting data.
     *
     * @return data buffering time in milliseconds.
     * @see StreamMocket#setDataBufferingTime(int)
     */
    public native int getDataBufferingTime ();

    /**
     * @return
     */
    public boolean isConnected ()
    {
        throw new UnsupportedOperationException("isConnected() is not implemented.");
    }

    /**
     * @return
     */
    public boolean isClosed ()
    {
        throw new UnsupportedOperationException("isClosed() is not implemented.");
    }

    /**
     * Removes <code>StreamMocket</code> object.
     */
    protected void finalize ()
    {
        dispose();
    }

    /**
     * Closes the connection.
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
