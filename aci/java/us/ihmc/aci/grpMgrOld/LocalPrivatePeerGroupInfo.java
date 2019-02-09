package us.ihmc.aci.grpMgrOld;

import javax.crypto.SecretKey;

/**
 * LocalPrivatePeerGroupInfo represents peer-to-peer groups that are protected
 * with a password
 * 
 * @author: Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class LocalPrivatePeerGroupInfo extends LocalGroupInfo
{
    public LocalPrivatePeerGroupInfo()
    {
    }

    public String password;
    public SecretKey key;             // Derived from the password - cached for efficiency
    public String nonce;              // A UUID - regenerated each time GroupManager is started
    public String encryptedGroupName; // Derived by encrypting the group name with the password
    public String encryptedNonce;     // Derived by encrypting the nonce with the password
    public byte[] data;               // The data
    public byte[] encryptedData;      // The encrypted data
}
