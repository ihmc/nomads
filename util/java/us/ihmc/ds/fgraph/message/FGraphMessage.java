/*
 * FGraphMessage.java
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

import java.util.Random;
import java.io.Serializable;

/**
 * FGraphMessage
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.9 $
 *          Created on May 18, 2004 at 8:28:03 AM
 *          $Date: 2016/06/09 20:02:46 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGraphMessage implements Serializable
{
    public FGraphMessage ()
    {
        _commitRequired = true;
        _fgcSenderID = null;
        Random rand = new Random();
        _messageID = "M" + Math.abs(rand.nextLong());
    }

    public FGraphMessage (String tag)
    {
        if (tag == null) {
            tag = "M";
        }
        Random rand = new Random();
        _messageID = tag + Math.abs(rand.nextLong());
    }

    public String getMessageID()
    {
        return (_messageID);
    }

    public void setCommitRequiredMode (boolean commitRequired)
    {
        _commitRequired = commitRequired;
    }

    public boolean isCommitRequired ()
    {
        return (_commitRequired);
    }

    public String getFGCSenderID ()
    {
        return (_fgcSenderID);
    }

    public void setFGCSenderID (String fgcSenderID)
    {
        _fgcSenderID = fgcSenderID;
    }

    public void setLocalConnHandlerID (String connHandlerID)
    {
        _localConnHandlerID = connHandlerID;
    }

    public String getLocalConnHandlerID ()
    {
        return (_localConnHandlerID);
    }


    private boolean _commitRequired;
    private String _messageID;
    private String _fgcSenderID;
    private String _localConnHandlerID;
}
