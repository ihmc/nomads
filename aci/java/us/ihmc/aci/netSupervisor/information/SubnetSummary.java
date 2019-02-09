package us.ihmc.aci.netSupervisor.information;

import java.util.Map;

/**
 * Author Roberto Fronteddu on 12/12/2016.
 */
public class SubnetSummary //This has to be a protobuf object
{
    public boolean setSubnetworkName(String sn)
    {
        if (sn != null) {
            _subnetName = sn;
            return true;
        } else {
            return false;
        }
    }

    public boolean addNode(String nodeIP)
    {
        Integer nodeObject = 2; //This has to be a node object
        nodes.put(nodeIP,nodeObject);
        return true;
    }

    Map<String, Integer> nodes;
    String _subnetName;
}
