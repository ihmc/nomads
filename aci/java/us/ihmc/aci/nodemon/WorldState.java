package us.ihmc.aci.nodemon;

import com.google.protobuf.Duration;
import com.google.protobuf.GeneratedMessage;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.data.process.ProcessStats;

import java.util.Collection;
import java.util.Map;

/**
 * WorldState.java
 * <p>
 * Interface <code>WorldState</code> defines a public API to retrieve the complete status of all the nodes of
 * the network at any given time.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public interface WorldState
{
    /**
     * Initializes the WorldState of the current local <code>NODE</code>.
     */
    void init (NodeMon nodeMon, String localNodeId, String localNodeName, String mainIPAddress);

    /**
     * Gets the id of the current local <code>Node</code>.
     *
     * @return the id of the current local <code>Node</code>.
     */
    String getLocalNodeId ();

    /**
     * Gets the representation of the current local <code>Node</code>.
     *
     * @return an instance of the current local <code>Node</code>.
     */
    Node getLocalNode ();

    /**
     * Gets the representation of the current local <code>Node</code> as a copy.
     *
     * @return an instance of the current local <code>Node</code>.
     */
    Node getLocalNodeCopy ();

    /**
     * Sets the current local <code>Node</code> to the given object
     *
     * @param node the instance of the <code>Node</code> to set
     */
    void setLocalNode (Node node);

    /**
     * Returns a collection of the current nodes of the <code>WorldState</code>.
     *
     * @return a <code>Collection</code> of the current <code>Node</code> instances.
     */
    Collection<Node> getNodes ();

    /**
     * Returns a defensive copy of the <code>Collection</code> of the current nodes of the <code>WorldState</code>.
     *
     * @return a defensive copy of the <code>Map</code> of the current <code>Node</code> instances.
     */
    Map<String, Node> getNodesMapCopy ();

    /**
     * Returns a defensive copy of the <code>Collection</code> of the current nodes of the <code>WorldState</code>
     * ordered by IP address.
     *
     * @return a defensive copy of the <code>Map</code> of the current <code>Node</code> instances ordered by IP
     * address.
     */
    Map<String, Node> getNodesMapByIPCopy ();

    /**
     * Updates the given <code>Protos.*</code> object inside the collection with the assigned id.
     * Updates are handled by the internal <code>WorldStateController</code>.
     *
     * @param nodeId the id of the <code>Node</code> to add
     * @param c     the <code>Container</code> of the data extending <code>GeneratedMessage</code>
     */
    void updateData (String nodeId, Container c);

    /**
     * Update the clients with the type of data specified by <code>NodeMonDataType</code>
     *
     * @param nodeId the id of the node
     * @param c    the <code>Container</code> of the data extending <code>GeneratedMessage</code>
     */
    void updateClients (String nodeId, Container c);


    /**
     * Retrieves the <code>Node</code> with the specified id.
     *
     * @param nodeId the id of the <code>Node</code> to retrieve
     * @return the retrieved <code>Node</code>, or null if not present
     */
    Node getNode (String nodeId);

    /**
     * Retrieves the <code>Node</code> with the specified ip address as its main interface.
     *
     * @param ipAddress the ipAddress of the <code>Node</code> to retrieve
     * @return the retrieved <code>Node</code>, or null if not present
     */
    Node getNodeByIP (String ipAddress);


    /**
     * Gets the process statistics, instances of  <code>Process</code> that have been sensed on this host
     * (fetched from various sensors).
     *
     * @return an instance of <code>TrafficStats</code>
     */
    ProcessStats getProcessStats ();

    /**
     * Gets a copy process statistics, instances of  <code>Process</code> that have been sensed on this host
     * (fetched from various sensors).
     *
     * @return an instance of <code>TrafficStats</code>
     */
    ProcessStats getProcessStatsCopy ();

    /**
     * Gets an integer representing the observed traffic between node IP source and node IP dest.
     * This is the traffic that a third node C has observed between network interface with IP source
     * and the network interface with IP dest.
     * Specifying a source and "*" as a destination will provide the sum of all the observed traffic
     * by the observer node, for the specified Source. Multicast traffic will be excluded from the sum.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param sourceIpAddress   the ip address of network interface node A
     * @param destIpAddress     the ip address of network interface node B
     * @param duration          specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getObservedTraffic (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration duration);

    /**
     * Gets an integer representing the outgoing multicast traffic from the node IP
     * This is the traffic that the node with network interface with IP has sent in the last <code>Duration</code>
     * seconds.
     *
     * @param observerIpAddress    the ip of the NodeMon node that is the observer of the traffic
     * @param sourceIpAddress      the ip address of network interface source on the node
     * @param destMulticastAddress the multicast address (does sum of all multicast if 0.0.0.0)
     * @param duration             specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getOutgoingMcastTraffic (String observerIpAddress, String sourceIpAddress, String destMulticastAddress,
                                 Duration duration);

    /**
     * Gets an integer representing the incoming multicast traffic to the node IP
     * This is the traffic that the node with network interface with IP has received in the last <code>Duration</code>
     * seconds.
     *
     * @param observerIpAddress    the ip of the NodeMon node that is the observer of the traffic
     * @param destMulticastAddress the multicast address (does sum of all multicast if 0.0.0.0)
     * @param sourceIpAddress      the ip address of network interface source on the node
     * @param duration             specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getIncomingMcastTraffic (String observerIpAddress, String destMulticastAddress, String sourceIpAddress,
                                 Duration duration);

    /**
     * Gets an integer representing the observed multicast traffic between node IP source and node IP dest.
     * This is the multicast traffic that a third node observer has observed between network interface with IP source
     * and the network interface with IP dest. Specifying "0.0.0.0" as a source and "*" as a destination will provide
     * the sum of all the multicast traffic out of the observer node.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param sourceIpAddress   the ip address of network interface node source
     * @param destIpAddress     the ip address of network interface node dest
     * @param duration          specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getObservedMcastTraffic (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration
            duration);

    /**
     * Gets an integer representing the outgoing traffic between node IP source and node IP dest.
     * This is the traffic that the node with network interface with IP source has sent to the network interface with
     * IP dest. Specifying "0.0.0.0" as a source and "*" as a destination will provide the sum of all the outgoing
     * traffic out of the observer node. Multicast traffic will be excluded from the sum.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param sourceIpAddress   the ip address of network interface node source
     * @param destIpAddress     the ip address of network interface node dest
     * @param duration          specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getOutgoingTraffic (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration duration);

    /**
     * Gets a integer representing the incoming traffic between node IP dest and node IP source.
     * This is the traffic that the node with network interface with IP dest has received from network interface with
     * IP source. Specifying "0.0.0.0" as a destination and "*" as a source will provide the sum of all the incoming
     * traffic going into the the observer node.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param destIpAddress     the ip address of network interface node dest
     * @param sourceIpAddress   the ip address of network interface node source
     * @param duration          specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an integer representing the traffic, 0 if not present
     */
    int getIncomingTraffic (String observerIpAddress, String destIpAddress, String sourceIpAddress, Duration duration);

    /**
     * Sets the <code>NetworkHealth</code> summary of the subnetwork with the specified networkName;
     *
     * @param networkName   the name through which the interested subnetwork is identified
     * @param networkHealth the network health message
     */
    void setNetworkHealth (String networkName, NetworkHealth networkHealth);

    /**
     * Gets the <code>NetworkHealth</code> summary associated with the specified networkName;
     *
     * @param networkName the name through which the interested subnetwork is identified
     * @return the network health message (if present for the specified subnetwork), null otherwise
     */
    NetworkHealth getNetworkHealth (String networkName);

    /**
     * Gets the latency of this link, calculated starting from round trip time (RTT) between node with network interface
     * specified by sourceIpAddress and node with network interface specified by destIpAddress.
     * The retrieved value depends on the implementation, at the moment it is retrieved using the aggregated statistics
     * produced by a Mockets connection.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param sourceIpAddress   the ip address of network interface node source
     * @param destIpAddress     the ip address of network interface node dest
     * @param duration          specify the interval of time that should be considered valid, returns 0 otherwise
     * @return an int representing the latency, or 0 if not present.
     */
    int getLatency (String observerIpAddress, String sourceIpAddress, String destIpAddress, Duration duration);

    /**
     * Attempts to update the Link by merging it into the WorldState.
     * The Link is specified by the observerIpAddress and the Link that has to be modified.
     *
     * @param observerIpAddress the ip of the NodeMon node that is the observer of the traffic
     * @param link              the link to update
     * @return an int representing the latency, or 0 if not present.
     */
    void updateLink (String observerIpAddress, Link link);
}