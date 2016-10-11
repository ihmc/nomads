/*
 * FGraphClientProxy.java
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

package us.ihmc.ds.fgraph;

import java.net.URI;
import java.net.Socket;
import java.net.ServerSocket;
import java.util.Hashtable;
import java.io.*;

/**
 * Created by IntelliJ IDEA.
 * User: mcarvalho
 * Date: Dec 18, 2004
 * Time: 4:16:46 PM
 * To change this template use File | Settings | File Templates.
 */
public class FGraphClientProxy extends Thread implements FGraphEventListener
{
    FGraphClientProxy (URI remoteURI, int localServerPort)
            throws Exception
    {
        _handlers = new Hashtable();
        connectToServer (remoteURI);
        startServer (localServerPort);
    }

    private void connectToServer (URI remoteURI)
            throws Exception
    {
        _fgraph = (FGraphClient) FGraph.getClient(remoteURI);
    }

    /**
     * Creates instance of an FGraphClient
     *
     * @param remoteURI
     * @param localServerPort
     * @return A handle to an FGraphClient
     * @throws Exception
     */
    public synchronized static FGraphClientProxy getInstance(URI remoteURI, int localServerPort)
            throws Exception
    {
        if (_fgClientProxy == null) {
            _fgClientProxy = new FGraphClientProxy (remoteURI, localServerPort);
        }
        return (_fgClientProxy);
    }

    public void startServer (int localServerPort)
            throws IOException
    {
        _serverSocket = new ServerSocket (localServerPort);
        debugMsg ("Listening on port " + localServerPort);
    }

    public void run()
    {
        while (_running) {
            try {
                Socket sock = _serverSocket.accept();
                FGCProxyConnHandler fgcConnHandler = new FGCProxyConnHandler (sock, this);
                _handlers.put(fgcConnHandler.getName(), fgcConnHandler);
                fgcConnHandler.start();
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void terminate ()
    {
        _running = false;
        try {
            if (_serverSocket != null) {
                _serverSocket.close();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void vertexAdded(String vertexID)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexRemoved(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribSet(String vertexID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribListSet(String vertexID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void vertexAttribRemoved(String vertexID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAdded(String edgeID, String sourceID, String destID)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeRemoved(String edgeID, String sourceID, String destID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribSet(String edgeID, String attKey, Object attribute)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribListSet(String edgeID, Hashtable attributes)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void edgeAttribRemoved(String edgeID, String attKey)
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connectionLost()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void connected()
    {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    //////////// Proxy ConnHandler - Handles Comms with a client (TCP)//////////////
    public class FGCProxyConnHandler extends Thread
    {
        public FGCProxyConnHandler (Socket sock, FGraphClientProxy fgProxy)
                throws IOException
        {
            _socket = sock;
            _fgProxy = fgProxy;
            _name = _socket.getInetAddress().getHostAddress() + ":" + _socket.getPort();
            _br = new BufferedReader(new InputStreamReader (_socket.getInputStream()));
            _bw = new BufferedWriter(new OutputStreamWriter (_socket.getOutputStream()));
        }

        public String getConnHandlerName ()
        {
            return (_name);
        }


        public synchronized void notify (String message)
                throws IOException
        {
            //this is a problem here... if the C client on the other side of the socket
            //stop reading, the output buffer here will get full and we will block.
            //TODO - This must be moved to a separate thread.
            _bw.write(message);
            _bw.newLine();
            _bw.flush();
        }

        public void terminate ()
        {
            _running = false;
            try {
                _socket.close();
            }
            catch (Exception e) {
            }
        }

        public void run()
        {
            while (_running) {
                try {
                    String request = _br.readLine();
                    _fgProxy.handleProxyRequest (_name, request);
                }
                catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        private String _name;
        private boolean _running = true;
        private BufferedReader _br;
        private BufferedWriter _bw;
        private FGraphClientProxy _fgProxy;
        private Socket _socket;
    }

    ////////////////////// Interfacing with Proxy Clients via TCP  ////////////////////////
    public synchronized void handleProxyRequest (String requestor, String request)
    {
        //TODO
        //parse the text command and covert it to a direct method call into fgraph...
    }

    ////////////////////// FGraph Event Listener Interface Methods ////////////////////////



    ////////////////////////// Private Mathods ////////////////////////////
    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println ("[FGClientProxy] " + msg);
        }
    }

    ///////////////////////// Member Variables ////////////////////////////
    public static FGraphClientProxy _fgClientProxy;
    private Hashtable _handlers;
    private ServerSocket _serverSocket;
    private boolean _debug = true;
    private boolean _running = true;
    private FGraphClient _fgraph;
}
