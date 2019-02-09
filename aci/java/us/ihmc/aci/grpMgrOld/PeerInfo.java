package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;
import java.security.PublicKey;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

/**
 * PeerInfo is the base class used to represent a peer group
 *
 * @author Niranjan Suri
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class PeerInfo
{
    /**
     * Put peer group information in a string.
     *
     * @return a string containing the elements of a peer group
     */
    public String toString()
    {
        String registeredGroups = new String();
        Enumeration en = groups.elements();

        while (en.hasMoreElements()) {
            RemoteGroupInfo rgi = (RemoteGroupInfo) en.nextElement();
            registeredGroups += rgi.groupName + " ";
        }

        return "uuid: " + nodeUUID +
               "\tstateSeqNo: " + stateSeqNo +
               "\tnodeName: " + nodeName +
               "\taddr: " + addr.getHostAddress() +
               "\tgroups: " + registeredGroups +
               "\tpingCount: " + pingCount;
    }

    public String nodeUUID;
    public int stateSeqNo;
    public int grpDataStateSeqNo;
    public int pingInterval;
    public String nodeName;
    public InetAddress addr; // used by manager groups
    public int port; // used by manager groups
    public Vector nicsInfo; // Vector<NICInfo>
    public long lastContactTime;
    public PublicKey pubKey;
    public Hashtable groups;          // Elements are instances of subclasses of RemoteGroupInfo
    public int pingCount;             // Counts the number of ping packets that have been received since the last reset
    public long pingCountResetTime;   // The time when the ping count was last reset
}
