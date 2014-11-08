/*
 * ServerMocketHandler.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

package us.ihmc.ds.fgraph.comm;

import us.ihmc.mockets.ServerMocket;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.MocketStatusListener;
import us.ihmc.util.ConfigLoader;

import java.net.URI;
import java.net.InetAddress;

/**
 * ServerMocketHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.9 $
 *          Created on Apr 30, 2004 at 7:18:53 PM
 *          $Date: 2014/11/06 22:00:30 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class ServerMocketHandler extends Thread implements MocketStatusListener
{
    public ServerMocketHandler (int portNumber, MessageHandler msgHandler)
    {
        ConfigLoader cl = ConfigLoader.getDefaultConfigLoader();
        _serverMocketTimeout = cl.getPropertyAsInt("fgraph.server.mocket.timeout", _serverMocketTimeout);
        System.out.println("Server Mocket Timeout is: " + _serverMocketTimeout);
        _portNumber = portNumber;
        _msgHandler = msgHandler;
        try {
            if (_portNumber <= 0) {
                _serverMocketURI = createServerMocket();
            }
            else {
                _serverMocketURI = createServerMocket(portNumber);
            }
            _running = true;
            start();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void run()
    {
        while (_running) {
            try {
                Mocket mocket = _servMocket.accept();
                System.out.println ("Received Connection: " + mocket.toString());
                mocket.addMocketStatusListener (this);
                _msgHandler.createConnHandler (mocket);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public boolean peerUnreachableWarning (long timeSinceLastContact)
    {
        if (timeSinceLastContact > _serverMocketTimeout) {
            System.out.println ("Client inactive for " + timeSinceLastContact + " ms; dropping connection");
            return true;
        }
        return false;
    }

    public void stopRunning()
    {
        try {
            _running = false;
            _servMocket.close();
        }
        catch (Exception e) {
        }
    }

    protected URI createServerMocket()
            throws Exception
    {
        int maxAttempts = 50;
        int attempts =0;
        int portNumber = _startingPortNumber;
        Exception ssException = null;
        URI _serverMocketURI = null;
        while (attempts < maxAttempts) {
            try {
                portNumber = _startingPortNumber + attempts;
                _servMocket = new ServerMocket (portNumber);
                _serverMocketURI = new URI("rudp://" + InetAddress.getLocalHost().getHostAddress() + "/" +
                                           _servMocket.getLocalPort());
                return (_serverMocketURI);
            }
            catch (Exception e) {
                attempts ++;
                ssException = new Exception(e);
            }
            throw ssException;
        }
        return (_serverMocketURI);
    }

    protected URI createServerMocket(int fixedPortNumber)
        throws Exception
    {
        URI _serverMocketURI = null;
        _servMocket = new ServerMocket (fixedPortNumber);
        _serverMocketURI = new URI("rudp://" + InetAddress.getLocalHost().getHostAddress() + "/" +
                                   _servMocket.getLocalPort());
        return (_serverMocketURI);
    }

    /**
     * Returns the current server URI
     * @return
     */
    public URI getServerURI()
    {
        return (_serverMocketURI);
    }

    private int _serverMocketTimeout = 180000;
    private URI _serverMocketURI;
    private int _portNumber;
    private boolean _running;
    private ServerMocket _servMocket;
    private int _startingPortNumber = 4288;
    private MessageHandler _msgHandler;
}
