package us.ihmc.aci.grpMgrOld;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.MulticastSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NoRouteToHostException;

/**
 * UDP Server
 *
 * @author: Niranjan Suri
 * @version $Revision$
 * $Date$
 */
public class UDPServer extends Thread
{
    public UDPServer()
    {
        _terminate = false;
        _running = false;
    }

    public void init (int port, NetworkInterface nic)
        throws NetworkException
    {
        init (port, nic, null); // wildcard address
    }

    public void init (int port, NetworkInterface nic, InetAddress listenAddr)
        throws NetworkException
    {
        _port = port;
        _nic = nic;
        InetSocketAddress sockAddr = new InetSocketAddress (listenAddr, port);

        try {
            _datagramSock = new MulticastSocket (sockAddr);
        }
        catch (IOException ioe) {
            throw new NetworkException ("error initializing UDP Server; nested exception = " + ioe.getMessage());
        }
    }

    public void init (int port, NetworkInterface nic, InetAddress listenAddr, 
                      InetAddress multicastGroup, int ttl) 
        throws NetworkException
    {
        init (port, nic, listenAddr); 

        try {
            _datagramSock.joinGroup (multicastGroup);
        }
        catch (IOException ioe) {
            // invalid multicast address
            throw new NetworkException ("error joining multicast group; nested exception = " + ioe.getMessage());
        }

        try {
            _datagramSock.setTimeToLive (ttl);
            _multicastTTL = ttl;
        }
        catch (IOException ioe) {
            throw new NetworkException ("error setting multicast TTL; nested exception = " + ioe.getMessage());
        }
    }

    /**
     * Join a multicast group
     *
     * @param multicastGroup
     * @thows NetworkException
     */
    void joinMulticastGroup (InetAddress multicastGroup) 
        throws NetworkException
    { 
        try {
            _datagramSock.joinGroup (multicastGroup);
        }
        catch (IOException ioe) { 
            // invalid multicast address
            throw new NetworkException ("error joining multicast group; nested exception = " + ioe.getMessage());
        }
    }

    /**
     * Leave a multicast group
     *
     * @param multicastGroup
     * @thows NetworkException
     */ 
    void leaveMulticastGroup (InetAddress multicastGroup)
        throws NetworkException
    {
        try {
            _datagramSock.leaveGroup (multicastGroup);
        }
        catch (IOException ioe) {
            // invalid multicast group
            throw new NetworkException ("error leaving multicast group; nested exception = " + ioe.getMessage());
        }
    } 

    /**
     * Get the multicast TTL
     *
     * @return multicast TTL
     */  
    int getMulticastTTL()
    {
        return _multicastTTL;
    }

    /**
     * Set the multicast TTL
     *
     * @param multicastTTL
     * @throws NetworkException
     */ 
    void setMulticastTTL (int multicastTTL)
        throws NetworkException
    {
        try {
            _datagramSock.setTimeToLive (multicastTTL);
            _multicastTTL = multicastTTL;
        }
        catch (IOException ioe) {
            throw new NetworkException ("error setting multicast TTL; nested exception = " + ioe.getMessage());
        }
    }

    /**
     * Return the running state of this udp server thread.
     *
     * @return <code>true</code> if the instance is running, false otherwise
     */
    public boolean isRunning()
    {
        return _running;
    }

    /**
     * Request to the udp server thread to terminate and close the socket.
     */
    public void terminate()
    {
        _terminate = true;
        _datagramSock.close();
    }

    /**
     * Send a datagram packet.
     *
     * @param inetAddr the packet inetAddress object
     * @param packet   the packet byte array
     * @param len      the length of the packet
     * @throws NetworkException
     */
    public synchronized void sendPacket (InetAddress inetAddr, byte[] packet, int len)
        throws NetworkException
    {
        try {
            DatagramPacket dp = new DatagramPacket (packet, packet.length, inetAddr, _port);
            dp.setLength (len);
            _datagramSock.send (dp);
        }
        catch (NoRouteToHostException e) {
            // Ignore NoRouteToHostException - which is thrown if a wireless card in ad-hoc mode
            // is not bound to another card due to a loss in connectivity
        }
        catch (IOException e) {
            throw new NetworkException ("failed to send packet; nested exception = " + e.getMessage());
        }
    }

    /**
     * Block receiving UDP packets and notity the NetworkAccessLayer when they are received.
     */
    public void run() 
    {
        if (_running) {
            return;
        }
    
        _running = true;
        try {
            byte[] packetBuf = new byte [PacketCodec.MAX_PACKET_SIZE];
            DatagramPacket dp = new DatagramPacket (packetBuf, packetBuf.length);

            while (!_terminate) {
                _datagramSock.receive (dp);
                _nic.receivedPacket (dp.getAddress(), dp.getData(), dp.getLength());
                packetBuf = new byte [PacketCodec.MAX_PACKET_SIZE];
            	dp = new DatagramPacket (packetBuf, packetBuf.length);
            }
            _running = false;
        }
        catch (IOException ioe) {
            _running = false;
            ioe.printStackTrace();
        }
    }

    //Class variables
    private boolean _running;
    private boolean _terminate;
    private int _port;
    private int _multicastTTL;
    private MulticastSocket _datagramSock;
    private NetworkInterface _nic;
}
