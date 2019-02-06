/*
 * MessageHandler.java
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

package us.ihmc.ds.fgraph.comm;

import us.ihmc.ds.fgraph.message.*;
import us.ihmc.ds.fgraph.FGraphException;
import us.ihmc.ds.fgraph.comm.msgQueue.MessageQueueHandler;
import us.ihmc.ds.fgraph.comm.msgQueue.MessageQueueListener;
import us.ihmc.mockets.Mocket;

import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * MessageHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Apr 30, 2004 at 5:24:20 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class MessageHandler implements MessageQueueListener
{
    public MessageHandler (MessageListener msgListener)
    {
        _msgListener = msgListener;
        _connHandlers = new Hashtable();
        _connHandlerQueues = new Hashtable();
        _msgQueueHandler = new MessageQueueHandler(this);
        _msgQueueHandler.start();
    }

    /**
     * Creates a new commHandler that will report to this messageHandler
     * @param sock
     */
    public void createConnHandler (Socket sock)
    {
        try {
            if (sock != null) {
                dbgMsg ("MessageHandler: create SocketConnHandler:" + sock.toString());
                SocketConnHandler sockConnHandler = new SocketConnHandler (sock, _msgQueueHandler);
                ConnHandler connHandler = (ConnHandler) sockConnHandler;
                _connHandlers.put(connHandler.getConnHandlerID(), connHandler);
                MessageQueueHandler connMsgHandler = new MessageQueueHandler ((MessageQueueListener) connHandler);
                _connHandlerQueues.put(connHandler.getConnHandlerID(), connMsgHandler);
                connMsgHandler.start();
                connHandler.start();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Creates a new commHandler that will report to this messageHandler
     * @param mocket
     */
    public void createConnHandler (Mocket mocket)
    {
        try {
            if (mocket != null) {
                dbgMsg ("MessageHandler: create MocketConnHandler:" + mocket.toString());
                MocketConnHandler sockConnHandler = new MocketConnHandler (mocket, _msgQueueHandler);
                ConnHandler connHandler = (ConnHandler) sockConnHandler;
                _connHandlers.put(connHandler.getConnHandlerID(), connHandler);
                MessageQueueHandler connMsgHandler = new MessageQueueHandler ((MessageQueueListener) connHandler);
                _connHandlerQueues.put(connHandler.getConnHandlerID(), connMsgHandler);
                connMsgHandler.start();
                connHandler.start();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void sendMessage (FGraphMessage fgMessage)
        throws FGraphException
    {
        dbgMsg (">>>>>>> sendMessage (" + fgMessage.getMessageID() + ")");
        if (fgMessage.getFGCSenderID() != null) {
            sendExceptTo (fgMessage, fgMessage.getFGCSenderID());
        }
        else {
            Enumeration en= _connHandlerQueues.elements();
            while (en.hasMoreElements()) {
                MessageQueueHandler msgQH = (MessageQueueHandler) en.nextElement();
                try {
                    msgQH.enqueueMessage(fgMessage);
                }
                catch (Exception e) {
                    throw new FGraphException (e);
                }
            }
        }
        dbgMsg ("<<<<<<< sent (" + fgMessage.getMessageID() + ")");
    }

    public void sendBlockingMessage (FGraphMessage fgMessage)
        throws FGraphException
    {
        dbgMsg ("sendBlockingMessage (" + fgMessage.getMessageID() + ")");
        Enumeration en= _connHandlers.keys();
        while (en.hasMoreElements()) {
            String sconnHandlerID = (String) en.nextElement();
            ConnHandler conn = (ConnHandler) _connHandlers.get(sconnHandlerID);
            if (fgMessage.getFGCSenderID() != null && fgMessage.getMessageID().compareTo(conn.getConnHandlerID())==0) {
                continue;
            }
            try {
                conn.sendBlockingMessage(fgMessage);
            }
            catch (Exception e) {
                throw new FGraphException (e);
            }
        }
        dbgMsg ("<<<<<<< sent (" + fgMessage.getMessageID() + ")");
    }

    public void sendReplyTo (FGraphMessage fgMessage, String connHandlerID)
        throws FGraphException
    {
        dbgMsg ("sendReplyTo Message (" + fgMessage.getMessageID() + ")");
        try {
            if (fgMessage instanceof FGACKMessage) {
                dbgMsg ("Sending ACK for Message (" + ((FGACKMessage)fgMessage).getReferenceMessageID() + ") to [" + connHandlerID + "]");
            }
            MessageQueueHandler msgQH = (MessageQueueHandler) _connHandlerQueues.get (connHandlerID);
            if (msgQH != null) {
                msgQH.enqueueMessage(fgMessage);
            }
        }
        catch (Exception e) {
            throw new FGraphException (e);
        }
        dbgMsg ("<<<<<<< sent (" + fgMessage.getMessageID() + ")");
    }

    public void sendExceptTo (FGraphMessage fgMessage, String connHandlerID)
        throws FGraphException
    {
        dbgMsg ("sendExceptTo Message (" + fgMessage.getMessageID() + ")");
        Enumeration en= _connHandlerQueues.keys();
        while (en.hasMoreElements()) {
            String sconnH = (String) en.nextElement();
            if (sconnH.compareTo(connHandlerID)!=0) {
                MessageQueueHandler msgQH = (MessageQueueHandler) _connHandlerQueues.get(sconnH);
                if (msgQH != null) {
                    try {
                        msgQH.enqueueMessage(fgMessage);
                    }
                    catch (Exception e) {
                        throw new FGraphException (e);
                    }
                }
            }
        }
        dbgMsg ("<<<<<<< sent (" + fgMessage.getMessageID() + ")");
    }

    public void lostConnection (String connHandlerID)
    {
        dbgMsg ("Received Message LOST_CONNECTION from  (" + connHandlerID + ")");
        MessageQueueHandler msgQH = (MessageQueueHandler) _connHandlerQueues.get(connHandlerID);
        if (msgQH != null) {
            dbgMsg ("\tTerminating MessageQueueHandler for (" + connHandlerID + ")");
            msgQH.terminate();
            dbgMsg ("\tRemoving MessageQueueHandler refereces for (" + connHandlerID + ")");
            _connHandlerQueues.remove(connHandlerID);
        }
        ConnHandler connHandler = (ConnHandler) _connHandlers.get(connHandlerID);
        if (connHandler != null) {
            dbgMsg ("\tTerminating connHandler (" + connHandlerID + ")");
            connHandler.terminate();
            dbgMsg ("\tRemoving References to connHandler (" + connHandlerID + ")");
            _connHandlers.remove(connHandlerID);
        }
        _msgListener.lostConnection(connHandlerID);
    }

    public void close()
    {
        try {
            Enumeration en= _connHandlers.elements();
            while (en.hasMoreElements()) {
                ConnHandler connHandler = (ConnHandler) en.nextElement();
                connHandler.terminate();
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void messageArrived (FGraphMessage fgMessage) throws Exception
    {
        if (fgMessage != null) {
            dbgMsg ("Received Message (" + fgMessage.getMessageID() + ")");
            if (fgMessage instanceof FGControlMessage) {
                FGControlMessage fgCtrMessage = (FGControlMessage) fgMessage;
                if (fgCtrMessage.getActionType() == FGControlMessage.NOTIFY_CONN_HANDLER_DICONNETED) {
                    lostConnection (fgCtrMessage.getTextMessage());
                }
                if (fgCtrMessage.getActionType() == FGControlMessage.NOTIFY_CONN_HANDLER_CONNETED) {
                    _msgListener.connected (fgCtrMessage.getTextMessage());
                }
            }
            else {
                _msgListener.messageArrived(fgMessage);
            }
        }
    }

    private void dbgMsg (String msg)
    {
        if (_debug == true) {
            System.out.println ("[MsgHandler] " + msg);
        }
    }

    private boolean _debug = false;
    private Hashtable _connHandlers;
    private Hashtable _connHandlerQueues;
    private MessageListener _msgListener;
    private MessageQueueHandler _msgQueueHandler;
}
