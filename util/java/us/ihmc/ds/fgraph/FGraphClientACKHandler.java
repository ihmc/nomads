/*
 * FGraphClientACKHandler.java
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

import us.ihmc.ds.fgraph.message.*;
import java.util.Hashtable;
/**
 * FGraphClientACKHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.6 $ Created on May 19, 2004 at 4:12:59 PM $Date: 2016/06/09 20:02:46 $ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphClientACKHandler
{

    public static FGraphClientACKHandler getInstance()
    {
        if (_fgACKHandler == null) {
            _fgACKHandler = new FGraphClientACKHandler();
        }
        return (_fgACKHandler);
    }


    FGraphClientACKHandler ()
    {
        _acks = new Hashtable();
    }


    public void acknowledge (FGACKMessage fgAck)
        throws Exception
    {
        _acks.put (fgAck.getReferenceMessageID(), fgAck );
    }

    public synchronized FGACKMessage waitForAck (FGraphMessage fgMessage)
            throws Exception
    {
        FGACKMessage ack = null;
        long reqTime = System.currentTimeMillis();
        while (ack == null) {
            if (_acks.containsKey(fgMessage.getMessageID())) {
                ack = (FGACKMessage) _acks.remove (fgMessage.getMessageID());
                if (ack.getACKType() == FGACKMessage.ERROR) {
                    throw new Exception (ack.getException());
                }
            }
            else {
                Thread.sleep (250);
                if ((System.currentTimeMillis() - reqTime) >= _ackTimeout) {
                    throw new Exception ("No acknowledgement from server after " +
                                         (System.currentTimeMillis() - reqTime) + " milliseconds. Giving up!");
                }
            }
        }
        return (ack);
    }

    private static FGraphClientACKHandler _fgACKHandler;
    private long _ackTimeout = 5000;        //5 seconds
    private Hashtable _acks;
}
