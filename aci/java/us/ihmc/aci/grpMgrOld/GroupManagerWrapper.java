package us.ihmc.aci.grpMgrOld;

import java.io.FileOutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Properties;
import java.util.Vector;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;

/**
 * GroupManagerWrapper instantiates the GroupManager and provide GroupManagerGUI
 * with methods to create, join, remove and leave groups.
 * They communicate using TCP sockets.
 *
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 * $Date$
 **/
public class GroupManagerWrapper extends Thread implements GroupManagerListener
{
    public GroupManagerWrapper (GroupManager grpMgr)
    {
        _grpMgr = grpMgr;
        try {
            _grpMgr.setNodeName (InetAddress.getLocalHost().getCanonicalHostName());
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        _grpMgr.setListener (this);
    }

    /**
     * Start a thread and a connection handler on port 8501
     */
    public void run()
    {
        System.out.println ("Waiting for Connection...");
        try {
            _adminSocket = new ServerSocket (PORT);
            _running = true;
            // Accept connections
            while (!_terminate) {
                Socket guiConnection = _adminSocket.accept();
                ConnectionHandler handler = new ConnectionHandler (_grpMgr, guiConnection);
                handler.start();
                //System.out.println ("Got a connection - created a handler");
            }
            _running = false;
            _terminated = true;
        }
        catch (Exception x){
            System.out.println ("-->>GroupManagerWrapper:Connection closed");
            x.printStackTrace();
        }
    }

    /**
     * Set the listener to the group manager listener for listening
     * to incoming events.
     *
     * @param gml  the group manager listener
     */
    public void setListener (GroupManagerListener gml)
    {
        _listener = gml;
    }

    /**
     * Returns the running state of this group manager wrapper thread.
     *
     * @return     true if the instance is running, false otherwise
     */
    public boolean isRunning()
    {
        return _running;
    }

    /**
     * Request the group manager wrapper thread to terminate. Caller should
     * wait until hasTerminated() returns true.
     */
    public void terminate()
    {
        _terminate = true;
        try {
            if (!_adminSocket.isClosed()) {
                _adminSocket.close();
            }
        }
        catch (Exception exc) {
            exc.printStackTrace();
        }
    }

    /**
     * Ask the group manager wrapper thread if it has terminated
     *
     * @return     true if the instance has terminated, false otherwise
     */
    public boolean hasTerminated()
    {
        return _terminated;
    }

    /**
     * Create a public group with the specified group name.
     *
     * @param   groupName      the public group name
     * @param   commHelper     application commHelper
     */
    protected synchronized void createPublicManagedGroup (String groupName, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.createPublicManagedGroup (groupName);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("CREATE_GROUP_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Create a Private group with the specified group name and password.
     *
     * @param   groupName   the private group name
     * @param   password    the password needed for private groups
     * @param   commHelper  application commHelper
     */
    protected synchronized void createPrivateManagedGroup (String groupName, String password, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.createPrivateManagedGroup (groupName, password);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("CREATE_GROUP_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Create a public peer group with the specified group name.
     *
     * @param groupName      the name of the group
     * @param commHelper     application commHelper
     */
    protected synchronized void createPublicPeerGroup (String groupName, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.createPublicPeerGroup (groupName, null);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("CREATE_GROUP_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
      * Create a private peer group with the specified group name, password, and optional parameter.
     *
     * @param groupName      the name of the private peer group
     * @param password       the password for the private peer group
     * @param commHelper     application commHelper
     */
    protected synchronized void createPrivatePeerGroup (String groupName, String password, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.createPrivatePeerGroup (groupName, password, null);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("CREATE_GROUP_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Join a private group
     *
     * @param   groupName   the private group name
     * @param   password    the password needed for private groups
     * @param commHelper     application commHelper
     */
    protected synchronized void joinPrivateManagedGroup (String groupName, String nodeUUID, byte[] joinData, int joinDataLen,
                                                         int mode, String password, CommHelper commHelper)
    {
        try {
            String creatorUUID = nodeUUID;
            _grpMgr.joinPrivateManagedGroup (groupName, creatorUUID, password, mode, joinData, joinDataLen);
            commHelper.sendLine ("OK");
        }
        catch (GroupManagerException gme) {
            try {
                commHelper.sendLine ("JOIN_EXCEPTION");
            }
            catch (Exception exx) {
                exx.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage() + ". The password might be invalid");
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Join a public group with the specified group name.
     *
     * @param groupName      the name of the group
     * @param nodeUUID           the nodeUUID of the group
     * @param commHelper     application commHelper
     */
    protected synchronized void joinPublicManagedGroup (String groupName, String nodeUUID, byte[] joinData,
                                                        int joinDataLen, int mode, CommHelper commHelper)
    {
        try {
            try {
                String creatorUUID = nodeUUID;
                _grpMgr.joinPublicManagedGroup (groupName, creatorUUID, mode, joinData, joinDataLen);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("JOIN_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("CommHelper ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Remove a group with the specified group name.
     *
     * @param groupName   the group name
     * @param commHelper  application commHelper
     */
    protected synchronized void removeGroup (String groupName, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.removeGroup (groupName);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("REMOVE_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Leave a group with the specified group name.
     *
     * @param   groupName   the group name
     */
    public synchronized void leaveGroup (String groupName, CommHelper commHelper)
    {
        try {
            try {
                _grpMgr.leaveGroup (groupName);
                //System.out.println ("-->>Wrapper:Leaving groupName: " + groupName);
                commHelper.sendLine ("OK");
            }
            catch (GroupManagerException gme) {
                commHelper.sendLine ("LEAVE_EXCEPTION");
                gme.printStackTrace();
            }
        }
        catch (CommException ce) {
            try {
                commHelper.sendLine ("ERROR " + ce.getMessage());
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    /**
     * Get the local group list (Private or Public)
     * from the GroupManager
     */
    protected synchronized void getLocalGroupList (CommHelper commHelper)
    {
        Hashtable groups = _grpMgr.getGroups();
        //System.out.println ("-->Wrapper:LOCAL GROUP:groups HT: " + groups);
        if (groups == null) {
            System.out.println ("-->GroupMangerWrapper:Local groups Hashtable is null!");
        }
        for (Enumeration e = groups.keys(); e.hasMoreElements();) {
            String lgName = (String) e.nextElement();
            GroupInfo grpInfo = (GroupInfo) groups.get (lgName);
            Hashtable members = grpInfo.members;
            String membership = null;
            if (members.isEmpty()) {
                membership = " NOMEMBER";
            }
            else {
                /*for (Enumeration en = members.keys(); en.hasMoreElements();) {
                    String nodeUUID = (String) en.nextElement();
                    System.out.println ("-->Wrapper:members HT nodeUUID: " + nodeUUID);
                }*/
                membership = " MEMBER";
            }
            if (grpInfo instanceof LocalPrivateManagedGroupInfo) {
                try {
                    commHelper.sendLine ("PRIVATE " + lgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("LocalPrivateManagedGroupInfo ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof LocalPublicManagedGroupInfo) {
                try {
                    commHelper.sendLine ("PUBLIC " + lgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("LocalPublicManagedGroupInfo ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof LocalPublicPeerGroupInfo) {
                try {
                    commHelper.sendLine ("PUBLIC_PEER " + lgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("LocalPublicPeerGroupInfo ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof LocalPrivatePeerGroupInfo) {
                try {
                    commHelper.sendLine ("PRIVATE_PUBLIC_PEER " + lgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("LocalPrivatePeerGroupInfo ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
        }
        try {
            commHelper.sendLine ("OK END OK");
        }
        catch (CommException exx) {
            exx.printStackTrace();
        }
    }

    /**
     * Get the remote group list from the GroupManager
     */
    protected synchronized void getRemoteGroupList (CommHelper commHelper)
    {
        Hashtable groups = _grpMgr.getGroups();
        //System.out.println ("-->GroupMangerWrapper:Groups Hashtable:Remote groups: " + groups);
        if (groups == null) {
            System.out.println ("-->GroupMangerWrapper:Remote groups Hashtable is null!");
        }
        for (Enumeration e = groups.keys(); e.hasMoreElements();) {
            String rgName = (String) e.nextElement();
            GroupInfo grpInfo = (GroupInfo) groups.get (rgName);
            String membership = null;

            if (grpInfo instanceof RemoteGroupInfo) {
                boolean isJoined = ((RemoteGroupInfo) grpInfo).joined;
                if (isJoined) {
                    membership = " MEMBER";
                }
                else {
                    membership = " NOMEMBER";
                }
            }

            if (grpInfo instanceof RemotePrivateManagedGroupInfo) {
                try {
                    commHelper.sendLine ("PRIVATE " + rgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("ERROR " + ce.getMessage());
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof RemotePublicManagedGroupInfo) {
                try {
                    commHelper.sendLine ("PUBLIC " + rgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof RemotePublicPeerGroupInfo) {
                try {
                    commHelper.sendLine ("REMOTE_PEER " + rgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
            else if (grpInfo instanceof RemotePrivatePeerGroupInfo) {
                try {
                    commHelper.sendLine ("REMOTE_PRIVATE_PEER " + rgName + membership);
                }
                catch (CommException ce) {
                    try {
                        commHelper.sendLine ("ERROR " + ce.getMessage() + " ");
                    }
                    catch (Exception ex) {
                        ex.printStackTrace();
                    }
                }
            }
        }
        try {
            commHelper.sendLine ("OK END OK");
        }
        catch (CommException exx) {
            exx.printStackTrace();
        }
    }

    /**
     * Terminates the Group Manager when a connection close is requested.
     *
     * @param commHelper    application commHelper
     */
    public void appCloseRequested (CommHelper commHelper)
    {
        Hashtable settings = null;
        try {
            settings = _grpMgr.getSettingsSnapshot();
        }
        catch (GroupManagerException gme) {
            gme.printStackTrace();
        }
        Enumeration e = settings.keys();
        Properties props = new Properties();
        while (e.hasMoreElements()) {
            String key = (String) e.nextElement();
            String value = (String) settings.get (key);
            props.setProperty (key, value);
        }
        try {
            FileOutputStream fos = new FileOutputStream ("grpMgr.cfg");
            props.store (fos, "GroupManager Settings");
            fos.close();
        }
        catch (Exception xcp) {
            xcp.printStackTrace();
        }
        try {
            //Sends an Ok to the GroupManagerUI so it can close the connection.
            commHelper.sendLine ("OK");
            commHelper.closeConnection();
        }
        catch (CommException exx) {
            exx.printStackTrace();
        }
    }

    /**
     * Add a commHelper connection to a connections vector.
     *
     * @param commHelper    application commHelper connection
     */
    public void addConnection (CommHelper commHelper)
    {
        //System.out.println ("-->Wrapper:adding Connection!");
        if (commHelper != null) {
            _connectionVector.addElement (commHelper);
        }
    }

    /**
     * A new peer event occurred then the GUI is updated.
     *
     * @param nodeUUID    the nodeUUID of the group.
     */
    public void newPeer (String nodeUUID)
    {
        if (_listener != null) {
            _listener.newPeer (nodeUUID);
        }
        // Generate and send an event to the GUI
        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a NEW_PEER event UUID: " + nodeUUID);
            try {
                ch.sendLine ("NEW_PEER " + nodeUUID + " " + "PEER" + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

    /**
     * A dead peer event occurred then the GUI is updated.
     *
     * @param nodeUUID    the nodeUUID of the group
     */
    public void deadPeer (String nodeUUID)
    {
        if (_listener != null) {
            _listener.deadPeer (nodeUUID);
        }
        // Generate and send an event to the GUI
        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a DEAD_PEER event UUID: " + nodeUUID);
            try {
                ch.sendLine ("DEAD_PEER " + nodeUUID + " " + "DEADPEER" + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

    /**
     * A new Group Member event occurred then the GUI is updated.
     *
     * @param groupName     the name of the group
     * @param memberUUID    the member nodeUUID of the group
     */
    public void newGroupMember (String groupName, String memberUUID, byte[] joinData)
    {
        if (_listener != null) {
            _listener.newGroupMember (groupName, memberUUID, joinData);
        }
        // Generate and send new group member event to the GUI
        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a NEW_GROUP_MEMBER event memberUUID: " + memberUUID);
            try {
                ch.sendLine ("NEW_GROUP_MEMBER " + groupName + " " + memberUUID + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

    /**
     * A group member left event occurred then the GUI is updated.
     *
     * @param groupName     the name of the group
     * @param memberUUID    the member nodeUUID of the group
     */
    public void groupMemberLeft (String groupName, String memberUUID)
    {
        if (_listener != null) {
            _listener.groupMemberLeft (groupName, memberUUID);
        }
        // Generate and send an event to the GUI
        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a GROUP_MEMBER_LEFT event memberUUID: " + memberUUID);
            try {
                ch.sendLine ("GROUP_MEMBER_LEFT " + groupName + " " + memberUUID + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

    /**
     * A new group list change event occurred then the GUI is updated.
     *
     * @param nodeUUID    the nodeUUID of the group.
     */
    public void groupListChange (String nodeUUID)
    {
        if (_listener != null) {
            _listener.groupListChange (nodeUUID);
        }
        // Generate and send an event to the GUI
        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a GROUP_LIST_CHANGED event nodeUUID: " + nodeUUID);
            try {
                ch.sendLine ("GROUP_LIST_CHANGED " + nodeUUID + " " + "GRPLIST" + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

    /**
     * A conflict with Private peer group event occurred then the GUI is updated.
     *
     * @param groupName     the name of the group
     * @param nodeUUID      the peer group nodeUUID
     */
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        if (_listener != null) {
            _listener.conflictWithPrivatePeerGroup (groupName, nodeUUID);
        }

        for (Enumeration e = _connectionVector.elements(); e.hasMoreElements();) {
            CommHelper ch = (CommHelper) e.nextElement();
            //System.out.println ("-->Wrapper:Sending a CONFLICT_PRIVATE_PEER event nodeUUID: " + nodeUUID);
            try {
                ch.sendLine ("CONFLICT_PRIVATE_PEER " + nodeUUID + " " + "CONFLICT" + " OK");
            }
            catch (Exception exp) {
                exp.printStackTrace();
            }
        }
    }

     /**
     * A peer group param changed event occurred then the GUI is updated.
     *
     * @param groupName     the name of the group
     * @param nodeUUID      the peer group nodeUUID
     * @param param         parameter attached to the group - can be null
     */
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] param)
    {
        /*!!*/ // Need to finish implementation
        if (_listener != null) {
            _listener.peerGroupDataChanged (groupName, nodeUUID, param);
        }
    }

    /**
     * A peer search request received event
     *
     * @param groupName
     * @param nodeUUID
     * @param searchUUID    the peer search nodeUUID
     * @param param         parameter attached to the group - can be null
     */
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("GroupManagerWrapper: peer search request received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID + ", param: " + new String (param));
    }

    /**
     * A peer search request received event
     *
     * @param groupName
     * @param nodeUUID
     * @param searchUUID    the peer search nodeUUID
     * @param param         parameter attached to the group - can be null
     */
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        System.out.println ("GroupManagerWrapper: peer search result received - groupName: " + groupName +
                            ", nodeUUID: " + nodeUUID + ", searchUUID: " + searchUUID + ", param: " + new String (param));
    }

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
    }

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String searchUUI)
    {
    }

     /**
      * Inner class that Handles the socket connection
      *
      * @param grpMgr    the groupManger instatiated by the group manager wrapper
      * @param sock      the server socket that accept connections
      *
      */
    protected class ConnectionHandler extends Thread
    {
        public ConnectionHandler (GroupManager grpMgr, Socket sock)
        {
            _grpMgr = grpMgr;
            _commHelper = new CommHelper();
            _commHelper.init (sock);
        }

        /**
         * Parse the request from the Group Manager GUI
         */
        public void run()
        {
            String[] requestList = null;
            try {
                while (true) {
                    requestList = _commHelper.receiveParsed();
                    //System.out.println (">>>>FirstArg:requestList length: " + requestList.length);
                    if (requestList.length <= 0) {
                        continue;
                    }
                    //System.out.println (">>>>FirstArg:Wrapper: " + requestList [0]);
                    if (requestList[0].equals ("CREATE_PRIVATE_GROUP")) {
                        String groupName = requestList[1];
                        String password = requestList[2];
                        createPrivateManagedGroup (groupName, password, _commHelper);
                    }
                    else if (requestList[0].equals ("CREATE_PUBLIC_GROUP")) {
                        String groupName = requestList[1];
                        //System.out.println (">>>>FirstArg:Wrapper:Name: " + requestList [1]);
                        createPublicManagedGroup (groupName, _commHelper);
                    }
                    else if (requestList[0].equals ("CREATE_PRIVATE_PEER_GROUP")) {
                        String groupName = requestList[1];
                        String password = requestList[2];
                        createPrivatePeerGroup (groupName, password, _commHelper);
                    }
                    else if (requestList[0].equals ("CREATE_PUBLIC_PEER_GROUP")) {
                        String groupName = requestList[1];
                        createPublicPeerGroup (groupName, _commHelper);
                    }
                    else if (requestList[0].equals ("JOIN_PRIVATE_GROUP")) {
                        String groupName = requestList[1];
                        String nodeUUID = requestList[2];
                        String password = requestList[3];
                        //byte[] joinData = requestList[3];
                        /**(String modeStr = requestList[4];
                        int mode = 0;
                        if (Integer.parseInt (modeStr) > 0) {
                            mode = Integer.parseInt (modeStr);
                        }**/

                        joinPrivateManagedGroup (groupName, nodeUUID, null, 0, 0, password, _commHelper);
                    }
                    else if (requestList[0].equals ("JOIN_PUBLIC_GROUP")) {
                        String groupName = requestList[1];
                        String nodeUUID = requestList[2];
                        /**byte[] joinData = requestList[3];
                        //String modeStr = requestList[4];
                        int mode = 0;
                        if (Integer.parseInt (modeStr) > 0) {
                            mode = Integer.parseInt (modeStr);
                        }**/
                        joinPublicManagedGroup (groupName, nodeUUID, null, 0, 0, _commHelper);
                    }
                    else if (requestList[0].equals ("REMOVE_GROUP")) {
                        String groupName = requestList[1];
                        removeGroup (groupName, _commHelper);
                    }
                    else if (requestList[0].equals ("LEAVE_GROUP")) {
                        String groupName = requestList[1];
                        leaveGroup (groupName, _commHelper);
                    }
                    else if (requestList[0].equals ("GET_LOCAL_GROUP_LIST")) {
                        getLocalGroupList (_commHelper);
                    }
                    else if (requestList[0].equals ("GET_REMOTE_GROUP_LIST")) {
                        getRemoteGroupList (_commHelper);
                    }
                    else if (requestList[0].equals ("CLOSE_CONNECTION")) {
                        appCloseRequested (_commHelper);
                        break;
                    }
                    else if (requestList[0].equals ("GET_EVENTS")) {
                        addConnection (_commHelper);
                        break;
                    }
                }
            }
            catch (CommException ce) {
                System.out.println ("-->>GroupManagerWrapper:CommException:Received Parsed Error");
                ce.printStackTrace();
            }
        }

        protected CommHelper _commHelper;
        protected Socket _sock;
    }

    //Class Variables
    protected boolean _running = false;
    protected boolean _terminate = false;
    protected boolean _terminated = false;
    protected GroupManager _grpMgr;
    protected GroupManagerListener _listener = null;
    protected ServerSocket _adminSocket;
    protected Vector _connectionVector = new Vector();
    public static final int PORT = 8501;
}
