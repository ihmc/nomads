/*
 * FGEdgeMessage.java
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

import java.io.Serializable;
/**
 * FGVertexMessage
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.7 $ Created on May 18, 2004 at 1:39:53 PM $Date: 2016/06/09 20:02:46 $ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGEdgeMessage extends FGraphMessage
{
    public FGEdgeMessage (int actionType)
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

    public void setEdgeID (String edgeID)
    {
        _edgeID = edgeID;
    }

    public String getEdgeID ()
    {
        return (_edgeID);
    }

    public void setSourceID (String sourceVertexID)
    {
        _sourceID = sourceVertexID;
    }

    public String getSourceID ()
    {
        return (_sourceID);
    }

    public void setTargetID (String targetVertexID)
    {
        _targetID = targetVertexID;
    }

    public String getTargetID ()
    {
        return (_targetID);
    }

    public String getAction ()
    {
        return (_action);
    }

    public void setObjectKey (String key)
    {
        _key = key;
    }

    public String getObjectKey ()
    {
        return (_key);
    }

    public void setObject (Serializable obj)
    {
        _obj = obj;
    }

    public Serializable getObject ()
    {
        return (_obj);
    }

    public void setUndirectedEdgeFlag (boolean flag)
    {
        _undirectedFlag = flag;
    }

    public boolean isEdgeUndirected()
    {
        return (_undirectedFlag);
    }


    //////////////////////// Default Actions ///////////////////////////
    public transient static final int EDGE_ADDED = 20;
    public transient static final int EDGE_ADDED_WITH_ATTRIBUTE_LIST = 21;
    public transient static final int EDGE_REMOVED = 22;
    public transient static final int EDGE_ATTRIBUTE_SET = 23;
    public transient static final int EDGE_ATTRIBUTE_LIST_SET = 24;
    public transient static final int EDGE_ATTRIBUTE_REMOVED = 25;

    ////////////////////// Private Member Variables /////////////////////
    private int _actionType;
    private boolean _undirectedFlag;
    private String _action;
    private String _edgeID;
    private String _sourceID;       //meaningful only if it is an edge-related message
    private String _targetID;       //meaningful only if it is an edge-related message
    private String _key;
    private Serializable _obj;
}
