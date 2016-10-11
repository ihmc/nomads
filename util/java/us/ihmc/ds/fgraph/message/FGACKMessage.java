/*
 * FGACKMessage.java
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
 * FGACKMessage
 * 
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision: 1.4 $
 *          Created on May 18, 2004 at 7:15:05 PM
 *          $Date: 2016/06/09 20:02:46 $
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class FGACKMessage extends FGraphMessage
{
    public FGACKMessage(int notificationType)
    {
        super("ACK");
        _notificationType = notificationType;
    }

    public int getACKType ()
    {
        return (_notificationType);
    }

    public void setAckMessage (String message)
    {
        _message = message;
    }

    public String getAckMessage ()
    {
        return (_message);
    }

    public void setReferenceMessageID (String refMsgID)
    {
        _referenceMsgID = refMsgID;
    }

    public String getReferenceMessageID ()
    {
        return (_referenceMsgID);
    }

    public void setException (Exception exp)
    {
        _exp = exp;
    }

    public Exception getException ()
    {
        return (_exp);
    }

    private Exception _exp;
    private int _notificationType;
    private String _referenceMsgID;
    private String _message;

    transient public static final int SUCCESS = 0;
    transient public static final int ERROR = 1; 
}
