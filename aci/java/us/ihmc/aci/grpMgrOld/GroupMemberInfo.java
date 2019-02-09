package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;
import java.security.PublicKey;


/**
 * GroupMemberInfo is the base class used to represent members of a group
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class GroupMemberInfo
{
    public String nodeUUID;      // UUID of the peer group manager
    public InetAddress addr;     // IP address of the peer group manager
    public int port;             // Port number of the peer group manager
    public PublicKey pubKey;     // The public key of the peer group manager
    public byte[] data;          // The application param.
}
