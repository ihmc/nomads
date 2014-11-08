/*
 * FGSyncMessage.java
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

package us.ihmc.ds.fgraph.message;

import java.io.Serializable;
/**
 * FGSyncMessage
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.3 $ Created on May 18, 2004 at 7:02:22 PM $Date: 2014/11/06 22:00:39 $ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGSyncMessage extends FGraphMessage
{
    public FGSyncMessage (int actionType)
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

    public void setGraphObject (Serializable graphObj)
    {
        _obj = graphObj;
    }

    public Object getGraphObject ()
    {
        return (_obj);
    }

    private int _actionType;
    private String _action;
    private Serializable _obj;

    public transient static final int REQUEST_GRAPH_SYNC = 30;
    public transient static final int REPLY_GRAPH_SYNC = 31;
}
