/*
 * SocketFactoryImpl.java
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
 * This class is the default implementation of the SocketFactory interface
 *
 * @author      Adrian Granados <agranados@ai.uwf.edu>
 *
 * @version     $Revision: 1.5 $
 *              $Date: 2016/06/09 20:02:46 $
 *
 * Created on October 17, 2003, 11:30 AM.
 */

package us.ihmc.net;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

public class SocketFactoryImpl implements SocketFactory
{        
    public SocketFactoryImpl()
    {
    }        
        
    public Socket createSocket(InetAddress host, int port) 
        throws IOException 
    {
        return new Socket(host, port);
    }
    
    public Socket createSocket(String host, int port) 
        throws IOException, UnknownHostException 
    {
        return new Socket(host, port);
    }
    
    
	public Socket createSocket(InetAddress host, int port, InetAddress localAddress, int localPort) 
        throws IOException 
    {
        return new Socket (host, port, localAddress, localPort);
    }
    
    public Socket createSocket(String host, int port, InetAddress localAddress, int localPort) 
        throws IOException, UnknownHostException 
    {
        return new Socket (host, port, localAddress, localPort);
    }    
}
