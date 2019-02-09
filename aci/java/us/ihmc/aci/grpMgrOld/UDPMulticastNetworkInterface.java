package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;

import us.ihmc.net.NICInfo;
import us.ihmc.net.NetUtils;

/**
 *
 * @author Matteo Rebeschini
 */
public class UDPMulticastNetworkInterface extends UDPBroadcastNetworkInterface
{

    public UDPMulticastNetworkInterface()
    {   
        try {
            _multicastGroup = InetAddress.getByName (MULTICAST_GROUP);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     *
     * @param nal
     * @throws NetworkException
     */ 
    protected void init (NICInfo nicInfo, NetworkAccessLayer nal)
        throws NetworkException
    {
        _nal = nal;
        _nicInfo = nicInfo;

        try {
            _udpServer.init (_nal._port, this, _nicInfo.ip);
            _udpServer.joinMulticastGroup (_multicastGroup);
            _udpServer.setMulticastTTL (1);
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new NetworkException ("failed initializing UDP server on interface: " + 
                                        this + "; nested exception = " + e.getMessage());
        }

        // Initialize the UDP broadcast server
        //
        // When binding to a specific address, the UDP socket is not receiving broadcast
        // packets any more. This is the defacto behavior for UDP sockets ever. Win32 takes
        // a stance that diverges from the standard, as usual, allowing the reception of
        // broadcasts anyways.
        //
        // To receive broadcasts in Linux (and UNIX) systems we have to bind and additional
        // UDP socket to the wildcard address.
        //
        // NOTE: the UDP server is needed when the xlayer is in use for receiving unicast
        // UDP messages (e.g. peer search replies).

        // UDP Server used for receiving UDP broadcasts
        if (!_isWindows) {
            try {
                _udpBroadcastServer.init (_nal._port, this);
            }
            catch (NetworkException ne) {
                throw new NetworkException ("failed initializing UDP broadcast server");
            }
        }
    }

    protected void setMulticastGroup (InetAddress multicastGroup)
        throws NetworkException
    {
        if (multicastGroup != null) {
            if (NetUtils.isMulticastAddress (multicastGroup)) {
                _multicastGroup = multicastGroup;
            }
            else {
                throw new NetworkException ("invalid multicast group: " + multicastGroup.getHostAddress());
            }
        }
    }
    
    /**
     *
     * @param destAddr
     * @param packet
     * @param packetLen
     * @throws NetworkException
     */ 
    protected void broadcastPacket (byte[] packet, int packetLen, int hopCount)
        throws NetworkException
    {
        if (_udpServer.getMulticastTTL() != hopCount) {
            _udpServer.setMulticastTTL (hopCount);
        }

        try {
            _udpServer.sendPacket (_multicastGroup, packet, packetLen); 
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed broadcasting packet on interface: " + this + 
                                        "; nested exception = " + ne);
        }
    }
    
    public String toString()
    {
        if (_nicInfo != null) {
            return _nicInfo.toString() + " [mode: UDP_MULTICAST] [group: " + 
                   _multicastGroup.getHostAddress() + "]";
        }
        else {
            return "UDP_MULTICAST - group: " + _multicastGroup.getHostAddress();
        }
    }

    // 
    protected InetAddress _multicastGroup;
    public static final String MULTICAST_GROUP = "225.0.0.1";
}

