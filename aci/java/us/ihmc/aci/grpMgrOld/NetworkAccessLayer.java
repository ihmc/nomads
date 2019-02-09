package us.ihmc.aci.grpMgrOld;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.KeyPair;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Random;
import java.util.Vector;
import java.util.logging.Logger;

import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.net.NetUtils;
import us.ihmc.net.NICInfo;
import us.ihmc.util.LogHelper;


/**
 * Network Access Layer
 * <p>
 * This class provides the network communication substrate to
 * the Group Manager.
 * <p>
 * Created on October 04, 2006
 *
 * @author Matteo Rebeschini
 * @version $Revision$
 * $Date$
 */
public class NetworkAccessLayer extends Thread
{
    /**
     * Class Constructor
     */
    public NetworkAccessLayer()
    {
        _rand = new Random (System.currentTimeMillis());
        _logger = LogHelper.getLogger ("us.ihmc.aci.grpMgr.NetworkAccessLayer");
        _packetSeqInfos = new Hashtable();
        _packetsToAck = new Hashtable();
    }

    /**
     * Initialize the NetworkAccessLayer
     * <p>
     * netIFsConf is a hashtable of NetworkInterfaceConf objects specifing 
     * the subset of all the available network interfaces that will be
     * used by the NetworkAccessLayer. 
     * <p>
     * If netIFConfs is null, all the available network interfaces will be used, and
     * UDP Multicast will be used as the default advertising mode
     * 
     * @param nodeUUID
     * @param port
     * @param publicKey
     * @param netIFConfs Vector<NetworkInterfaceConf>
     * @param grpMgr
     * @throws NetworkException
     */
    public void init (String nodeUUID, int port, Vector netIFConfs, GroupManager grpMgr)
        throws NetworkException
    {
        _nodeUUID = nodeUUID;
        _port = port;
        _grpMgr = grpMgr;
        _netIFs = new Vector(); // Vector<NetworkInterface>

        if (netIFConfs == null) { // use all the available interfaces (unique networks)
            _nicsInfo = NetUtils.getNICsInfo (false, true);
            for (int i = 0;  i < _nicsInfo.size(); i++) {
                String advMode = System.getProperty ("advMode");
                NetworkInterface netIF;
                if (advMode != null) {
                    if (advMode.equalsIgnoreCase ("UDP_BROADCAST")) {
                        netIF = new UDPBroadcastNetworkInterface();
                    }
                    else if (advMode.equalsIgnoreCase ("UDP_MULTICAST")) {
                        netIF = new UDPMulticastNetworkInterface();
                    }
                    else if (advMode.equalsIgnoreCase ("UDP_MULTICAST_NORELAY")) {
                        netIF = new UDPMulticastNoRelayNetworkInterface();
                    }
                    else if (advMode.equalsIgnoreCase ("CROSS_LAYER")) {
                        netIF = new CrossLayerNetworkInterface();
                    }
                    else {
                        throw new NetworkException ("invalid advertisement mode: " + advMode);
                    }
                }
                else {
                    // Use UDP Multicast No Relay as the default advetisement mode
                    netIF = new UDPMulticastNoRelayNetworkInterface();
                }
                _netIFs.addElement (netIF);
            }
        }
        else {
            _nicsInfo = new Vector(); // Vector<NICInfo>
            Vector nicsInfo = NetUtils.getNICsInfo (false, false); // Vector<NICInfo>
            for (int i = 0;  i < nicsInfo.size(); i++) {
                for (int j = 0; j < netIFConfs.size(); j++) {
                    if (NetUtils.isLocalAddress (((NetworkInterfaceConf) netIFConfs.elementAt (j)).ip)) {
                        if (((NICInfo) nicsInfo.elementAt (i)).ip.equals (((NetworkInterfaceConf) netIFConfs.elementAt (j)).ip)) {
                            NetworkInterfaceConf nicConf = (NetworkInterfaceConf) netIFConfs.elementAt (j);
                            NetworkInterface netIF = null;
                            NICInfo nicInfo = NetUtils.getNICInfo (nicConf.ip);
                            if (nicInfo != null) {
                                if (nicConf.advMode.equalsIgnoreCase ("UDP_BROADCAST")) {
                                    netIF = new UDPBroadcastNetworkInterface();
                                }
                                else if (nicConf.advMode.equalsIgnoreCase ("UDP_MULTICAST")) {
                                    netIF = new UDPMulticastNetworkInterface();
                                    ((UDPMulticastNetworkInterface) netIF).setMulticastGroup (nicConf.multicastGroup);
                                }
                                else if (nicConf.advMode.equalsIgnoreCase ("UDP_MULTICAST_NORELAY")) {
                                    netIF = new UDPMulticastNoRelayNetworkInterface();
                                    ((UDPMulticastNoRelayNetworkInterface) netIF).setMulticastGroup (nicConf.multicastGroup);
                                }
                                else if (nicConf.advMode.equalsIgnoreCase ("CROSS_LAYER")) {
                                    netIF = new CrossLayerNetworkInterface();
                                }
                                else {
                                    // Invalid advertisement mode
                                    throw new NetworkException ("invalid advertisement mode: " + nicConf.advMode);
                                }
                                _netIFs.addElement (netIF);
                                _nicsInfo.addElement (nicInfo);
                            }
                            else {
                                throw new NetworkException ("error getting information about network interface: " + 
                                    (NetworkInterfaceConf) netIFConfs.elementAt (j));
                            }
                        }
                    }
                    else {
                        // Invalid network interface
                        throw new NetworkException ("invalid network interface: " + (NetworkInterfaceConf) netIFConfs.elementAt (j));
                    }
                }
            }
        }

        if (_netIFs.size() == 0) {
            throw new NetworkException ("no network interface detected");
        }
        else {
            // Initialize the Network Interfaces
            for (int i = 0;  i < _netIFs.size(); i++) {
                NetworkInterface netIF = (NetworkInterface)_netIFs.elementAt (i);
                _logger.info ("Initializing interface: " +  (NICInfo) _nicsInfo.elementAt (i) + " [" + netIF + "]");
                netIF.init ((NICInfo) _nicsInfo.elementAt (i), this);
                
            }

            // Add the virtual network interface used for data relaying
            TCPNetworkInterface tcpNIC = new TCPNetworkInterface();
            tcpNIC.init (null, this);
            _netIFs.addElement (tcpNIC);
            _logger.info ("Initializing virtual TCP interface");
        }
    }

    /**
     * run() method of the thread. 
     * <p>
     * It periodically checks the packetToAck hashtable. This hashtable contains
     * the sent packet that have still to be acknoledged. These packets are resent
     * until the ACK is received (or if UNICAST_RELIABLE_RETRANSMISSION_TIMEOUT 
     * elapses).
     */
    public void run()
    {
        if (_running) {
            return;
        }

        _running = true;

        for (int i = 0;  i < _netIFs.size(); i++) {
            ((NetworkInterface)_netIFs.elementAt (i)).start();
        }

        while (_running) {
            try {
                Thread.sleep (UNICAST_RELIABLE_RETRANSMISSION_TIMEOUT);
            }
            catch (InterruptedException ioe) {
                ioe.printStackTrace();
            }

            synchronized (this)
            {
                Enumeration keys = _packetsToAck.keys();
                while (keys.hasMoreElements()) {
                    String packetID = (String) keys.nextElement();
                    PacketToAck packetToAck = (PacketToAck) _packetsToAck.get (packetID);
                    long currentTime = System.currentTimeMillis();
                    if ((packetToAck.ttl = (int) (currentTime - packetToAck.timestamp)) > 0) {
                        packetToAck.timestamp = currentTime;
                        //re-send packet
                        try {
                            sendPacket (packetToAck.destAddr, packetToAck.packet, packetToAck.packetLen);
                        }
                        catch (NetworkException ne) {
                            ne.printStackTrace();
                        }
                    }
                    else {
                        // give up, and remove the packet from the cache
                        _packetsToAck.remove (packetID);
                        // TODO: should we notify the group manager about this?
                    }
                }
            }
        }
    }

    /**
     * Return the running state of this NetworkAccessLayer thread.
     *
     * @return <code>true</code> if thread is running
     */
    public boolean isRunning()
    {
        return _running;
    }

    /**
     * Terminate the thread.
     */
    public void terminate()
    {
        _running = false;

        // Terminate TCP and UDP servers
        for (int i = 0;  i < _netIFs.size(); i++) {
            ((NetworkInterface) _netIFs.elementAt (i)).terminate();
        }
    }

    /**
     * Send a unicast message to destAddr
     *
     * @param message
     * @param messageLen
     * @param destAddr
     * @param messageType
     * @param maxAckWait
     * @param includeNetIFs
     * @throws NetworkException
     */
    public void sendMessage (byte[] message, int messageLen, InetAddress destAddr,
                             int messageType, int maxAckWait, boolean includeNetIFs)
        throws NetworkException
    {
        byte[] packet = new byte[PacketCodec.MAX_PACKET_SIZE];
        int packetType = (maxAckWait != 0) ? PacketCodec.UNICAST_RELIABLE_PACKET
                                             : PacketCodec.UNICAST_UNRELIABLE_PACKET;
        int packetLen = buildPacket (packet, packetType, messageType, 0, 0,
                                     message, messageLen, includeNetIFs);

        if (packetType == PacketCodec.UNICAST_RELIABLE_PACKET) {
            //cache the packet
            PacketToAck packetToAck = new PacketToAck();
            packetToAck.timestamp = System.currentTimeMillis();
            packetToAck.packet = (byte[]) packet.clone();
            packetToAck.packetLen = packetLen;
            packetToAck.destAddr = destAddr;
            packetToAck.ttl = (maxAckWait > 0) ? maxAckWait : MAX_UNICAST_RELIABLE_PACKET_TTL;
            _packetsToAck.put (_sequenceNum + "", packetToAck);
        }

        sendPacket (destAddr, packet, packetLen);
    }

    /**
     * Send a unicast message to a remote node. 
     * <p>
     * The destNodeNetIFs vector contains information about the active network 
     * interfaces of the remote node. The message will be sent to an IP address 
     * of the remote node that is reachable from this node.
     *
     * @param message
     * @param messageLen
     * @param destNodeNetIFs
     * @param messageType
     * @param maxAckWait
     * @param includeNetIFs
     * @throws NetworkException
     */
    public void sendMessage (byte[] message, int messageLen, Vector destNodeNetIFs,
                             int messageType, int maxAckWait, boolean includeNetIFs)
        throws NetworkException
    {
        InetAddress destAddr = NetUtils.determineDestIPAddr (destNodeNetIFs, _nicsInfo);
        if (destAddr == null) {
            throw new NetworkException ("node is not reachable");
        }
        sendMessage (message, messageLen, destAddr, messageType, maxAckWait, includeNetIFs);
    }

    /**
     * Broadcast a message.
     *
     * @param message
     * @param messageLen
     * @param messageType
     * @param hopCount
     * @param floodProb
     * @param includeNetIFs
     * @throws NetworkException
     */
    public void broadcastMessage (byte[] message, int messageLen, int messageType,
                                  int hopCount, int floodProb, boolean includeNetIFs)
        throws NetworkException
    {
        byte[] packet = new byte[PacketCodec.MAX_PACKET_SIZE];
        int packetLen = buildPacket (packet, PacketCodec.BROADCAST_PACKET, messageType, hopCount,
                                     floodProb, message, messageLen, includeNetIFs);

        // Store the packet information to avoid reprocessing if the same packet is received
        String packetID = _nodeUUID + messageType;
        PacketSeqInfo psi = new PacketSeqInfo();
        psi.sequenceNum = _sequenceNum;
        psi.timestamp = System.currentTimeMillis();
        // synchronizing this would create a dead lock - this method has to be called
        // from a synchronized method
        _packetSeqInfos.put (packetID, psi);
        broadcastPacket (packet, packetLen, hopCount, null);
    }

    /**
     * This method is called by the UDP server when a new packet is received.
     *
     * @param remoteAddr
     * @param packet
     * @param packetLen
     * @param callBackHandle
     * @return
     */
    public int receivedPacket (InetAddress remoteAddr, byte[] packet, int packetLen, NetworkInterface nic)
    {
        return handlePacket (packet, packetLen, nic);
    }

    /////////// Protected Methods ///////////

    /**
     * Return the network interfaces that are currently used
     * by the NetworkAccessLayer. This can be a subset of all the
     * available interfaces.
     *
     * @return Vector<NICInfo>
     */
    protected Vector getActiveNICsInfo()
    {
        return _nicsInfo;
    }

    /**
     * 
     * @param relayNodes - Vector<InetSocketAddress>
     */
    /**
     * 
     * @param relayNodes - Vector<InetSocketAddress>
     */
    protected void setRelayNodes (Vector relayNodes)
    {
    	Vector remoteRelayNodes = new Vector(); //Vector<InetSocketAddress>
    	Enumeration e = relayNodes.elements();
    	while (e.hasMoreElements()) {
    		InetSocketAddress isa = (InetSocketAddress) e.nextElement();
    		if (!NetUtils.isReachableDirectly(isa.getAddress(), _nicsInfo)) {
    			remoteRelayNodes.add (isa);
    		}
    		else {
    			_logger.warning ("Relaying node: " + isa.getAddress().getHostAddress() + 
    					         " is on our network: no need to relay packets");
    		}
    	}
    	
    	for (int i = 0;  i < _netIFs.size(); i++) {
            NetworkInterface netIF = (NetworkInterface)_netIFs.elementAt (i);
            if (netIF instanceof TCPNetworkInterface) {
            	TCPNetworkInterface tcpNetIF = (TCPNetworkInterface) netIF;
            	tcpNetIF.setRelayNodes (remoteRelayNodes);
            	
            	String relayNodesStr = "";
            	Enumeration e2 = remoteRelayNodes.elements();
            	while (e2.hasMoreElements()) {
            		InetSocketAddress isa = (InetSocketAddress) e2.nextElement();
            		relayNodesStr += isa.getAddress().getHostAddress();
            		if (isa.getPort() != 0) {
            			relayNodesStr += ":" + isa.getPort();
            		}
            		relayNodesStr += " ";
            	}
            	_logger.info ("Adding relay nodes: " + relayNodesStr);
            	
            	break;
            }
    	}
    }

    
    /////////// Private Methods ///////////
    
    /**
     * Construct a packet, creating the header and appending the message
     * as payload.
     *
     * @return packet length
     */
    private int buildPacket (byte[] packet, int packetType, int msgType,
                             int hopCount, int floodProb, byte[] message,
                             int messageLen, boolean includeNetIFs)
    {
        Vector networkIFs = null; // Vector<NICInfo>
        _sequenceNum = ((_sequenceNum+1) < MAX_SEQUENCE_NUM) ? (_sequenceNum+1) : 0;
        if (includeNetIFs) {
            networkIFs = _nicsInfo;
        }
        return PacketCodec.createPacket (packet, packetType, _sequenceNum, msgType, _nodeUUID,
                                         hopCount, floodProb, message, messageLen, networkIFs);
    }

    /**
     * Handle a received packet.
     * <p>
     * Four type of packets are currently supported:
     * <ul>
     * <li>BROADCAST_PACKET</li>
     * <li>UNICAST_UNRELIABLE_PACKET</li>
     * <li>UNICAST_RELIABLE_PACKET</li>
     * <li>ACK_PACKET</li>
     * </ul>
     *
     * @param packet
     * @param packetLen
     * @return
     */
    private synchronized int handlePacket (byte[] packet, int packetLen, NetworkInterface nic)
    {
        if (PacketCodec.isPacketValid (packet, packetLen)) {
            String senderUUID = PacketCodec.getNodeUUID (packet);
            int packetType = PacketCodec.getPacketType (packet);
            int messageType = PacketCodec.getMessageType (packet);
            int sequenceNum = PacketCodec.getSequenceNumber (packet);

            if (packetType == PacketCodec.BROADCAST_PACKET) {
                PacketSeqInfo psi;
                String packetID = senderUUID + messageType;
                if (null != (psi = (PacketSeqInfo) _packetSeqInfos.get (packetID))) {
                    if ((sequenceNum <= psi.sequenceNum) && (sequenceNum > (psi.sequenceNum - MAX_SEQUENCE_NUM/2))) {
                        // we have already received this packet: discard it
                        return 0;
                    }
                }
                else {
                    psi = new PacketSeqInfo();
                }

                // this is the first time we receive this packet: store its information
                psi.sequenceNum = sequenceNum;
                psi.timestamp = System.currentTimeMillis();
                _packetSeqInfos.put (packetID, psi);
            }
            else if (packetType == PacketCodec.ACK_PACKET) {
                // remove the ACK_PACKET from the cache
                _packetsToAck.remove (sequenceNum + "");
                return 0;
            }

            // Process the packet
            int hopCount = PacketCodec.getHopCount (packet);
            int floodProb = PacketCodec.getFloodProbability (packet);
            Vector nicsInfo = PacketCodec.getNICsInfo (packet);

            if (! senderUUID.equals (_nodeUUID)) {
                _grpMgr.handleMessage (senderUUID, messageType, hopCount, floodProb,
                                       nicsInfo, PacketCodec.getMessage (packet, packetLen),
                                       packetLen - PacketCodec.getHeaderSize (packet));

                // Rebroadcast the packet if needed
                if ((packetType == PacketCodec.BROADCAST_PACKET) && (hopCount > 1) && (floodProb > _rand.nextInt (100))) {
                    PacketCodec.setHopCount (packet, --hopCount);
                    if ((nic instanceof UDPBroadcastNetworkInterface) || (nic instanceof UDPMulticastNetworkInterface)) {
                        // rebroadcast the packet in all the interfaces
                        try {
                            broadcastPacket (packet, packetLen, hopCount, null);
                        }
                        catch (NetworkException ne) {
                            ne.printStackTrace();
                        }
                    }
                    else if ((nic instanceof UDPMulticastNoRelayNetworkInterface) || 
                    		 (nic instanceof CrossLayerNetworkInterface) || 
                    		 (nic instanceof TCPNetworkInterface)) {
                        // relay the packet in the interfaces different than the one the packet came from 
                        try {
                            broadcastPacket (packet, packetLen, hopCount, nic);
                        }
                        catch (NetworkException ne) {
                            ne.printStackTrace();
                        }
                    }
                }
                // Send an ACK_PACKET if needed
                else if (packetType == PacketCodec.UNICAST_RELIABLE_PACKET && (nicsInfo != null)) { // I can't send the ACK if I don't know the IP.
                    // create the ACK_PACKET
                    byte[] ackPacket = new byte[PacketCodec.MAX_PACKET_SIZE];
                    int ackPacketLen = PacketCodec.createPacket (ackPacket, PacketCodec.ACK_PACKET, sequenceNum, messageType,
                                                                 _nodeUUID, 0, 0, null, 0,  null);

                    // send the ACK packet
                    InetAddress destAddr = NetUtils.determineDestIPAddr (nicsInfo, _nicsInfo);
                    try {
                        sendPacket (destAddr, ackPacket, ackPacketLen);
                    }
                    catch (NetworkException ne) {
                        ne.printStackTrace();
                    }
                }
            }
        }
        else {
            // Invalid packet received
            return -1;
        }

        return 0;
    }

    /**
     * Broadcast a packet
     * <p>
     * The packet is broadcasted through all the active network interfaces.
     * <p>
     * In order to multiplex the packet over all the active interfaces,
     * the packet is sent once per interface, using each time the interface broadcast
     * address (e.g. 192.168.0.255).
     *
     * In the case of packets relaying between different networks, nic specifies the 
     * NetworkInterface from which the packet originally came from. The packet is 
     * re-broadcasted on the network interfaces different than nic (if any)
     *
     * @param packet
     * @param packetLen
     * @param hopCount
     * @param nic
     * @throws NetworkException
     */
    private void broadcastPacket (byte[] packet, int packetLen, int hopCount, NetworkInterface nic)
        throws NetworkException
    {
        for (int i = 0;  i < _netIFs.size(); i++) {
            if (nic == null || (nic != ((NetworkInterface) _netIFs.elementAt (i)))) {
                ((NetworkInterface) _netIFs.elementAt (i)).broadcastPacket (packet, packetLen, hopCount);
            }
        }
    }

    /**
     * Unicast a packet.
     *
     * @param destAddr
     * @param packet
     * @param packetLen
     * @throws NetworkException
     */
    private void sendPacket (InetAddress destAddr, byte[] packet, int packetLen)
        throws NetworkException
    {
        // the OS will route the packet throught the correct interface
        ((NetworkInterface) _netIFs.elementAt (0)).sendPacket (destAddr, packet, packetLen);
    }

    /////////// Inner Classes ///////////

    /**
     * Class used for storing information about the received
     * packets. Sequence numbers are used in the packets header 
     * in order to avoid relaying/processing the same packet more
     * than once.
     */
    private class PacketSeqInfo
    {
        public int sequenceNum;
        public long timestamp;
    }

    /**
     * Class used for caching UNICAST_RELIABLE_PACKETS. These
     * packets are cached until their ACK is received or 
     * UNICAST_RELIABLE_RETRANSMISSION_TIMEOUT elapses.
     */
    private class PacketToAck
    {
        public long timestamp;
        public byte[] packet;
        public int packetLen;
        public InetAddress destAddr;
        public int ttl;
    }
   

    // Instance Variables
    private Vector _netIFs; // Vector<NetworkInterface>
    private Vector _nicsInfo;
    private int _sequenceNum = (int) ((System.currentTimeMillis() / 1000) % MAX_SEQUENCE_NUM);
    private String _nodeUUID;
    protected int _port;
    private boolean _running = false;
    private GroupManager _grpMgr;
    private Hashtable _packetSeqInfos; // Hashtable<String, PacketSeqInfo> - keys are NodeUUIDs+PacketSequenceNum
    private Hashtable _packetsToAck; // Hashtable<String, PacketToAck> - keys are NodeUUIDs+PacketSequenceNum
    private Random _rand;
    private Logger _logger;
    private static final int MAX_SEQUENCE_NUM = 65535; // 2^16
    private static final int MAX_UNICAST_RELIABLE_PACKET_TTL = 10000; // this has to be the same of the dead peer timeout
    private static final int UNICAST_RELIABLE_RETRANSMISSION_TIMEOUT = 1000;
}

