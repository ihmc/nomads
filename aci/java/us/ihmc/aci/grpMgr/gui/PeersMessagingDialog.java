/*
 * PeersMessagingDialog.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr.gui;

import us.ihmc.aci.grpMgr.GroupManagerListener;
import us.ihmc.aci.grpMgr.GroupManager;
import us.ihmc.aci.grpMgr.TransportMode;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Toolkit;
import java.awt.Color;
import java.awt.Insets;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.io.UnsupportedEncodingException;
import java.io.IOException;
import java.util.Calendar;
import java.text.SimpleDateFormat;

import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.DefaultListModel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.ImageIcon;

import javax.swing.border.EtchedBorder;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

/** 
 * Date: Jan 16, 2013
 * Time: 2:45:07 PM
 * @author Maggie Breedy <NOMADS team>
 * @version $Revision$
 */
public class PeersMessagingDialog extends JDialog implements GroupManagerListener
{
    public PeersMessagingDialog (String ip, int port, String nodeUUID) throws IOException
    {
        createGui();
        _nodeUUID = nodeUUID;
        //System.out.println ("******_nodeUUID: " + _nodeUUID);
        _msgDialog = new MessageDialog();
        _grpMgr = new GroupManager();
        _grpMgr.setGroupManagerListener (this);
        //String nodeUUID = UUID.randomUUID().toString();
        _grpMgr.init (_nodeUUID, port, ip, TransportMode.UDP_BROADCAST);
        _grpMgr.start();
        _grpMgr.setNodeName ("MessagingNode");
        _grpMgr.createPublicPeerGroup("BA4Group", null);
    }

    protected void createGui()
    {
        setTitle ("Peers Messaging Window");
        setModal (true);
        setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        setBounds ((screenSize.width-400)/2, (screenSize.height-500)/2, 400, 500);
        getContentPane().setLayout (new GridBagLayout());

        //Main Panels
        GridBagConstraints gbc = new GridBagConstraints();
        JPanel mainPanel = new JPanel (new GridBagLayout());
        mainPanel.setBorder (new EtchedBorder());
        mainPanel.setBackground (new Color (198,216,226));
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.fill = java.awt.GridBagConstraints.BOTH;
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        getContentPane().add (mainPanel, gbc);

        _peersListModel = new DefaultListModel();
        _peersList = new JList (_peersListModel);
        JScrollPane jsp = new JScrollPane (_peersList);
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.insets = new Insets (0, 5, 5, 5);
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        mainPanel.add (jsp, gbc);
        _peersList.addMouseListener (new ListMouseEventListener());

        _peersList.setCellRenderer (new PeersListCellRenderer());

//        ListSelectionListener listSelectionListener = new ListSelectionListener() {
//            public void valueChanged(ListSelectionEvent listSelectionEvent) {
//                boolean adjust = listSelectionEvent.getValueIsAdjusting();
//                System.out.println(", Adjusting? " + adjust);
//                if (!adjust) {
//                    JList list = (JList) listSelectionEvent.getSource();
//                    MessageObject obj = (MessageObject) list.getSelectedValue();
//                    System.out.print ("***Selected: " + obj._peerName);
//
//                }
//            }
//        };
//        _peersList.addListSelectionListener (listSelectionListener);

        this.addWindowListener (new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                dispose();
                System.exit (1);
            }
        });
    }

    protected void sendMessageToPeer (MessageObject mo, String message)
    {
        _grpMgr.sendPeerMessage (mo._peerName, message, true, 0);
        //_historyMsg.append ("\n" + message);
    }

    @Override
    public void peerNodeDataChanged(String nodeUUID) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void peerUnhealthy(String nodeUUID) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void hopCountChanged(int hopCount) {
        throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    protected class ListMouseEventListener implements MouseListener
    {
        public void mouseClicked (MouseEvent mouseEvent)
        {
            JList list = (JList) mouseEvent.getSource();
            if (mouseEvent.getClickCount() == 1) {
                MessageObject mo = (MessageObject) list.getSelectedValue();
                //System.out.println ("One-clicked on:name: " + mo._peerName);
                int index = list.locationToIndex (mouseEvent.getPoint());
                Dimension d = _thisDialog.getSize();
                double screenX = d.getWidth();
                //System.out.println ("One-clicked on:screenX: " + screenX);
                int iconIndex = (int) screenX - (mouseEvent.getX());
                System.out.println ("One-clicked on:point:x: " + iconIndex);
                if (162 <= iconIndex && iconIndex <= 178) {
                    //System.out.println ("It is a text icon");
                    //_msgDialog = new MessageDialog();
                    _msgDialog.init (mo);
                    _msgDialog.setMsgText (_message, mo._peerName, getCurrentTime());
                    //JTextArea jta = _msgDialog.getMsgTextArea();
                    _msgDialog.setVisible (true);
                    //jta.requestFocus();
                }
                else if (132 <= iconIndex && iconIndex <= 146) {
                    System.out.println ("It is a voice icon");
                }
                else if (92 <= iconIndex && iconIndex <= 115) {
                    //System.out.println ("It is a msg received icon");
                    //_msgDialog = new MessageDialog (mo);
                    _msgDialog.init (mo);
                    //_msgDialog.setMsgText (_message, mo._peerName);
                    _msgDialog.setVisible (true);
                }
                else if (51 <= iconIndex && iconIndex <= 74) {
                    System.out.println ("It is a speaker icon");
                }
            }
        }
        public void mousePressed(MouseEvent e) {}

        public void mouseReleased(MouseEvent e) {}

        public void mouseEntered(MouseEvent e) {}

        public void mouseExited(MouseEvent e) {}
    }

    protected String getCurrentTime()
    {
        Calendar cal = Calendar.getInstance();
    	cal.getTime();
    	SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
    	System.out.println ( sdf.format(cal.getTime()));
        return (sdf.format(cal.getTime()));
    }

    /**
     * New Peer is called when a New Peer event is trigger. It updates the remote
     * group list.
     *
     * @param nodeUUID  the uuid of the new remote group
     */
    public void newPeer (String nodeUUID)
    {
        _grpMgr.getPeerNodeName (nodeUUID);
        MessageObject msgObj = new MessageObject (nodeUUID, _msgIcon, _voiceIcon, _msgRecIcon, "0", _speakerIcon, "0");
        _peersListModel.addElement (msgObj);
        System.out.println ("-->GUI:newPeer: " + _grpMgr.getPeerNodeName (nodeUUID));
    }

    /**
     * Dead Peer is called when a peer event is dead. It updates the remote
     * group list.
     *
     * @param nodeUUID  the uuid of the new remote group
     */
    public void deadPeer (String nodeUUID)
    {
        for (int i=0; i<_peersListModel.size(); i++) {
            MessageObject msgObj = (MessageObject) _peersListModel.getElementAt (i);
            String peerID = msgObj._peerName;
            if (peerID.equals (nodeUUID)) {
                _peersListModel.removeElement (msgObj);
                break;
            }
        }
        _peersList.revalidate();
        System.out.println ("-->GUI:deadPeer: " + _grpMgr.getPeerNodeName (nodeUUID));
    }

    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] param)
    {
        System.out.println ("peer search request received - groupName: " + groupName + ", nodeUUID: " +
                             nodeUUID + ", paramlength: " + param.length);
    }

    /**
     * Group List Change is called when change event occur in the GroupManager. It
     * updates the local and remote group list.
     *
     * @param nodeUUID  the uuid of the local or remote group
     */
    public void groupListChange (String nodeUUID)
    {
        System.out.println ("peer search request received - nodeUUID: " + nodeUUID);
    }

    public void newGroupMember (String groupName, String memberUUID, byte[] joinData)
    {
        System.out.println ("peer search request received - groupName: " + groupName +
                            ", memberUUID: " + memberUUID + ", datalength: " + joinData.length);
    }

    /**
     * Group Member Left is generated when a group leave a remote group. It
     * updates the local and remote group list.
     *
     * @param groupName  the group Name of the remote group
     * @param memberUUID  the uuid of the local or remote group
     */
    public void groupMemberLeft (String groupName, String memberUUID)
    {
        System.out.println ("peer search request received - groupName: " + groupName +
                            ", memberUUID: " + memberUUID);
    }

    /**
     * Conflict With Private PeerGroup
     * 
     * @param nodeUUID  the uuid of the remote peer group
     */
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        System.out.println ("peer search request received - groupName: " + groupName + ", nodeUUID: " + nodeUUID);
    }

    // Invoked by the GroupManager when a search request is recieved from another node
    // Application may choose to respond by invoking the searchReply() method in the GroupManager
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search request received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID +
                            ", param: " + new String (param));
    }

    // Invoked by the GroupManager when a response to a search request is received
    // This may be invoked multiple times, once per response received
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search result received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID +
                            ", param: " + new String (param));
    }

    // Message received from another peer
    // groupName is null if the message is a direct message to the node (unicast)
    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
        System.out.println ("peer search result received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", datalength: " + data.length);

        MessageObject msgObj = null;
        for (int i=0; i<_peersListModel.size(); i++) {
            msgObj = (MessageObject) _peersListModel.getElementAt (i);
            String peerID = msgObj._peerName;
            if (peerID.equals (nodeUUID)) {
                msgObj._messageIconText = String.valueOf (_numberOfMsgRec);
                _peersList.repaint();
                break;
            }
        }

        try {
            _message = new String (data, "UTF-8");
            _msgDialog.setMsgText (_message, msgObj._peerName, getCurrentTime());
        }
        catch (UnsupportedEncodingException e) {
            System.out.println ("Unsupported encoding UTF-8 for the received message");
        }
        _numberOfMsgRec++;
    }

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID)
    {
        System.out.println ("peer search result received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", peerSearchUUID: " + peerSearchUUID);
    }

    public static void main (String args[])
    {
        String ip = "";
        String nodeID = "";
        int port = -1;
        if (args.length == 3) {
            ip = args[0];
            port = Integer.parseInt(args[1]);
            nodeID = args[2];
        }
        else {
            System.out.println ("*Missing arguments: <ip> <port> <nodeUUID name>. The system is using the defaults.");
            ip = "127.0.0.1";
            port = 7000;
            nodeID = "localhost";
        }

        try {
            PeersMessagingDialog pmd = new PeersMessagingDialog (ip, port, nodeID);
            pmd.setVisible (true);		
        }
        catch (IOException e) {
            System.out.println("ERROR - Unable to initialize GroupManager");
        }
    }

    protected class MessageDialog extends JDialog
    {
        public MessageDialog()
        {
            createGui();
        }

        public void init (MessageObject mo)
        {
            _mo = mo;
        }

        protected void createGui()
        {
            setTitle ("Send Message");
            setModal (true);
            setResizable (true);
            Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
            setBounds ((screenSize.width-380)/2, (screenSize.height-280)/2, 380, 280);
            getContentPane().setLayout (new GridBagLayout());

            //Main Panels
            GridBagConstraints gbc = new GridBagConstraints();
            JPanel mainPanel = new JPanel (new GridBagLayout());
            mainPanel.setBorder (new EtchedBorder());
            //mainPanel.setBackground (new Color (198,216,226));
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.fill = java.awt.GridBagConstraints.BOTH;
            gbc.weightx = 0.9;
            gbc.weighty = 0.9;
            getContentPane().add (mainPanel, gbc);

            gbc = new GridBagConstraints();
            JPanel lowPanel = new JPanel();
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.fill = java.awt.GridBagConstraints.BOTH;
            gbc.weightx = 0.2;
            gbc.weighty = 0.2;
            mainPanel.add (lowPanel, gbc);

            JPanel histPanel = new JPanel();
            gbc.gridx = 0;
            gbc.gridy = 0;
            gbc.fill = java.awt.GridBagConstraints.BOTH;
            gbc.weightx = 0.8;
            gbc.weighty = 0.8;
            mainPanel.add (histPanel, gbc);

            _histTextArea = new JTextArea();
            histPanel.setLayout(new BoxLayout (histPanel, BoxLayout.PAGE_AXIS));
            histPanel.add (Box.createRigidArea (new Dimension(0,5)));
            histPanel.add (_histTextArea);
            histPanel.setBorder (BorderFactory.createEmptyBorder (10,10,2,10));
            _histTextArea.setEditable (false);
            _histTextArea.setLineWrap (true);
            _histTextArea.setWrapStyleWord (true);

            _textArea = new JTextArea();
            //_textArea.requestFocusInWindow();
            lowPanel.setLayout(new BoxLayout (lowPanel, BoxLayout.PAGE_AXIS));
            lowPanel.add (Box.createRigidArea (new Dimension(0,2)));
            lowPanel.add (_textArea);
            lowPanel.setBorder (BorderFactory.createEmptyBorder (0,10,10,10));
            _textArea.setEditable (true);
            _textArea.setLineWrap (true);
            _textArea.setWrapStyleWord (true);

            JPanel btnPanel = new JPanel();
            btnPanel.setBorder (new EtchedBorder());
            gbc.gridx = 0;
            gbc.gridy = 1;
            gbc.fill = java.awt.GridBagConstraints.BOTH;
            gbc.weightx = 0.1;
            gbc.weighty = 0.1;
            getContentPane().add (btnPanel, gbc);

            btnPanel.setLayout(new BoxLayout (btnPanel, BoxLayout.LINE_AXIS));
            btnPanel.setBorder(BorderFactory.createEmptyBorder(0, 10, 10, 10));
            btnPanel.add(Box.createHorizontalGlue());
            JButton cancelButton = new JButton ("Cancel");
            btnPanel.add (cancelButton);
            btnPanel.add(Box.createRigidArea(new Dimension(10, 0)));
            JButton sendBtn = new JButton ("Send");
            btnPanel.add (sendBtn);

            _textArea.addKeyListener (new KeyAdapter() {
                public void keyPressed (KeyEvent event) {
                    if (event.getKeyCode() == KeyEvent.VK_ENTER) {
                        String msg = _textArea.getText();
                        sendMessageToPeer (_mo, msg);
                        setMsgText (msg, _nodeUUID, getCurrentTime());
                        _textArea.setText ("");
                    }
                }
            });

            sendBtn.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    String msg = _textArea.getText();
                    sendMessageToPeer (_mo, msg);
                    setMsgText (msg, _nodeUUID, getCurrentTime());
                    _textArea.setText ("");
                }
            });

            cancelButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    _textArea.setText("");
                    dispose();
                }
            });

            //This is the only way to request the focus in the text area.
            this.addWindowListener(new WindowAdapter() {
                 public void windowActivated (WindowEvent e) {
                    _textArea.requestFocus();
                 }
            });

            this.addWindowListener (new WindowAdapter() {
                public void windowClosing (WindowEvent e) {
                    dispose();
                }
            });
        }

        protected void setMsgText (String msg, String peerName, String time)
        {
            _historyMsg.append ("<" + time + "> " + peerName + ": " + msg + "\n");
            _histTextArea.setText (_historyMsg.toString());
            _histTextArea.repaint();
        }

        //Variables
        protected MessageObject _mo;
        protected JTextArea _histTextArea;
        protected JTextArea _textArea;
    }

    //Class variables
    protected GroupManager _grpMgr;

    protected int _numberOfMsgRec = 1;
    protected DefaultListModel _peersListModel;
    protected JDialog _thisDialog = this;
    protected MessageDialog _msgDialog;
    protected JLabel _peerNameLabel;
    protected JList _peersList;
    protected String _message = "";
    protected StringBuffer _historyMsg = new StringBuffer();
    protected String _nodeUUID;

    protected ImageIcon _msgIcon = new ImageIcon (getClass().getResource ("text.gif"));
    protected ImageIcon _voiceIcon = new ImageIcon (getClass().getResource ("voice.gif"));
    protected ImageIcon _msgRecIcon = new ImageIcon (getClass().getResource ("message.gif"));
    protected ImageIcon _speakerIcon = new ImageIcon (getClass().getResource ("speaker.gif"));
}
