/*
 * StreamServerMocket.java
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
 * The ServerMocket class represents an endpoint for mockets that is capable
 * of receiving incoming connections. This class is designed to be a drop-in
 * replacement for the Socket class in the Java Platform API.
 *
 * @author Niranjan Suri
 * @author Maggie Breedy
 * @author Marco Arguedas
 * @version $Revision: 1.11 $
 */

package us.ihmc.mockets;

import java.io.IOException;

import java.net.InetSocketAddress;
import java.net.SocketAddress;

public class StreamServerMocket
{
    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified SocketAddress.
     *
     * @param sockAddress      the address to use.
     *
     * @exception IOException     if the specified port is in use.
     */
    public StreamServerMocket (SocketAddress sockAddress)
        throws IOException
    {
        InetSocketAddress ina = (InetSocketAddress) sockAddress;
        _listenAddress = ina.getAddress().getHostAddress();
        _listenPort = ina.getPort();
        init();

        listen();
    }

    /**
     * Construct a new server mocket that listens for incoming connections on
     * the specified port.
     *
     * @param port      the port number to use.
     *
     * @exception IOException     if the specified port is in use.
     */
    public StreamServerMocket (int port)
        throws IOException
    {
        _listenPort = port;
        init();

        listen();
    }

    /**
     * Returns the port on which this socket is listening.
     */
    public int getLocalPort()
    {
        return _listenPort;
    }

//    /**
//     *
//     */
//    protected void finalize()
//    {
//        dispose();
//    }

    /**
     * Accept a new connection and return an instance of Mocket that represents
     * the local endpoint for the new connection. The method will block until a
     * new connection is received.
     *
     * @return a new Mocket instance that represents the new connection
     *
     * @exception IOException if there was a problem in accepting the connection
     */
    public native StreamMocket accept() throws IOException;

    /**
     *  Close the server mocket and no longer accept new incoming connections.
     */
    public native void close();
    private native void listen() throws IOException;
    private native void dispose();
    private native void init();

    // /////////////////////////////////////////////////////////////////////////

    // this field is used in the JNI code. DO NOT delete or rename it.
    private String _listenAddress = null;

    // this field is used in the JNI code. DO NOT delete or rename it.
    private int _listenPort = -1;

    // this field is used in the JNI code. DO NOT delete or rename it.
    private long _serverMocket = 0;

    // /////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary("mocketsjavawrapper");
    }

}
