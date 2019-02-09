package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;

import us.ihmc.net.NICInfo;

/**
 *
 * @author Matteo Rebeschini
 */
public class UDPMulticastNoRelayNetworkInterface extends UDPMulticastNetworkInterface
{
    public String toString()
    {
        if (_nicInfo != null) {
            return _nicInfo.toString() + " [mode: UDP_MULTICAST_NORELAY] [group: " +
                   _multicastGroup.getHostAddress() + "]";
        }
        else {
            return "UDP_MULTICAST - group: " + _multicastGroup.getHostAddress();
        }
    }
}

