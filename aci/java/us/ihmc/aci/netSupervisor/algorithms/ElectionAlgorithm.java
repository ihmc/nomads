package us.ihmc.aci.netSupervisor.algorithms;

import com.google.protobuf.Duration;
import com.google.protobuf.util.TimeUtil;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.aci.netSupervisor.link.LinkTypeContainer;
import us.ihmc.aci.netSupervisor.topology.TopologyManager;
import us.ihmc.aci.netSupervisor.topology.Subnetwork;
import us.ihmc.aci.netSupervisor.netSupervisorEnum.enums;
//import us.ihmc.aci.nodemon.WorldState;
import us.ihmc.aci.ddam.*;
//import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.aci.test.ddam.LinkType;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import static us.ihmc.aci.netSupervisor.algorithms.ElectionAlgorithm.LinkBetweenSubnetworkType.*;

/**
 * Created by Roberto on 4/26/2016.
 */
public class ElectionAlgorithm {


    /*--------------------------------------------------------------------------------------------------------------
    TWO NETWORKS PART
    --------------------------------------------------------------------------------------------------------------*/

    /**
     * Created by Emanuele on 08/31/2016.
    **/
    public ElectionAlgorithm()
    {
        _lowPerfPointB = new HashMap<Integer,Integer>();
        _highPerfPointB = new HashMap<Integer,Integer>();
        _lowPerfPointL = new HashMap<Integer,Integer>();
        _highPerfPointL = new HashMap<Integer,Integer>();
        _monitoredNodes = new ArrayList<String>();
        //timeValidity = Duration.newBuilder().setSeconds(validity).build();
    }

    public static void SetTimeValidity(int validity)
    {
        timeValidity = Duration.newBuilder().setSeconds(validity).build();
    }

    /**
     * Assigning points with a lot of cases on bandwidth
     * @param bw bandwidth
     * @param ID identification of the network type (BAD, GOOD)
     * @return the points assigned to the ID network
     */
    public static int countBWTwoNet(double bw, int ID)
    {

        double goodB = _highPerformanceBandwidth;
        double badB = _lowPerformanceBandwidth;

        if(ID == 1) {
            //good network (1G)
            if(bw >= goodB)
                return 100;
            if(((0.9 * goodB) <= bw) && (bw < goodB))
                return 90;
            if(((0.75 * goodB) <= bw) && (bw < (goodB * 0.9)))
                return 90;
            if(((0.50 * goodB) <= bw) && (bw < (goodB * 0.75)))
                return 80;
            if(((0.25 * goodB) <= bw) && (bw < (goodB * 0.50)))
                return 70;
            if(((0.10 * goodB) <= bw) && (bw < (goodB * 0.25)))
                return 60;
            if(bw < (goodB * 0.10))
                return 40;
        }

        if(ID == 2) {
            //bad network (1M)
            if(bw >= badB)
                return -20;
            if(((0.9 * badB) <= bw) && (bw < badB))
                return 90;
            if(((0.75 * badB) <= bw) && (bw < (badB * 0.9)))
                return 100;
            if(((0.50 * badB) <= bw) && (bw < (badB * 0.75)))
                return 80;
            if(((0.25 * badB) <= bw) && (bw < (badB * 0.50)))
                return 70;
            if(((0.10 * badB) <= bw) && (bw < (badB * 0.25)))
                return 60;
            if(bw < (badB * 0.10))
                return 40;
        }

        log.warn ("countBW no match?");
        return 0;
    }

    /**
     * Assigning points with a few cases on bandwidth
     * @param bw bandwidth
     * @param ID identification of the network type (BAD, GOOD)
     * @return the points assigned to the ID network
     */

    public static int countBWTwoNetSimple(double bw, int ID)
    {

        double goodB = _highPerformanceBandwidth;
        double badB = _lowPerformanceBandwidth;

        if(ID == 1) {
            //good network (1G)
            if(bw >= goodB)
                return _highPerfPointB.get(1);
            if(((0.4 * goodB) <= bw) && (bw < goodB))
                return _highPerfPointB.get(2);
            if(((0.1 * goodB) <= bw) && (bw < 0.4 * goodB))
                return _highPerfPointB.get(3);
            if((0 < bw) && (bw < (0.1 * goodB)))
                return _highPerfPointB.get(4);
        }

        if(ID == 2) {
            //bad network (1M)
            //first case: impossible to obtain a throughput heavier than the max
            if(bw >= badB)
                return _lowPerfPointB.get(4);
            if(((0.4 * badB) <= bw) && (bw < badB))
                return _lowPerfPointB.get(1);
            if(((0.1 * badB) <= bw) && (bw < (0.4 * badB)))
                return _lowPerfPointB.get(2);
            if((0 < bw) && (bw < (0.1 * badB)))
                return _lowPerfPointB.get(3);
        }

        log.warn ("countBW no match?");
        return 0;
    }

    /**
     * Assigning points with a lot of cases on latency
     * @param lat latency
     * @param ID identification of the network type (BAD, GOOD)
     * @return the points assigned to the ID network
     */

    public static int countLATTwoNet(double lat, int ID)
    {

        int goodL = _highPerformanceLatency;
        int badL = _lowPerformanceLatency;

        if(ID == 1) {
            //good network (100m)
            if(((0.8 * goodL) <= lat) && (lat < (1.2 * goodL)))
                return 100;
            if(((1.2 * goodL) <= lat) && (lat <= (2 * goodL)))
                return 100;
            if(lat > (2 * goodL))
                return 50;
            if(((0.5 * goodL) <= lat) && (lat <= (0.8 * goodL)))
                return 90;
            if(((0.2 * goodL) <= lat) && (lat <= (0.5 * goodL)))
                return 90;
            if(((0.1 * goodL) <= lat) && (lat <= (0.2 * goodL)))
                return 90;
            if(lat < (0.1 * goodL))
                return 90;
        }

        if(ID == 2) {
            //bad network (400m)
            if(((0.8 * badL) <= lat) && (lat < (1.2 * badL)))
                return 100;
            if(((1.2 * badL) <= lat) && (lat <= (2 * badL)))
                return 100;
            if(lat > (2 * badL))
                return 100;
            if(((0.5 * badL) <= lat) && (lat <= (0.8 * badL)))
                return 40;
            if(((0.2 * badL) <= lat) && (lat <= (0.5 * badL)))
                return 30;
            if(((0.1 * badL) <= lat) && (lat <= (0.2 * badL)))
                return 20;
            if(lat < (0.1 * badL))
                return 10;
        }

        log.warn ("countLAT no match?");
        return 0;
    }



    /**
     * Assigning points with a few cases on latency
     * @param lat latency
     * @param ID identification of the network type (BAD, GOOD)
     * @return the points assigned to the ID network
     */

    public static int countLATTwoNetSimple(double lat, int ID)
    {

        int goodL = _highPerformanceLatency;
        int badL = _lowPerformanceLatency;

        if(ID == 1) {
            //good network (100m)
            if(((0.7 * goodL) <= lat) && (lat < (2 * goodL)))
                return _highPerfPointL.get(1);
            if(lat >= (2 * goodL))
                return _highPerfPointL.get(4);
            if(((0.1 * goodL) <= lat) && (lat <= (0.7 * goodL)))
                return _highPerfPointL.get(2);
            if((0 < lat) && (lat < (0.1 * goodL)))
                return _highPerfPointL.get(3);
        }

        if(ID == 2) {
            //bad network (400m)
            if(((0.8 * badL) <= lat) && (lat < (2 * badL)))
                return _lowPerfPointL.get(1);
            if(lat > (2 * badL))
                return _lowPerfPointL.get(2);
            if(((0.5 * badL) <= lat) && (lat <= (0.8 * badL)))
                return _lowPerfPointL.get(3);
            if(((0.1 * badL) <= lat) && (lat <= (0.5 * badL)))
                return _lowPerfPointL.get(4);
            if((0 < lat) && (lat < (0.1 * badL)))
                return _lowPerfPointL.get(5);
        }

        log.warn ("countLAT no match?");
        return 0;
    }

    /**
     * ElectionAlgorithm on two types of networks
     * @param throughput checked bandwidth
     * @param latency checked latency
     * @param packetLoss checked packet loss
     * @return the supposed Lnk type
     */
    static public String parametricElectionAlgorithm(double throughput, double latency, int packetLoss)
    {
        int points, max = -100;
        String idMax = "UNKNOWN";

        LinkType linkType;

        int numberOfLinkTypes = LinkTypeContainer.getLinkTypesNumber();
        int accuracy = LinkTypeContainer.getAccuracy();

        if(numberOfLinkTypes != 0) {
            for (String linkTypeId : LinkTypeContainer.getLinkTypes().keySet()) {
                linkType = LinkTypeContainer.getLinkTypeById(linkTypeId);
                log.debug("Calculating points for the link type " + linkTypeId);
                points = pointsForThroughput(throughput, numberOfLinkTypes * accuracy, linkType);
                points = points + pointsForLatency(latency, numberOfLinkTypes * accuracy, linkType);
                log.debug(linkTypeId + " points: " + points);
                if (points > max) {
                    max = points;
                    idMax = linkTypeId;
                }
            }
        }

        return idMax;
    }


    /**
     * parametric method to give a score on bw for a type of network
     * @param bwChecked bandwidth given from the worldstate
     * @param count number of link types
     * @param linkType linkType to analize
     * @return
     */
    private static int pointsForThroughput(double bwChecked, double count, LinkType linkType)
    {
        double upperBandwidth, lowerBandwidth;
        double step = linkType.getBounds().getLinkThroughputBounds().getThroughputStep();
        int points = 100;
        int pointStep;
        pointStep = (int)(points/count);
        upperBandwidth = linkType.getValues().getThroughput();
        for(int i=1; i <= count; i++){
            lowerBandwidth = upperBandwidth - step;
            if((bwChecked <= upperBandwidth) && (bwChecked > lowerBandwidth)) {
                return (points - i * pointStep);
            }
            upperBandwidth = lowerBandwidth;
        }
        if(bwChecked <= upperBandwidth) {
            return (int)(points / (count + 1));
        }
        points = - pointStep;
        log.debug("Throughput points: " + points);
        return points;
    }

    /**
     * parametric method to give a score on latency for a type of network
     * @param latChecked latency given from the worldstate
     * @param count number of link types
     * @param linkType linkType to analize
     * @return
     */
    private static int pointsForLatency(double latChecked, double count, LinkType linkType)
    {
        int upperLatency, lowerLatency;
        int step = linkType.getBounds().getLinkLatencyBounds().getLowerLatencyStep();
        int points = 100;
        int pointStep;
        pointStep = (int)(points/count);
        upperLatency = linkType.getValues().getLatency();
        for(int i=1; i <= count; i++){
            lowerLatency = upperLatency - step;
            if((latChecked <= upperLatency) && (latChecked > lowerLatency)) {
                return (points - i * pointStep);
            }
            upperLatency = lowerLatency;
        }

        points = 100;
        lowerLatency = linkType.getValues().getLatency();
        step = linkType.getBounds().getLinkLatencyBounds().getUpperLatencyStep();
        for (int i = 1; i <= count; i++) {
            upperLatency = lowerLatency + step;
            if ((latChecked <= upperLatency) && (latChecked > lowerLatency)) {
                return (points - i * pointStep);
            }
            lowerLatency = upperLatency;
        }
        log.debug("Throughput points: " + points);
        return (int)(points / (count + 1));
    }

    /**
     * ElectionAlgorithm on two types of networks
     * @param throughput checked bandwidth
     * @param latency checked latency
     * @param packetLoss checked packet loss
     * @return the supposed Lnk type
     */
    static public String electionAlgorithmTwoNet(double throughput, double latency, int packetLoss)
    {
        int sumGood = 0;
        int sumBad = 0;

        int pointGoodB = 0;
        int pointBadB = 0;

        int pointGoodL = 0;
        int pointBadL = 0;

        //calculating bandwidth score
        pointGoodB = countBWTwoNetSimple(throughput, 1) + pointGoodB;
        pointBadB = countBWTwoNetSimple(throughput, 2) + pointBadB;

        log.debug("Good B points: " + pointGoodB);
        log.debug("Bad B points: " + pointBadB);

        //calculating latency score
        pointGoodL = countLATTwoNetSimple(latency, 1) + pointGoodL;
        pointBadL = countLATTwoNetSimple(latency, 2) + pointBadL;

        log.debug("Good L points: " + pointGoodL);
        log.debug("Bad L points: " + pointBadL);

        //calculating total score
        sumGood = pointGoodB + pointGoodL;
        sumBad = pointBadB + pointBadL;

        log.debug("Good Sum: " + sumGood);
        log.debug("Bad Sum: " + sumBad);

        return electWinnerTwoNet(sumGood,sumBad);
    }

    /**
     * election of the best network between the 2 network
     * @param sumGood sum of the good network points
     * @param sumBad sum of the good network points
     * @return the String with the description of the network type
     */
    public static String electWinnerTwoNet(double sumGood, double sumBad)
    {
        if (sumGood > sumBad) {
            return "GOOD";
        }else{
            if (sumGood < sumBad){
                return "BAD";
            }
        }

        //something is wrong with the scores if we get here
        log.warn("sumGood and sumBad have the same score, something is wrong?");
        return "UNKNOWN";
    }



    /*----------------------------------------------------------------------------
                        BETWEEN TWO NODES (WITH THE OBSERVER)
    ------------------------------------------------------------------------------ */

    /**
     * METHOD USED : calculates the link type between two nodes (giving the IP)
     * @param ipN1 first node IP
     * @param ipN2 second node IP
     * @param observer1 first observer IP (master)
     * @param observer2 second observer IP (master)
     * @param localWorldState actual worldstate
     * @return the obj link
     */
    /*
    static public Link firstGuessWithIP(String ipN1, String ipN2, String observer1, String observer2, WorldState localWorldState)
    {
        String ConnType;
        ConnType = "UNKNOWN";
        String condition = "UNRECOGNIZED";
        double throughput = 0;
        int packetLoss = 0;
        int latency = 0;

        Description linkDescription;
        linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();

        if((ipN1 == null) || (ipN2 == null) || (localWorldState == null)) {
            //for now ignore missing data
            return Link.newBuilder().setDescription(linkDescription).build();
        }
        else {

            int tp1 = 0;
            int tp2 = 0;

            try{
                tp1 = localWorldState.getIncomingTraffic(observer2, ipN2, ipN1,timeValidity);
                log.trace("Get traffic node 2 received from node 1: ");

                tp2 = localWorldState.getIncomingTraffic(observer1, ipN1, ipN2, timeValidity);
                log.trace("Get traffic node 1 sent to node 2: ");

                latency = localWorldState.getLatency(observer1, ipN1, ipN2, timeValidity);
                log.trace("LATENCY between: " + ipN1 + " " + ipN2 + " is " + latency);
            }catch(NullPointerException e){
                log.trace("No traffic between " + ipN1 + " and " + ipN2);
            }

            if(((tp1 + tp2) == 0) || (latency < 0)) {
                //for now ignore missing data
                linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
            }
            else {
                throughput = tp1 + tp2;
                //packetLoss = (int) ((tp2 - throughput) * 100 / tp2);
                log.trace(ipN2 + " in is " + throughput);
                //log.info("PL between" + ipN1 + " and " + ipN1 +  " is " + packetLoss);
                log.trace("LATENCY between: " + ipN1 + " " + ipN2 + " is " + latency);
                //setting connection type between the nodes using ElectionAlgorithm
                ConnType = electionAlgorithmTwoNet(throughput,latency,packetLoss);
                linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
            }
        }
        return Link.newBuilder().setDescription(linkDescription).build();
    }
*/



    /**
     * Method called to obtain the obj Link with the new link type
     * @param ipN1 first node IP
     * @param ipN2 second node IP
     * @param observer1 first observer IP (master)
     * @param observer2 second observer IP (master)
     * @param localWorldState actual worldstate
     * @return the obj link
     */
    /*
    static public Link GuessConnectionElectionAlgorithmWithIP(String ipN1, String ipN2, String observer1, String observer2, WorldState localWorldState)
    {
        Link oldGuess;
        if((oldGuess = idsCouple.get(ipN1+ipN2)) == null) {
            //no old guess
            Link newGuess;
            newGuess = firstGuessWithIP(ipN1,ipN2, observer1, observer2, localWorldState);
            idsCouple.put(ipN1+ipN2,newGuess);
            return newGuess;
        }
        else {
            //old guess present
            return newGuessWithIP(ipN1, ipN2, observer1, observer2, localWorldState, oldGuess);
        }
    }
    */


    /**
     * Return the obj Link for the new state on the worldstate
     * @param ipN1 first node IP
     * @param ipN2 second node IP
     * @param observer1 first observer IP (master)
     * @param observer2 second observer IP (master)
     * @param localWorldState actual worldstate
     * @param oldGuess old obj link
     * @return the new obj link
     */
    /*
    static public Link newGuessWithIP(String ipN1, String ipN2, String observer1, String observer2, WorldState localWorldState, Link oldGuess)
    {
        String ConnType = "UNKNOWN";
        double throughput = 0;
        int packetLoss = 0;
        int latency = 0;

        //old part for describing the changes on a network

        //if(oldGuess.getDescription().getType() == "UNKNOWN") {
         //   return firstGuessWithIP(ipN1, ipN2, observer1, observer2, localWorldState);
        //}else{
         //   if (oldGuess.getDescription().getType() == "UNRECOGNIZED") {
          //      return firstGuessWithIP(ipN1, ipN2, observer1, observer2, localWorldState);
           // }
            //else {
             //   log.warn("Link was Unknown but Projection was not Unrecognized?");
           // }
       // }


        //if(oldGuess.getDescription().getType() == "LAN") {
         //   Link newLink = firstGuessWithIP(ipN1, ipN2, observer1, observer2, localWorldState);

//            if(newLink.getDescription().getType() == oldGuess.getDescription().getType()) {
//
  //              return Link.newBuilder(newLink).setDescription(Description.newBuilder().setType(calculateProjection(throughput, packetLoss, latency, oldGuess)).build()).build();
    //        }
      //      else {
        //        return newLink;
          //  }

//        }


        return firstGuessWithIP(ipN1,ipN2, observer1, observer2, localWorldState);
    }
    */


    /**
     * Adds a list of nodes to be monitored
     * @param monitoredNodes
     */
    public void setMonitoredNodes(Collection<String> monitoredNodes)
    {
        _monitoredNodes.addAll(monitoredNodes);
    }

    /**
     * sets the low performance max bandwidth
     * @param lowPerformanceBandwidth
     */
    public void setLowPerformanceBandwidth(double lowPerformanceBandwidth)
    {
        _lowPerformanceBandwidth = lowPerformanceBandwidth;
    }

    /**
     * sets the high performance max bandwidth
     * @param highPerformanceBandwidth
     */
    public void setHighPerformanceBandwidth(double highPerformanceBandwidth)
    {
        _highPerformanceBandwidth = highPerformanceBandwidth;
    }

    /**
     * sets the low performance avg latency
     * @param lowPerformanceLatency
     */
    public void setLowPerformanceLatency(int lowPerformanceLatency)
    {
        _lowPerformanceLatency = lowPerformanceLatency;
    }

    /**
     * sets the high performance avg latency
     * @param highPerformanceLatency
     */
    public void setHighPerformanceLatency(int highPerformanceLatency)
    {
        _highPerformanceLatency = highPerformanceLatency;
    }

    /**
     * sets the points for the low performance bandwidth in the election algorithm
     * @param lowPerfPointB
     */
    public void setLowPerfPointB(Map<Integer,Integer> lowPerfPointB)
    {
        _lowPerfPointB.putAll(lowPerfPointB);
    }

    /**
     * sets the points for the high performance bandwidth in the election algorithm
     * @param highPerfPointB
     */
    public void setHighPerfPointB(Map<Integer,Integer> highPerfPointB)
    {
        _highPerfPointB.putAll(highPerfPointB);
    }

    /**
     * sets the points for the low performance latency in the election algorithm
     * @param lowPerfPointL
     */
    public void setLowPerfPointL(Map<Integer,Integer> lowPerfPointL)
    {
        _lowPerfPointL.putAll(lowPerfPointL);
    }

    /**
     * sets the points for the high performance latency in the election algorithm
     * @param highPerfPointL
     */
    public void setHighPerfPointL(Map<Integer,Integer> highPerfPointL)
    {
        _highPerfPointL.putAll(highPerfPointL);
    }

    /**
     * returns the collection of the monitored nodes
     * @return
     */
    public Collection getMonitoredNodes()
    {
        return _monitoredNodes;
    }

    /**
     * returns the low performance bandwidth
     * @return
     */
    public double getLowPerformanceBandwidth()
    {
        return _lowPerformanceBandwidth;
    }

    /**
     * returns the high performance bandwidth
     * @return
     */
    public double getHighPerformanceBandwidth()
    {
        return _highPerformanceBandwidth;
    }

    /**
     * returns the low performance latency
     * @return
     */
    public int getLowPerformanceLatency()
    {
        return _lowPerformanceLatency;
    }

    /**
     * returns the high performance latency
     * @return
     */
    public int getHighPerformanceLatency()
    {
        return _highPerformanceLatency;
    }

    /**
     * returns the points list of the low performance bandwidth
     * @return
     */
    public Map<Integer,Integer> getLowPerfPointB()
    {
        return _lowPerfPointB;
    }

    /**
     * returns the points list of the high performance bandwidth
     * @return
     */
    public Map<Integer,Integer> getHighPerfPointB()
    {
        return _highPerfPointB;
    }

    /**
     * returns the points list of the low performance latency
     * @return
     */
    public Map<Integer,Integer> getLowPerfPointL()
    {
        return _lowPerfPointL;
    }

    /**
     * returns the points list of the high performance latency
     * @return
     */
    public Map<Integer,Integer> getHighPerfPointL()
    {
        return _highPerfPointL;
    }

    //NEW METHODS

    /**
     * updates all the link types of the network
     * @param localWorldState
     * @param topologyManager
     */
    /*
    public void detectLinksBetweenMonitoredNodes(WorldState localWorldState, TopologyManager topologyManager)
    {
        LinkBetweenSubnetworkType linkBetweenSubnetworkType = UNKNOWN;
        Description linkDescription = null;
        Subnetwork subnetA;
        Subnetwork subnetB;

        for (String nodeA : _monitoredNodes) {
            for (String nodeB : _monitoredNodes) {
                subnetA = topologyManager.getNodeSubnetwork(nodeA);
                subnetB = topologyManager.getNodeSubnetwork(nodeB);
                if (subnetA != null && subnetB != null) {
                    linkBetweenSubnetworkType = selectConfigurationBetweenSubnets(subnetA, subnetB, localWorldState);
                    linkDescription = findLinkType(linkBetweenSubnetworkType, subnetA, subnetB, localWorldState);
                    if (linkDescription != null && subnetA.getNodeMonitorMaster() != null) {
                        int ipNodeA = Utils.convertIPToInteger(nodeA);
                        int ipNodeB = Utils.convertIPToInteger(nodeB);
                        localWorldState.updateLink(subnetA.getNodeMonitorMaster(), Link.newBuilder().setTimestamp(
                                TimeUtil.getCurrentTime()).setDescription(linkDescription).setIpDst(
                                ipNodeB).setIpSrc(ipNodeA).build());
                        log.debug("[EA] Link type updated between " + nodeA + " and " + nodeB + " : " +
                                linkDescription.getType());
                        System.out.println("[EA] Link type updated between " + nodeA + " and " + nodeB + " : " +
                                linkDescription.getType());
                    }
                    else {
                        log.debug("Error updating the link type of the connection between " +
                                subnetA.getName() + " and " + subnetB.getName());
                    }
                }
                else {
                    log.debug("One of the subnetwork isn't set.");
                }
            }
        }
    }
*/

    /**
     * Finds the type of the two endpoints : proxy to proxy, no proxy in one subnet, no proxy in both the subnets, same subnet
     * @param subnetA first subnet
     * @param subnetB second subnet
     * @param localWorldState
     * @return the type of the environment
     */
    /*
    private LinkBetweenSubnetworkType selectConfigurationBetweenSubnets(Subnetwork subnetA, Subnetwork subnetB,
                                                                        WorldState localWorldState)
    {

        if(subnetA.getNodeMonitorMaster() != null && subnetB.getNodeMonitorMaster() != null) {
            if (!subnetA.getName().equals(subnetB.getName())) {
                String netProxyIp1 = subnetA.getNetProxy();
                String netProxyIp2 = subnetB.getNetProxy();
                //if there are netProxy

                if(netProxyIp1 != null) {
                    if(netProxyIp2 != null) {
                        log.debug("Type of the connection between " + subnetA.getName() + " and " + subnetB.getName() +
                                " : PROXY_TO_PROXY");
                        return PROXY_TO_PROXY;
                    }
                    else {
                        log.debug("Type of the connection between " + subnetA.getName() + " and " + subnetB.getName() +
                                " : PROXY_TO_SUBNETWORK");
                        return PROXY_TO_SUBNETWORK;
                    }
                }
                else {
                    if(netProxyIp2 != null) {
                        log.debug("Type of the connection between " + subnetA.getName() + " and " + subnetB.getName() +
                                " : SUBNETWORK_TO_PROXY");
                        return SUBNETWORK_TO_PROXY;
                    }
                    else {
                        log.debug("Type of the connection between " + subnetA.getName() + " and " + subnetB.getName() +
                                " : SUBNETWORK_TO_SUBNETWORK");
                        return SUBNETWORK_TO_SUBNETWORK;
                    }
                }
            }
            else {
                log.debug("Type of the connection between " + subnetA.getName() + " and " + subnetB.getName() +
                        " : SAME_SUBNETWORK");
                return SAME_SUBNETWORK;
            }
        }

        log.debug("Connection UNKNOWN");
        return UNKNOWN;
    }
*/
    /**
     * Calculate the main Description between two Objects of a network
     * @param linkBetweenSubnetworkType the type of interconnection
     * @param subnetA first subnetwork
     * @param subnetB second subnetwork
     * @param localWorldState
     * @return the main description of the link
     */
    /*
    private Description findLinkType(LinkBetweenSubnetworkType linkBetweenSubnetworkType, Subnetwork subnetA,
                                     Subnetwork subnetB, WorldState localWorldState)
    {

        Description linkDescription = null;
        double totThroughput = 0;
        double totLatency = 0;
        int nodeCounter = 0;
        int latency = 0;
        int packetLoss = 0;

        String connType = "UNKNOWN";
        String condition = "UNRECOGNIZED";

        Description linkParameters = null;

        String nodeMonitorMasterIp1 = subnetA.getNodeMonitorMaster();
        String nodeMonitorMasterIp2 = subnetB.getNodeMonitorMaster();

        switch(linkBetweenSubnetworkType){
            case SAME_SUBNETWORK:
                return null;


            case PROXY_TO_PROXY:
                linkParameters = getConnectionParameters(subnetA.getNetProxy(), subnetB.getNetProxy(),
                        nodeMonitorMasterIp1, nodeMonitorMasterIp2, localWorldState);
                if(linkParameters != null) {
                    totThroughput = totThroughput + linkParameters.getThroughput();
                    totLatency = totLatency + linkParameters.getLatency();
                    nodeCounter++;
                }
                break;

            case PROXY_TO_SUBNETWORK:

                for (String ipNode2 : subnetB.getInternalNodes()) {
                    linkParameters = getConnectionParameters(subnetA.getNetProxy(), ipNode2,
                            nodeMonitorMasterIp1, nodeMonitorMasterIp2, localWorldState);
                    if(linkParameters != null) {
                        totThroughput = totThroughput + linkParameters.getThroughput();
                        totLatency = totLatency + linkParameters.getLatency();
                        nodeCounter++;
                    }
                }

                break;

            case SUBNETWORK_TO_PROXY:

                for (String ipNode1 : subnetA.getInternalNodes()) {
                    linkParameters = getConnectionParameters(ipNode1, subnetB.getNetProxy(), nodeMonitorMasterIp1,
                            nodeMonitorMasterIp2, localWorldState);
                    if(linkParameters != null) {
                        totThroughput = totThroughput + linkParameters.getThroughput();
                        totLatency = totLatency + linkParameters.getLatency();
                        nodeCounter++;
                    }
                }

                break;

            case SUBNETWORK_TO_SUBNETWORK:


                for (String ipNode1 : subnetA.getInternalNodes()) {
                    for (String ipNode2 : subnetB.getInternalNodes()) {
                        linkParameters = getConnectionParameters(ipNode1, ipNode2, nodeMonitorMasterIp1, nodeMonitorMasterIp2, localWorldState);
                        if(linkParameters != null) {
                            totThroughput = totThroughput + linkParameters.getThroughput();
                            totLatency = totLatency + linkParameters.getLatency();
                            nodeCounter++;
                        }
                    }
                }

                break;

        }

        if(nodeCounter > 0) {
            latency = (int) (totLatency/nodeCounter);
        }

        if(totThroughput == 0 || latency < 0) {
            log.debug("No data connection between " + subnetA.getName() + " and " + subnetB.getName());
            linkDescription = Description.newBuilder().setType(connType).setThroughput(totThroughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
        }
        else {
            //log.info("PL between" + ipN1 + " and " + ipN1 +  " is " + packetLoss);
            log.debug("Average latency between " + subnetA.getName() + " " + subnetB.getName() + " is " + latency);
            log.debug("Total throughput between " + subnetA.getName() + " " + subnetB.getName() + " is " + totThroughput);

            //setting connection type between the nodes using ElectionAlgorithm
            connType = parametricElectionAlgorithm(totThroughput,latency,packetLoss);
            linkDescription = Description.newBuilder().setType(connType).setThroughput(totThroughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
        }


        return linkDescription;
    }
*/

    /**
     * Finds the link description between two nodes
     * @param nodeIp1 first node
     * @param nodeIp2 second node
     * @param observerIp1 master of the first node
     * @param observerIp2 master of the second node
     * @param localWorldState
     * @return the object Description
     */
    /*
    private Description getConnectionParameters(String nodeIp1, String nodeIp2, String observerIp1, String observerIp2, WorldState localWorldState)
    {

        int tp1 = 0;
        int tp2 = 0;

        int latency = 0;

        try {
            log.trace("Get traffic node 2 received from node 1: ");
            tp1 = localWorldState.getIncomingTraffic(observerIp2, nodeIp2, nodeIp1,timeValidity);

            log.trace("Get traffic node 1 sent to node 2: ");
            tp2 = localWorldState.getIncomingTraffic(observerIp1, nodeIp1, nodeIp2, timeValidity);

            latency = localWorldState.getLatency(observerIp1, nodeIp1, nodeIp2, timeValidity);

            if(latency == 0){
                latency = localWorldState.getLatency(observerIp2, nodeIp2, nodeIp1, timeValidity);
            }

            log.debug("LATENCY between: " + nodeIp1 + " " + nodeIp2 + " is " + latency);
        }
        catch(NullPointerException e) {
            log.debug("No traffic between " + nodeIp2 + " and " + nodeIp2);
        }

        Description descr = Description.newBuilder().setLatency(latency).setThroughput(tp1 + tp2).build();

        return descr;

    }
*/

    //class variables

    static Duration timeValidity;
    private static final Logger log = LoggerFactory.getLogger(SimpleAlgorithm.class);
    static private HashMap<String,Link> idsCouple = new HashMap<String,Link>();

    //networks parameters variables
    private static Double _lowPerformanceBandwidth;
    private static Double _highPerformanceBandwidth;
    private static int _lowPerformanceLatency;
    private static int _highPerformanceLatency;

    private static Map<Integer,Integer> _highPerfPointB;
    private static Map<Integer,Integer> _lowPerfPointB;
    private static Map<Integer,Integer> _highPerfPointL;
    private static Map<Integer,Integer> _lowPerfPointL;

    private static Collection<String> _monitoredNodes;

    enum LinkBetweenSubnetworkType{
        PROXY_TO_PROXY,
        SUBNETWORK_TO_PROXY,
        PROXY_TO_SUBNETWORK,
        SUBNETWORK_TO_SUBNETWORK,
        SAME_SUBNETWORK,
        UNKNOWN
    }


    /*------------------------------------------------------------------------------------
                                        OLD STUFF
     --------------------------------------------------------------------------------------*/


    /**
     * The method checks the behavior of the network on the link. It could be worse or better.
     * @param throughput the harvested bandwidth in the last check
     * @param packetLoss the harvested packet loss in the last check
     * @param latency the harvested latency in the last check
     * @param oldGuess the previous characteristics of the link
     * @return a String with the Condition of the link
     */
    static public String calculateProjection(double throughput,int packetLoss,int latency, Link oldGuess)
    {

        timeValidity =  Duration.getDefaultInstance();
        timeValidity.newBuilderForType().setSeconds(5);


        double deltaThroughput = throughput - oldGuess.getDescription().getThroughput();
        double deltaPacketLoss = packetLoss - oldGuess.getDescription().getPacketLoss();
        double deltaLatency = latency - oldGuess.getDescription().getLatency();

        enums.trend throughputTrend;
        enums.trend packetLossTrend;
        enums.trend latencyTrend;

        throughputTrend = checkTrend(deltaThroughput);
        packetLossTrend = checkTrend(deltaPacketLoss);
        latencyTrend = checkTrend(deltaLatency);


        if((packetLossTrend == enums.trend.POSITIVE) || (latencyTrend == enums.trend.POSITIVE)) {
            return "Deteriorating";
        }

        if((packetLossTrend == enums.trend.NEGATIVE) || (latencyTrend == enums.trend.NEGATIVE)) {
            return "IMPROVING";
        }
        if((packetLossTrend == enums.trend.STABLE) || (latencyTrend == enums.trend.STABLE)) {
            return "STABLE";
        }
        return "DETERIORATING";
    }

    /**
     * Check the trend on the link (better, worse, same)
     * @param delta delta parameter
     * @return an enum to show the trend
     */
    static public enums.trend checkTrend(double delta)
    {
        //should put here something different than zero
        if(delta > 0) {
            //value increased
            return  enums.trend.POSITIVE;
        }

        if(delta < 0) {
            //value decreased
            return enums.trend.NEGATIVE;
        }

        if(delta == 0) {
            //value constant
            return enums.trend.STABLE;
        }
        log.warn("Should not be here!");
        return enums.trend.UNKNOWN;
    }


    public static int countBW(double bw, int ID)
    {

        double lanB = 1000000;
        double satB = 1000000;
        double hfB  = 56000;

        if(ID == 1) {
            //LAN
            if(bw >= lanB)
                return 100;
            if(((0.9 * lanB) <= bw) && (bw < lanB))
                return 90;
            if(((0.75 * lanB) <= bw) && (bw < (lanB * 0.9)))
                return 90;
            if(((0.50 * lanB) <= bw) && (bw < (lanB * 0.75)))
                return 80;
            if(((0.25 * lanB) <= bw) && (bw < (lanB * 0.50)))
                return 70;
            if(((0.10 * lanB) <= bw) && (bw < (lanB * 0.25)))
                return 60;
            if(bw < (lanB * 0.10))
                return 40;
        }

        if(ID == 2) {
            //SATCOM
            if(bw >= satB)
                return -20;
            if(((0.9 * satB) <= bw) && (bw < satB))
                return 90;
            if(((0.75 * satB) <= bw) && (bw < (satB * 0.9)))
                return 100;
            if(((0.50 * satB) <= bw) && (bw < (satB * 0.75)))
                return 80;
            if(((0.25 * satB) <= bw) && (bw < (satB * 0.50)))
                return 70;
            if(((0.10 * satB) <= bw) && (bw < (satB * 0.25)))
                return 60;
            if(bw < (satB * 0.10))
                return 40;
        }

        if(ID == 3) {
            //HF
            if(bw >= hfB)
                return -20;
            if(((0.9 * hfB) <= bw) && (bw < hfB))
                return 90;
            if(((0.75 * hfB) <= bw) && (bw < (hfB * 0.9)))
                return 100;
            if(((0.50 * hfB) <= bw) && (bw < (hfB * 0.75)))
                return 100;
            if(((0.25 * hfB) <= bw) && (bw < (hfB * 0.50)))
                return 90;
            if(((0.10 * hfB) <= bw) && (bw < (hfB * 0.25)))
                return 80;
            if(bw < (hfB * 0.10))
                return 80;
        }
        log.warn ("countBW no match?");
        return 0;
    }

    public static int countLAT(double lat, int ID)
    {

        int lanL = 10;
        int satL = 200;
        int hfL  = 100;

        if(ID == 1) {
            //LAN
            if(((0.8 * lanL) <= lat) && (lat < (1.2 * lanL)))
                return 100;
            if(((1.2 * lanL) <= lat) && (lat <= (2 * lanL)))
                return 100;
            if(lat > (2 * lanL))
                return 50;
            if(((0.5 * hfL) <= lat) && (lat <= (0.8 * lanL)))
                return 90;
            if(((0.2 * hfL) <= lat) && (lat <= (0.5 * lanL)))
                return 90;
            if(((0.1 * lanL) <= lat) && (lat <= (0.2 * lanL)))
                return 90;
            if(lat < (0.1 * lanL))
                return 90;
        }

        if(ID == 2) {
            //SATCOM
            if(((0.8 * satL) <= lat) && (lat < (1.2 * satL)))
                return 100;
            if(((1.2 * satL) <= lat) && (lat <= (2 * satL)))
                return 100;
            if(lat > (2 * hfL))
                return 100;
            if(((0.5 * satL) <= lat) && (lat <= (0.8 * satL)))
                return 40;
            if(((0.2 * satL) <= lat) && (lat <= (0.5 * satL)))
                return 30;
            if(((0.1 * satL) <= lat) && (lat <= (0.2 * satL)))
                return 20;
            if(lat < (0.1 * satL))
                return 10;
        }

        if(ID == 3) {
            //HF
            if(((0.8 * hfL) <= lat) && (lat < (1.2 * hfL)))
                return 100;
            if(((1.2 * hfL) <= lat) && (lat <= (2 * hfL)))
                return 100;
            if(lat > (2 * hfL))
                return 60;
            if(((0.5 * hfL) <= lat) && (lat <= (0.8 * hfL)))
                return 70;
            if(((0.2 * hfL) <= lat) && (lat <= (0.5 * hfL)))
                return 40;
            if(((0.1 * hfL) <= lat) && (lat <= (0.2 * hfL)))
                return 30;
            if(lat < (0.1 * hfL))
                return 20;
        }
        log.warn ("countLAT no match?");
        return 0;
    }

    static public String electionAlgorithm(double throughput, double latency, int packetLoss)
    {
        int sumLAN = 0;
        int sumSAT = 0;
        int sumHF = 0;

        int pointLANB = 0;
        int pointSATB = 0;
        int pointHFB = 0;

        int pointLANL = 0;
        int pointSATL = 0;
        int pointHFL = 0;

        pointLANB = countBW(throughput, 1) + pointLANB;
        pointSATB = countBW(throughput, 2) + pointSATB;
        pointHFB = countBW(throughput, 3) + pointHFB;

        log.debug("Lan B points: " + pointLANB);
        log.debug("Sat B points: " + pointSATB);
        log.debug("Hf B points: " + pointHFB);

        pointLANL = countLAT(latency, 1) + pointLANL;
        pointSATL = countLAT(latency, 2) + pointSATL;
        pointHFL = countLAT(latency, 3) + pointHFL;

        log.debug("Lan L points: " + pointLANL);
        log.debug("Sat L points: " + pointSATL);
        log.debug("Hf L points: " + pointHFL);

        sumLAN = pointLANB + pointLANL;
        sumSAT = pointSATB + pointSATL;
        sumHF = pointHFB + pointHFL;

        log.debug("Lan Sum: " + sumLAN);
        log.debug("Sat Sum: " + sumSAT);
        log.debug("Hf Sum: " + sumHF);

        return electWinner(sumLAN,sumSAT,sumHF);
    }

    public static String electWinner(double sumLAN, double sumSAT, double sumHF)
    {
        if (sumLAN > sumSAT) {
            if (sumLAN > sumHF) {
                return "LAN";
            }
            if (sumLAN < sumHF) {
                return "HF";
            }
            //something is wrong with the scores if we get here
            log.warn("sumLAN and sumHF have the same score, something is wrong?");
            return "UNKNOWN";
        }
        if (sumLAN < sumSAT) {
            if (sumSAT > sumHF) {
                return "SATCOM";
            }
            if (sumSAT < sumHF) {
                return "HF";
            }
            log.warn("sumSAT and sumHF have the same score, something is wrong?");
            return "UNKNOWN";
        }
        if (sumLAN == sumSAT) {
            if (sumSAT > sumHF) {
                return "SATCOM";
            }
            if (sumLAN < sumHF) {
                return "HF";
            }
            log.warn("sumSAT, sumHF, and sumLAN have the same score, something is wrong?");
            return "UNKNOWN";
        }
        log.warn("electWinner(), No match?");
        return "UNKNOWN";
    }

/*
    static public Link firstGuess(Node n1, Node n2, WorldState localWorldState)
    {
        String ConnType;
        ConnType = "UNKNOWN";
        String condition = "UNRECOGNIZED";
        double throughput = 0;
        int packetLoss = 0;
        int latency = 0;

        Description linkDescription;
        linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();

        if((n1.getId() == null) || (n2.getId() == null) || (localWorldState == null)) {
            //for now ignore missing data
            return Link.newBuilder().setDescription(linkDescription).build();
        } else {
            int tp1 = localWorldState.getIncomingTraffic(n2.getId(),n2.getId(), n1.getId(),timeValidity);
            log.debug("Get traffic node 2 received from node 1: ");

            int tp2 = localWorldState.getOutgoingTraffic(n1.getId(),n1.getId(), n2.getId(),timeValidity);
            log.debug("Get traffic node 1 sent to node 2: ");

            latency = localWorldState.getLatency(n2.getId(), n1.getId(), n2.getId(),timeValidity);
            log.debug("LATENCY between: " + n1.getId() + " " + n2.getId() + " is " + latency);

            if((tp1 == 0) || (tp2 == 0) || (latency < 0)) {
                //for now ignore missing data
                linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
            }
            else {
                throughput = tp1;
                packetLoss = (int) ((tp2 - throughput) * 100 / tp2);
                log.info(n2.getId() + " in is " + throughput);
                log.info("PL between" + n1.getId() + " and " + n2.getId() +  " is " + packetLoss);
                log.info("LATENCY between: " + n1.getId() + " " + n2.getId() + " is " + latency);
                //setting connection type between the nodes using ElectionAlgorithm
                ConnType = electionAlgorithmTwoNet(throughput,latency,packetLoss);
                linkDescription = Description.newBuilder().setType(ConnType).setThroughput(throughput).setPacketLoss(packetLoss).setLatency(latency).setCondition(condition).build();
            }
        }
        return Link.newBuilder().setDescription(linkDescription).build();
    }
*/
    /*
    static public Link GuessConnectionElectionAlgorithm(Node n1, Node n2, WorldState localWorldState)
    {
        Link oldGuess;
        if((oldGuess = idsCouple.get(n1.getId()+n2.getId())) == null) {
            //no old guess
            Link newGuess;
            newGuess = firstGuess(n1,n2,localWorldState);
            idsCouple.put(n1.getId()+n2.getId(),newGuess);
            return newGuess;
        }
        else {
            //old guess present
            return newGuess(n1, n2, localWorldState, oldGuess);
        }
    }
*/
/*
    static public Link newGuess(Node n1, Node n2, WorldState localWorldState, Link oldGuess)
    {
        String ConnType = "UNKNOWN";
        double throughput = 0;
        int packetLoss = 0;
        int latency = 0;

        if(oldGuess.getDescription().getType() == "UNKNOWN") {
            return firstGuess(n1, n2, localWorldState);
        }else{
            if (oldGuess.getDescription().getType() == "UNRECOGNIZED") {
                return firstGuess(n1, n2, localWorldState);
            }
            else {
                log.warn("Link was Unknown but Projection was not Unrecognized?");
            }
        }

        if(oldGuess.getDescription().getType() == "LAN") {
            Link newLink = firstGuess(n1, n2, localWorldState);

            if(newLink.getDescription().getType() == oldGuess.getDescription().getType()) {

                return Link.newBuilder(newLink).setDescription(Description.newBuilder().setType(calculateProjection(throughput, packetLoss, latency, oldGuess)).build()).build();
            }
            else {
                return newLink;
            }

        }
        return Link.newBuilder().setDescription(Description.newBuilder().setType(calculateProjection(throughput, packetLoss, latency, oldGuess)).build()).build();
    }
*/



}

