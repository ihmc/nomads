/*
 * SocketConnHandler.java
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

import us.ihmc.ds.fgraph.message.*;
import us.ihmc.ds.fgraph.comm.msgQueue.MessageQueueHandler;

import java.net.Socket;
import java.net.InetAddress;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.util.Random;
import java.util.Hashtable;
import java.util.zip.CRC32;

/**
 * ConnHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.3 $
 *          Created on Apr 30, 2004 at 5:22:23 PM
 *          $Date: 2014/11/07 17:58:06 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class SocketConnHandler extends ConnHandler
{
    protected SocketConnHandler (Socket sock, MessageQueueHandler msgQueue)
        throws  Exception
    {
        super();
        _sock = sock;
        _clientMode = FULL_CLIENT;          //default
        if (_sock == null) {
            throw new Exception ("Invalid socket : " + _sock);
        }
        _rand = new Random();
        _msgQueueHandler = msgQueue;
        _receivedACKList = new Hashtable();
        _connHandlerID = generateID();
        _running = true;
    }

    public void run()
    {
        while (_running)
        {
            try {
                ObjectInputStream ois = new ObjectInputStream (_sock.getInputStream());
                FGraphMessage fgMessage = (FGraphMessage) ois.readObject();
                fgMessage.setLocalConnHandlerID (_connHandlerID);
                debugMsg ("Received " +  fgMessage.getMessageID());
                if (fgMessage instanceof FGACKMessage) {
                    addAckMessage ((FGACKMessage) fgMessage);
                }
                else if (fgMessage instanceof FGControlMessage) {
                    if (((FGControlMessage) fgMessage).getActionType() == FGControlMessage.SET_CLIENT_MODE_FULL) {
                        _clientMode = ConnHandler.FULL_CLIENT;
                    }
                    else if (((FGControlMessage) fgMessage).getActionType() == FGControlMessage.SET_CLIENT_MODE_THIN) {
                        _clientMode = ConnHandler.THIN_CLIENT;
                    }
                    else {
                        debugMsg("Will add Message (" + fgMessage.getMessageID() + ") into messageQueueHandler");
                        _msgQueueHandler.enqueueMessage (fgMessage);
                    }
                }
                else {
                    debugMsg ("Will add Message (" + fgMessage.getMessageID() + ") into messageQueueHandler");
                    _msgQueueHandler.enqueueMessage(fgMessage);
                }
            }
            catch (Exception e) {
                debugMsg ("Got Exception: " + e.getMessage());
                FGControlMessage fgCtrMessage = new FGControlMessage(FGControlMessage.NOTIFY_CONN_HANDLER_DICONNETED);
                fgCtrMessage.setTextMessage(_connHandlerID);
                debugMsg ("Will add Message (" + fgCtrMessage.getMessageID() + ") into messageQueueHandler");
                _msgQueueHandler.enqueueMessage(fgCtrMessage);
                _running = false;
            }
        }
    }

    public void messageArrived(FGraphMessage fgMessage) throws Exception
    {
        sendMessage (fgMessage);
    }

    protected synchronized void sendMessage (FGraphMessage fgMessage)
      throws Exception
    {
        try {
            if (_sock != null) {
                ObjectOutputStream oos = new ObjectOutputStream (_sock.getOutputStream());
                oos.writeObject (fgMessage);
                oos.flush();
            }
        }
        catch (Exception e) {
            debugMsg ("Got Exception: " + e.getMessage());
            FGControlMessage fgCtrMessage = new FGControlMessage(FGControlMessage.NOTIFY_CONN_HANDLER_DICONNETED);
            fgCtrMessage.setTextMessage(_connHandlerID);
            _msgQueueHandler.enqueueMessage(fgCtrMessage);
            _running = false;
            //throw new FGraphException (e);
        }
    }

    protected synchronized void sendBlockingMessage (FGraphMessage fgMessage)
        throws Exception
    {
        debugMsg (">>>>>>>>  ENTERING sendBlockingMEssage (" + fgMessage.getMessageID() + ")");
        if (_sock != null) {
            ObjectOutputStream oos = new ObjectOutputStream (_sock.getOutputStream());
            oos.writeObject (fgMessage);
            oos.flush();
            debugMsg ("Finished Writing message (" + fgMessage.getMessageID() + ")");
        }
        long elapsed = System.currentTimeMillis();
        while(!_receivedACKList.containsKey(fgMessage.getMessageID())) {
            this.wait(_ackTimeout);
            if (!_receivedACKList.containsKey(fgMessage.getMessageID())) {
                if ((System.currentTimeMillis()-elapsed)>_ackTimeout) {
                    throw new Exception ("No Acknowledgement of message (" +
                                         fgMessage.getMessageID()+ ") after " + _ackTimeout + " ms.");
                }
            }
            else {
                FGACKMessage fgAckMessage = (FGACKMessage) _receivedACKList.remove(fgMessage.getMessageID());
                if (fgAckMessage.getACKType() == FGACKMessage.ERROR) {
                    throw fgAckMessage.getException();
                }
                break;
            }
        }
        debugMsg ("<<<<<<<<<< LEAVING sendBlockingMEssage (" + fgMessage.getMessageID() + ")");
    }

    private synchronized void addAckMessage (FGACKMessage fgAckMessage)
    {
        debugMsg ("Received addACK for message (" + fgAckMessage.getReferenceMessageID() + ")");
        //System.out.println ("Received addACK for message (" + fgAckMessage.getReferenceMessageID() + ")") ;
        _receivedACKList.put(fgAckMessage.getReferenceMessageID(), fgAckMessage);
        this.notifyAll();
    }

    /**
     * Shutsdown and closes the socket to stop the server.
     *
     */
    protected void terminate()
    {
        _running = false;
        try {
            _sock.shutdownOutput();
            _sock.shutdownInput();
            _sock.close();
        }
        catch (Exception e) {
        }
    }

    /**
     * Generates a unique UUID for this connection handler.
     * Every message sent by this commHandler will have this ID in the commHandler-sender field.
     * Also, every message recived by this commHandler will have this ID set in the commHandler-receiver filed.
     * These IDs are unimpurtant for the end user but are necessary to manage the broadcaset of messages and
     * the exclusive reply.
     *
     * @return a unique UUID for this commHandler
     */
    private String generateID()
    {
        InetAddress inetAdd = _sock.getInetAddress();
        double dbl = Math.abs(_rand.nextDouble());
        String shash =  String.valueOf (System.currentTimeMillis()) + ":" +
                        String.valueOf (Math.abs (_rand.nextInt())) + ":" +
                        String.valueOf (dbl * System.currentTimeMillis());
        CRC32 crc32 = new CRC32();
        crc32.update(shash.getBytes());
        return ( inetAdd.getHostAddress() + "-" + String.valueOf(crc32.getValue()));
    }

    private Random _rand;
    private Socket _sock;
    private boolean _running;
    private MessageQueueHandler _msgQueueHandler;
    private Hashtable _receivedACKList;
    private long _ackTimeout = 6000;
}
