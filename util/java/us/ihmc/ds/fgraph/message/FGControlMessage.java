/*
 * FGControlMessage.java
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

package us.ihmc.ds.fgraph.message;

/**
 * FGControlMessage
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.5 $
 *          Created on May 18, 2004 at 10:52:46 AM
 *          $Date: 2016/06/09 20:02:46 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGControlMessage extends FGraphMessage
{
    public FGControlMessage (int actionType)
    {
        super();
        _actionType = actionType;
    }

    public int getActionType ()
    {
        return (_actionType);
    }

    public void setAction (String action)
    {
        _action = action;
    }

    public String getAction ()
    {
        return (_action);
    }

    public void setTextMessage (String msg)
    {
        _textMessage = msg;
    }

    public String getTextMessage ()
    {
        return (_textMessage);
    }

    private int _actionType;
    private String _textMessage;
    private String _action;

    public transient static final int SET_CLIENT_MODE_FULL = 0;
    public transient static final int SET_CLIENT_MODE_THIN = 1;
    public transient static final int NOTIFY_CONN_HANDLER_DICONNETED = 2;
    public transient static final int NOTIFY_CONN_HANDLER_CONNETED = 3;
}
