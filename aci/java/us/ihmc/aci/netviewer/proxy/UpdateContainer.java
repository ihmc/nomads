package us.ihmc.aci.netviewer.proxy;

import us.ihmc.aci.nodemon.data.traffic.TrafficByIP;
import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Container for updated info coming from the node monitor
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class UpdateContainer
{
    private UpdateContainer (Builder builder)
    {
        _nodeId = builder.nodeId;
        _nodeName = builder.nodeName;
        _ips = builder.ips;
        _nodeInfo = builder.nodeInfo;

        _incomingSummary = builder.incomingSummary;
        _outgoingSummary = builder.outgoingSummary;

        _trafficByIPMap = builder.trafficByIPMap;

        _neighborLinkInfo = builder.neighborLinkInfo;
    }

    /**
     * Gets the node id
     * @return the node id
     */
    public String getNodeId()
    {
        return _nodeId;
    }

    /**
     * Gets the node name
     * @return the node name
     */
    public String getNodeName()
    {
        return _nodeName;
    }

    /**
     * Gets the list of IPs
     * @return the list of IPs
     */
    public List<String> getIPs()
    {
        return _ips;
    }

    /**
     * Gets the node info
     * @return the node info
     */
    public String getNodeInfo()
    {
        return _nodeInfo;
    }

    /**
     * Gets the summary of the incoming traffic for the node which the statistics refer to (aggregated incoming traffic)
     * @return the summary of the incoming traffic
     */
    public TrafficParticle getIncomingSummary()
    {
        return _incomingSummary;
    }

    /**
     * Gets the summary of the outgoing traffic for the node which the statistics refer to (aggregated outgoing traffic)
     * @return the summary of the outgoing traffic
     */
    public TrafficParticle getOutgoingSummary()
    {
        return _outgoingSummary;
    }

    /**
     * Gets the map of the traffic by ip
     * @return the traffic by ip map
     */
    public Map<String, TrafficParticle> getTrafficByIPMap()
    {
        return _trafficByIPMap;
    }

    /**
     * Gets the traffic corresponding to the given ip
     * @param ip ip to look for
     * @return the traffic corresponding to the given ip
     */
    public TrafficParticle getTrafficByIP (String ip)
    {
        if (_trafficByIPMap == null) {
            return null;
        }

        return _trafficByIPMap.get (ip);
    }

    /**
     * Gets the map of the neighbors link info
     * @return the map of the neighbors link info
     */
    public Map<String, NeighborLinkInfo> getNeighborLinkInfo()
    {
        return _neighborLinkInfo;
    }

    private final String _nodeId;
    private final String _nodeName;
    private final List<String> _ips;
    private final String _nodeInfo;

    private final TrafficParticle _incomingSummary;
    private final TrafficParticle _outgoingSummary;

    private final Map<String, TrafficParticle> _trafficByIPMap;

    private final Map<String, NeighborLinkInfo> _neighborLinkInfo;


    /**
     * Implements the builder design pattern for the class <code>UpdateContainer</code>
     */
    static class Builder
    {
        /**
         * Sets the node id
         * @param s node id
         * @return the <code>Builder</code> instance
         */
        Builder nodeId (String s)
        {
            nodeId = s;
            return this;
        }

        /**
         * Sets the node name
         * @param s node name
         * @return the <code>Builder</code> instance
         */
        Builder nodeName (String s)
        {
            nodeName = s;
            return this;
        }

        /**
         * Sets the node ips
         * @param l list of node ips
         * @return the <code>Builder</code> instance
         */
        Builder ips (List<String> l)
        {
            ips = l;
            return this;
        }

        /**
         * Adds an ip to the list
         * @param s ip to add
         * @return the <code>Builder</code> instance
         */
        Builder addIp (String s)
        {
            if (ips == null) {
                ips = new ArrayList<>();
            }

            ips.add (s);
            return this;
        }

        /**
         * Sets the node info string
         * @param s node info string
         * @return the <code>Builder</code> instance
         */
        Builder nodeInfo (String s)
        {
            nodeInfo = s;
            return this;
        }

        /**
         * Sets the neighbor link info
         * @param map neighbor link info map
         * @return the <code>Builder</code> instance
         */
        Builder neighborLinkInfo (Map<String, NeighborLinkInfo> map)
        {
            neighborLinkInfo = map;
            return this;
        }

        /**
         * Adds a new neighbor link info
         * @param id neighbor id
         * @param linkInfo link info
         * @return the <code>Builder</code> instance
         */
        Builder addNeighborLinkInfo (String id, String linkInfo)
        {
            if (neighborLinkInfo == null) {
                neighborLinkInfo = new HashMap<>();
            }

            neighborLinkInfo.put(id, NeighborLinkInfo.create(linkInfo));
            return this;
        }

        /**
         * Sets the summary of the incoming traffic for the node which the statistics refer to (aggregated incoming traffic)
         * @param incomingSummary summary of the incoming traffic
         * @return the <code>Builder</code> instance
         */
        public Builder setIncomingSummary (TrafficParticle incomingSummary)
        {
            this.incomingSummary = incomingSummary;
            return this;
        }

        /**
         * Sets the summary of the outgoing traffic for the node which the statistics refer to (aggregated outgoing traffic)
         * @param outgoingSummary summary of the outgoing traffic
         * @return the <code>Builder</code> instance
         */
        public Builder setOutgoingSummary (TrafficParticle outgoingSummary)
        {
            this.outgoingSummary = outgoingSummary;
            return this;
        }

        /**
         * Sets the traffic by ip map
         * @param traffic traffic by ip map to be used
         * @return the <code>Builder</code> instance
         */
        Builder trafficByIP (Map<String, TrafficParticle> traffic)
        {
            trafficByIPMap = traffic;
            return this;
        }

        /**
         * Adds a new <code>TrafficByIP</code> instance
         * @param ip ip which the traffic refers to
         * @param traffic traffic for a given ip
         * @return the <code>Builder</code> instance
         */
        Builder addTrafficByIP (String ip, TrafficParticle traffic)
        {
            if (trafficByIPMap == null) {
                trafficByIPMap = new HashMap<>();
            }
            trafficByIPMap.put (ip, traffic);
            return this;
        }

        /**
         * Builds the <code>UpdateContainer</code> instance
         * @return the new <code>UpdateContainer</code> instance
         */
        UpdateContainer build()
        {
            return new UpdateContainer (this);
        }


        String nodeId = null;
        String nodeName = null;
        List<String> ips = null;
        String nodeInfo = null;

        TrafficParticle incomingSummary = null;
        TrafficParticle outgoingSummary = null;

        Map<String, TrafficParticle> trafficByIPMap = new HashMap<>();

        Map<String, NeighborLinkInfo> neighborLinkInfo = null;
    }
}
