package us.ihmc.aci.netSupervisor.information;

import com.google.protobuf.Timestamp;
import us.ihmc.aci.ddam.NetworkHealth;
import us.ihmc.aci.netSupervisor.topology.Subnetwork;
import us.ihmc.aci.netSupervisor.topology.Topology;
import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;

import java.util.Map;

/**
 * Author Roberto Fronteddu on 11/18/2016.
 */
public class InformationBroker
{
    public Topology computeAndGetMergedTopology(WorldStateSummary ws)
    {
        Topology mergedTopology = new Topology();
        //write code here






        return mergedTopology;
    }

    public NetworkHealth getHealthMessage(WorldStateSummary ws)
    {
        NetworkHealth msg = null;
        Timestamp timestamp = getTimestamp(System.currentTimeMillis());
        String localSubnet = ws.getLocalSubnetwork().getName();
        if (localSubnet != null) {
            msg = NetworkHealth
                    .newBuilder()
                    .setSubnet(localSubnet)
                    .setCreationTime(timestamp)
                    .setCollector("NetSupervisor")
                    .setVersion("1.0")
                    .setInterconnectedP(ws.getInterconnectedP())
                    .setBackhaulP(ws.getBackhaulP())
                    .setDelay(ws.getDelay())
                    .setSaturation(ws.getSaturation())
                    .setTotalBandwidthEstimated(ws.getTotalEstimatedBW())
                    .setWithinBandwidthEstimated(ws.getWithinBandwidthEstimated())
                    .build();
        }
        return msg;
    }


    public InformationBroker ()
    {
    }

    //Get the full topology object
    public Map<String, Subnetwork> getTopology()
    {
        // array of topology info divided by subnet
        Map<String, Subnetwork> topology = null;
        return topology;
    }

    //retrieve a map containing the connected couples indexed by subnet ID
    public Map<String, Integer> getConnectedCouplesInformation()
    {
        Map<String, Integer> cc = null;
        return cc;
    }

    public Map<String, Integer> getsubnetCardinality()
    {
        Map<String, Integer> cc = null;
        return cc;
    }

    //Get traffic between two subnetworks
    public Map<String, Double> getTrafficBetweenSubnets()
    {
        Map<String, Double> ttbs = null;
        return ttbs;
    }

    public boolean enqueueHealthMessage(String netname, NetworkHealth msg)
    {
        return true;
    }

    public boolean update()
    {
        return true;
    }

    //Get traffic exchanged by two NetProxies
    public Map<String, Double> getTrafficBetweenNPs()
    {
        Map<String, Double> tbnp = null;
        return tbnp;
    }

    // This will be traffic between two subnet seen by the external interface of a netproxy (if it was mapped we would
    // not be able to see this)
    public Map<String, Double> getNonMappedTraffic()
    {
        Map<String, Double> nmt = null;
        return nmt;
    }

    private Timestamp getTimestamp(long millis)
    {
        return Timestamp.newBuilder().setSeconds(millis / 1000).setNanos((int) ((millis % 1000) * 1000000)).build();
    }




    Map<String, SubnetSummary> _ssMap;
    Map<String, Double> _ceDetectedTopologies;
}
