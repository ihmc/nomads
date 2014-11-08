/*
 * MessageQueueHandler.java
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

package us.ihmc.ds.fgraph.comm.msgQueue;

import us.ihmc.ds.fgraph.message.FGraphMessage;

import java.util.Vector;

/**
 * MessageQueueHandler
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.9 $
 *          Created on Jul 13, 2004 at 8:11:57 PM
 *          $Date: 2014/11/06 22:00:39 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class MessageQueueHandler extends Thread
{
    public MessageQueueHandler (MessageQueueListener msgListener)
    {
        _running = true;
        _msgQueue = new Vector();
        _msgListener = msgListener;
    }

    public void run()
    {
        while (_running) {
            try {
                FGraphMessage fgMessage = null;
                synchronized (this) {
                    while (_msgQueue.size() == 0 && _running) {
                        wait();
                    }
                    if (_msgQueue.size() > 0) {
                        fgMessage = (FGraphMessage) _msgQueue.remove(0);
                    }
                }
                if (fgMessage != null) {
                    _msgListener.messageArrived(fgMessage);
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void enqueueMessage (FGraphMessage fgMessage)
    {
        synchronized (this) {
            _msgQueue.addElement(fgMessage);
            notifyAll();
        }
    }

    public boolean containsMessage (FGraphMessage fgMessage)
    {
        return (_msgQueue.contains(fgMessage));
    }

    public void terminate()
    {
        debugMsg ("terminating thread");
        synchronized (this) {
            _running = false;
            notifyAll();
        }
    }

    private void debugMsg (String msg)
    {
        if (_debug)
        {
            System.out.println("[MsgQueueHandler] " + msg);
        }
    }

    private boolean _debug = false;
    private boolean _running;
    private Vector _msgQueue;
    private MessageQueueListener _msgListener;
}
