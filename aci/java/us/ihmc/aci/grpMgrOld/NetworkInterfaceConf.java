package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;

/**
 * 
 */
public class NetworkInterfaceConf
{
    public NetworkInterfaceConf (InetAddress ip, String advMode)
    {
        this.ip = ip;
        this.advMode = advMode;
    }

    public NetworkInterfaceConf (InetAddress ip, String advMode, InetAddress multicastGroup)
    {
        this.ip = ip;
        this.advMode = advMode; 
        this.multicastGroup = multicastGroup;
    }
    
    public String toString()
    {
        return ip.getHostAddress();
    }

    public InetAddress ip;
    public InetAddress multicastGroup;
    public String advMode;
}

