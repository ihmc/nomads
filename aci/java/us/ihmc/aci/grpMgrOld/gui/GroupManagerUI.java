package us.ihmc.aci.grpMgrOld.gui;

import us.ihmc.aci.grpMgr.GroupManagerListener;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.CommException;

import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Toolkit;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.net.Socket;

import java.util.Enumeration;
import java.util.Hashtable;

import javax.swing.JButton;
import javax.swing.border.EtchedBorder;
import javax.swing.DefaultListModel;
import javax.swing.DefaultListCellRenderer;
import javax.swing.ImageIcon;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.ListSelectionModel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSplitPane;
import javax.swing.JTextField;
import javax.swing.SwingConstants;

import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

/**
 * GroupManagerGUI displays two lists: the Local and Remote Groups and allows to
 * create, join, remove and leave a group. It communicates with the GroupManager
 * using a GroupManagerWrapper over TCP sockets.
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 * $Date$
 **/

public class GroupManagerUI implements GroupManagerListener
{
    public GroupManagerUI()
    {
        createGUI();
        _mainWnd.setVisible (true);
        
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
            _localTable = new Hashtable();
            _localMemberTable = new Hashtable();
            _remMemberTable = new Hashtable();
            _remoteTable = new Hashtable();
            _createGroupBtn.setEnabled (true);
            _connPanel.remove (_notConnectedLabel);
			_connPanel.add (_connectedLabel);
            //_connPanel.add (_connectedLabel, JPanel.LEFT_ALIGNMENT);
            _connPanel.revalidate();
            updateLists();            
            commHelper.sendLine ("CLOSE_CONNECTION");
            commHelper.closeConnection();
        }
        catch (Exception e) {
            _createGroupBtn.setEnabled (false);
            _connPanel.remove (_connectedLabel);
			_connPanel.add (_notConnectedLabel);
            //_connPanel.add (_notConnectedLabel, JPanel.LEFT_ALIGNMENT);
            _connPanel.revalidate();
            System.out.println ("-->>Unable to connect to a GroupMananger");
        }        
        if (commHelper == null) {
            initUI();
        }
        
        GroupMgrUIEventMonitor gmem = new GroupMgrUIEventMonitor();
        gmem.start();                
    }
    
    protected void initUI()
    {
        boolean notDone = true;
        while (notDone) {
            try {
                Thread.sleep (5000);  //10000
                CommHelper commHelper = connectToGroupManager (PORT);
                //System.out.println ("-->Connected to port: " + PORT);
                _connPanel.remove (_notConnectedLabel);
				_connPanel.add (_connectedLabel);
                _connPanel.revalidate();
                _createGroupBtn.setEnabled (true);
                notDone = false;
                _localTable = new Hashtable();
                _localMemberTable = new Hashtable();
                _remMemberTable = new Hashtable();
                _remoteTable = new Hashtable();
                updateLists();
                //_connPanel.revalidate();
            }
            catch (Exception ce) {
                _createGroupBtn.setEnabled (false);
                _connPanel.remove (_connectedLabel);
				_connPanel.add (_notConnectedLabel);
                _connPanel.revalidate();
                System.out.println ("-->Waiting to connect to a group manager...");
            }
        }
    }

    /**
     * Close the application window
     */
    public void appCloseRequested()
    {
        try {
            _commHelperEvMonitor.sendLine ("CLOSE_CONNECTION");
            _commHelperEvMonitor.closeConnection();
        }
        catch (CommException ce) {
            System.out.println ("->Connection is clossing<-");
            //ce.printStackTrace();
        }
        
        if (_localTable != null) {
            _localTable.clear();
        }
        if (_localMemberTable != null) {
            _localMemberTable.clear();
        }
        if (_remMemberTable != null) {
            _remMemberTable.clear();
        }
        if (_remoteTable != null) {
            _remoteTable.clear();
        }
        System.exit (0);
    }

    /**
     * Activate the buttons if a local group name is selected
     *
     * @param    selectionIndex    the selected line on the list
     */
    public void localGroupSelectionChanged (int selectionIndex)
    {

        if (selectionIndex < 0) {
            // Nothing is selected
            _removeGroupBtn.setEnabled (false);
            _joinGroupBtn.setEnabled (false);
            _leaveGroupBtn.setEnabled (false);
        }
        else {
            GroupObject localObj = (GroupObject) _localGroupListModel.getElementAt (selectionIndex);
            String localGrpName = localObj._groupName;
            if (localGrpName == null) {
                _removeGroupBtn.setEnabled (false);
                _joinGroupBtn.setEnabled (false);
                _leaveGroupBtn.setEnabled (false);
            }
            else {
                if (localGrpName != null) {
                    _removeGroupBtn.setEnabled (true);
                    _joinGroupBtn.setEnabled (false);
                    _leaveGroupBtn.setEnabled (false);
                }
                else {
                    System.out.println ("-->The local group name is null");
                }
            }
        }
    }

    /**
     * Activate the buttons if a remote group name is selected
     *
     * @param    selectionIndex    the selected line on the list
     */
    public void remoteGroupSelectionChanged (int selectionIndex)
    {
        if (selectionIndex < 0) {
            // Nothing is selected
            _removeGroupBtn.setEnabled (false);
            _joinGroupBtn.setEnabled (false);
            _leaveGroupBtn.setEnabled (false);
        }
        else {
            GroupObject obj = (GroupObject) _remoteGroupListModel.getElementAt (selectionIndex);
            String remoteGrpName = obj._groupName;
            if (remoteGrpName == null) {
                _removeGroupBtn.setEnabled (false);
                _joinGroupBtn.setEnabled (false);
                _leaveGroupBtn.setEnabled (false);
            }
            else {
                if (remoteGrpName != null) {
                    //System.out.println ("-->!!The remote group name: " + remoteGrpName);
                    _removeGroupBtn.setEnabled (false);
                    if (obj._status.indexOf ("PEER") > 0) {
                        //System.out.println ("-->The remote group is a Peer group");
                        _joinGroupBtn.setEnabled (false);
                        _leaveGroupBtn.setEnabled (false);
                    }
                    else {
                        _joinGroupBtn.setEnabled (true);
                    }
                    //System.out.println ("-->The remote group member: " + obj._member);
                    if (obj._member.equals ("MEMBER")) {
                        if (obj._status.indexOf ("PEER") > 0) {
                            _leaveGroupBtn.setEnabled (false);
                        }
                        else {
                            _leaveGroupBtn.setEnabled (true);
                        }
                        _leaveGroupBtn.setEnabled (true);
                        _joinGroupBtn.setEnabled (false);
                    }
                    else {
                        _leaveGroupBtn.setEnabled (false);
                        _joinGroupBtn.setEnabled (true);
                        if (obj._status.indexOf ("PEER") > 0) {
                            _joinGroupBtn.setEnabled (false);
                        }
                         else {
                            _joinGroupBtn.setEnabled (true);
                         }
                    }
                }
                else {
                    System.out.println ("-->The remote group name is null");
                }
            }
        }
    }

    /**
     * Create a Private or Public group using the GroupOptionDialog
     */
    public void handleCreateGroup()
    {
        GroupOptionDialog god = new GroupOptionDialog (_mainWnd, "Create Group");
        god.setVisible (true);
        String reply = null;
        boolean dialogClosed = god.dialogIsClosing();
        if ((!god.wasCancelled()) && (dialogClosed == false)) {
            String groupName = god.getGroupName().trim();
            if (groupName == "") {
                JOptionPane.showMessageDialog (_mainWnd, "Please Enter the Group Name", "Group Error",
                                               JOptionPane.ERROR_MESSAGE);
                return;
            }
            boolean isPrivate = god.isPrivateGroup();
            boolean isPeerGroup = god.isPeerGroup();
            CommHelper commHelper = null;
            try {
                commHelper = connectToGroupManager (PORT);
                if ((isPrivate) && (isPeerGroup == false)) {
                    String password = god.getPassword().trim();
                    if (password == "") {
                        JOptionPane.showMessageDialog (_mainWnd, "Please Enter the Password", "Password Error",
                                                       JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    commHelper.sendLine ("CREATE_PRIVATE_GROUP " + groupName + " " + password);
                }
                else if ((isPrivate) && (isPeerGroup)) {
                    String password = god.getPassword().trim();
                    if (password == "") {
                        JOptionPane.showMessageDialog (_mainWnd, "Please Enter the Password", "Password Error",
                                                       JOptionPane.ERROR_MESSAGE);
                        return;
                    }
                    commHelper.sendLine ("CREATE_PRIVATE_PEER_GROUP " + groupName + " " + password);
                }
                else if ((isPeerGroup) && (isPrivate == false)) {
                    commHelper.sendLine ("CREATE_PUBLIC_PEER_GROUP " + groupName);
                }
                else {
                    commHelper.sendLine ("CREATE_PUBLIC_GROUP " + groupName);
                }
                reply = commHelper.receiveLine();
                if (reply.equals ("OK")) {
                    updateLists();
                }
                else if (reply.equals ("CREATE_GROUP_EXCEPTION")) {
                    printErrorMessage ("Unable to Create Group: " + groupName);
                }
                else {
                    printErrorMessage ("CREATE_GROUP_REPLY: " + reply);
                }
                commHelper.sendLine ("CLOSE_CONNECTION");
                commHelper.closeConnection();
            }
            catch (Exception e) {
                // Group already exists or an error occurred.
                JOptionPane.showMessageDialog (_mainWnd, "Unable to create Group: " + groupName, "Duplicate Group Error",
                                               JOptionPane.ERROR_MESSAGE);
            }
            
        }
    }

    /**
     * Remove a group from the local or remote group list
     */
    public void removeGroup()
    {
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        String groupName = null;
        String reply = null;
        //Only local groups can be removed
        int localIndex = _localGroupList.getSelectedIndex();
        GroupObject localObj = (GroupObject) _localGroupListModel.getElementAt (localIndex);
        String name = localObj._groupName;
        //System.out.println ("-->The name from remove: " + name);
        if (name.indexOf ("@") > 0) {
            groupName = parseGroupName (name);
        }
        else {
            groupName = name;
        }
        if (0 == JOptionPane.showConfirmDialog (_mainWnd, "Remove group " + groupName + "?",
                                                "Confirm Group Deletion", JOptionPane.YES_NO_OPTION,
                                                JOptionPane.QUESTION_MESSAGE)) {
            try {
                commHelper.sendLine ("REMOVE_GROUP " + groupName);
                reply = commHelper.receiveLine();
                //System.out.println ("-->name to remove: " + name);
                if (reply.equals ("OK")) {
                    if (_localTable.containsKey (name)) {
                        _localTable.remove (name);
                        if (_localMemberTable.containsKey (name)) {
                            _localMemberTable.remove (name);
                        }
                        _localGroupListModel.removeElementAt (localIndex);
                    }
                    //System.out.println ("-->UI:REMOVE:_localTable: " + _localTable);
                    //System.out.println ("-->UI:REMOVE:_remoteTable: " + _remoteTable);
                    updateLists();
                }
                else if (reply.equals ("REMOVE_EXCEPTION")) {
                    printErrorMessage ("Unable to Remove the Group: "+ groupName);
                }
                else {
                    printErrorMessage ("REMOVE_GROUP_REPLY: " + reply);
                }
                commHelper.sendLine ("CLOSE_CONNECTION");
                commHelper.closeConnection();
            }
            catch (Exception e) {
                // Group does not exist - should not happen
                e.printStackTrace();
            }
        }        
    }

    /**
     * Join a Private or Public group
     */
    public void joinGroup()
    {
        String reply = null;
        String uuid = null;
        GroupObject obj = (GroupObject) _remoteGroupList.getSelectedValue();
        if (obj == null) {
            return;
        }
        String gname = obj._groupName;
        String groupName = parseGroupName (obj._groupName);
        if (groupName == null) {
            return;
        }
        else if (groupName.equals ("")) {
            _joinGroupBtn.setEnabled (false);
            return;
        }
        else {
            int in = gname.indexOf ("@");
            uuid = gname.substring (in+1, gname.length());
        }
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
            String status = obj._status;
            //System.out.println ("-->UI:The status is: " + status);
            if (status.equals ("PRIVATE")) {
                PasswordDialog pd = new PasswordDialog (_mainWnd);
                pd.setVisible (true);
                String password = pd.getPassword().trim();
                if ((password.equals ("")) || (password == null)) {
                    JOptionPane.showMessageDialog (_mainWnd, "Please Enter the Password", "Password Error",
                                                   JOptionPane.ERROR_MESSAGE);
                    return;
                }
                commHelper.sendLine ("JOIN_PRIVATE_GROUP " + groupName + " " + uuid + " " + password);
            }
            else {
                commHelper.sendLine ("JOIN_PUBLIC_GROUP " + groupName + " " + uuid);
            }
            reply = commHelper.receiveLine();
            //System.out.println ("-->UI:reply: " + reply);
            if (reply.equals ("OK")) {
                updateLists();
            }
            else if (reply.equals ("JOIN_EXCEPTION")) {
                printErrorMessage ("Error Joining the Group: " + groupName);
            }
            else {
                printErrorMessage ("JOIN_GROUP_REPLY: " + reply);
            }
            commHelper.sendLine ("CLOSE_CONNECTION");
            commHelper.closeConnection();
        }
        catch (Exception e) {
            // Unable to join the Group
            JOptionPane.showMessageDialog (_mainWnd, "Unable to Join Group " + groupName,
                                           "Join Group Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    /**
     * Leave a group
     */
    public void leaveGroup()
    {
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        String groupName = null;
        String reply = null;
        int remIndex = _remoteGroupList.getSelectedIndex();
        GroupObject remObj = (GroupObject) _remoteGroupListModel.getElementAt (remIndex);
        String rname = remObj._groupName;
        groupName = parseGroupName (rname);
        if (0 == JOptionPane.showConfirmDialog (_mainWnd, "Leave Group " + groupName + "?",
                                                "Confirm Leave a Group", JOptionPane.YES_NO_OPTION,
                                                JOptionPane.QUESTION_MESSAGE)) {
            try {
                commHelper.sendLine ("LEAVE_GROUP " + groupName);
                reply = commHelper.receiveLine();
                //System.out.println ("-->UILeave group:reply: " + reply);
                if (reply.equals ("OK")) {
                    //System.out.println ("-->UI:Leave:rname: " + rname);
                    if (_remMemberTable.containsKey (rname)) {
                        _remMemberTable.remove (rname);
                        _remMemberTable.put (rname, "NOMEMBER");
                    }
                    String key = parseGroupName (rname);
                    if (_localMemberTable.containsKey (key)) {
                        _localMemberTable.remove (key);
                        _localMemberTable.put (key, "NOMEMBER");
                    }
                    updateLists();
                }
                else if (reply.equals ("LEAVE_EXCEPTION")) {
                    printErrorMessage ("Error: Unable to Leave the Group: " + groupName);
                }
                else {
                    printErrorMessage ("LEAVE_GROUP_REPLY: " + reply);
                }
                commHelper.sendLine ("CLOSE_CONNECTION");
                commHelper.closeConnection();
            }
            catch (Exception e) {
                // Group does not exist - should not happen
                e.printStackTrace();
            }
        }
    }

    /**
     * New Peer is called when a New Peer event is trigger. It updates the remote
     * group list.
     *
     * @param uuid  the uuid of the new remote group
     */
    public void newPeer (String nodeUUID)
    {
        updateLists();
    }
    
    /**
     * Dead Peer is called when a peer event is dead. It updates the remote
     * group list.
     *
     * @param uuid  the uuid of the new remote group
     */
    public void deadPeer (String nodeUUID)
    {
        //System.out.println ("-->UI:deadPeer:then update Lists");
        updateLists();
    }
    
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] param)
    {
        updateLists();
    }
        
    /**
     * Group List Change is called when change event occur in the GroupManager. It
     * updates the local and remote group list.
     *
     * @param uuid  the uuid of the local or remote group
     */
    public void groupListChange (String nodeUUID)
    {
        //System.out.println ("-->UI:groupListChange:then update Lists");
        updateLists();
    }

    public void newGroupMember (String groupName, String memberUUID, byte[] joinData)
    {
        updateLists();
    }
    
    /**
     * Group Member Left is generated when a group leave a remote group. It
     * updates the local and remote group list.
     *
     * @param groupName  the group Name of the remote group
     * @param uuid  the uuid of the local or remote group
     */
    public void groupMemberLeft (String groupName, String memberUUID)
    {
        updateLists();
    }
    
    /**
     * Needs to implement
     * 
     * @param peerUUID  the uuid of the remote peer group
     */
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        updateLists();
    }
    
    /**
     * Needs to implement
     * 
     * @param groupName
     * @param nodeUUID
     * @param searchUUID  the uuid of the peer search
     * @param param       the application defined parameter      
     */
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search request received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID + 
                            ", param: " + new String (param));
    }
    
    /**
     * Needs to implement
     * 
     * @param groupName
     * @param nodeUUID
     * @param searchUUID  the uuid of the peer search
     * @param param       the application defined parameter
     */
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("peer search result received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID + 
                            ", param: " + new String (param));
    }

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
    }

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String searchUUI)
    {
    }
    
    /**
     * Connect to the Group Manager Wrapper using sockets
     *
     * @param PORT port to start connection
     * @param address a string of the machine address
     */
    private CommHelper connectToGroupManager (int port)
        throws Exception
    {
        CommHelper commHelper = new CommHelper();
        String host = "localhost";
        commHelper.init (new Socket (host, port));
        return commHelper;
    }
    
    /**
     * Prints an error message when an exception is thrown.
     * 
     * @param msg   the error message to display.
     */
    protected void printErrorMessage (String msg)
    {
        JOptionPane.showMessageDialog (_mainWnd, msg, "Group Manager Error",
                                       JOptionPane.ERROR_MESSAGE);
        return;
    }

    /**
     * Update the local and remote groups
     */
    private void updateLists()
    {
        getLocalGroupList();
        getRemoteGroupList();
    }        

    /**
     * Update the local groups
     */
    private synchronized void getLocalGroupList()
    {
        _localGroupListModel.clear();
        _localTable.clear();
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
            commHelper.sendLine ("GET_LOCAL_GROUP_LIST");
            while (true) {
                String[] groupNames = commHelper.receiveParsed();
                //System.out.println ("-->LOCAL GROUP Line received 0: " + groupNames[0]);
                //System.out.println ("-->LOCAL GROUP Line received 1: " + groupNames[1]);
                //System.out.println ("-->LOCAL GROUP Line received 2: " + groupNames[2]);
                if (groupNames[0].equalsIgnoreCase ("OK") && groupNames [1].equalsIgnoreCase ("END")) {
                    break;
                }
                else if (groupNames.length == 0) {
                    System.err.println ("No Group has been received");
                }
                else {
                    if (groupNames[0].equals ("PRIVATE")) {
                        String name = groupNames [1];
                        if (!_localTable.containsKey (name)) {
                            _localTable.put (name, "PRIVATE");
                            _localMemberTable.put (name, groupNames[2]);
                        }
                        else {
                            _localTable.remove (name);
                            _localTable.put (name, "PRIVATE");
                            _localMemberTable.remove (name);
                            _localMemberTable.put (name, groupNames[2]);
                        }
                    }
                    else if (groupNames[0].equals ("PUBLIC")) {
                        String name = groupNames [1];
                        if (!_localTable.containsKey (name)) {
                            _localTable.put (name, "PUBLIC");
                            _localMemberTable.put (name, groupNames[2]);
                        }
                        else {
                            _localTable.remove (name);
                            _localTable.put (name, "PUBLIC");
                            _localMemberTable.remove (name);
                            _localMemberTable.put (name, groupNames[2]);
                        }
                    }
                    else if (groupNames[0].equals ("PUBLIC_PEER")) {
                        String name = groupNames [1];
                        //System.out.println ("-->UI:LOCAL GROUP: _localTable1: " + _localTable);
                        if (!_localTable.containsKey (name)) {
                            _localTable.put (name, "PUBLIC_PEER");
                            _localMemberTable.put (name, groupNames[2]);
                        }
                        else {
                            _localTable.remove (name);
                            _localTable.put (name, "PUBLIC_PEER");
                            _localMemberTable.remove (name);
                            _localMemberTable.put (name, groupNames[2]);
                        }
                    }
                    else if (groupNames[0].equals ("PRIVATE_PUBLIC_PEER")) {
                        String name = groupNames [1];
                        if (!_localTable.containsKey (name)) {
                            _localTable.put (name, "PRIVATE_PUBLIC_PEER");
                            _localMemberTable.put (name, groupNames[2]);
                        }
                        else {
                            _localTable.remove (name);
                            _localTable.put (name, "PRIVATE_PUBLIC_PEER");
                            _localMemberTable.remove (name);
                            _localMemberTable.put (name, groupNames[2]);
                        }
                    }
                }
            }
            commHelper.sendLine ("CLOSE_CONNECTION");
            commHelper.closeConnection();
        }
        catch (Exception e) {
            System.out.println ("-->GetLocalGroup:Exception!!!!!");
            e.printStackTrace();
        }
        
        if (!_localTable.isEmpty()) {
            //System.out.println ("-->GetLocalGroup:_localTable: " + _localTable);
            for (Enumeration e = _localTable.keys(); e.hasMoreElements();) {
                String gname = (String) e.nextElement();
                String value = (String) _localTable.get (gname);
                String member = (String) _localMemberTable.get (gname);
                GroupObject obj = new GroupObject (gname, value, member, gname);
                _localGroupListModel.addElement (obj);
                _localGroupList.revalidate();
            }
        }
        else {
            _localGroupList.revalidate();
        }
    }

    /**
     * Update the remote groups
     */
    private synchronized void getRemoteGroupList()
    {
        _remoteGroupListModel.clear();
        _remoteTable.clear();
        CommHelper commHelper = null;
        try {
            commHelper = connectToGroupManager (PORT);
            commHelper.sendLine ("GET_REMOTE_GROUP_LIST");
            while (true) {
                String[] groupRemNames = commHelper.receiveParsed();
                //System.out.println ("-->REMOTE GROUP Line received 0: " + groupRemNames[0]);
                //System.out.println ("-->REMOTE GROUP Line received 1: " + groupRemNames[1]);
                //System.out.println ("-->REMOTE GROUP Line received 2: " + groupRemNames[2]);
                if (groupRemNames [0].equalsIgnoreCase ("OK") && groupRemNames [1].equalsIgnoreCase ("END")) {
                    break;
                }
                if (groupRemNames.length == 0) {
                    System.err.println ("No Group has been received");
                }
                else {
                    if (groupRemNames [0].equals ("PRIVATE")) {
                        String name = groupRemNames [1];
                        if (!_remoteTable.containsKey (name)) {
                            _remoteTable.put (name, "PRIVATE");
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                        else {
                            _remoteTable.remove (name);
                            _remoteTable.put (name, "PRIVATE");
                            _remMemberTable.remove (name);
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                    }
                    else if (groupRemNames [0].equals ("PUBLIC")) {
                        String name = groupRemNames [1];
                        if (!_remoteTable.containsKey (name)) {
                            _remoteTable.put (name, "PUBLIC");
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                        else {
                            _remoteTable.remove (name);
                            _remoteTable.put (name, "PUBLIC");
                            _remMemberTable.remove (name);
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                    }
                    else if (groupRemNames [0].equals ("REMOTE_PEER")) {
                        String name = groupRemNames [1];
                        //System.out.println ("-->UI:Remote GROUP: _remoteTable1: " + _remoteTable);
                        if (!_remoteTable.containsKey (name)) {
                            _remoteTable.put (name, "REMOTE_PEER");
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                        else {
                            _remoteTable.remove (name);
                            _remoteTable.put (name, "REMOTE_PEER");
                            _remMemberTable.remove (name);
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                        //System.out.println ("-->UI:Remote GROUP: _remoteTable2: " + _remoteTable);
                        //System.out.println ("-->UI:Remote GROUP: _remMemberTable: " + _remMemberTable);
                    }
                    else if (groupRemNames [0].equals ("REMOTE_PRIVATE_PEER")) {
                        String name = groupRemNames [1];
                        if (!_remoteTable.containsKey (name)) {
                            _remoteTable.put (name, "REMOTE_PRIVATE_PEER");
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                        else {
                            _remoteTable.remove (name);
                            _remoteTable.put (name, "REMOTE_PRIVATE_PEER");
                            _remMemberTable.remove (name);
                            _remMemberTable.put (name, groupRemNames[2]);
                        }
                    }
                }
            }
            commHelper.sendLine ("CLOSE_CONNECTION");
            commHelper.closeConnection();
        }
        catch (Exception e) {
            System.out.println ("-->GetRemoteGroup:Exception!!!!!");
            e.printStackTrace();
        }
        
        if (!_remoteTable.isEmpty()) {
            for (Enumeration e = _remoteTable.keys(); e.hasMoreElements();) {
                String gname = (String) e.nextElement();
                String value = (String) _remoteTable.get (gname);
                String member = (String) _remMemberTable.get (gname);
                //System.out.println ("-->UI:GetRemoteGroup:The grp member is: " + member);
                
                //Removed the @ symbol from the group name
                String grName = null;
                if (gname.indexOf ("@") > 0) {
                    grName = parseGroupName (gname);
                }
                else {
                    grName = gname;
                }
                GroupObject obj = new GroupObject (gname, value, member, grName);
                _remoteGroupListModel.addElement (obj);
                _remoteGroupList.revalidate();
            }
        }
        else {
            _remoteGroupList.revalidate();
        }        
    }

    protected String parseGroupName (String name)
    {
        String grpName = null;
        if (name.indexOf ("@") > 0) {        
            int in = name.indexOf ("@");
            grpName = name.substring (0, in);
        }
        else {
            grpName = name;
        }
        return grpName;
    }
    
    /**
     * Create the GUI components
     */
    private void createGUI()
    {
        // Create the JFrame
        _mainWnd = new JFrame ("Group Manager");
        _mainWnd.setSize (400, 450);
        _mainWnd.setResizable (true);
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        _mainWnd.setBounds ((screenSize.width-570)/2, (screenSize.height-500)/2, 570, 500);

        // Setup the Layout Manager
        GridBagConstraints gbc = new GridBagConstraints();
        Container contentPane = _mainWnd.getContentPane();
        contentPane.setLayout (new GridBagLayout());

        //Create three panels
        //Buttons Panel
        JPanel btnPanel = new JPanel (new GridBagLayout());
        btnPanel.setBorder (new EtchedBorder());
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 0, 0, 0);
        gbc.weightx = 0.05;
        gbc.weighty = 0.05;
        contentPane.add (btnPanel, gbc);

         // SplitPane for the Lists
        gbc = new GridBagConstraints();
        JPanel rightListPanel = new JPanel (new GridBagLayout());
        JPanel leftListPanel = new JPanel (new GridBagLayout());        
        JSplitPane splitPane = new JSplitPane (JSplitPane.HORIZONTAL_SPLIT);
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.insets = new Insets (0, 0, 0, 0);
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;        
        splitPane.setDividerLocation (265);
        splitPane.setRightComponent (rightListPanel);
        splitPane.setLeftComponent (leftListPanel);
        contentPane.add (splitPane, gbc);
        
        //Text Panel
        gbc = new GridBagConstraints();
        JPanel textPanel = new JPanel (new GridBagLayout());
        textPanel.setBorder (new EtchedBorder());
        textPanel.setBackground (new Color (215,232,242));
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.insets = new Insets (5, 5, 5, 5);
        gbc.weightx = 0.05;
        gbc.weighty = 0.05;
        contentPane.add (textPanel, gbc);

        // Create the Add Group Button
        gbc = new GridBagConstraints();
        _createGroupBtn = new JButton ("Create...");
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (5, 5, 5, 5);
        gbc.weightx = 0.5;
        gbc.weighty = 0;
        btnPanel.add (_createGroupBtn, gbc);
        _createGroupBtn.setEnabled (false);

        _createGroupBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                handleCreateGroup();
            }
        });

         // Create the Join Group Button
        _joinGroupBtn = new JButton ("Join");
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.insets = new Insets (5, 5, 5, 5);
        gbc.weightx = 0.5;
        gbc.weighty = 0;
        btnPanel.add (_joinGroupBtn, gbc);
        _joinGroupBtn.setEnabled (false);

        _joinGroupBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                joinGroup();
            }
        });

        // Create the Delete Group Button
        _removeGroupBtn = new JButton ("Delete");
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.insets = new Insets (5, 5, 5, 5);
        gbc.weightx = 0.5;
        gbc.weighty = 0;
        btnPanel.add (_removeGroupBtn, gbc);
        _removeGroupBtn.setEnabled (false);

        _removeGroupBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                removeGroup();
            }
        });

        // Create the Leave Group Button
        _leaveGroupBtn = new JButton ("Leave");
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridheight = 1;
        gbc.gridwidth = 1;
        gbc.gridx = 3;
        gbc.gridy = 0;
        gbc.insets = new Insets (5, 5, 5, 5);
        gbc.weightx = 0.5;
        gbc.weighty = 0;
        btnPanel.add (_leaveGroupBtn, gbc);
        _leaveGroupBtn.setEnabled (false);

        _leaveGroupBtn.addActionListener (new ActionListener() {
            public void actionPerformed (ActionEvent e)
            {
                leaveGroup();
            }
        });

        //Create local group lists
        gbc = new GridBagConstraints();
        JLabel localLabel = new JLabel ("Local Groups");
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 5, 0, 5);
        gbc.weightx = 0.5;
        leftListPanel.add (localLabel, gbc);

        _localGroupListModel = new DefaultListModel();
        _localGroupList = new JList (_localGroupListModel);
        _localGroupList.setCellRenderer (new GroupListCellRenderer());
        JScrollPane jsp = new JScrollPane (_localGroupList);
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.insets = new Insets (0, 5, 5, 5);
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        leftListPanel.add (jsp, gbc);

        //Create remote group lists
        gbc = new GridBagConstraints();
        JLabel remLabel = new JLabel ("Remote Groups");
        gbc.anchor = GridBagConstraints.EAST;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 5, 0, 5);
        gbc.weightx = 0.5;
        rightListPanel.add (remLabel, gbc);

        _remoteGroupListModel = new DefaultListModel();
        _remoteGroupList = new JList(_remoteGroupListModel);
        _remoteGroupList.setCellRenderer (new GroupListCellRenderer());
        /*ListSelectionModel selectionModel = _remoteGroupList.getSelectionModel();
        selectionModel.setSelectionMode (ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);*/
        JScrollPane jsp2 = new JScrollPane (_remoteGroupList);
        gbc.anchor = GridBagConstraints.NORTHWEST;
        gbc.fill = GridBagConstraints.BOTH;
        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.insets = new Insets (0, 5, 5, 5);
        gbc.weightx = 1.0;
        gbc.weighty = 1.0;
        rightListPanel.add (jsp2, gbc);
        
        //Create textPanel
        gbc = new GridBagConstraints();
        JLabel label1 = new JLabel ("Legend:");
        label1.setFont (new Font ("Dialog", Font.BOLD, 13));
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 10, 0, 5);
        gbc.weightx = 0.1;
        textPanel.add (label1, gbc);
        
        gbc = new GridBagConstraints();
        JLabel label2 = new JLabel ("Member", m, JLabel.RIGHT);
        label2.setHorizontalTextPosition (JLabel.LEFT);
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 0, 0, 5);
        gbc.weightx = 0.1;
        textPanel.add (label2, gbc);
        
        gbc = new GridBagConstraints();
        JLabel label3 = new JLabel ("Peer Group", p, JLabel.RIGHT);
        label3.setHorizontalTextPosition (JLabel.LEFT);
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 2;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 0, 0, 5);
        gbc.weightx = 0.1;
        textPanel.add (label3, gbc);
        
        gbc = new GridBagConstraints();
        JLabel label4 = new JLabel ("Private Group", r, JLabel.RIGHT);
        label4.setHorizontalTextPosition (JLabel.LEFT);
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 3;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 0, 0, 5);
        gbc.weightx = 0.1;
        textPanel.add (label4, gbc);
        
        gbc = new GridBagConstraints();
        _connPanel = new JPanel();
        _connPanel.setBackground (new Color (215,232,242));
        _connPanel.setBorder (new EtchedBorder());
        gbc.anchor = GridBagConstraints.SOUTH;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 4;
        gbc.gridy = 0;
        gbc.insets = new Insets (0, 0, 0, 0);
        gbc.weightx = 0.1;
        textPanel.add (_connPanel, gbc);
        
        //Connection icon
        _notConnectedLabel = new JLabel ("not Connected", notConnected, JLabel.RIGHT);
        _notConnectedLabel.setHorizontalTextPosition (JLabel.LEFT);
        //_connPanel.add (_notConnectedLabel, JPanel.LEFT_ALIGNMENT);
		_connPanel.add (_notConnectedLabel);
        
        _connectedLabel = new JLabel ("Connected", connected, JLabel.RIGHT);
        _connectedLabel.setHorizontalTextPosition (JLabel.LEFT);        
            
        _localGroupList.addListSelectionListener (new ListSelectionListener() {
            public void valueChanged (ListSelectionEvent e)
            {
                if (e.getValueIsAdjusting() == false) {
                    int selIndex = _localGroupList.getSelectedIndex();
                    if (selIndex < 0) {
                        _joinGroupBtn.setEnabled (false);
                        _leaveGroupBtn.setEnabled (false);
                    }
                    else {
                        _remoteGroupList.clearSelection();
                        _isLocalGroup = true;
                        localGroupSelectionChanged (selIndex);
                    }
                }
            }
        });

        _remoteGroupList.addListSelectionListener (new ListSelectionListener() {
			public void valueChanged (ListSelectionEvent e)
			{
				if (e.getValueIsAdjusting() == false) {
					//int[] selectedIndicesArray = _remoteGroupList.getSelectedIndices();
					//if (selectedIndicesArray.length <= 1) {
					int selIndex = _remoteGroupList.getSelectedIndex();
					if (selIndex < 0) {
						_joinGroupBtn.setEnabled (false);
						_leaveGroupBtn.setEnabled (false);
					}
					else {
						GroupObject obj = (GroupObject) _remoteGroupListModel.getElementAt (selIndex);
						if (obj._status.indexOf ("PEER") > 0) {
							_joinGroupBtn.setEnabled (true);
						}
						else {
							_joinGroupBtn.setEnabled (false);
						}
						_joinGroupBtn.setEnabled (true);
						_localGroupList.clearSelection();
						_isLocalGroup = false;
						remoteGroupSelectionChanged (selIndex);
					}
				}
			}
        });

        _mainWnd.addWindowListener(new WindowAdapter() {
            public void windowClosing (WindowEvent e) {
                appCloseRequested();
                _mainWnd.dispose();
            }
        });
    }   

    public static void main (String[] args)
    {
        GroupManagerUI gmui = new GroupManagerUI();
    }

    /**
     * The GroupMgrUIEventMonitor thread is listening for incoming events.
     */
    protected class GroupMgrUIEventMonitor extends Thread
    {
        public GroupMgrUIEventMonitor()
        {
        }

        public void run()
        {
            try {
                _commHelperEvMonitor = connectToGroupManager (PORT);
                _commHelperEvMonitor.sendLine ("GET_EVENTS ");
                
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
            try {
                while (true) {
                    String uuid = null;
                    String[] groupEventNames = _commHelperEvMonitor.receiveParsed();
                    //System.out.println ("-->Events:UI:Line received 0: " + groupEventNames[0]);
                    //System.out.println ("-->Events:UI:Line received 1: " + groupEventNames[1]);
                    //System.out.println ("-->Events:UI:Line received 2: " + groupEventNames[2]);
                    //System.out.println ("-->Events:UI:Line received 3: " + groupEventNames[3]);
                    if (groupEventNames.length <= 0) {
                        continue;
                    }
                    else {
                        String eventName = groupEventNames [0].trim();
                        if (eventName.equalsIgnoreCase ("NEW_PEER")) {
                            uuid = groupEventNames [1];
                            newPeer (uuid);
                        }
                        else if (eventName.equalsIgnoreCase ("DEAD_PEER")) {
                            uuid = groupEventNames [1];
                            deadPeer (uuid);
                        }
                        else if (eventName.equalsIgnoreCase ("GROUP_LIST_CHANGED")) {
                            uuid = groupEventNames [1];
                            groupListChange (uuid);
                        }
                        else if (eventName.equalsIgnoreCase ("NEW_GROUP_MEMBER")) {
                            String newGroupName = groupEventNames [1];
                            uuid = groupEventNames [2];
                            newGroupMember (newGroupName, uuid, null);                            
                        }
                        else if (eventName.equalsIgnoreCase ("GROUP_MEMBER_LEFT")) {
                            String newGroupName = groupEventNames [1];
                            uuid = groupEventNames [2];
                            groupMemberLeft (newGroupName, uuid);
                        }
                        else if (eventName.equalsIgnoreCase ("CONFLICT_PRIVATE_PEER")) {
                            String newGroupName = groupEventNames [1];
                            String peerUUID = groupEventNames [2];
                            conflictWithPrivatePeerGroup (newGroupName, peerUUID);
                        }
                        else {
                            System.out.println ("-->ERROR: Unknown event occurred: " + groupEventNames[0]);
                        }
                    }
                }
            }
            catch (Exception e) {
                _createGroupBtn.setEnabled (false);
                _connPanel.remove (_connectedLabel);
				_connPanel.add (_notConnectedLabel);
                _localGroupListModel.clear();
                _remoteGroupListModel.clear();
                _connPanel.revalidate();
                initUI();
                //e.printStackTrace();               
            }
        }
    }
    
    //Class Variables
    protected boolean _isLocalGroup = false;
    protected CommHelper _commHelperEvMonitor = null;
    protected DefaultListModel _localGroupListModel;
    protected DefaultListModel _remoteGroupListModel;
    protected Hashtable _localTable;
    protected Hashtable _localMemberTable;
    protected Hashtable _remoteTable;
    protected Hashtable _remMemberTable;
    protected ImageIcon blank = new ImageIcon (getClass().getResource ("blank.gif"));
    protected ImageIcon connected = new ImageIcon (getClass().getResource ("connected.gif"));
    protected ImageIcon notConnected = new ImageIcon (getClass().getResource ("notConnected.gif"));
    protected ImageIcon m = new ImageIcon (getClass().getResource ("M.gif"));
    protected ImageIcon p = new ImageIcon (getClass().getResource ("P.gif"));
    protected ImageIcon r = new ImageIcon (getClass().getResource ("R.gif"));
    protected JButton _createGroupBtn;
    protected JButton _removeGroupBtn;
    protected JButton _joinGroupBtn;
    protected JButton _leaveGroupBtn;
    protected JFrame _mainWnd;
    protected JLabel _connectedLabel;
    protected JLabel _notConnectedLabel;
    protected JList _localGroupList;
    protected JList _remoteGroupList;
    protected JPanel _connPanel;

    public static final int PORT = 8501;
}
