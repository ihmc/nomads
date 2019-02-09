package us.ihmc.aci.grpMgrOld;

/**
 * RemotePrivateManagedGroupInfo represents advertized remote private groups
 * 
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class RemotePrivateManagedGroupInfo extends RemoteGroupInfo
{
    public RemotePrivateManagedGroupInfo (String groupName, String creatorUUID, String encryptedNonce)
    {
        this.groupName = groupName;
        this.creatorUUID = creatorUUID;
        this.encryptedNonce = encryptedNonce;
    }

    public String encryptedNonce;    
}
