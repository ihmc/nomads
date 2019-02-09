/*
 * PeersListCellRenderer.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr.gui;

import java.awt.*;
import java.awt.event.*;

import javax.swing.*;

/**
 * Cell Renderer for the Peers Lists.
 *
 * @author Maggie Breedy <NOMADS team>
 *
 * @version $Revision$
 */

public class PeersListCellRenderer extends JComponent implements ListCellRenderer
{
    public PeersListCellRenderer()
    {
        setLayout (new BorderLayout());
        Box b = Box.createHorizontalBox();
        b.add (_textLabel);
        //b.add(Box.createHorizontalStrut (200));
        b.add(Box.createHorizontalGlue());
        b.add (_icon1Label);
        //b.add(Box.createHorizontalGlue());
        b.add(Box.createHorizontalStrut (20));
        b.add (_icon2Label);
        b.add(Box.createHorizontalStrut (20));
        b.add (_icon3Label);
        b.add(Box.createHorizontalStrut (20));
        b.add (_icon4Label);
        b.add(Box.createHorizontalStrut (20));
        add (b);
    }

    // Get the renderer component from parent class
    public Component getListCellRendererComponent (JList list, Object value,
                                                   int index, boolean isSelected,
                                                   boolean cellHasFocus)
    {
        MessageObject msgObj = (MessageObject) value;

        _textLabel.setText(msgObj._peerName);

        _icon1Label.setIcon (msgObj._msgIcon);

        _icon2Label.setIcon (msgObj._voiceIcon);

        _icon3Label.setIcon (msgObj._messageIcon);
        //set number of messages received
        _icon3Label.setText (msgObj._messageIconText);
        _icon3Label.setFont (new Font ("Dialog", Font.BOLD, 9));
        _icon3Label.setHorizontalTextPosition (JLabel.RIGHT);
        _icon3Label.setVerticalTextPosition (JLabel.TOP);

        _icon4Label.setIcon (msgObj._speakerIcon);
        _icon4Label.setText (msgObj._speakerIconText);
        _icon4Label.setFont (new Font ("Dialog", Font.BOLD, 9));
        _icon4Label.setHorizontalTextPosition (JLabel.RIGHT);

        if (isSelected) {
            setBackground (list.getSelectionBackground());
            setForeground (list.getSelectionForeground());
        }
        else {
            setBackground (list.getBackground());
            setForeground (list.getForeground());
        }

        setEnabled (list.isEnabled());
        setFont (list.getFont());
        setOpaque (true);

        return this;
    }

//    public  class IncomingMessageThread implements Runnable
//    {
//        public IncomingMessageThread ()
//        {
//        }
//
//        public void run ()
//        {
//            while (true) {
//                try {
//                    Thread.sleep (3000);
//                }
//                catch (InterruptedException ie) {
//                    ie.printStackTrace();
//                }
//            }
//        }
//    }

    protected JLabel _textLabel = new JLabel();
    protected JLabel _icon1Label = new JLabel();
    protected JLabel _icon2Label = new JLabel();
    protected JLabel _icon3Label = new JLabel();
    protected JLabel _icon4Label = new JLabel();
    //private ImageIcon blank = new ImageIcon (getClass().getResource ("blank.gif"));
}
