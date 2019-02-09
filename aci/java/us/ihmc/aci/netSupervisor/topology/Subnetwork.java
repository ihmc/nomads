package us.ihmc.aci.netSupervisor.topology;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.ddam.Host;
import us.ihmc.aci.ddam.Node;

/**
 * Created by Roberto on 8/8/2016.
 */
public class Subnetwork {
    /**
     * Constructor
     * @param name subnetwork name
     * @param subnetMask subnetwork mask
     */
    public Subnetwork (String name, String subnetMask)
    {
        _name = name;
        _subnetMask = subnetMask;
        _nodes = new ArrayList<>();
        _internalGateways = new HashMap<>();
        _exitPoints = new HashMap<>();
    }

    /**
     * Gets the subnetwork name
     * @return the subnetwork name
     */
    public String getName()
    {
        if(_name == null)
            return "";
        else
            return _name;
    }

    /**
     * Gets the subnet mask
     * @return the subnet mask
     */
    public String getSubnetMask()
    {
        return _subnetMask;
    }

    /**
     * Adds a new node internal to the subnetwork
     * @param internalIP node IP
     */
    public void addInternal (String internalIP)
    {
        if (internalIP == null) {
            log.error ("Null internal IP");
            return;
        }

        if (_nodes.contains (internalIP) || internalIP.equals("") || internalIP.equals("128.49.235.242") || internalIP.equals("128.49.235.98")
                || internalIP.equals("128.49.235.202")) {
            return;
        }

        _nodes.add (internalIP);
    }

    /**
     * Gets the internal nodes of the subnetwork
     * @return the internal nodes of the subnetwork
     */
    public List<String> getInternalNodes()
    {
        return _nodes;
    }

    /**
     * Adds the new internal gateway
     * @param internalId gateway node internal id
     * @param host gateway host info
     */
    public void addInternalGW (String internalId, Host host)
    {
        if (internalId == null) {
            log.error ("Null internal id for internal gw");
            return;
        }

        if ((host == null) || (host.getGatewayName() == null) || (host.getGatewayName().isEmpty())) {
            log.error ("Null host or null/empty name for internal gw");
            return;
        }

        if (_internalGateways.containsKey (internalId)) {
            return;
        }

        _internalGateways.put (internalId, host);
    }

    /**
     * Gets the internal id of the node corresponding to the gateway name
     * @param gatewayName gateway name
     * @return the internal id of the node corresponding to the gateway name
     */
    public String getGatewayInternalId (String gatewayName)
    {
        if (gatewayName == null) {
            log.error ("Null gateway name");
            return null;
        }

        for (String internalId : _internalGateways.keySet()) {
            if (gatewayName.equals (_internalGateways.get (internalId).getGatewayName())) {
                return internalId;
            }
        }

        for (String internalId : _exitPoints.keySet()) {
            if (gatewayName.equals (_exitPoints.get (internalId).getGatewayName())) {
                return internalId;
            }
        }

        return null;
    }

    /**
     * Gets the internal gateways for the subnetwork
     * @return the internal gateways
     */
    public Map<String, Host> getInternalGateways()
    {
        return _internalGateways;
    }

    /**
     * Adds the new external gateway or exit point
     * @param internalId gateway node internal id
     * @param host gateway host info
     */
    public void addExitPoint (String internalId, Host host)
    {
        if (internalId == null) {
            log.error ("Null internal id for exit point");
            return;
        }

        if ((host == null) || (host.getGatewayName() == null) || (host.getGatewayName().isEmpty())) {
            log.error ("Null host or null/empty name for exit point");
            return;
        }

        if (_exitPoints.containsKey (internalId)) {
            return;
        }

        _exitPoints.put (internalId, host);
    }

    /**
     * Gets the exit points for the subnetwork
     * @return the exit points
     */
    public Map<String, Host> getExitPoints()
    {
        return _exitPoints;
    }

    /**
     * Check if ipNode is an internal node in the subnetwork
     * @param ipNode node that has to be checked
     * @return the answer
     */
    public boolean isInternalNode(String ipNode){
        if (ipNode != null) {
            if(_nodes != null) {
                for (String ipNs : _nodes) {
                    if(ipNode.equals(ipNs))
                        return true;
                }
            }
        }
        return false;
    }

    /**
     * sets the netproxy of the subnetwork
     * @param netProxyIp
     */
    public void setNetProxy(String netProxyIp){
        _netProxy = netProxyIp;
    }

    /**
     * sets the master of the subnetwork
     * @param nodeMonitorMasterIp
     */
    public void setNodeMonitorMaster(String nodeMonitorMasterIp){
        _nodeMonitorMaster = nodeMonitorMasterIp;
    }

    /**
     *
     * @return netproxy ip
     */
    public String getNetProxy(){
        return _netProxy;
    }

    /**
     *
     * @return master ip
     */
    public String getNodeMonitorMaster(){
        return _nodeMonitorMaster;
    }

    private final String _name;
    private final String _subnetMask;
    private final List<String> _nodes;                  // nodes internal id
    private final Map<String, Host> _internalGateways;  // <internal id for the gw node, host>
    private final Map<String, Host> _exitPoints;        // <internal id for the gw node, host>

    //netProxy and master
    private String _netProxy;
    private String _nodeMonitorMaster;

    private static final Logger log = LoggerFactory.getLogger (Subnetwork.class);
}
