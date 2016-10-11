/*
 * ConnHandler.java
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
import us.ihmc.ds.fgraph.comm.msgQueue.MessageQueueListener;

import java.net.InetAddress;
import java.util.Random;
import java.util.zip.CRC32;

/**
 * ConnHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.14 $
 *          Created on Apr 30, 2004 at 5:22:23 PM
 *          $Date: 2016/06/09 20:02:46 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public abstract class ConnHandler extends Thread implements MessageQueueListener
{
    protected ConnHandler ()
        throws  Exception
    {
        _rand = new Random();
        _clientMode = FULL_CLIENT;
        _connHandlerID = generateID();
    }


    public void messageArrived(FGraphMessage fgMessage) throws Exception
    {
        sendMessage (fgMessage);
    }

    protected abstract void sendMessage (FGraphMessage fgMessage) throws Exception;

    protected abstract void sendBlockingMessage (FGraphMessage fgMessage) throws Exception;

    protected abstract void terminate();

    /**
     * Returns the unique UUID of this connHandler
     * @return connHandler's unique uuid
     */
    protected String getConnHandlerID()
    {
        return (_connHandlerID);
    }

    /**
     * Returns the mode of the client (FULL or THIN). This is an int value that can be compared with
     * static final variables FULL_CLIENT or THIN_CLIENT
     * @return mode of the client
     */
    protected int getClientMode ()
    {
        return (_clientMode);
    }

    public void setClientMode (int clientMode)
    {
        _clientMode = clientMode;
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
        String inetAdd = "localhost";
        try {
            inetAdd = InetAddress.getLocalHost().getHostAddress();
        }
        catch (Exception e) { }
        double dbl = Math.abs(_rand.nextDouble());
        String shash =  String.valueOf (System.currentTimeMillis()) + ":" +
                        String.valueOf (Math.abs (_rand.nextInt())) + ":" +
                        String.valueOf (dbl * System.currentTimeMillis());
        CRC32 crc32 = new CRC32();
        crc32.update(shash.getBytes());
        return ( inetAdd + "-" + String.valueOf(crc32.getValue()));
    }

    protected void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[connHandler(" + _connHandlerID + ")] " + msg);
        }
    }

    protected String _connHandlerID;
    private boolean _debug = false;
    protected int _clientMode;
    private Random _rand;

    public static final int FULL_CLIENT = 0;
    public static final int THIN_CLIENT = 1;

}
