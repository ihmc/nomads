/**
 * SocketFactory.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * This interface defines a SocketFactory 
 * There is a current socket factory definition called javax.net.SocketFactory
 * but that factory doesn't have methods to create sockets using a timeout 
 * when connecting to the endpoint address and that class is not an interface, 
 * it is an abstract class 
 *
 * @author      Adrian Granados <agranados@ai.uwf.edu>
 *
 * @version     $Revision: 1.4 $
 *              $Date: 2016/06/09 20:02:46 $
 *
 * Created on October 17, 2003, 11:00 AM.
 */

package us.ihmc.net;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;


public interface SocketFactory 
{
    /**
     * Creates a socket and connects it to the specified port number at the 
     * specified address.
     *
     * @param host - the server address
     * @param port - the server port
     *
     * @return the Socket
     *
     * @throw IOException - if an I/O error occurs when creating the socket
     *
     */    
    public Socket createSocket (InetAddress address, int port)
        throws IOException;
    
    /**
     * Creates a socket and connects it to the specified remote host at the specified remote port 
     * using the defaul proxy.
     * This socket is configured using the socket options established for this factory.
     *
     * @param host - the server host
     * @param port - the server port
     * 
     * @return the Socket 
     *
     * @throw IOException - if an I/O error occurs when creating the socket 
     * @throw UnknownHostException - if the host is not known
     *
     */ 
    public Socket createSocket (String host, int port)
        throws IOException, UnknownHostException;
    
    /**
     * Creates a socket and connects it to the specified port number at the 
     * specified address.
     *
     * @param address - the server address
     * @param port - the server port
     * @param timeout - the connection timeout in milliseconds
     *
     * @return the Socket
     *
     * @throw IOException - if an I/O error occurs when creating the socket
     *
     */  
    public Socket createSocket (InetAddress address, int port, InetAddress localAddress, int localPort)
        throws IOException;
    
    /**
     * Creates a socket and connects it to the specified remote host at the specified remote port 
     * using the defaul proxy.
     * This socket is configured using the socket options established for this factory.
     *
     * @param host - the server host
     * @param port - the server port
     * @param timeout - the connection timeout in milliseconds
     * 
     * @return the Socket 
     *
     * @throw IOException - if an I/O error occurs when creating the socket 
     * @throw UnknownHostException - if the host is not known
     *
     */
    public Socket createSocket (String host, int port, InetAddress localAddress, int localPort)
        throws IOException, UnknownHostException;
}
