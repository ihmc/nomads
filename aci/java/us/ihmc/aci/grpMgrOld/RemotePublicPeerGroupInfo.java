package us.ihmc.aci.grpMgrOld;

/**
 * RemotePublicPeerGroupInfo represents advertised remote peer groups that do not have passwords
 *
 * @author  Niranjan Suri
 * @version $Revision$
 * $Date$
 */
public class RemotePublicPeerGroupInfo extends RemoteGroupInfo
{
    public RemotePublicPeerGroupInfo (String groupName, String creatorUUID, byte[] data)
    {
        this.groupName = groupName;
        this.creatorUUID = creatorUUID;
        this.data = data;
    }

    public byte[] data;
}
