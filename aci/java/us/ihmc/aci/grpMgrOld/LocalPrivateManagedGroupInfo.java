package us.ihmc.aci.grpMgrOld;

import javax.crypto.SecretKey;

/**
 * LocalPrivateManagedGroupInfo represents locally created private groups
 * 
 * @author  Maggie Breedy
 * @version $Revision$
 * $Date$
 */
public class LocalPrivateManagedGroupInfo extends LocalGroupInfo
{
    public LocalPrivateManagedGroupInfo()
    {
    }

    public String password;
    public SecretKey key;         // Derived from the password - cached for efficiency
    public String nonce;          // A UUID - regenerated each time GroupManager is started
    public String encryptedNonce; // Derived by encrypting the nonce with the password
}
