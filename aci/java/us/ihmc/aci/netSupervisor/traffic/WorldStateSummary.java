package us.ihmc.aci.netSupervisor.traffic;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.netSupervisor.topology.Subnetwork;
//import us.ihmc.aci.nodemon.WorldState;

/**
 * Author Roberto Fronteddu on 9/12/2016.
 */
public class WorldStateSummary
{
    public int getBackhaulP()
    {
        //write code here
        return 0;
    }
    public int getDelay()
    {
        //write code here
        return 0;
    }
    public int getSaturation()
    {
        //write code here
        return 0;
    }
    public int getTotalEstimatedBW()
    {
        //write code here
        return 0;
    }
    public int getWithinBandwidthEstimated()
    {
        //write code here
        return 0;
    }

    public int getInterconnectedP()
    {
        //write code here
        return 0;
    }

    public double getIncomingTraffic()
    {
        return 0;
    }

    public double getOutgoingTraffic()
    {
        return 0;
    }

    public int getCommunicatingInternalNodes()
    {
        return _communicatingInternalNodes;
    }

    public int getNumberOfInternalNodes ()
    {
        return _numberOfInternalNodes;
    }

    public Node[] getWorldStateNodes ()
    {
        return _nodes;
    }

    public String getLocalGroup ()
    {
        return _localGroup;
    }

    public double getTrafficBetweenInternalNodes()
    {
        return _trafficBetweenInternalNodes;
    }

    public double getObservedTrafficBetweenInternalNodes()
    {
        return _observedTrafficBetweenInternalNodes;
    }

    public double getTrafficGeneratedByLocalNetproxy()
    {
        return _trafficGeneratedByLocalNetproxy;
    }

    public double getTrafficReceivedByLocalNetproxy()
    {
        return _trafficReceivedByLocalNetproxy;
    }

    public boolean getLocalAndRemoteConnectionStatus()
    {
        return _localAndRemoteNetproxyAreConnected;
    }

    public boolean getForwardedTrafficStatus()
    {
        return _forwardedTrafficIsPresent;
    }

    public Subnetwork getLocalSubnetwork()
    {
        return _localSubNetwork;
    }

    public double getLatency()
    {
        return _latency;
    }

    public void setLatencyFromBackhaulUsingNetproxy (double latency)
    {
        _latency = latency;
    }

    public void setCommunicatingInternalNodes(int communicatingInternalNodes)
    {
        _communicatingInternalNodes = communicatingInternalNodes;
    }

    public void setNumberOfInternalNodes(int numberOfInternalNodes)
    {
        log.trace("Number of internal nodes: " + numberOfInternalNodes);
        _numberOfInternalNodes = numberOfInternalNodes;
    }

    public void setWorldStateNodes (Node[] nodes)
    {
        _nodes = nodes;
    }

    public void setLocalGroup (String localGroup)
    {
        log.trace("Local group: " + localGroup);
        _localGroup = localGroup;
    }

    public void setLocalAndRemoteConnection(boolean status) {
        _localAndRemoteNetproxyAreConnected = status;
    }

    public void setLocalSubnetwork(Subnetwork localSubNetwork)
    {
        log.trace("Local subnetwork: " + localSubNetwork);
        _localSubNetwork = localSubNetwork;
    }

    public void setForwardedTraffic(boolean Status)
    {
        _forwardedTrafficIsPresent = Status;
    }


    public void setTrafficBetweenInternalNodes(double trafficBetweenInternalNodes)
    {
        _trafficBetweenInternalNodes = trafficBetweenInternalNodes;
    }

    public void setObservedTrafficBetweenInternalNodes(double observedTrafficBetweenInternalNodes)
    {
        _observedTrafficBetweenInternalNodes = observedTrafficBetweenInternalNodes;
    }

    public void setTrafficGeneratedByLocalNetproxy(double trafficGeneratedByLocalNetproxy)
    {
        _trafficGeneratedByLocalNetproxy = trafficGeneratedByLocalNetproxy;
    }

    public void setTrafficReceivedByLocalNetproxy(double trafficReceivedByLocalNetproxy)
    {
        _trafficReceivedByLocalNetproxy = trafficReceivedByLocalNetproxy;
    }

    private double _trafficReceivedByLocalNetproxy;
    private double _trafficGeneratedByLocalNetproxy;
    private boolean _localAndRemoteNetproxyAreConnected;
    private double _observedTrafficBetweenInternalNodes;
    private double _trafficBetweenInternalNodes;
    private boolean _forwardedTrafficIsPresent;
    private String _observerMaster;
    private String _localNetProxy;
    private Subnetwork _localSubNetwork;
    private Node[] _nodes;
    private String _localGroup;
    private int _communicatingInternalNodes;
    private int _numberOfInternalNodes;
    private static final Logger log = Logger.getLogger(WorldStateSummary.class);
    private double _latency;
    Node[] nodes;
    String localGroup;

}
