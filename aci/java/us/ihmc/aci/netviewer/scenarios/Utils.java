package us.ihmc.aci.netviewer.scenarios;

import us.ihmc.aci.nodemon.data.*;
import us.ihmc.aci.nodemon.data.info.*;

import java.util.List;

/**
 * Utility class for scenarios
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Utils
{
    /**
     * Creates a node instance
     * @param id node id
     * @param name node name
     * @param ips node ips list
     * @return the node instance
     */
    public static Node createNode (String id, String name, List<String> ips)
    {
        return createNode (id, name, ips, null);
    }

    /**
     * Creates a node instance
     * @param id node id
     * @param name node name
     * @param ips node ips list
     * @param neighborsIds neighbors ids list
     * @return the node instance
     */
    public static Node createNode (String id, String name, List<String> ips, List<String> neighborsIds)
    {
        Node n = new BaseNode(id, name);

        OperatingSystem os = createDefaultOS();


        CPUInfo[] cpus = createDefaultCPUInfoArray (1);

        NetworkInfo[] nics = createDefaultNetInfoArray (ips);
        HardwareAbstractionLayer hal = new BaseHardwareAbstractionLayer (cpus, nics);

        NodeInfo nodeInfo = new BaseNodeInfo (os, hal);
        n.setNodeInfo (nodeInfo);

        if (neighborsIds != null) {
            for (String nId : neighborsIds) {
                n.getNeighbors().add (new BaseNeighbor (nId));
            }
        }

        return n;
    }

    /**
     * Creates an instance of <code>OperatingSystem</code> with default values
     * @return the instance of <code>OperatingSystem</code> with default values
     */
    public static OperatingSystem createDefaultOS()
    {
        return new BaseOperatingSystem.Builder()
                .name ("Linux")
                .arch ("x86_64")
                .version ("3.13.0.32-generic")
                .machine ("x86_64")
                .vendor ("Ubuntu")
                .vendorVersion ("14.04")
                .description ("Ubuntu 14.04")
                .build();
    }

    /**
     * Creates an array of <code>CPUInfo</code> instances with default values
     * @param n number of object in the array
     * @return the array of <code>CPUInfo</code> instances with default values
     */
    public static CPUInfo[] createDefaultCPUInfoArray (int n)
    {
        CPUInfo[] cpus = new CPUInfo[n];
        for (int i=0; i<n; i++) {
            cpus[i] = createDefaultCPUInfo();
        }

        return cpus;
    }

    /**
     * Creates an instance of <code>CPUInfo</code> with default values
     * @return the instance of <code>CPUInfo</code> with default values
     */
    public static CPUInfo createDefaultCPUInfo()
    {
        return new BaseCPUInfo.Builder()
                .model ("Core(TM) i5-2300 CPU @ 2.80 GHz")
                .vendor ("Intel")
                .totalCores (4)
                .freq (2801)
                .build();
    }

    /**
     * Creates an array of <code>NetworkInfo</code> instances, each of them with an ip from the list
     * @param ips list of ips
     * @return the array of <code>NetworkInfo</code> instances, each of them with an ip from the list
     */
    public static NetworkInfo[] createDefaultNetInfoArray (List<String> ips)
    {
        NetworkInfo[] nics = new BaseNetworkInfo[ips.size()];
        int count = 0;
        for (String ip : ips) {
            nics[count] = createDefaultNetInfo (count, ip);
            count++;
        }

        return nics;
    }

    /**
     * Create an instance of <code>NetworkInfo</code> with the identifier
     * @param i
     * @param ip
     * @return
     */
    public static NetworkInfo createDefaultNetInfo (int i, String ip)
    {
        BaseNetworkInfo.Builder builder = new BaseNetworkInfo.Builder()
                .interfaceName ("eth" + i)
                .macAddress ("E0:69:95:B0:E7:73")
                .ipAddress (ip)
                .broadcast ("10.255.255.255")
                .netmask ("255.255.255.0")
                .mtu (1500);
        if (i == 0) {
            builder.isPrimary (true);
        }

        return builder.build();
    }
}
