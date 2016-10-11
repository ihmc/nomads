/*
 * DatagramPacketMessage.java
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
 */

package us.ihmc.net;

import java.io.Serializable;
import java.net.InetAddress;

/*******************************************************
 * @author Marco Carvalho <mcarvalho@ai.uwf.edu>
 * @version $Revision: 1.4 $
 * $Date: 2016/06/09 20:02:46 $
 * 
 * DatagramPacketMessage 
 *****************************************************/
public class DatagramPacketMessage implements Serializable
{

    public int getLength()
    {
        return (_length);
    }
    
    public void setLength (int len)
    {
        _length = len;
    }
    
    public int getPort()
    {
        return (_port);
    }
    
    public void setPort (int port)
    {
        _port = port;
    }
    
    public InetAddress getInetAddress()
    {
        return (_inetAdd);
    }
    
    public void setInetAddress (InetAddress inetAdd)
    {
        _inetAdd = inetAdd;
    }
    
    public byte[] getData()
    {
        return (_data);
    }
    
    public void setData (byte[] data)
    {
        _data = data;
    }
    
    private int _length;
    private int _port;
    private InetAddress _inetAdd;
    private byte[] _data;
}

