package us.ihmc.aci.grpMgrOld;

/**
 * RemoteGroupInfo is the base class that represents advertized remote groups
 *
 * @author  Niranjan Suri
 * @version $Revision$
 * $Date$
 */
public class RemoteGroupInfo extends GroupInfo
{
    public RemoteGroupInfo()
    {
    }

    public String creatorUUID;
    public boolean joined;
}
