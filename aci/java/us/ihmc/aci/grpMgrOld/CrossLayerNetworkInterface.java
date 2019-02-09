package us.ihmc.aci.grpMgrOld;

import java.io.File;
import java.lang.reflect.Method;
import java.net.InetAddress;

import us.ihmc.net.NICInfo;

/**
 * NetworkInterface using the XLayer substrate for broadcasting
 * packets.
 *
 * @author Matteo Rebeschini
 */
public class CrossLayerNetworkInterface extends NetworkInterface
{
    public CrossLayerNetworkInterface()
    {
        _udpServer = new UDPServer(); // needed for receiveing unicast messages
    }

    protected void init (NICInfo nicInfo, NetworkAccessLayer nal)
        throws NetworkException
    {
        _nicInfo = nicInfo;
        _nal = nal; 

        try {
            _udpServer.init (_nal._port, this, _nicInfo.ip);
        }
        catch (NetworkException ne) {
            throw new NetworkException ("failed initializing UDP server on interface: " +
                                        this + "; nested exception = " + ne);
        }

        try {
            Class cls = Class.forName ("us.ihmc.xlayer.XLayerImpl");
            String nomadsHome = (String) System.getProperties().get ("nomads.home");
            if (nomadsHome != null) {
                String xlayerCfgPath = nomadsHome + File.separator + "aci" + File.separator + "conf" + 
                                       File.separator + "xlayer.properties";
                Method setConfigFileMethod = cls.getMethod ("setConfigFile", new Class[] { String.class });
                setConfigFileMethod.invoke (null, new Object[] {xlayerCfgPath});
            }

            Method setControllerHostMethod = cls.getMethod ("setControllerHost", new Class[] { String.class });
            setControllerHostMethod.invoke (null, new Object[] { _nicInfo.ip.getHostAddress() });

            if (_debug) {
                Method setDebugMethod = cls.getMethod ("setDebug", new Class[] {boolean.class});
                setDebugMethod.invoke (null, new Object[] { new Boolean (true) });
            }

            _getInstanceMethod = cls.getMethod ("getInstance", new Class[0]);
            _xlayerObj = _getInstanceMethod.invoke (null, new Class[0]);

            _receiveBroadcastMessageMethod = cls.getMethod ("receiveBroadcastMessage", new Class[0]);
            _sendBroadcastMessageMethod = cls.getMethod ("sendBroadcastMessage", new Class[] {(new byte[0]).getClass(), int.class});
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new NetworkException ("CROSSLAYER is NOT supported by this implementation!");
        }
    }
    
    public void run()
    {
        if (_running) {
            return;
        }
        _running = true;
        _terminate = false;
        _udpServer.start();

        try {
            Class cls = Class.forName ("us.ihmc.xlayer.ds.BroadcastMessage");
            Method getDataMethod = cls.getMethod ("getData", new Class[0]);
            Method getAddressMethod = cls.getMethod ("getAddress", new Class[0]);
            Object broadcastMessageObj;

            while (!_terminate) {
                // This method may block until a message is received.
                broadcastMessageObj = _receiveBroadcastMessageMethod.invoke (_xlayerObj, new Object[0]);
                byte[] data = (byte[]) getDataMethod.invoke (broadcastMessageObj, new Object[0]);
                String ipAddr = (String) getAddressMethod.invoke (broadcastMessageObj, new Object[0]);
                receivedPacket (InetAddress.getByName (ipAddr), data, data.length);
            }
        }
        catch (Exception e) {
            _running = false;
            e.printStackTrace();
        }
        _running = false;
    }
    
    protected void terminate()
    {
        if (_terminate) {
            return;
        }

        _udpServer.terminate();
        _terminate = true;
    }
   
    protected void broadcastPacket (byte[] packet, int packetLen, int hopCount)
        throws NetworkException
    {
        try {
            // temporary fix until sendBroadcastMessage() will accept the packetLen
            byte[] trimmedPacket = new byte[packetLen]; 
            System.arraycopy (packet, 0, trimmedPacket, 0, packetLen);
            _sendBroadcastMessageMethod.invoke (_xlayerObj, new Object[] {trimmedPacket, new Integer(hopCount)});
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new NetworkException ("Error broadcasting message: " + e.getMessage());
        }
    }

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

    protected void receivedPacket (InetAddress remoteAddr, byte[] packet, int packetLen)
    {
        if (!_nicInfo.ip.equals (remoteAddr)) {
            _nal.receivedPacket (remoteAddr, packet, packetLen, this);
        }
    }
    
    public String toString()
    {
        if (_nicInfo != null) {
           return _nicInfo.toString() + " [CROSS_LAYER]";
        }
        else {
            return "CROSS_LAYER";
        }
    }

    // Instance Variables
    private Object _xlayerObj;
    private Method _getInstanceMethod;
    private Method _receiveBroadcastMessageMethod;
    private Method _sendBroadcastMessageMethod;
    private UDPServer _udpServer;
    private boolean _running = false;
    private boolean _terminate = false;
    private static final boolean _debug = false; // enable XLayer debug
}

