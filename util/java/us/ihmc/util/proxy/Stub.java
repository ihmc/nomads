/*
 * Stub.java
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

package us.ihmc.util.proxy;

import java.net.Socket;
import java.util.ArrayList;
import java.util.ConcurrentModificationException;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.logging.Level;
import java.util.logging.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.comm.CommHelper;
import us.ihmc.sync.ConcurrentProxy;

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Stub extends ConcurrentProxy
{
    private short _applicationId;
    private final String _host;
    private final int _port;
    private final int _reinitializationAttemptInterval;
    private final CallbackHandlerFactory _handlerFactory;
    private CallbackHandler _handler;
    protected AtomicBoolean _isInitialized = new AtomicBoolean(false);
    private final List<ConnectionStatusListener> _connectionStatusListeners = new ArrayList<>();
    private long _callbackTheadId;
    private final String _protocol;
    private final String _version;
    private final AtomicBoolean _reinitializing = new AtomicBoolean(false);

    protected CommHelper _commHelper;

    protected Stub (CallbackHandlerFactory handlerFactory, short applicationId,
                    String host, int port, String protocol, String version)
    {
        _applicationId = applicationId;
        _host = host;
        _port = port;
        _reinitializationAttemptInterval = 5000;
        _handlerFactory = handlerFactory;
        _protocol = protocol;
        _version = version;
    }

    public int init() throws Exception
    {
        int rc = 0;

        try {
            try { _commHelper.closeConnection(); } catch (Exception e) {}
            if (_handler != null) {
                _handler.requestTermination();
                _handler.closeConnection();
            }

            CommHelper ch = connectToServer (_host, _port);
            CommHelper chCallback = connectToServer (_host, _port);
            if (ch != null && chCallback != null) {
                int applicationId = registerProxy (ch, chCallback, _applicationId);
                if (applicationId >= 0) {
                    _applicationId = (short)applicationId; // The server may have assigned
                                                           // a different id than requested
                    _commHelper = ch;
                    _handler = _handlerFactory.getHandler (this, chCallback);
                    Thread t = new Thread (_handler);
                    t.setDaemon (true);
		            t.start();
                    _isInitialized.set (true);
                }
                else {
                    rc = -1;
                }
            }
        }
        catch (Exception e) {
            rc = -1;
            throw e;
        }

        if (rc < 0) {
            throw new Exception ("DisseminationServiceProxy:init:Failed to register listener.");
        }
        return rc;
    }
 
    public synchronized void reinitialize()
    {
        Thread t = new Thread(){
            @Override
            public void run() {
                if(_reinitializing.get()) {
                    return;
                }
                _reinitializing.set(true);
                while (!isInitialized()) {
                    try {
                        init();
                        Thread.sleep (_reinitializationAttemptInterval);
                    }
                    catch (InterruptedException ex) {}
                    catch (Exception ex) {
                        Logger.getLogger(Stub.class.getName()).log(Level.SEVERE, null, ex);
                    }
                }
                _reinitializing.set(false);
            }
		};
        t.setDaemon(true);
        t.setName(Stub.class.getName() + "-ReInit");
        t.start();
    }

    public boolean isInitialized()
    {
        return _isInitialized.get();
    }

    public void registerConnectionStatusListener (ConnectionStatusListener listener)
    {
        checkConcurrentModification ("registerConnectionStatusListener");

        if (listener == null) {
            System.out.println ("Error: ConnectionStatusListener is null");
            return;
        }

        _connectionStatusListeners.add (listener);
    }

    public void notifyConnectionLoss()
    {
        _isInitialized.set (false);
        for (int i = 0 ; i < _connectionStatusListeners.size() ; i++) {
            _connectionStatusListeners.get(i).connectionLost();
        }
    }

    void setCallbackThreadId (long callbackTheadId)
    {
        _callbackTheadId = callbackTheadId;
    }

    protected CommHelper connectToServer (String host, int port)
        throws Exception
    {
        CommHelper commHelper = new CommHelper();
        commHelper.init (new Socket (host, port));
        return commHelper;
    }

    protected int registerProxy (CommHelper ch, CommHelper chCallback, int desiredApplicationId) throws CommException, ProtocolException
    {
        try {
            // First register the proxy using the desired application id
            // The ProxyServer will return the assigned application id
            doHandshake (ch);
            ch.sendLine ("RegisterProxy " + desiredApplicationId);
            String[]  array = ch.receiveRemainingParsed ("OK");
            int applicationId = Short.parseShort (array[0]);

            // Now register the callback using the assigned application id
            doHandshake (chCallback);
            chCallback.sendLine ("RegisterProxyCallback " + applicationId);
            chCallback.receiveMatch ("OK");
            return applicationId;
        }
        catch (Exception e) {
            if (e instanceof CommException) {
                _isInitialized.set (false);
                throw (CommException) e;
            }
            Logger.getLogger (Stub.class.getName()).log(Level.SEVERE, null, e);
            return -1;
        }
    }

    protected void checkConcurrentModification (String exceptionMsg)
    {
        if (Thread.currentThread().getId() == _callbackTheadId) {
            throw new ConcurrentModificationException (exceptionMsg);
        }
    }

    private void doHandshake(CommHelper ch) throws CommException, ProtocolException {
        String handshakeMsg = _protocol + " " + _version;
        ch.sendLine (handshakeMsg);
        String[] response = ch.receiveParsed();
        if ((response == null) || (response.length != 2)) {
            throw new ProtocolException("Received unparsable response to handshake");
        }
        if ((_protocol.compareTo (response[0]) != 0) || (_version.compareTo (response[1]) != 0)) {
            throw new ProtocolException("Handshake failed: expected <" + handshakeMsg + ">, received <"
                    + response[0] + " " + response[1] + ">");
        }
    }
}

