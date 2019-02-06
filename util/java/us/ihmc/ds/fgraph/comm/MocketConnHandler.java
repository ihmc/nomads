/*
 * MocketConnHandler.java
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
import us.ihmc.ds.fgraph.comm.msgQueue.MessageQueueHandler;
import us.ihmc.mockets.Mocket;
import us.ihmc.mockets.MocketStatusListener;
import us.ihmc.util.ConfigLoader;
import us.ihmc.util.serialization.SerializationException;
import us.ihmc.util.serialization.SerializerFactory;
import us.ihmc.util.serialization.SerializerType;

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
 * @version $Revision$
 *          Created on Apr 30, 2004 at 5:22:23 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class MocketConnHandler extends ConnHandler implements MocketStatusListener
{
    protected MocketConnHandler (Mocket mocket, MessageQueueHandler msgQueue)
        throws  Exception
    {
        super();

        ConfigLoader cl = ConfigLoader.getDefaultConfigLoader();
        _mocketTimeout = cl.getPropertyAsInt("fgraph.client.mocket.timeout", _mocketTimeout);
        System.out.println("FGraph Mocket Timeout is: " + _mocketTimeout);
        _mocket = mocket;
        _mocket.setStatusListener(this);
        if (_mocket == null) {
            throw new Exception ("Invalid Mocket : " + _mocket);
        }
        _rand = new Random();
        _msgQueueHandler = msgQueue;
        _receivedACKList = new Hashtable();
        _running = true;
    }

    public void run()
    {
        while (_running)
        {
            try {
                debugMsg ("Entering run method");
                byte[] data = _mocket.receive (-1);
                FGraphMessage fgMessage = SerializerFactory.getSerializer (SerializerType.EXTERNALIZABLE).deserialize (
                        data, FGraphMessage.class);
                if (fgMessage == null) {
                    throw new SerializationException ("The deserialized object is null");
                }

//                ObjectInputStream ois = new ObjectInputStream (_mocket.getInputStream());
//                debugMsg ("Have ObjectInputStream from mocket.");
//                FGraphMessage fgMessage = (FGraphMessage) ois.readObject();
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
            if (_mocket != null) {
                byte[] data;
                try {
                    data = SerializerFactory.getSerializer (SerializerType.EXTERNALIZABLE).serialize (fgMessage);
                }
                catch (SerializationException e) {
                    debugMsg ("Got Exception: " + e.getMessage());
                    return;
                }
                try {
                    int rc = _mocket.send (true, true, data, 0, (short) 5, 0, 0);
                    if (rc < 0) {
                        throw new Exception ("The mockets return value for the send is " + rc);
                    }
                }
                catch (Exception e) {
                    debugMsg ("Got Exception: " + e.getMessage());
                    return;
                }

//                ObjectOutputStream oos = new ObjectOutputStream (_mocket.getOutputStream());
//                oos.writeObject (fgMessage);
//                oos.flush();
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
        if (_mocket != null) {
            byte[] data;
            try {
                data = SerializerFactory.getSerializer (SerializerType.EXTERNALIZABLE).serialize (fgMessage);
            }
            catch (SerializationException e) {
                debugMsg ("Got Exception: " + e.getMessage());
                return;
            }
            try {
                int rc = _mocket.send (true, true, data, 0, (short) 5, 0, 0);
                if (rc < 0) {
                    throw new Exception ("The mockets return value for the send is " + rc);
                }
            }
            catch (Exception e) {
                debugMsg ("Got Exception: " + e.getMessage());
                return;
            }
//            ObjectOutputStream oos = new ObjectOutputStream (_mocket.getOutputStream());
//            oos.writeObject (fgMessage);
//            oos.flush();
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
            _mocket.close();
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
        String inetAdd = "0.0.0.0";
        try {
            inetAdd = InetAddress.getLocalHost().getHostAddress();
        }
        catch (Exception e) {}
        double dbl = Math.abs(_rand.nextDouble());
        String shash =  String.valueOf (System.currentTimeMillis()) + ":" +
                        String.valueOf (Math.abs (_rand.nextInt())) + ":" +
                        String.valueOf (dbl * System.currentTimeMillis());
        CRC32 crc32 = new CRC32();
        crc32.update(shash.getBytes());
        return ( inetAdd + "-" + String.valueOf(crc32.getValue()));
    }

    public boolean peerUnreachableWarning(long timeSinceLastContact)
    {
        if (timeSinceLastContact > _mocketTimeout) {
            //System.out.println ("WARNING - Inactive for " + timeSinceLastContact + " ms. WILL MAINTAIN CONNECTION");
            System.out.println ("Peer has been silent for " + timeSinceLastContact + " ms; dropping connection");
            //FGControlMessage fgCtrMessage = new FGControlMessage(FGControlMessage.NOTIFY_CONN_HANDLER_DICONNETED);
            //fgCtrMessage.setTextMessage(_connHandlerID);
            //_msgQueueHandler.enqueueMessage(fgCtrMessage);
            //_running = false;
            return true;
        }
        return false;
    }

    public boolean peerReachable (long unreachabilityIntervalLength)
    {
        return false;
    }

    public boolean suspendReceived (long timeSinceSuspension)
    {
        return false;
    }

    private int _mocketTimeout = 180000;
    private Random _rand;
    private Mocket _mocket;
    private boolean _running;
    private MessageQueueHandler _msgQueueHandler;
    private Hashtable _receivedACKList;
    private long _ackTimeout = 6000;

}
