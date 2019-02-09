package us.ihmc.aci.grpMgrOld;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.util.logging.Logger;

import us.ihmc.comm.CommHelper;
import us.ihmc.net.NICInfo;
import us.ihmc.util.LogHelper;


/**
 * Virtual Interface used to relay packets between nodes over a TCP connection.
 *
 * @author Matteo Rebeschini <mrebeschini@ihmc.us>
 */
public class TCPNetworkInterface extends NetworkInterface
{
    public TCPNetworkInterface()
    {
        _terminate = false;
        _running = false;
        _relayNodes = new Hashtable();
        _udpServer = new UDPServer(); //used for sending unicasts
        _logger = LogHelper.getLogger ("us.ihmc.aci.grpMgr.TCPNetworkInterface");
    }

    public void init (NICInfo nicInfo, NetworkAccessLayer nal)
        throws NetworkException
    {
        _nal = nal;
        try {
            _serverSock = new ServerSocket (DEFAULT_PACKETS_RELAY_PORT);
        }
        catch (IOException ioe) {
            throw new NetworkException (ioe.getMessage());
        }

        try {
            _udpServer.init (_nal._port, this);
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed initializing UDP server on interface: " +
                                        this + "; nested exception = " + ne);
        }
    }

    public void run()
    {
        if (_running) {
            return;
        }

        _running = true;
        try {
            while (!_terminate) {
                Socket s = _serverSock.accept();
                _logger.info ("Incoming TCP connection from remote GM: " + s.getInetAddress().getHostAddress());
                ConnHandler ch = new ConnHandler (s);
                ch.start();
                RemoteGroupManagerConnector connector = new RemoteGroupManagerConnector (ch);
                _relayNodes.put (s.getInetAddress(), connector);
            }
        }
        catch (IOException ioe) {
            ioe.printStackTrace();
        }
        _running = false;
    }

    /**
     *
     * @param relayNodes - Vector<InetSocketAddress
     */
    public void setRelayNodes (Vector relayNodes)
    {
        Enumeration e = _relayNodes.keys();
        while (e.hasMoreElements()) {
            RemoteGroupManagerConnector relayNode = (RemoteGroupManagerConnector) e.nextElement();
            if (! relayNodes.contains(relayNode.getAddress())) {
                RemoteGroupManagerConnector connector = (RemoteGroupManagerConnector) _relayNodes.remove (relayNode);
                connector.terminate();
            }
        }

        for (int i = 0; i < relayNodes.size(); i++) {
            try {
                InetSocketAddress relayNode = (InetSocketAddress) relayNodes.elementAt(i);
                RemoteGroupManagerConnector connector = new RemoteGroupManagerConnector (relayNode);
                _relayNodes.put (relayNode, connector);
                connector.start();
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void terminate()
    {
        _terminate = true;
        try {
            _serverSock.close();
        }
        catch (IOException ioe) {}
    }

    public void broadcastPacket (byte[] packet, int packetLen, int hopCount)
        throws NetworkException
    {
        // If any relay node is present forward the packet to the node
        if (!_relayNodes.isEmpty()) {
            Enumeration e = _relayNodes.elements();
            while (e.hasMoreElements()) {
                RemoteGroupManagerConnector connector = (RemoteGroupManagerConnector) e.nextElement();
                connector.sendPacket (packet, packetLen);
            }
        }
    }

    public void sendPacket (InetAddress destAddr, byte[] packet, int packetLen)
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

    public void receivedPacket (InetAddress destAddr, byte[] packet, int packetLen)
    {
        _nal.receivedPacket (destAddr, packet, packet.length, this);
    }


    /**
     *
     * @author mrebeschini
     */
    private class RemoteGroupManagerConnector extends Thread
    {
        public RemoteGroupManagerConnector (InetSocketAddress remoteGMAddr)
        {
            _remoteGMAddr = remoteGMAddr.getAddress();
            _port = (remoteGMAddr.getPort() != 0) ? remoteGMAddr.getPort() : DEFAULT_PACKETS_RELAY_PORT;
            _terminate = false;
            _incomingConnection = false;
        }
        
        public RemoteGroupManagerConnector (ConnHandler ch) 
        {
        	_ch = ch;
        	_remoteGMAddr = ch.getRemoteAddress();
        	_incomingConnection = true;
        }
        	
        public InetAddress getAddress()
        {
            return _remoteGMAddr;
        }

        public void sendPacket (byte[] packet, int packetLen)
        {
            if (_ch != null) {
            	if (_ch.isConnected()) {
            		_ch.sendPacket (packet, packetLen);
            	}
            	else if (_incomingConnection){
            		_relayNodes.remove (_remoteGMAddr);
            	}
            }
        }

        public void run()
        {
            _terminate = false;

            while (!_terminate) {
                try {
                    if ((_ch == null) || (! _ch.isConnected())) {
                    	Socket s = new Socket (_remoteGMAddr, _port);
                        _ch = new ConnHandler (s);
                        _ch.start();
                        _logger.info ("Established connection with remote GM: " + _remoteGMAddr.getHostAddress() + ":" + _port);
                    }
                }
                catch (IOException ioe) {
                	if (_ch != null) {
                		_logger.info ("Lost connection with remote GM: " + _remoteGMAddr.getHostAddress() + ":" + _port);
                		_ch = null;
                	}
                }

                try {
                    Thread.sleep (RECONNECT_TIMEOUT);
                }
                catch (InterruptedException ie) {
                    ie.printStackTrace();
                }
            }
        }

        public void terminate()
        {
            if (_ch != null) {
                _ch.terminate();
                _ch = null;
            }
            _terminate = true;
        }

        private boolean _terminate;
        private boolean _incomingConnection;
        private InetAddress _remoteGMAddr;
        private int _port;
        private ConnHandler _ch;
        private static final int RECONNECT_TIMEOUT = 30000;
    } //RemoteGroupManagerConnector


    /**
     *
     * @author mrebeschini
     */
    private class ConnHandler extends Thread
    {
        public ConnHandler (Socket s)
        {
            _ch = new CommHelper (s);
        }

        public void sendPacket (byte[] packet, int packetLen)
        {
            byte[] buff = new byte[packetLen];
            System.arraycopy (packet, 0, buff, 0, packetLen);
            try {
                _ch.sendBlock (buff);
            }
            catch (Exception e) {
                _ch = null;
                _terminate = true;
            }
        }

        public boolean isConnected()
        {
            return (_ch != null);
        }

        public InetAddress getRemoteAddress()
        {
            if (_ch != null) {
                return _ch.getSocket().getInetAddress();
            }
            return null;
        }

        public void terminate()
        {
            _ch = null;
            _terminate = true;
        }

        public void run()
        {
            byte[] packet = null;
            _terminate = false;

            while (!_terminate) {
                try {
                    packet = _ch.receiveBlock();
                }
                catch (Exception e) {
                    _ch = null;
                    _terminate = true;
                    return;
                }
                receivedPacket (null, packet, packet.length);
            }
        }

        public boolean equals (ConnHandler ch)
        {
            return ch.getRemoteAddress().equals (this.getRemoteAddress());
        }

        private boolean _terminate;
        private CommHelper _ch;
    } //ConnHandler


    // Private instance variables
    private boolean _running, _terminate;
    private NetworkAccessLayer _nal;
    private ServerSocket _serverSock;
    private UDPServer _udpServer;
    private Logger _logger;
    private Hashtable _relayNodes; //Hashtable<InetAddress, RemoteGroupManagerConnector>
    private static final int DEFAULT_PACKETS_RELAY_PORT = 1515; //TODO: change this port number
}
