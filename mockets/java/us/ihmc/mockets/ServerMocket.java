/*
 * ServerMocket.java
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
 
 * The main class for a server application to use the Mockets communication library.
 * Similar in functionality to a server socket - used by a server to accept connections
 * from client applications.
 *
 * @author Marco Bindini
 */


package us.ihmc.mockets;

import java.io.IOException;
import java.lang.IllegalArgumentException;

/**
 */
public class ServerMocket
{
    /*
     * Creates a new <code>ServerMocket</code>.
     */
    public ServerMocket()
        throws IOException
    {
        init (null);
    }

    /*
     * Creates a new <code>ServerMocket</code> and uses
     * the specified file to load the configuration for the ServerMocket
     * and any Mocket instances created for accepted connections.
     *
     * @param configFile the path to the configuration file that should be loaded
     */
    public ServerMocket (String configFile)
            throws IOException
    {
        init (configFile);
    }

    /*
     * Creates a new <code>ServerMocket</code> with DTLS and uses
     * the specified file to load the configuration for the ServerMocket
     * and any Mocket instances created for accepted connections.
     *
     * @param configFile the path to the configuration file that should be loaded
     * @param pathToCertificate the path to the certificate file that should be loaded
     * @param pathToPrivateKey the path to the Private Key file that should be loaded
     */
    public ServerMocket (String configFile, String pathToCertificate, String pathToPrivateKey)
            throws IOException
    {
        initDtls (configFile, pathToCertificate, pathToPrivateKey);
    }

    /**
     * Creates a new <code>ServerMocket</code> and binds it to the specified port on localhost
     * ready to accept incoming connections.
     * Specifying a value of 0 for the port causes a random port to be allocated.
     * 
     * @param port  integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public ServerMocket (int port)
        throws IOException, IllegalArgumentException
    {
        init (null);
        _port = listen (port);
        if ((port > 0) && (port != _port)) {
            throw new IOException ("failed to bind server socket to port " + port);
        }
    }

    /**
     * Creates a new <code>ServerMocket</code> and binds it to the specified address
     * ready to accept incoming connections.
     * Specifying a value of 0 for the port causes a random port to be allocated.
     * 
     * @param port          integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @param listenAddr    string value specifying the host's listening address. The hostname will be resolved.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public ServerMocket (int port, String listenAddr)
        throws IOException, IllegalArgumentException
    {
        init (null);
        _port = listenAddr(port, listenAddr);
        if ( (port > 0) && (port != _port) ) {
            throw new IOException ("failed to bind server socket to port " + port + " with address " + listenAddr);
        }
    }

    /**
     * Creates a new <code>ServerMocket</code> and binds it to the specified address
     * ready to accept incoming connections.
     * Specifying a value of 0 for the port causes a random port to be allocated.
     * Uses the specified file to load the configuration for the ServerMocket
     * and any Mocket instances created for accepted connections.
     * 
     * @param port          integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @param listenAddr    string value specifying the host's listening address. The hostname will be resolved.
     * @param configFile    the path to the configuration file that should be loaded
     *
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public ServerMocket (int port, String listenAddr, String configFile)
        throws IOException, IllegalArgumentException
    {
        init (configFile);
        _port = listenAddr(port, listenAddr);
        if ( (port > 0) && (port != _port) ) {
            throw new IOException ("failed to bind server socket to port " + port + " with address " + listenAddr);
        }
    }

    /**
     * Binds the server mocket to the specified port on localhost ready to accept incoming connections.
     * Specifying a value of 0 for the port causes a random port to be allocated.
     *
     * @param port  integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public void bind (int port)
        throws IOException, IllegalArgumentException
    {
        _port = listen (port);
        if ((port > 0) && (port != _port)) {
            throw new IOException ("failed to bind server socket to port " + port);
        }
    }

    /**
     * Binds the server mocket to the specified port on localhost ready to accept incoming connections.
     * Specifying a value of 0 for the port causes a random port to be allocated.
     *
     * @param port  integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public void bind (int port, String listenAddr)
        throws IOException, IllegalArgumentException
    {
        _port = listenAddr (port, listenAddr);
        if ((port > 0) && (port != _port)) {
            throw new IOException ("failed to bind server socket to port " + port);
        }
    }

    /**
     * Return the port where the <code>ServerMocket</code> is listening for connections.
     * 
     * @return  port on which this <code>ServerMocket</code> is listening.
     */
    public int getLocalPort()
    {
        return _port;
    }

//    /**
//     * Removes the <code>ServerMocket</code> object.
//     */
//    protected void finalize()
//    {
//        dispose();
//    }

    /**
     * Listens for a connection to be made and accepts it.
     * A new <code>Mocket</code> is created.
     * 
     * @return  the new <code>mocket</code> created;
     *          <code>NULL</code> if an error occurred.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public Mocket accept() throws IOException, IllegalArgumentException
    {
        return acceptNoPort();
    }

    /**
     * When the network has a firewall this method is convenient to specify the desired
     * port where the low level socket will be bound once the connection starts.
     * 
     * @param portForNewConnection
     * @return
     * @throws IOException
     * @throws IllegalArgumentException
     */
    public Mocket accept (int portForNewConnection)  throws IOException, IllegalArgumentException
    {
        return acceptWPort (portForNewConnection);
    }

    /**
     * Closes the current open connection to a remote endpoint.
     * 
     * @return  <code>0</code>
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    public native int close() throws IOException, IllegalArgumentException;

    /**
     * Native method that removes the <code>ServerMocket</code> object.
     */
    private native void dispose();

    /**
     * Native method that initializes an object <code>ServerMocket</code>.
     * 
     * @param configFile    the path to the configuration file that should be loaded
     *
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    private native void init (String configFile) throws IOException, IllegalArgumentException;

    /**
     * Native method that initializes an object <code>ServerMocket DTLS</code>.
     *
     * @param configFile    the path to the configuration file that should be loaded
     * @param pathToCertificate
     * @param pathToPrivateKey    
     *
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    private native void initDtls (String configFile, String pathToCertificate, String pathToPrivateKey) throws IOException, IllegalArgumentException;

    /**
     * Native method that initializes the server mocket to accept incoming connections on the specified port.
     * Specifying a 0 for the port causes a random port to be allocated.
     * 
     * @param port  integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @return      the port number that was assigned;
     *              <code>&#60;0</code> if an error occurred.
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    private native int listen (int port) throws IOException, IllegalArgumentException;

    /**
     * Native method that initializes the server mocket to accept incoming connections on the specified port and address.
     * Specifying a 0 for the port causes a random port to be allocated.
     * 
     * @param port          integer value specifying the listening port. A value of 0 causes a random port to be allocated.
     * @param listenerAddr  string value specifying the host's listening address. The hostname will be resolved.
     * @return
     * @throws java.io.IOException
     * @throws java.lang.IllegalArgumentException
     */
    private native int listenAddr (int port, String listenerAddr) throws IOException, IllegalArgumentException;

    private native Mocket acceptNoPort() throws IOException, IllegalArgumentException;

    private native Mocket acceptWPort (int port) throws IOException, IllegalArgumentException;

    /**
     * Sets a string to use as the application or user friendly identifier for this mocket instance.
     * The identifier is used when sending out statistics and when logging information.
     * Some suggestions include the name of the application, the purpose for this mocket, etc.
     * May be set to NULL to clear a previously set identifier.
     * NOTE: The string is copied internally, so the caller does not need to preserve the string
     * 
     * @param identifier    string to be used to identify this mocket instance.
     */
    public native void setIdentifier (String identifier);
    
    /**
     * Returns the identifier for this mocket instance.
     * 
     * @return  identifier of this mocket instance;
     *          <code>NULL</code> if no identifier is set.
     * @see ServerMocket#setIdentifier(java.lang.String) 
     */
    public native String getIdentifier();

    // /////////////////////////////////////////////////////////////////////////
    private long _serverMocket;
    private int _port;

    // ////////////////////////////////////////////////////////////////////////
    static {
        System.loadLibrary ("mocketsjavawrapper");
    }
}
