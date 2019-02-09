package us.ihmc.aci.grpMgrOld;

/**
 * Interface that define a listener for group manager events
 * 
 * @author  Maggie Breedy <Nomads Team>
 * @version $Revision$
 * $Date$
 */
public interface GroupManagerListener
{
    public void newPeer (String nodeUUID);
    public void deadPeer (String nodeUUID);
    public void groupListChange (String nodeUUID);
    public void newGroupMember (String groupName, String memberUUID, byte[] data);
    public void groupMemberLeft (String groupName, String memberUUID);
    public void conflictWithPrivatePeerGroup (String groupName, String nodeUUID);
    public void peerGroupDataChanged (String groupName, String nodeUUID, byte[] data);
    
    // Invoked by the GroupManager when a search request is recieved from another node
    // Application may choose to respond by invoking the searchReply() method in the GroupManager
    public void peerSearchRequestReceived (String groupName, String nodeUUID, String searchUUID, byte[] param);

    // Invoked by the GroupManager when a response to a search request is received
    // This may be invoked multiple times, once per response received
    public void peerSearchResultReceived (String groupName, String nodeUUID, String searchUUID, byte[] param);

    // Message received from another peer
    // groupName is null if the message is a direct message to the node (unicast)
    public void peerMessageReceived (String groupName, String nodeUUID, byte[] data);

    public void persistentPeerSearchTerminated (String groupName, String nodeUUID, String peerSearchUUID);
}
