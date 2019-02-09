/*
 * GroupManager.java
 *
 * This file is part of the IHMC GroupManagers Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

package us.ihmc.aci.grpMgr;

import DiscoveryService.DiscoveryService;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.lang.String;

/**
 * GroupManager.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class GroupManager
{
    /**
     * Creates a new <code>GroupManager</code> instance.
     */
    public GroupManager ()
    {
        constructor();
    }

    /**
     * Native method that builds an object <code>GroupManager</code> (constructor equivalent).
     */
    private native void constructor ();

    /**
     * Native method to set the Logger to write on file.
     */
    public native int setLogger (String logFile);

    /**
     * Initializes a <code>GroupManager</code> instance.
     *
     * @param nodeUUID      nodeUUID the UUID of the node.
     * @param port          the port through which the <code>GroupManager</code> should connect.
     * @param ipAddress     the IP address of the interface through which the <code>GroupManager</code> should connect.
     * @param transportMode the desired transport layer mode for the connection (es: "UDP_BROADCAST")
     */
    public int init (String nodeUUID, int port, String ipAddress, TransportMode transportMode) throws IOException
    {
        int rc = initNative(nodeUUID, port, ipAddress, transportMode.toString(), 0);

        if (rc != 0) {
            String msgErr = "Unable to initializing GroupManager, result code is: " + rc;
            LOG.error(msgErr);
            throw new IOException(msgErr);
        }

        return rc;
    }

    /**
     * Initializes a <code>GroupManager</code> instance.
     *
     * @param nodeUUID      nodeUUID the UUID of the node.
     * @param port          the port through which the <code>GroupManager</code> should connect.
     * @param ipAddress     the IP address of the interface through which the <code>GroupManager</code> should connect.
     * @param transportMode the desired transport layer mode for the connection (es: "UDP_BROADCAST")
     */
    public int init (String nodeUUID, int port) throws IOException
    {
        int rc = initNativeNoIP(nodeUUID, port);

        if (rc != 0) {
            String msgErr = "Unable to initializing GroupManager, result code is: " + rc;
            LOG.error(msgErr);
            throw new IOException(msgErr);
        }

        return rc;
    }
    
    private native int initNativeNoIP (String nodeUUID, int port);

    public int init (String configFile)
    {
        int rc = initNativeConfig(configFile);

        if (rc != 0) {
            String msgErr = "Unable to initializing GroupManager, result code is: " + rc;
            LOG.error(msgErr);
//            throw new IOException(msgErr);
        }

        return rc;
    }

    /**
     * Gets the <code>PeerInfo</code> of the specified node given its <code>nodeUUID</code>.
     *
     * @param nodeUUID the UUID of the node.
     * @return the peer group manager info.
     */
    public PeerInfo getPeerInfo (String nodeUUID)
    {
        if (nodeUUID == null)
            throw new NullPointerException("nodeUUID can't be null");

        PeerInfo peerInfo = new PeerInfo(nodeUUID, null);
        return getPeerInfoNative(nodeUUID, peerInfo);
    }

    /**
     * Get the <code>PeerInfo</code> of the specified node given its <code>nodeUUID</code>.
     *
     * @param nodeUUID the UUID of the node.
     * @return the peer group manager info or null if no peer info has been found.
     */
    public String getPeerIPAddress (String nodeUUID)
    {
        if (nodeUUID == null) {
            throw new NullPointerException("nodeUUID can't be null");
        }

        long ip = getPeerIPAddressNative(nodeUUID);

        if (ip < 0) {
            LOG.warn("Failed to retrieve ip - rc: " + ip);
            return null;
        }
        LOG.trace("Found IP. long value: " + ip);

        return Util.intToIp((int) ip);
    }

    public String getActivePeerIPAddress ()
    {
        long ip = getActivePeerIPAddressNative();

        if (ip < 0) {
            LOG.warn("Failed to retrieve ip - rc: " + ip);
            return null;
        }
        LOG.trace("Found IP. long value: " + ip);

        return Util.intToIp((int) ip);
    }

    /**
     * Sets a the <code>GroupManagerListener</code> for this <code>GroupManager</code> in order to receive event
     * notifications (callback API).
     *
     * @param listener
     */
    public void setGroupManagerListener (GroupManagerListener listener)
    {
        if (listener == null) {
            return;
        }

        setGroupManagerListenerNative(listener);
    }

    /**
     * Starts the GroupManager main thread.
     */
    public native void start ();

    /**
     * Gets the group <code>nodeUUID</code>.
     *
     * @return a String that represents the group <code>nodeUUID</code>.
     */
    public native String getNodeUUID ();

    /**
     * Checks whether the peer specified by <code>nodeUUID</code> is alive or not.
     *
     * @param nodeUUID nodeUUID of the peer.
     * @return <code>true</code> if the peer is alive.
     */
    public native boolean isPeerAlive (String nodeUUID);

    /**
     * Sets the name for this Node.
     *
     * @param nodeName the name of the Node.
     * @return 0 if the set was successful.
     */
    public native int setNodeName (String nodeName);

    /**
     * Sets the interval of time (in ms) between transmission of ping packets.
     *
     * @param pingInterval the interval between packets.
     * @return 0 if the set was successful.
     */
    public native int setPingInterval (int pingInterval);

    /**
     * Sets the interval of time (in ms) between transmission of info packets.
     *
     * @param infoInterval the interval between info packets.
     * @return 0 if the set was successful.
     */
    public native int setInfoBCastInterval (int infoInterval);

    /**
     * Sets the default number of hops that PING packets are propagated for.
     *
     * @param hopCount the number of hops that PING packets are propagated for.
     * @return 0 if the set was successful.
     */
    public native int setPingHopCount (int hopCount);

    /**
     * Get the name of the specified node given its UUID.
     *
     * @param nodeUUID the UUID of the node.
     * @return the name of the node.
     */
    public native String getPeerNodeName (String nodeUUID);

    /**
     * Creates a Public Managed Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     */
    public native int createPublicManagedGroup (String groupName);

    /**
     * Creates a Private Managed Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     * @param password  password to access this group.
     */
    public native int createPrivateManagedGroup (String groupName, String password);

    /**
     * Creates a Public Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     * @param data      application defined data byte array attached to the group - can be null.
     */
    public native int createPublicPeerGroup (String groupName, byte[] data);

    /**
     * Creates a Private Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     * @param password  password to access this group.
     * @param data      application defined data byte array attached to the group - can be null.
     */
    public native int createPrivatePeerGroup (String groupName, String password, byte[] data);

    /**
     * Creates a Public Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     * @param data      application defined data byte array attached to the group - can be null.
     * @param hopCount  hop count - limits the number of hops through which the packet is propagated (1-255).
     */
    //public native int createPublicPeerGroup (String groupName, byte[] data, int hopCount);

    /**
     * Creates a Public Peer Group.
     *
     * @param groupName name of the group - it cannot contain spaces and each group has to have a unique name.
     * @param data      application defined data byte array attached to the group - can be null.
     * @param hopCount  hop count - limits the number of hops through which the packet is propagated (1-255).
     * @param floodProb flood probability - controls the willingness of a node to retransmit the packet (0-100).
     */
    //public native int createPublicPeerGroup (String groupName, byte[] data, int hopCount, int floodProb);

    /**
     * Changes the data associated with the peer group identified by <code>groupName</code>.
     *
     * @param groupName name of the group for which the data should be updated - it cannot
     *                  contain spaces and each group has to have a unique name.
     * @param data      application defined data byte array.
     */
    public native int updatePeerGroupData (String groupName, byte[] data);

    /**
     * Deletes the specified group identified by <code>groupName</code>.
     *
     * @param groupName the name of the group to be deleted.
     */
    public native int removeGroup (String groupName);

    /**
     * Leaves the group identified by the group name.
     *
     * @param groupName name of the group.
     */
    public native int leaveGroup (String groupName);

    /**
     * Starts a peer search within the context of the specified group using the specified parameter.
     * <p/>
     * If the group name identifies a private group, the search request is encrypted using the password for the group).
     *
     * @param groupName   name of the group.
     * @param searchParam the application defined parameter.
     * @return an UUID that represents the search request.
     */
    public native String startPeerSearch (String groupName, byte[] searchParam);

    /**
     * Starts a peer search within the context of the specified group using the specified parameter.
     * <p/>
     * If the group name identifies a private group, the search request is encrypted using the password for the group).
     *
     * @param groupName   name of the group.
     * @param hopCount    specifies the number of hops (retransmissions) that must be performed for
     *                    the search request (from 1 to 255).
     * @param floodProb   probability (from 0 to 100) specifies the probability with which a node
     *                    that receives the search request will retransmit the request.
     * @param searchParam the application defined parameter.
     * @param timeToLive  the time to live of the peer search.
     * @return an UUID that represents the search request.
     */
    //public native String startPeerSearch (String groupName, byte[] searchParam, int hopCount, int floodProb,
    //                                      int timeToLive);

    /**
     * Starts a persistent peer search within the context of the specified group using the specified parameter.
     * <p/>
     * If the <code>groupName</code> identifies a private group, the search request is encrypted using the password
     * for the group).
     *
     * @param groupName   name of the group.
     * @param searchParam the application defined parameter.
     * @return an UUID that represents the search request.
     */
    public native String startPersistentPeerSearch (String groupName, byte[] searchParam);

    /**
     * Stops a persistent search with the specified <code>searchUUID</code>.
     *
     * @param searchUUID peer search UUID.
     */
    public native int stopPersistentPeerSearch (String searchUUID);

    /**
     * Responds to a peer search.
     *
     * @param searchUUID       nodeUUID of the peer search
     * @param searchReplyParam application defined parameter
     */
    public native int respondToPeerSearch (String searchUUID, byte[] searchReplyParam);

    /**
     * Sends a message to the destination node UUID.
     * <p/>
     * Message is encrypted using symmetrical cryptography (i.e. using the public key of the destination node).
     * NOTE: data is encrypted only if the public key of the destination node is known (see PROPAGATE_PUBLIC_KEY).
     *
     * @param nodeUUID nodeUUID of the peer.
     * @param message  the message to be sent.
     * @param reliable specifies whether the send should be reliable or not.
     * @param timeout  a default time to wait for the message ACK.
     */
    public native int sendPeerMessage (String nodeUUID, String message, boolean reliable, int timeout);

    /**
     * Broadcasts a message to the destination group.
     * <p/>
     * Message is encrypted using asymmetrical cryptography when broadcasting to private groups (key based on the
     * group password).
     *
     * @param groupName name of the group.
     * @param message   the message to be sent.
     */
    public native int broadcastPeerMessageToGroup (String groupName, String message);

    /**
     * Sends a byte array message to the destination node UUID.
     * <p/>
     * Message is encrypted using symmetrical cryptography (i.e. using the public key of the destination node).
     * NOTE: data is encrypted only if the public key of the destination node is known (see PROPAGATE_PUBLIC_KEY).
     *
     * @param nodeUUID nodeUUID of the peer.
     * @param message  the message to be sent.
     * @param reliable specifies whether the send should be reliable or not.
     * @param timeout  a default time to wait for the message ACK.
     */
    public native int sendPeerMessageAsBytes (String nodeUUID, byte[] message, boolean reliable, int timeout);

    /**
     * Broadcasts a byte array message to the destination group.
     * <p/>
     * Message is encrypted using asymmetrical cryptography when broadcasting to private groups (key based on the
     * group password).
     *
     * @param groupName name of the group.
     * @param message   the message to be sent.
     */
    public native int broadcastPeerMessageToGroupAsBytes (String groupName, byte[] message);

    /**
     * Checks whether the group manager thread is terminated
     *
     * @return true if the thread is terminated
     */
    public native boolean hasTerminated ();

    /**
     * Terminates the group manager thread
     */
    public native void terminate ();

    private native int initNative (String nodeUUID, int port, String ipAddress, String transportMode,
                                   int multicastGroup);

    private native int initNativeConfig (String configFile);

    private native void setGroupManagerListenerNative (GroupManagerListener listener);

    private native PeerInfo getPeerInfoNative (String nodeUUID, PeerInfo peerInfo);

    private native long getPeerIPAddressNative (String nodeUUID);

    private native long getActivePeerIPAddressNative ();

    /**
     * Changes the data associated with the peer
     *
     * @param data      application defined data byte array.
     */
    public native int updatePeerNodeData ();

    /**
     * Gets the <code>PeerInfo</code> of the specified node given its <code>nodeUUID</code>.
     *
     * @param nodeUUID the UUID of the node.
     * @return the peer group manager info.
     */
    public DiscoveryService getDiscoveryService ()
    {

        DiscoveryService discoveryService = new DiscoveryService();
        return getDiscoveryServiceNative(discoveryService);
    }

    private native DiscoveryService getDiscoveryServiceNative (DiscoveryService discoveryService);

    public native int setMulticastGroup(String multicastGroup);

    public native String getMulticastGroup();

    public native int setNodeTimeout(int timeout);

    public native int setNodeUnhealthy(int unhealthy);
    
    //TTL of 0 is only sent once. A value of -1 is an indefinite persistent peer search
    public native String startGroupsSearch(String[] groupNames, byte[] searchParam, int timeToLive);

    //////////////////////////
    private long _groupManager;
    private long _configManager;

    private final static Logger LOG = Logger.getLogger(GroupManager.class);

    static {
        System.loadLibrary("grpmgrjavawrapper");
    }
}