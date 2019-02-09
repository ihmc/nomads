package us.ihmc.aci.netSupervisor.link;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.netSupervisor.algorithms.ElectionAlgorithm;
import us.ihmc.aci.netSupervisor.topology.TopologyManager;
//import us.ihmc.aci.nodemon.WorldState;
import us.ihmc.aci.test.ddam.LinkLatencyBounds;
import us.ihmc.aci.test.ddam.LinkThroughputBounds;
import us.ihmc.aci.test.ddam.LinkType;
import us.ihmc.aci.test.ddam.LinkTypeOrBuilder;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;


/**
 * Author Roberto Fronteddu on 10/4/2016.
 */
public class LinkManager {
/*
   //change this in a object!
    public LinkManager(double highPerfBw, double lowPerfBw,
                       int highPerfLat, int lowPerfLat, Map<Integer,Integer> pointsGoodNetworkBandwidth, Map<Integer,Integer> pointsBadNetworkBandwidth,
                       Map<Integer,Integer> pointsGoodNetworkLatency, Map<Integer,Integer> pointsBadNetworkLatency,
                       Collection<String> nodesMonitored, int validity)
    {

        LinkTypeContainer.setAccuracy(2);
        LinkTypeContainer.setJsonDirectory("/home/nomads/IHMC/code/aci/conf/netsupervisor/linktypes/");
        LinkTypeContainer.loadAllLinkTypesFromFiles();

        _electionAlgorithm = new ElectionAlgorithm();
        _electionAlgorithm.SetTimeValidity(validity);

        _electionAlgorithm.setHighPerformanceBandwidth(highPerfBw);
        _electionAlgorithm.setLowPerformanceBandwidth(lowPerfBw);
        _electionAlgorithm.setHighPerformanceLatency(highPerfLat);
        _electionAlgorithm.setLowPerformanceLatency(lowPerfLat);

        _electionAlgorithm.setHighPerfPointB(pointsGoodNetworkBandwidth);
        _electionAlgorithm.setLowPerfPointB(pointsBadNetworkBandwidth);
        _electionAlgorithm.setHighPerfPointL(pointsGoodNetworkLatency);
        _electionAlgorithm.setLowPerfPointL(pointsBadNetworkLatency);

        _electionAlgorithm.setMonitoredNodes(nodesMonitored);

    }

    public boolean analyzeWorldState (WorldState localWorldState, TopologyManager topologyManager)
    {
        _electionAlgorithm.detectLinksBetweenMonitoredNodes(localWorldState, topologyManager);
        return true;
    }

    private ElectionAlgorithm _electionAlgorithm;

    private static final Logger log = LoggerFactory.getLogger(LinkManager.class);
    */
}
