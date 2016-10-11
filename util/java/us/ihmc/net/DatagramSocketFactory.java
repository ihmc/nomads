/*
 * DatagramSocketFactory.java
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
 * Created on April 7, 2003, 5:53 PM
 */

package us.ihmc.net;

import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;

/**
 * This class is a factory of Datagram Sockets
 *
 * @author  agranados@ai.uwf.edu
 */
public interface DatagramSocketFactory {
     
    /** 
     * Constructs a datagram socket and binds it to any available port on the 
     * local host machine. The socket will be bound to the wildcard address, 
     * an IP address chosen by the kernel.
     *
     * @return the Datagram socket
     */
    public DatagramSocket createDatagramSocket() 
        throws SocketException;
    
    /** 
     * Constructs a datagram socket and binds it to the specified port on the 
     * local host machine. The socket will be bound to the wildcard address, 
     * an IP address chosen by the kernel. 
     * 
     * @param port - port to use.
     *
     * @return the Datagram socket
     */
    public DatagramSocket createDatagramSocket (int port) 
        throws SocketException;
    
    /**
     * Creates a datagram socket, bound to the specified local address. 
     * The local port must be between 0 and 65535 inclusive. 
     * If the IP address is 0.0.0.0, the socket will be bound to the wildcard 
     * address, an IP address chosen by the kernel.
     *
     * @param port - port to use
     * @param laddr - local address to bind
     *
     * @return the Datagram socket          
     */
    public DatagramSocket createDatagramSocket (int port, InetAddress laddr) 
        throws SocketException;   
}
