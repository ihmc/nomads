/*
 * ServerSocketHandler.java
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

import java.net.URI;
import java.net.ServerSocket;
import java.net.InetAddress;
import java.net.Socket;

/**
 * ServerSocketHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.4 $
 *          Created on Apr 30, 2004 at 7:18:53 PM
 *          $Date: 2014/11/06 22:00:30 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class ServerSocketHandler extends Thread
{
    public ServerSocketHandler (int portNumber, MessageHandler msgHandler)
    {
        _portNumber = portNumber;
        _msgHandler = msgHandler;
        try {
            if (_portNumber <= 0) {
                _serverURI =  createServerSocket();
            }
            else {
                _serverURI =  createServerSocket(portNumber);
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
                Socket sock = _servSocket.accept();
                System.out.println ("Received Connection: " + sock.toString());
                _msgHandler.createConnHandler (sock);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }


    public void stopRunning()
    {
        try {
            _running = false;
            _servSocket.close();
        }
        catch (Exception e) {
        }
    }


    protected URI createServerSocket()
        throws Exception
    {
        int maxAttempts = 50;
        int attempts =0;
        int portNumber = _startingPortNumber;
        Exception ssException = null;
        URI serverURI = null;
        while (attempts < maxAttempts) {
            try {
                portNumber = _startingPortNumber + attempts;
                _servSocket = new ServerSocket (portNumber);
                InetAddress inetAdd = _servSocket.getInetAddress();
                serverURI = new URI("tcp://" + inetAdd.getHostAddress() + "/" + portNumber);
                return (serverURI);
            }
            catch (Exception e) {
                attempts ++;
                ssException = new Exception(e);
            }
            throw ssException;
        }
        return (serverURI);
    }

    protected URI createServerSocket(int fixedPortNumber)
        throws Exception
    {
        URI serverURI = null;
        _servSocket = new ServerSocket (fixedPortNumber);
        InetAddress inetAdd = _servSocket.getInetAddress();
        serverURI = new URI("tcp://" + inetAdd.getHostAddress() + "/" + fixedPortNumber);
        return (serverURI);
    }

    /**
     * Returns the current server URI
     * @return
     */
    public URI getServerURI()
    {
        return (_serverURI);
    }

    private URI _serverURI;
    private int _portNumber;
    private boolean _running;
    private ServerSocket _servSocket;
    private int _startingPortNumber = 4288;
    private MessageHandler _msgHandler;
}
