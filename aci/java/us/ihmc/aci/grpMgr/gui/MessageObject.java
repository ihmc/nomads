/*
 * MessageObject.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr.gui;

import javax.swing.ImageIcon;

/**
 * User: mbreedy
 * Date: Jan 17, 2013
 * Time: 10:28:19 AM
 * @author Maggie Breedy <NOMADS team>
 * @version $Revision$
 */
public class MessageObject
{
     public MessageObject (String peerName, ImageIcon msgIcon, ImageIcon voiceIcon, ImageIcon messageIcon, String messageIconText,
                           ImageIcon speakerIcon, String speakerIconText)
    {
        _peerName = peerName;
        _msgIcon = msgIcon;
        _voiceIcon = voiceIcon;
        _messageIcon = messageIcon;
        _messageIconText = messageIconText;
        _speakerIcon = speakerIcon;
        _speakerIconText = speakerIconText;
    }

    public ImageIcon _msgIcon;
    public ImageIcon _voiceIcon;
    public ImageIcon _messageIcon;
    public ImageIcon _speakerIcon;
    public String _peerName;
    public String _messageIconText;
    public String _speakerIconText;
}
