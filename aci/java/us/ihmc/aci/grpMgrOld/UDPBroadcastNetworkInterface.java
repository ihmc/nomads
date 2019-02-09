package us.ihmc.aci.grpMgrOld;

import java.net.InetAddress;

import us.ihmc.net.NICInfo;

/**
 *
 * @author Matteo Rebeschini
 */
public class UDPBroadcastNetworkInterface extends NetworkInterface
{
    public UDPBroadcastNetworkInterface()
    {
        _udpServer = new UDPServer();
        _udpBroadcastServer = new UDPServer();
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
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed initializing UDP server on interface: " + 
                                        this + "; nested exception = " + ne);
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
    
    /**
     *
     */ 
    public void run()
    {
        _udpServer.start();

        if (!_isWindows) {
            _udpBroadcastServer.start();
        }

        while (true) {
            try {
                Thread.sleep (1000);
            }
            catch (InterruptedException ie) {
                ie.printStackTrace();
            }
        }
    }
    
    /**
     *
     */ 
    protected void terminate()
    {
       _udpServer.terminate(); 

        if (!_isWindows) {
            _udpBroadcastServer.terminate();
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
        try {
            _udpServer.sendPacket (_nicInfo.broadcast, packet, packetLen); 
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed broadcasting packet on interface: " + this + 
                                        "; nested exception = " + ne);
        }
    }
    
    /**
     * 
     * @param destAddr
     * @param packet
     * @param packetLen
     * @throws NetworkException
     */  
    protected void sendPacket (InetAddress destAddr, byte[] packet, int packetLen)
        throws NetworkException
    {    
        try {
            _udpServer.sendPacket (destAddr, packet, packetLen);
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed sending packet on interface: " + this + 
                                        "; nested exception = " + ne);
        }
    }
    
    /**
     * 
     * @param remoteAddr
     * @param packet
     * @param packetLen 
     */
    protected void receivedPacket (InetAddress remoteAddr, byte[] packet, int packetLen)
    {
        if (!_nicInfo.ip.equals (remoteAddr)) {
            _nal.receivedPacket (remoteAddr, packet, packetLen, this);
        }
    }

    public String toString()
    {
        if (_nicInfo != null) {
            return _nicInfo.toString() + " [mode: UDP_BROADCAST]";
        }
        else {
            return "UDP_BROADCAST";
        }
    }

    // 
    protected UDPServer _udpServer;
    protected UDPServer _udpBroadcastServer;
    protected boolean _isWindows = System.getProperty ("os.name").startsWith ("Windows");
}
