/*
 * SSLConnection.java
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

package us.ihmc.util.comm;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class SSLConnection
{

    public boolean connect (String hostName, String hostEnv)
    {
    	if (_internalState == UNINITIALIZED) {
            _remoteHostName = hostName;
            _remoteHostEnv = hostEnv;
    		return connectNative (hostName, hostEnv);
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
        return false;
    }
    
    public boolean getServerSocket (int hostEnv)
    {
        return getServerSocket (Integer.valueOf (hostEnv).toString());
    }

    public boolean getServerSocket (String hostEnv)
    {
        if (_internalState == UNINITIALIZED) {
            _localHostEnv = hostEnv;
            return establishServerNative (hostEnv);
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
       return false;
    }
    /**
     * Will invoke the native acceptSocket to spawn native SSL sockets for 
     * new incoming socket connections. The native accept will create a new
     * SSLConnection object, with the appropriate socket pointer and remote
     * hostname, and then return that. The caller would put this method in a 
     * while loop in order to maintain a persistent serversocket connection.
     * 
     * @return
     */
    public SSLConnection acceptSocket()
    {
        if (_internalState == SOCKET_BOUND || _internalState == ACCEPTING_CONNECTIONS) {
            return (SSLConnection) acceptSocketNative();
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
        return null;
    }
    
    public int sendLine (String line)
    {
        if (_internalState == CONNECTED) {
            return sendLineNative (line);
        }
        return -1;
    }
    
    public String recvLine ()
    {
        if (_internalState == CONNECTED) {
            return recvLineNative();
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
        return null;
    }
    
    public int sendBlock (byte[] block)
    {
        if (_internalState == CONNECTED) {
            return sendBlockNative (block);
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
        return 0;
    }
    
    public byte[] recvBlock ()
    {
        if (_internalState == CONNECTED) {
            return recvBlockNative ();
        }
        else {
            System.out.println ("SSLConnection is in Invalid State for attempted operation");
        }
        return null;
    }
    
    public void close ()
    {
        closeNative();
    }
    
    public InetAddress getInetAddress()
    {
        InetAddress add = null;
    	try {
			add = InetAddress.getByName (getRemoteHostName());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        return add;
    }
    
    public String getRemoteHostName()
    {
    	return _remoteHostName;
    }
    
    protected native boolean connectNative (String hostName, String hostEnv);
    protected native boolean establishServerNative (String hostEnv);
    protected native Object acceptSocketNative();
    protected native int sendLineNative (String line);
    protected native String recvLineNative ();
    protected native int sendBlockNative (byte[] block);
    protected native byte[] recvBlockNative ();
    protected native void closeNative ();
    
    private final static byte UNINITIALIZED = -1;
    private final static byte SOCKET_BOUND = 0;
    private final static byte ACCEPTING_CONNECTIONS = 1;
    private final static byte CONNECTED = 2;
    private byte _internalState = UNINITIALIZED;
    private int _socketPointer = 0;
    private String _remoteHostName;
    private String _remoteHostEnv;
    private String _localHostEnv;
}