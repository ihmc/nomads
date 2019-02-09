/**
 * GroupManagerProxy
 *
 * @author      Marco Arguedas      <marguedas@ihmc.us>
 *
 * @version     $Revision$
 *              $Date$
 */

package us.ihmc.aci.grpMgrOld;

import java.util.Enumeration;
import java.util.Vector;

/**
 *
 */
public abstract class GroupManagerProxy implements GroupManagerListener
{
    public abstract String getNodeUUID();
    public abstract String getNodeName (String nodeUUID);
    public abstract void createPublicManagedGroup (String groupName);
    public abstract void createPrivateManagedGroup (String groupName, String password);
    public abstract void createPublicPeerGroup (String groupName, byte[] data);
    public abstract void createPrivatePeerGroup (String groupName, String password, byte[] data);
    public abstract void updatePeerGroupData (String groupName, byte[] data);
    public abstract void joinPublicManagedGroup (String groupName, String creatorUUID);
    public abstract void joinPrivateManagedGroup (String groupName, String creatorUUID, String password);
    public abstract void leaveGroup (String groupName);
    public abstract String startPersistentPeerSearch (String groupName, byte[] param);
    public abstract void stopPersistentPeerSearch (String searchUUID);
    public abstract void respondToPeerSearch (String searchUUID, byte[] param);
    public abstract void sendPeerMessage (String destNodeUUID, byte[] data, boolean reliable, int timeout);
    public abstract void broadcastPeerMessageToGroup (String groupName, byte[] data);
    public abstract void statusChanged();
    public abstract void removeGroup (String groupName);

    /**
     *
     */
    public void addListener (GroupManagerListener gml)
    {
        if (!_listeners.contains(gml)) {
            _listeners.add (gml);
        }
    } //addListener()

    /**
     *
     */
    public void removeListener (GroupManagerListener gml)
    {
        _listeners.remove (gml);
    } //removeListener()

    /**
     *
     */
    public void newPeer (String nodeUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.newPeer (nodeUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //newPeer()

    /**
     *
     */
    public void deadPeer (String nodeUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.deadPeer (nodeUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //deadPeer()

    /**
     *
     */
    public void groupListChange (String nodeUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.groupListChange (nodeUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //groupListChange()

    /**
     *
     */
    public void newGroupMember (String groupName, String memberUUID, byte[] data)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.newGroupMember (groupName, memberUUID, data);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //newGroupMember()

    /**
     *
     */
    public void groupMemberLeft (String groupName, String memberUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.groupMemberLeft (groupName, memberUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //groupMemberLeft()

    /**
     *
     */
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.conflictWithPrivatePeerGroup (groupName, nodeUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //conflictWithPrivatePeerGroup()

    /**
     *
     */
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] data)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.peerGroupDataChanged (groupName, nodeUUID, data);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //peerGroupDataChanged()

    /**
     *
     */
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.peerSearchRequestReceived (groupName, nodeUUID, searchUUID, param);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //peerSearchRequestReceived()

    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.peerMessageReceived (groupName, nodeUUID, data);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    }

    /**
     *
     */
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.peerSearchResultReceived (groupName, nodeUUID, searchUUID, param);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //peerSearchResultReceived()

    /**
     *
     */
    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID)
    {
        Enumeration en = _listeners.elements();
        while (en.hasMoreElements()) {
            try {
                GroupManagerListener gml = (GroupManagerListener) en.nextElement();
                gml.persistentPeerSearchTerminated (groupName, nodeUUID, peerSearchUUID);
            }
            catch (Exception ex) {
                if (DEBUG) {
                    ex.printStackTrace();
                }
            }
        }
    } //peerSearchTerminated()

    // /////////////////////////////////////////////////////////////////////////
    private final static boolean DEBUG = true;

    // /////////////////////////////////////////////////////////////////////////
    private Vector _listeners = new Vector();
}
