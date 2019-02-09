package us.ihmc.aci.grpMgrOld;

/**
 * RemotePublicGroupInfo represents advertized remote public groups
 * 
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class RemotePublicManagedGroupInfo extends RemoteGroupInfo
{
    public RemotePublicManagedGroupInfo (String groupName, String creatorUUID)
    {
        this.groupName = groupName;
        this.creatorUUID = creatorUUID;
    }    
}
