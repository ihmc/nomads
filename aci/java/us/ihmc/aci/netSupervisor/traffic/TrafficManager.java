package us.ihmc.aci.netSupervisor.traffic;

import com.google.protobuf.Duration;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Timestamp;

import us.ihmc.aci.ddam.*;
import us.ihmc.aci.netSupervisor.information.InformationBroker;
import us.ihmc.aci.netSupervisor.topology.Subnetwork;
import us.ihmc.aci.netSupervisor.topology.TopologyManager;
//import us.ihmc.aci.nodemon.util.Utils;

import java.net.InetAddress;
import java.util.*;
import org.apache.log4j.Logger;

import static us.ihmc.aci.netSupervisor.utilities.Utilities.getElapsedTime;
import static us.ihmc.aci.netSupervisor.utilities.Utilities.startTimeCounter;

/**
 * Author Roberto Fronteddu on 8/9/2016.
 *
 */
public class TrafficManager
{
    /**
     * Default constructor of TrafficManager. It initializes all the variables. Information
     * about the period of health message sending and sensor resolution are use to calculate
     * the number of measure before an health message is ready. For example, if the sensor
     * has a resolution of 5 seconds and the health message sending period is 20 seconds
     * we will be able to harvest 4 measures before sending the message and all the field in
     * the message will have to be normalized to that number
     *
     * @param federationMessagePeriod specifies how often a health message is sent.
     * @param sensorResolution        specifies the resolution of the sensor harvesting information.
     */
    public TrafficManager (int federationMessagePeriod, int sensorResolution, int validity)
    {
        log.trace("Initializing traffic manager");
        _state = "init";
        setTimeValidity(validity);
        _currentMeasureIndex = 0;
        _numberOfMeasures = federationMessagePeriod / sensorResolution;

        log.trace("Traffic manager parameters are: ");
        log.trace("Federation update period: " + federationMessagePeriod + "s");
        log.trace("Sensor resolution: " + sensorResolution + "s");
        log.trace("Sensor data validity: " + sensorResolution + "s");
        log.trace("Number of expected measures: " + _numberOfMeasures);

        _totalBandwithEstimated = 0;
        _maxLinkBW = 0;
        _maxLinkBWCounter = 0;
        _maxWithinBWDetected = 0;
        _withinBWCounter = 0;
        //counters
        _counterInterconnectedP = 0;
        _counterBackhaulP = 0;
        _lastBackaulP = 0;
        _counterObservedLatency = 0;
        _counterObservedSaturation = 0;
        _currentTrafficInTheLink = 0;

        _interconnectedPs = new int[_numberOfMeasures];
        _backhaulPs = new int[_numberOfMeasures];
        _observedLatencies = new double[_numberOfMeasures];
        _observedSaturations = new double[_numberOfMeasures];

        _calculateInterconnectedP = false;
        _calculateBackhaulP = false;
        _calculateObserveLatency = false;
        _calculateObservedSaturation = false;
        _calculateTotalBandwidthEstimated = false;
        _calculateWithinBandwithEstimated = false;
        _nodesArray = null;
    }



    public boolean analizeSampling(WorldStateSummary worldStateSummary, TopologyManager topologyManager)
    {
        double startTime = System.currentTimeMillis();
        int timerCounter = startTimeCounter();
        int samplingCounter = 0; //should be a private variable

        _interconnectedPs[samplingCounter]      = interconnectedP (worldStateSummary);
        _backhaulPs[samplingCounter]            = backaulP (worldStateSummary);
        _totalBW[samplingCounter]               = calculateTotalEstimatedBW(worldStateSummary);
        _withinBW[samplingCounter]              = calculateWithinEstimatedBW(worldStateSummary);
        _observedSaturations[samplingCounter]   = observedSaturation(worldStateSummary);
        _observedLatencies[samplingCounter]     = getLatency(worldStateSummary);
        samplingCounter++;

        getElapsedTime(timerCounter);
        return true;
    }
/*
    public boolean analizeTraffic (InformationBroker ib, TopologyManager topologyManager)
    {
        //setLocalSubnetString(localWorldState, subnetworks)
        harvestDataPhase(localWorldState, topologyManager);
        return true;
    }
*/
    /**
     *Methods to enable SOI health message population
     */
    public void enableBackaulPCalculation() {
        log.debug("Enabling backhaul P calculation");
        _calculateBackhaulP = true;
    }
    public void enableInterconnectedPCalculation() {
        log.debug("Enabling interconnected P calculation");
        _calculateInterconnectedP = true;
    }
    public void enableLatenceCalculation() {
        log.debug("Enabling latency calculation");
        _calculateObserveLatency = true;
    }
    public void enableLinkBWCalculation() {
        log.debug("Enabling link bw calculation");
        _calculateTotalBandwidthEstimated = true;
    }
    public void enableSaturationCalculation() {
        log.debug("Enabling saturation calculation");
        _calculateObservedSaturation = true;
    }
    public void enableWithinBWCalculation() {
        log.debug("Enabling within bw calculation");
        _calculateWithinBandwithEstimated = true;
    }

//PRIVATE METHODS
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method calculate the backhaul probability
     * @return 1 of there is a connection, 0 otherwise
     */
    private int backaulP (WorldStateSummary worldStateSummary)
    {
        if(worldStateSummary.getLocalAndRemoteConnectionStatus()) {
            return 1;
        }

        if(worldStateSummary.getForwardedTrafficStatus()) {
            return 1;
        }
        return 0;
    }


/*
    private int calculateCommunicatingInternalNodes(String observerIp, WorldState worldState, List<String> nodes)
    {
        int nodesCommunicating = 0;

        log.trace("Internal Nodes: ");
        for (int indexA = 0; indexA < nodes.size(); indexA++) {
            log.trace(nodes.get(indexA));
        }
        log.trace("ObserverIp: " + observerIp);
        log.trace("Nodes size:" + nodes.size());

        if(nodes.size() == 1) {
            String nodeA = nodes.get(0);
            nodesCommunicating++;
            log.trace("Traffic generated by" + nodeA + "was observed by netproxy, increasing the counter");
        }
        else {
            for (int indexA = 0; indexA < nodes.size(); indexA++) {
                String nodeA = nodes.get(indexA);
                boolean alreadyConsidered = false;

                if(netproxyCanSeeTheNode (worldState, observerIp, nodeA)) { //this should be penalize
                    nodesCommunicating++;
                    log.trace("Traffic generated by" + nodeA + "was observed by netproxy, increasing the counter");
                }
                else {
                    //netproxy can't see the node
                    for (int indexB = 0; indexB < nodes.size(); indexB++) {
                        if(indexB != indexA) {
                            String nodeB = nodes.get(indexB);
                            if (thereIsTraffic (worldState, nodeA, nodeB)){
                                nodesCommunicating++;
                                log.trace("Traffic was detected between: " + nodeA + " and " + nodeB + " " +
                                        ",increasing counter: " + nodesCommunicating);
                            }
                            else if((observedTrafficIsPresent(worldState, nodeA, nodeB))) {
                                nodesCommunicating++;
                                log.trace("Traffic was observed between: " + nodeA + " and " + nodeB + " " +
                                        ", increasing counter: " + nodesCommunicating);
                            }
                            else {
                                log.trace("No traffic detected between " + nodeA + " and " + nodeB);
                            }
                        }
                    }
                }
            }
        }
        log.trace("Nodes communicating " + nodesCommunicating);
        return nodesCommunicating;
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the world state in search of observed traffic between internal nodes
     * @param worldState world state of the local Node Monitor.
     * @param topologyManager: Topology Manager object
     * @return return the sum of observed traffic between internal nodes
     */
    /*
    private double calculateObservedTrafficBetweenInternalNodes (WorldState worldState, TopologyManager topologyManager)
    {
        String observerMaster = Utils.getPrimaryIP(worldState.getLocalNode());
        Subnetwork localSubnetwork = getLocalSubnetwork(worldState, topologyManager.getSubnetworks());
        double observedTraffic;
        double observedTrafficSum = 0;
        List<String> nodes = localSubnetwork.getInternalNodes();
        for (String nodeA : nodes) {
            for (String nodeB : nodes) {
                observedTraffic = worldState.getObservedTraffic(observerMaster, nodeA, nodeB, getTimeValidity());
                log.debug("Observed traffic from node: " + nodeA + "to node:" + nodeB + " is " + observedTraffic);
                observedTrafficSum = observedTrafficSum + observedTraffic;
            }
        }
        return observedTrafficSum;
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the world state in search of traffic between internal nodes
     * @param worldState world state of the local Node Monitor.
     * @param topologyManager: Topology Manager object
     * @return return the sum of traffic between internal nodes
     */
    /*
    private double calculateTrafficBetweenInternalNodes (WorldState worldState, TopologyManager topologyManager)
    {
        Subnetwork localSubnetwork = getLocalSubnetwork(worldState, topologyManager.getSubnetworks());
        List<String> nodes = localSubnetwork.getInternalNodes();
        double trafficBetweenInternalNodes = 0;
        List<String> ipList = getIPList(WORLDSTATE_MULTICAST_ADDRESSES);
        for (String nodeA : nodes) {
            for (String nodeB : nodes) {
                double incomingTraffic = worldState.getIncomingTraffic(nodeA, nodeA, nodeB, getTimeValidity());
                log.debug("Incoming, observer: " + nodeA  + " dest " + nodeA + " source:" + nodeB + " value " + incomingTraffic);

                trafficBetweenInternalNodes = trafficBetweenInternalNodes + incomingTraffic;
                for(String multicastIP : ipList) {
                    incomingTraffic = worldState.getIncomingMcastTraffic(nodeA, multicastIP, nodeB, getTimeValidity());
                    log.debug("Incoming multicast, Observer: " + nodeA + " mIP: " + multicastIP + "source:" + nodeB +
                            " value " + incomingTraffic);

                    trafficBetweenInternalNodes = trafficBetweenInternalNodes + incomingTraffic;
                }
            }
        }
        return trafficBetweenInternalNodes;
    }
    */

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method estimates the total traffic generated by netproxy as a sum of normal and multicast traffic
     * @param worldState world state of the local Node Monitor.
     * @param topologyManager: Topology Manager object
     * @return a double representing the total traffic generated by the netproxy external interface
     */
    /*
    private double calculateTrafficGeneratedByLocalNetproxy (WorldState worldState, TopologyManager topologyManager)
    {
        String observerMaster = Utils.getPrimaryIP(worldState.getLocalNode());
        double outgoingTraffic;
        double outgoingMulticastTraffic;
        outgoingTraffic = worldState.getOutgoingTraffic(observerMaster, "0.0.0.0", "*", getTimeValidity());
        outgoingMulticastTraffic = worldState.getOutgoingMcastTraffic(observerMaster, "0.0.0.0", "*", getTimeValidity());
        log.trace("Outgoing traffic without multicast detected by NetProxy: " + outgoingTraffic);
        log.trace("Outgoing Multicast traffic detected by NetProxy (should be zero): " + outgoingMulticastTraffic);
        return outgoingTraffic + outgoingMulticastTraffic;
    }
    */
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method estimates the total traffic received by netproxy as a sum of normal and multicast traffic
     * @return a double representing the total traffic received by the netproxy external interface
     */
    /*
    private double calculateTrafficReceivedByLocalNetproxy (WorldState worldState, TopologyManager topologyManager)
    {
        String observerMaster = Utils.getPrimaryIP(worldState.getLocalNode());
        double incomingTraffic;
        double incomingMulticastTraffic;
        incomingTraffic = worldState.getIncomingTraffic(observerMaster, "0.0.0.0", "*", getTimeValidity());
        incomingMulticastTraffic = worldState.getIncomingMcastTraffic(observerMaster, "0.0.0.0", "*", getTimeValidity()); //should be 0
        log.trace("Incoming traffic detected by NetProxy: " + incomingTraffic);
        log.trace("Incoming multicast traffic detected by NetProxy (should be zero): " + incomingMulticastTraffic);
        return incomingTraffic + incomingMulticastTraffic;
    }
    */

    private void updateMaxDetectedBW(double newMaxLinkBW)
    {
        _maxLinkBW = newMaxLinkBW * 1.1;
        log.info("Updating the max bandwidth to: " + _maxLinkBW);
    }

    private void reduceMaxBW(double trafficInTheLink)
    {
        _maxLinkBWCounter++;
        if(_maxLinkBWCounter > 20) {
            _maxLinkBWCounter = 0;
            double tmp = _maxLinkBW - (_maxLinkBW * 10 / 100);
            if(tmp > trafficInTheLink) {
                log.info("Max detected link bandwidth: " + _maxLinkBW +
                        " was not reached again, decreasing the estimate to: " + tmp);
                _maxLinkBW = tmp;
            }
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method estimates the total Bandwithin
     * @param worldStateSummary fgg
     * @return a double representing the estimated Bandwith.
     */
    private double calculateTotalEstimatedBW(WorldStateSummary worldStateSummary)
    {
        double incomingTraffic = worldStateSummary.getIncomingTraffic();
        double outgoingTraffic = worldStateSummary.getOutgoingTraffic();

        double trafficInTheLink = incomingTraffic + outgoingTraffic;
        if (trafficInTheLink >= _maxLinkBW) {
            updateMaxDetectedBW(trafficInTheLink);
            return _maxLinkBW;
        }
        else {
            reduceMaxBW(trafficInTheLink);
            return _maxLinkBW;
        }
    }

    private void reduceWithinBW()
    {
        _withinBWCounter++;
        if(_withinBWCounter > 20) {
            _withinBWCounter = 0;
            double tmp = _maxWithinBWDetected - (_maxWithinBWDetected * 15 / 100);
            log.trace("Within bandwidth: " + _maxWithinBWDetected +
                    " was never reached again, lowering the estimate of the within bandwidth to" + tmp);
            _maxWithinBWDetected = tmp;
        }
    }

    private void updateMaxWithin(double traffic)
    {
        if(traffic > _maxWithinBWDetected) {
            _maxWithinBWDetected = traffic;
            _withinBWCounter = 0;
        }
        else {
            reduceWithinBW();
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method estimates the total within Bandwith, the actual value is calculate in the
     * calculateObservedSaturation method.
     * @param worldStateSummary World State summary to use for computation
     * @return a double representing the estimated Bandwith.
     */
    private double calculateWithinEstimatedBW(WorldStateSummary worldStateSummary)
    {
        double consideredTraffic = 0;

        double traffic = worldStateSummary.getTrafficBetweenInternalNodes();
        double observedTraffic = worldStateSummary.getObservedTrafficBetweenInternalNodes();

        //This has to be smarter
        if((observedTraffic > 2 * traffic) || (traffic == 0)) {
            log.debug("Using observed traffic");
            consideredTraffic = observedTraffic;
        }
        else {
            log.debug("Using normal traffic");
            consideredTraffic = traffic;
        }
        updateMaxWithin(consideredTraffic);
        return _maxWithinBWDetected;
    }


    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _backhaulP
     * @return an int representing the backhail probability
     */
    private int getBackhaulP ()
    {
        return _backhaulP;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _observedLatency
     * @return an int representing the observed latency
     */
    private int getDelay ()
    {
        return (int) _observedLatency;
    }


    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _interconnectedP
     * @return an int representing the interconnected probability
     */
    private int getInterconnectedP ()
    {
        return _interconnectedP;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _observedSaturation
     * @return an int representing the estimated saturation of the link
     */
    private int getSaturation ()
    {
        return (int) _observedSaturation;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _totalBandwithEstimated
     * @return an int representing the estimated bandwidth in the link
     */
    private int getTotalEstimatedBW ()
    {
        return (int) _totalBandwithEstimated;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the _withinBandwithEstimated
     * @return an int representing the estimated within bandwidth
     */
    private int getWithinBandwidthEstimated ()
    {
        return (int) _withinBandwithEstimated;
    }

    /*
    private WorldStateSummary harvestDataPhase(InformationBroker ib, TopologyManager topologyManager)
    {
        if(ib == null) {
            log.warn("ib is null?");
            return null;
        }
        if(topologyManager == null) {
            log.warn("topology manager is null?");
            return null;
        }

        log.info("Analyze world state iteration: " + _currentMeasureIndex + " out of: " + _numberOfMeasures);
        WorldStateSummary worldStateSummary;
        double startTime = System.currentTimeMillis();
        worldStateSummary = populateWorldStateSummary (localWorldState, topologyManager);
        log.info("World state summary populated, time elapsed: " + (System.currentTimeMillis() - firstTime) + "ms");
        return worldStateSummary;
    }
*/
    private double getLatency(WorldStateSummary worldStateSummary)
    {
        return worldStateSummary.getLatency();
    }


    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     */
    private int interconnectedP (WorldStateSummary worldStateSummary)
    {
        int numberOfInternalNodes = worldStateSummary.getNumberOfInternalNodes();
        int nodesCommunicating = worldStateSummary.getCommunicatingInternalNodes();
        return nodesCommunicating * 100 / numberOfInternalNodes;
    }

    /**
     *

         * Author: Roberto Fronteddu
         * Year of last revision: 2016
         * This method explore the World State looking for traffic cooming from the nodeIP detected by netproxy
         * @param worldState world state of the local Node Monitor.
         * @param observerIp Master associated with the netproxy
         * @param nodeA Node supposed to generate traffic detected by the netproxy
         * @return True if traffic is detected, False otherwise

        boolean netproxyCanSeeTheNode (WorldState worldState, String observerIp, String nodeA)
        {
            log.trace("observerIp: " + observerIp + " source: " + nodeA);
            if((worldState.getObservedTraffic(observerIp, nodeA, "*", getTimeValidity()) > 0) ||
                    (worldState.getObservedMcastTraffic(observerIp, nodeA, "*", getTimeValidity()) > 0) ||
                    (worldState.getOutgoingTraffic(observerIp, nodeA, "0.0.0.0", getTimeValidity()) > 0) ||
                    (worldState.getOutgoingTraffic(observerIp, nodeA, "*", getTimeValidity()) > 0) ||
                    (worldState.getOutgoingMcastTraffic(observerIp, nodeA, "*", getTimeValidity()) > 0) ||
                    (worldState.getIncomingTraffic(observerIp, "0.0.0.0", nodeA, getTimeValidity()) > 0))
                     {
                return true;

            }
            return false;
        }
    */

    /**
     * Author: Roberto Fronteddu rfronteddu@ihmc.us
     * Year of last revision: 2016
     * This method calculates the observed link saturation and as byproducts also calculate:
     * 1) Within bandwidth
     * 2)
     *
     * @return a double representing the estimated saturation.
     */
    private int observedSaturation(WorldStateSummary worldStateSummary)
    {
        double trafficInTheLink =
                worldStateSummary.getTrafficGeneratedByLocalNetproxy() +
                worldStateSummary.getTrafficReceivedByLocalNetproxy();

        if(_maxLinkBW != 0) {
            return (int) (100 * trafficInTheLink / _maxLinkBW);
        }
        return 0;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method check in the world state if observed traffic is present between nodeA and nodeB
     * @param worldState world state of the local Node Monitor.
     * @param nodeA: NodeA IP
     * @param nodeB: NodeB IP
     * @return return true if observed traffic between nodeA and nodeB is detected
     */
    /*
    private boolean observedTrafficIsPresent (WorldState worldState, String nodeA, String nodeB)
    {
        String observer = Utils.getPrimaryIP(worldState.getLocalNode());
        if(observer != null && nodeA != null && nodeB != null && worldState != null) {
            return (worldState.getObservedTraffic(observer, nodeA, nodeB, getTimeValidity()) > 0) &&
                    (worldState.getObservedTraffic(observer, nodeB, nodeA, getTimeValidity()) > 0) ? true : false;
        }
        return false;
    }
    */
    /*
    private boolean receivedForwardedTraffic(WorldState worldState, String nodeIP)
    {
        String observer = Utils.getPrimaryIP(worldState.getLocalNode());
        if(observer != null && worldState != null && nodeIP != null) {
            if((worldState.getIncomingTraffic(observer, "0.0.0.0", nodeIP, getTimeValidity()) > 0))
                return true;
        }
        return false;
    }
    */

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the Backhaul Probability
     */
    private void setHealhtBackhaulP()
    {
        _backhaulP = 0;
        for (int index = 0; index < _counterBackhaulP; index++) {
            _backhaulP = _backhaulP + _backhaulPs[index];
        }
        if (_counterBackhaulP != 0) {
            _backhaulP = (_backhaulP * 100) / _counterBackhaulP;
            _lastBackaulP = _backhaulP;
            System.out.println("SOI BackhaulP: " + _backhaulP);
        }
        else {
            if(_lastBackaulP > 0) {
                _lastBackaulP = _backhaulP - (_backhaulP * 15 / 100);
                _backhaulP = _lastBackaulP;
                System.out.println("SOI BackhaulP (decreased since no connection was detected): " + _backhaulP);
            }
            else {
                System.out.println("SOI BackhaulP: " + _backhaulP);
            }
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the Interconected Probability
     */
    private void setHealthInerconnectedP()
    {
        _interconnectedP = 0;
        for (int index = 0; index < _counterInterconnectedP; index++) {
            _interconnectedP = _interconnectedP + _interconnectedPs[index];
        }

        if (_counterInterconnectedP != 0) {
            _interconnectedP = _interconnectedP / _counterInterconnectedP;
            _lastInterconnectedP = _interconnectedP;
            System.out.println("SOI InerconnectedP: " + _interconnectedP + "%");
        }
        else {
            if(_lastInterconnectedP > 0) {
                _lastInterconnectedP = _lastInterconnectedP - (_lastInterconnectedP * 15 / 100);
                _interconnectedP = _lastInterconnectedP;
                System.out.println("SOi InerconnectedP (decreased since no value was detected): " + _interconnectedP);
            }
            else {
                System.out.println("SOI InerconnectedP: " + _interconnectedP);
            }
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This set of methods set all the values that will be used to populate the health message.
     * @return true if no problem is detected
     */
    private boolean setHealthMessageValues ()
    {
        setHealthInerconnectedP();
        setHealtBackhaulP();
        setHealtObservedLatency();
        setHealtObservedSaturation();
        setHealtTotalBandwithEstimated();
        setHealtWithinBandwithEstimated();
        return true;
    }

     public boolean healthMessageIsReady()
     {
         return true;
     }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the observed latency
     */
    private void setHealtObservedLatency()
    {
        _observedLatency = 0;
        for (int index = 0; index < _counterObservedLatency; index++) {
            _observedLatency = _observedLatency + _observedLatencies[index];
        }

        if (_counterObservedLatency != 0) {
            _observedLatency = _observedLatency / _counterObservedLatency;
            _lastObservedLatency = _observedLatency;
            log.debug("SOI ObservedLatency: " + _observedLatency);
        }
        else {
            log.warn("_counterObservedLatency is 0");
            if(_lastObservedLatency > 0) {
                _lastObservedLatency = _lastObservedLatency + (15 * _lastObservedLatency / 100);
                _observedLatency = _lastObservedLatency;
                if (_observedLatency > 999) {
                    _observedLatency = 999;
                }
                log.debug("SOI ObservedLatency (increased since it was not detected): " + _observedLatency);
            }
            else {
                log.debug("SOI ObservedLatency: " + _observedLatency);
            }
        }
    }

    public void setHealtBackhaulP()
    {

    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the observed saturation
     */
    private void setHealtObservedSaturation()
    {
        _observedSaturation = 0;
        for (int index = 0; index < _counterObservedSaturation; index++) {
            _observedSaturation = _observedSaturation + _observedSaturations[index];
        }
        if (_counterObservedSaturation != 0) {
            _observedSaturation = _observedSaturation / _counterObservedSaturation;
            log.debug("SOI observedSaturation: " + _observedSaturation);
        }
        else {
            _observedSaturation = 1;
            log.debug("SOI observedSaturation: " + _observedSaturation);
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the estimated bandwidth
     */
    private void setHealtTotalBandwithEstimated()
    {
        _totalBandwithEstimated = _maxLinkBW * 8 / 1000; //Kbps
        log.debug("SOI Total Estimated BW to and from the identified network: " + _totalBandwithEstimated + "Kbps");
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method sets the estimated within bandwidth
     */
    private void setHealtWithinBandwithEstimated()
    {
        _withinBandwithEstimated = _maxWithinBWDetected  * 8 / 1000;
        log.debug("SOI WithinBandwithEstimated: " + _withinBandwithEstimated + "Kbps");
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the worldstate trying to find traffic between internal and external nodes
     * @param worldStateSummary World State summary to use for computation
     * @param worldState world state of the local Node Monitor.
     * @return True if traffic is detected, False otherwise.
     */
    /*
    private boolean trafficBetweenInternalAndExternalNodesIsPresent (WorldStateSummary worldStateSummary, WorldState worldState)
    {
        Node[] nodes = worldStateSummary.getWorldStateNodes();
        String localGroup = worldStateSummary.getLocalGroup();
        Subnetwork localSubnetwork = worldStateSummary.getLocalSubnetwork();
        for(String internalNode : localSubnetwork.getInternalNodes()) {
            // A node is external if it is not in the local group
            for (Node node : nodes) {
                if (!nodeIsInGroup(node, localGroup)) {
                    log.trace("NodeB " + node.getId() + "is not in the local group " + localGroup);
                    String externalNode = Utils.getPrimaryIP(node);
                    if(thereIsTraffic (worldState, internalNode, externalNode)) {
                        log.trace("Traffic detected between " + internalNode + " and " + externalNode);
                        return true;
                    }

                    if(observedMulticastBetweenNodesIsPresent(worldState, internalNode, externalNode)) {
                        log.trace("Multicast Traffic observed between " + internalNode + " and " + externalNode);
                        return true;
                    }

                    //check for observed traffic in the local netproxy
                    if(observedTrafficIsPresent(worldState, internalNode, externalNode)) {
                        log.trace("Traffic observed between " + internalNode + " and " + externalNode);
                        return true;
                    }

                    //Check for received traffic that will be forwarded in the local netproxy
                    if(receivedForwardedTraffic(worldState, externalNode)) {
                        log.trace("Local NetProxy detected traffic from " + externalNode);
                        return true;
                    }
                }
            }
        }
        log.trace("No traffic detected between internal and external nodes");
        return false;
    }
    */
    /*
    private boolean observedMulticastBetweenNodesIsPresent(WorldState worldState, String nodeA, String nodeB)
    {
        List<String> ipList = getIPList(WORLDSTATE_MULTICAST_ADDRESSES);
        String observer = Utils.getPrimaryIP(worldState.getLocalNode());
        for(String multicastIP : ipList) {
            if(multicastIP != null && observer != null && nodeA != null && nodeB != null && worldState != null) {
                if((worldState.getObservedTraffic(nodeB, nodeA, multicastIP, getTimeValidity()) > 0)) {
                    log.trace("Detected multicast traffic observed by " + nodeB
                            + " generated by " + nodeA + " multicast IP " + multicastIP);
                    return true;
                }

                if(worldState.getObservedTraffic(nodeA, nodeB, multicastIP, getTimeValidity()) > 0) {
                    log.trace("Detected multicast traffic observed by " + nodeA
                            + " generated by " + nodeB + " multicast IP " + multicastIP);
                    return true;
                }
            }
        }
        return false;
    }
    */

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method returns an estimate of the internal bandwidth estimates using netproxy information
     * @param worldStateSummary World State summary to use for computation
     * @return A double representing the within bandwidth
     */
    /*
    private double withinBandwithEstimatedWithNetproxy(WorldStateSummary worldStateSummary)
    {
        _maxWithinBWDetected = calculateMaxWithinBW (worldStateSummary.getObservedTrafficBetweenInternalNodes(),
                worldStateSummary.getTrafficBetweenInternalNodes());
        return _maxWithinBWDetected;
    }
    */

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the World State summarizing a set of useful information that will be used by the traffic manager
     * @return A World State Summary
     */
    /*
    private WorldStateSummary populateWorldStateSummary (WorldState localWorldState, TopologyManager topologyManager)
    {
        String observerMaster = Utils.getPrimaryIP(localWorldState.getLocalNode());
        Subnetwork localSubNetwork = getLocalSubnetwork(localWorldState, topologyManager.getSubnetworks());
        WorldStateSummary worldStateSummary = new WorldStateSummary();

        if (localSubNetwork != null) {
            List<String> nodes = localSubNetwork.getInternalNodes();
            log.trace("test start");
            log.trace("WorldState summary snapshot: ");

            log.trace("Method: calculateCommunicatingInternalNodes");
            worldStateSummary.setCommunicatingInternalNodes (
                    calculateCommunicatingInternalNodes(observerMaster, localWorldState, nodes));

            log.trace("Method: setNumberOfInternalNodes");
            worldStateSummary.setNumberOfInternalNodes (nodes.size());

            log.trace("Method: setWorldStateNodes");
            worldStateSummary.setWorldStateNodes(getNodesArray(localWorldState));

            log.trace("Method: setLocalGroup");
            worldStateSummary.setLocalGroup(getGroupRoot(localWorldState.getLocalNode()));

            log.trace("Method: setLocalSubnetwork");
            worldStateSummary.setLocalSubnetwork(localSubNetwork);

            log.trace("Method: localAndRemoteNetProxyAreCommunicating");
            worldStateSummary.setLocalAndRemoteConnection (
                    localAndRemoteNetProxyAreCommunicating(localWorldState, topologyManager,worldStateSummary));

            log.trace("Method: trafficBetweenInternalAndExternalNodesIsPresent");
            worldStateSummary.setForwardedTraffic(trafficBetweenInternalAndExternalNodesIsPresent(worldStateSummary, localWorldState));

            log.trace("calculateTrafficBetweenInternalNodes");
            worldStateSummary.setTrafficBetweenInternalNodes(
                    calculateTrafficBetweenInternalNodes(localWorldState, topologyManager));

            log.trace("calculateObservedTrafficBetweenInternalNodes");
            worldStateSummary.setObservedTrafficBetweenInternalNodes(
                    calculateObservedTrafficBetweenInternalNodes (localWorldState, topologyManager));

            log.trace("calculateTrafficGeneratedByLocalNetproxy");
            worldStateSummary.setTrafficGeneratedByLocalNetproxy(
                    calculateTrafficGeneratedByLocalNetproxy (localWorldState, topologyManager));

            log.trace("calculateTrafficReceivedByLocalNetproxy");
            worldStateSummary.setTrafficReceivedByLocalNetproxy(
                    calculateTrafficReceivedByLocalNetproxy(localWorldState, topologyManager));

            log.trace("calculateLatencyFromBackhaulUsingNetproxy");
            worldStateSummary.setLatencyFromBackhaulUsingNetproxy(
                    calculateLatencyFromBackhaulUsingNetproxy(localWorldState, topologyManager));

            log.trace("test end");

        }
        log.trace("WorldState snapshot end");
        return worldStateSummary;
    }
    */
    public void setRemoteNetproxyIps(Collection<String> remoteProxyIps)
    {
        _remoteProxyOverride = true;
        _remoteProxyIps = remoteProxyIps;
    }

    /*
    private double calculateLatencyFromBackhaulUsingNetproxy(WorldState worldState, TopologyManager topologyManager)
    {
        int tmpLatency = 0;
        int latency = 0;
        int counter = 0;

        String lNP = topologyManager.findNetProxyNode(worldState,  Utils.getPrimaryIP(worldState.getLocalNode()));
        String observerMaster = getLocalSubnetwork(worldState, topologyManager.getSubnetworks()).getNodeMonitorMaster();
        log.debug("Local netproxy: " + lNP);
        log.debug("Observer: " + observerMaster);
        Collection<String> remoteNPs;
        if(_remoteProxyOverride) {
            log.debug("Using overrided remote netproxy IPs");
            remoteNPs = _remoteProxyIps;
        }
        else {
            log.debug("Using autodetected remote netproxy IPs");
            remoteNPs =  topologyManager.getRemoteNetProxyCollection();
        }

        for (String rNP : remoteNPs) {
            log.debug("Remote Netproxy IP: " + rNP);
            if(rNP != null) {
                if((tmpLatency = worldState.getLatency(observerMaster, lNP, rNP, getTimeValidity())) > 0) {
                    log.debug("Detected latency tuple: Observer: " + observerMaster + " local netproxy: " + lNP +
                            " remote netproxy:" + rNP + " latency: " + tmpLatency);
                    counter++;
                    log.debug(("Number of latency detected: " + counter));
                    latency = latency + tmpLatency;
                }
                else {
                    log.debug("NetSupervisor was not able to detect latency for tuple: Observer: " +
                            observerMaster + " local netproxy: " + lNP +
                            " remote netproxy:" + rNP);
                }
            }
            else {
                log.debug("No Remote NetProxy was detected");
            }
        }

        if (counter == 0) {
            log.debug("No latency detected");
            return 0;
        }
        else {
            log.debug("Latency average: " + latency / counter + "With counter: " + counter);
            return latency / counter;
        }
    }
*/
//UTILITY METHODS
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method return the CIDR of a netmask
     * @param netmask an InetAddress containing the netmask for which we wish to know the CIDR.
     * @return an int representing the CIDR
     */
    private int getCIDR (InetAddress netmask)
    {
        byte[] netmaskBytes = netmask.getAddress();
        int cidr = 0;
        boolean zero = false;
        for (byte b : netmaskBytes) {
            int mask = 0x80;

            for (int i = 0; i < 8; i++) {
                int result = b & mask;
                if (result == 0) {
                    zero = true;
                }
                else if (zero) {
                    throw new IllegalArgumentException("Invalid netmask.");
                }
                else {
                    cidr++;
                }
                mask >>>= 1;
            }
        }
        return cidr;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for a string representing the localSubnetwork ID
     * @return an String representing the localSubnetwork ID
     */
    private String getLocalSubnetString ()
    {
        return _localSubnetString;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method returns the "root" part of the local group name. As a convention, each group name should be
     * something like "root-ID". This method returns the root part which can be used to identify groups in the same
     * group domain.
     * @param node A node for which we wish to obtain the local group name
     * @return A string representing the root of the local group name.
     */
    private String getGroupRoot (Node node)
    {
        Map<String, Group> groups = node.getGrump().getGroups();
        for (String groupName : groups.keySet()) {
            if (!groupName.equals("masters")) {
                int index = groupName.indexOf("-");
                if (index == -1) {
                    log.error("The local group needs to contain a dash!");
                    return "";
                }
                return groupName.substring(0, index);
            }
        }
        return "";
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method returns a string containing the IP of the local network
     * @return A String containing the IP of the local network
     */
    public String getLocalNetworkName()
    {
        if (_localSubnetString == null) {
            return "";
        }
        else {
            return _localSubnetString;
        }
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method populate an array filled with all the nodes contained in the World State
     * @param localWorldState WorldState
     * @return Array filled with the nodes contained in the local World State
     */
    /*
    public Node[] getNodesArray (WorldState localWorldState)
    {
        if (_nodesArray == null) {
            //get nodes from world state
            Map<String, Node> nodesMap = localWorldState.getNodesMapCopy();
            Node[] _nodesArray = new Node[nodesMap.size()];
            int index = 0;
            for (String k : nodesMap.keySet()) {
                _nodesArray[index] = nodesMap.get(k);
                index++;
            }
            return _nodesArray;
        }
        else {
            return _nodesArray; //array already obtained
        }
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This object explore the subnetworks contained in the Topology Manager looking for a subnetwork where
     * the IP of the local Node Monitor is classified as internal.
     * @param localWorldState WorldState.
     * @param subnetworks subnetworks summary harvested by the Topology Manager.
     * @return a subnetwork object representing the local subnetwork.
     */
    /*
    private Subnetwork getLocalSubnetwork (WorldState localWorldState, Map<String, Subnetwork> subnetworks)
    {
        if (_localSubNetwork != null) {
            return _localSubNetwork;
        }
        else {
            List<Network> nicList = localWorldState.getLocalNode().getInfo().getNicsList();
            Map<String, Topology> topology = localWorldState.getLocalNode().getTopology();
            if (topology == null) {
                log.trace("Topology is null!");
                return null;
            }
            else {
                for (String netName : topology.keySet()) {
                    log.trace("Net name under exam: " + netName);

                    _localSubNetwork = subnetworks.get(netName);
                    if(_localSubNetwork != null) {
                        if (localNodeIsInTheSubNet(_localSubNetwork, nicList)) {
                            break;
                        }
                    }
                    else {
                        log.warn("_localSubNetwork is still null");
                    }

                }
                if(_localSubNetwork != null) {
                    log.trace("_localSubNetwork set to: " + _localSubNetwork.getName());
                    return _localSubNetwork;
                }
                else {
                    log.warn("Couldn't find an entry for localsubnetwork");
                    return null;
                }

            }
        }
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the world state looking for traffic between the remote and local netproxy
     * @param worldState WorldState.
     * @param topologyManager Topology Manager Object.
     * @param worldStateSummary World State summary to use for computation
     * @return True if the remote and local netproxy are communicating, false otherwise
     */
    /*
    private boolean localAndRemoteNetProxyAreCommunicating(WorldState worldState, TopologyManager topologyManager,
                                                           WorldStateSummary worldStateSummary)
    {
        String lNP = topologyManager.findNetProxyNode(worldState,  Utils.getPrimaryIP(worldState.getLocalNode()));
        String observerMaster = getLocalSubnetwork(worldState,topologyManager.getSubnetworks()).getNodeMonitorMaster();

        log.trace("Local netproxy: " + lNP);
        log.trace("Observer: " + observerMaster);

        Collection<String> remoteNPs = topologyManager.getRemoteNetProxyCollection();
        for (String rNP : remoteNPs) {
            log.trace("Remote netproxy: " + rNP);
            if(rNP != null) {
                if((worldState.getIncomingTraffic(observerMaster, "0.0.0.0", rNP, getTimeValidity()) > 0)) {
                    log.trace("Detected received traffic in local netproxy from remote one: " + rNP);
                    return true;
                }
                else if (worldState.getIncomingTraffic(getNetProxyMaster(rNP, worldState, worldStateSummary,topologyManager),
                        "0.0.0.0", lNP, getTimeValidity()) > 0) {
                    log.trace("Detected received traffic remote netproxy " + rNP + " from local one");
                    return true;
                }
            }
            else {
                log.trace("No Remote NetProxy was detected yet");
            }
        }
        log.trace("No traffic detected between local and remote netproxy");
        return false;
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore the world state returning a String representing the IP of the netproxy master
     * @param netproxy IP of the netproxy we want to find the master for
     * @param worldState WorldState.
     * @param worldStateSummary World State summary to use for computation
     * @param topologyManager Topology Manager Object.
     * @return A String representing the IP of the netproxy's master
     */
    /*
    private String getNetProxyMaster(String netproxy, WorldState worldState, WorldStateSummary worldStateSummary, TopologyManager topologyManager)
    {
        if(netproxy != null &&  worldState != null && worldStateSummary != null && topologyManager == null) {
            Node[] nodes = worldStateSummary.getWorldStateNodes();
            for(Node master: nodes) {
                if(master.getGrump().getGroups().get("masters") != null) {
                   if((topologyManager.findNetProxyNode(worldState,  Utils.getPrimaryIP(master))).equals(netproxy)) {
                       System.out.print("Netproxy: " + netproxy + " master is: " + Utils.getPrimaryIP(master));
                       return Utils.getPrimaryIP(master);
                   }
               }
            }
        }
        return "";
    }
    */

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method explore a subnetwork to determine if one of the nic is part of the subnet
     * @param subNetwork Subnetwork objext
     * @param nicList  List of network interfaces
     * @return True if one of the nic is in the subnet, false otherwise
     */
    private boolean localNodeIsInTheSubNet (Subnetwork subNetwork, List<Network> nicList)
    {
        //System.out.println("Cycle all the IP of all the interfaces..");
        for (Network aNicList : nicList) {
            log.trace("Ip under exam: " + aNicList.getIpAddress());
            if (subNetwork.getInternalNodes().contains(aNicList.getIpAddress())) {
                return true;
            }
        }
        return false;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method check if Node node is in the Group group
     * @param node  Node we wish to know if it is part of the Group group
     * @param group Group we wish to know if node is part of.
     * @return true if node is part of group, false otherwise.
     */
    private boolean nodeIsInGroup (Node node, String group)
    {
        return !(group == null) && getGroupRoot(node).equals(group);
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method verifies if a node is in the local subnet
     * @param node Node we wish to know if it is in the subnetwork or not.
     * @param subnetwork Subnetwork we wish to check the Node against.
     * @return true if node is in the subnetwork, false otherwise.
     */
    /*
    private boolean nodeIsInTheLocalSubnet (Node node, Subnetwork subnetwork)
    {
        List<String> nodes = subnetwork.getInternalNodes();
        if (nodes != null) {
            return nodes.contains(Utils.getPrimaryIP(node));
        }
        else {
            log.warn("Subnet is still empty?");
            return false;
        }
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method resets all the fields that are message dependent
     */
    private void prepareTrafficManagerForNextHarvest ()
    {
        _state = "init";
        _counterInterconnectedP = 0;
        _counterBackhaulP = 0;
        _counterObservedLatency = 0;
        _counterObservedSaturation = 0;
        _currentMeasureIndex = 0;
        _nodesArray = null;
        _localSubnetString = null;
        _localSubNetwork = null;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method retrieve and set the local subnetwork name (which will be a String in the form "x.x.x.x/CIDR")
     * @param localWorldState world state of the local Node Monitor.
     * @param subnetworks subnetworks summary harvested by the Topology Manager.
     * @return true if no problem was detected, false otherwise.
     */
    /*
    private boolean setLocalSubnetString (WorldState localWorldState, Map<String, Subnetwork> subnetworks)
    {
        if (subnetworks != null) {
            //Topology Manager was able to create a subnetworks container
            if (_localSubnetString == null) {
                //we haven't already calculated the local subnetwork name

                Subnetwork localSubnetwork = getLocalSubnetwork(localWorldState, subnetworks);
                if (localSubnetwork != null) {
                    String subnetName = localSubnetwork.getName();
                    log.trace("SubnetMask: " + localSubnetwork.getSubnetMask());

                    InetAddress netmask;
                    try {
                        netmask = InetAddress.getByName(localSubnetwork.getSubnetMask());
                        int cidr = getCIDR(netmask);
                        _localSubnetString = subnetName.concat("/" + cidr);
                        log.trace("Local subnet is: " + _localSubnetString);
                        return true;
                    }
                    catch (UnknownHostException e) {
                        //could not resolve the host
                        log.error(e.toString());
                        e.printStackTrace();
                    }
                }
                else {
                    log.warn("Could not retrieve subnet info from worldState, is the local master receiving topology info from netsensor?");
                }
            }
        }
        else {
            log.info("subnetworks is null");
        }
        return false;
    }
    */
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method check if any of the two nodes is receiving data from the other one.
     * @param localWorldState WorldState
     * @param nodeA IP of nodeA
     * @param nodeB IP of nodeB
     * @return true if there is traffic between the two nodes, false otherwise.
     */
    /*
    private boolean thereIsTraffic (WorldState localWorldState, String nodeA, String nodeB)
    {
        List<String> ipList = getIPList(WORLDSTATE_MULTICAST_ADDRESSES);
        if (nodeA != null && nodeB != null) {
            if ((localWorldState.getIncomingTraffic(nodeA, nodeA, nodeB, getTimeValidity()) > 0) ||
                    (localWorldState.getIncomingTraffic(nodeB, nodeB, nodeA, getTimeValidity()) > 0)) {
                log.trace("Incoming traffic detected between " + nodeA + " and " + "node" + nodeB);
                return true;
            }

            for(String multicastAddr : ipList) {
                if (localWorldState.getIncomingMcastTraffic(nodeA, multicastAddr, nodeB, getTimeValidity()) > 0) {
                    log.trace("Multicast A: observer " + nodeA +  ", multicast ip " + multicastAddr + ", source" + nodeB);
                    return true;
                }

                if (localWorldState.getIncomingMcastTraffic(nodeB, multicastAddr, nodeA, getTimeValidity()) > 0) {
                    log.trace("Multicast A: observer " + nodeB +  ", multicast ip " + multicastAddr + ", source" + nodeA);
                    return true;
                }
            }
        }
        return false;
    }
*/
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * This method check if there is observed traffic between two nodes
     * @param localWorldState WorldState
     * @param nodeA IP of nodeA
     * @param nodeB IP of nodeB
     * @param observer IP of the observer
     * @return true if there is traffic observed between the two nodes, false otherwise.
     */
    /*
    private boolean noticedObservedTraffic (WorldState localWorldState, String nodeA, String nodeB, String observer)
    {
        if((localWorldState.getObservedTraffic(observer, nodeA, nodeB, getTimeValidity()) >0) &&
                (localWorldState.getObservedTraffic(observer, nodeB, nodeA, getTimeValidity()) > 0)){
            return true;
        }
        return false;
    }
    */
    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Getter method for the private variable _timeValidity
     * @retur _timeValidity
     */
    private Duration getTimeValidity ()
    {
        return _timeValidity;
    }

    /**
     * Author: Roberto Fronteddu
     * Year of last revision: 2016
     * Setter method for the private variable _timeValidity
     * @param sec Seconds representing the delta
     */
    private boolean setTimeValidity (int sec)
    {
        _timeValidity = Duration.newBuilder().setSeconds(sec).build();
        return true;
    }

    //VARIABLES
    private int[]       _interconnectedPs;
    private int         _lastInterconnectedP;
    private int[]       _backhaulPs;
    private double[]    _totalBW;
    private double[]    _withinBW;
    private int _lastBackaulP;

    private double[] _observedLatencies;
    private double[] _observedSaturations;

    private int _interconnectedP;
    private int _backhaulP;
    private double _observedLatency;
    private double _lastObservedLatency;
    private double _observedSaturation;     //"Estimated saturation of the link expressed as a percentage.  0 = 0%
    private double _withinBandwithEstimated; //Bandwidth estimate within the identified network in kbps
    private Duration _timeValidity;
    private Subnetwork _localSubNetwork = null;
    private String _localSubnetString;
    private int _currentMeasureIndex;
    private int _numberOfMeasures;
    private double _maxLinkBW;
    private int _maxLinkBWCounter;
    private double _maxWithinBWDetected;
    private int _withinBWCounter;
    private double _totalBandwithEstimated;

    private int _counterInterconnectedP;
    private int _counterBackhaulP;
    private int _counterObservedLatency;
    private int _counterObservedSaturation;

    //private String _netProxyIP;
    private String _state;
    private Node[] _nodesArray;

    private double _currentTrafficInTheLink;
    boolean _remoteProxyOverride;
    Collection<String> _remoteProxyIps;
    private boolean _calculateInterconnectedP;
    private boolean _calculateBackhaulP;
    private boolean _calculateObserveLatency;
    private boolean _calculateObservedSaturation;
    private boolean _calculateTotalBandwidthEstimated;
    private boolean _calculateWithinBandwithEstimated;

    private static final Logger log = Logger.getLogger(TrafficManager.class);
}