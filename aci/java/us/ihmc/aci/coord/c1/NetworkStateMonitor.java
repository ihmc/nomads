package us.ihmc.aci.coord.c1;

import java.util.Enumeration;

import us.ihmc.aci.coord.NodeInfo;

/**
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 * @ $Date$
 * @ Created on Oct 11, 2006 at 6:40:42 PM
 * @ Copyright (c) 2006 Florida Institute for Human and Machine Cognition
 */
public class NetworkStateMonitor
{
    public NetworkStateMonitor(NodeStateMonitor nodeStateMonitor)
    {
        _nodeStateMonitor = nodeStateMonitor;
    }

    /**
     * This method estimates the bandwidth avaiable between nodes (sourceUUID) and
     * (destUUID). The estimate assumes that a) all nodes are within comms range from
     * each other - fully connected graph, b) There is no capacity degradation due to interference.
     *
     * @param sourceUUID
     * @param destUUID
     * @return the bandwidth available between sourceUUID and destUUID, in the same unit
     *         as NodeInfo.networkLinkSpeed. The method returns (-1) if such information
     *         is not available. Returns (0) if network is saturated.
     */
    public double getBandwidthAvailableBetween (String sourceUUID, String destUUID)
    {
        double accLoad = 0;
        if (_nodeStateMonitor != null) {
            _nodeStateMonitor.lockNodesHashtable();
            NodeInfo sourceNodeInfo = (NodeInfo) _nodeStateMonitor.getNodeInfo(sourceUUID); //sanity check
            NodeInfo destNodeInfo = (NodeInfo) _nodeStateMonitor.getNodeInfo(destUUID);     //sanity check
            if (sourceNodeInfo != null && destNodeInfo != null) {
                Enumeration en = _nodeStateMonitor.getNodes();
                while (en.hasMoreElements()) {
                    NodeInfo nInfo = (NodeInfo) en.nextElement();
                    double percentualLoad = (double) (nInfo.outboundNetworkUtilization)/100.0;
                    double linkSpeed = (double) nInfo.networkLinkSpeed;
                    if (linkSpeed <= 0) {
                        linkSpeed = _defaultLinkSpeed;
                    }
                    accLoad = accLoad + percentualLoad * linkSpeed;
                }
            }
            _nodeStateMonitor.unlockNodesHashtable();

            double linkSpeed = sourceNodeInfo.networkLinkSpeed;
            if (linkSpeed <= 0) {
                linkSpeed = _defaultLinkSpeed;
            }
            double available = linkSpeed - accLoad;
            if (available < 0) {
                available = 0;
            }
            return available;
        }
        return -1.0;
    }

    private double _defaultLinkSpeed = 11.0 * 1024 * 1024;      //theoretical 11 Mbps (we should use 4 or 5 instead).  
    private NodeStateMonitor _nodeStateMonitor;
}
