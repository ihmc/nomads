package us.ihmc.aci.grpMgrOld;

/**
 * RemotePrivatePeerGroupInfo represents advertised remote private peer groups
 *
 * @author: Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class RemotePrivatePeerGroupInfo extends RemoteGroupInfo
{
    public RemotePrivatePeerGroupInfo (String creatorUUID, String groupName, String encryptedGroupName,
                                       String encryptedNonce, int unencryptedDataLength, byte[] encryptedData)
    {
        this.creatorUUID = creatorUUID;
        this.groupName = groupName;
        this.encryptedGroupName = encryptedGroupName;
        this.encryptedNonce = encryptedNonce;
        this.unencryptedDataLength = unencryptedDataLength;
        this.encryptedData = encryptedData;
    }

    //Class variables
    public int unencryptedDataLength;
    public byte[] encryptedData;
    public String encryptedGroupName;
    public String encryptedNonce;
}
