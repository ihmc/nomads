/*
 * DatagramSocketFactoryImpl.java
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
 * Created on April 7, 2003, 6:02 PM
 */

package us.ihmc.net;

import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;

/**
 * This class is the default implementation of the DatagramSocketFactory interface
 * 
 * @author  agranados@ai.uwf.edu
 */
public class DatagramSocketFactoryImpl implements DatagramSocketFactory
{        
    public DatagramSocket createDatagramSocket() 
        throws SocketException 
    {
        return new DatagramSocket();
    }
    
    public DatagramSocket createDatagramSocket(int port) 
        throws SocketException 
    {
        return new DatagramSocket(port);
    }
    
    public DatagramSocket createDatagramSocket(int port, InetAddress laddr) 
        throws SocketException 
    {
        return new DatagramSocket(port, laddr);
    }    
}
