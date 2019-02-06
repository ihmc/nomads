/*
 * FGVertexMessage.java
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
 * @version $Revision$ Created on May 18, 2004 at 1:39:53 PM $Date$ Copyright (c) 2004, The
 *          Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGVertexMessage extends FGraphMessage
{
    public FGVertexMessage (int actionType)
    {
        super();
        _persistentMode = false;
        _actionType = actionType;
    }

    public int getActionType ()
    {
        return (_actionType);
    }

    public void setPersistentMode (boolean pVertexMode)
    {
        _persistentMode = pVertexMode;
    }

    public boolean isPersistentModeOn ()
    {
        return (_persistentMode);
    }

    public void setVertexID (String vertexID)
    {
        _vertexID = vertexID;
    }

    public String getVertexID ()
    {
        return (_vertexID);
    }

    public void setAction (String action)
    {
        _action = action;
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


    //////////////////////// Default Actions ///////////////////////////
    public transient static final int VERTEX_ADDED = 10;
    public transient static final int VERTEX_ADDED_WITH_ATTRIBUTE_LIST = 11;
    public transient static final int VERTEX_REMOVED = 12;
    public transient static final int VERTEX_ATTRIBUTE_SET = 13;
    public transient static final int VERTEX_ATTRIBUTE_LIST_SET = 14;
    public transient static final int VERTEX_ATTRIBUTE_REMOVED = 15;

    ////////////////////// Private Member Variables /////////////////////
    private int _actionType;
    private boolean _persistentMode;
    private String _action;
    private String _vertexID;
    private String _key;
    private Serializable _obj;
}
