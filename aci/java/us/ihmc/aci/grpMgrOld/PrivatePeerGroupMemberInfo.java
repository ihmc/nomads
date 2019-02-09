package us.ihmc.aci.grpMgrOld;

/**
 * PrivatePeerGroupMemberInfo represents peer-to-peer member groups that 
 * are protected with a password.
 * 
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class PrivatePeerGroupMemberInfo extends GroupMemberInfo
{
    public PrivatePeerGroupMemberInfo()
    {
    }
    
    String decryptedNonce;
}
