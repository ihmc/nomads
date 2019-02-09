package us.ihmc.aci.proxyudp;

import us.ihmc.nomads.Agent;
import us.ihmc.util.ConfigLoader;

import java.util.Random;
import java.util.StringTokenizer;
import java.util.zip.CRC32;
import java.net.InetAddress;
import java.net.DatagramPacket;
import java.net.DatagramSocket;

/**
 * EnvAgentUDP
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 22, 2004 at 4:30:02 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class EnvAgentUDP extends Agent
{
    public EnvAgentUDP()
    {
        try {
            debugMsg ("Initializing...");
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            String shost = cloader.getProperty("fgraph.proxyudp.host");
            int iport = cloader.getPropertyAsInt("fgraph.proxyudp.port");
            byte packetBuf[] = new byte [PACKET_SIZE];
            _dgramPacket = new DatagramPacket (packetBuf, packetBuf.length);
            _dgramPacket.setAddress(InetAddress.getByName(shost));
            _dgramPacket.setPort (iport);
            _dgramSock = new DatagramSocket ();
            _envName = createNameBasedOnThirdOctet();
            _envAgentUDP = this;
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void start(String[] args)
    {
        String domain = null;
        for (int i=0; i<args.length; i++) {
            if (args[i].compareToIgnoreCase("-name")==0 && i<(args.length-1)) {
                _envName = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-domain")==0 && i<(args.length-1)) {
                domain = args[i+1];
            }
        }

        debugMsg ("Parsed arguments (name=" + _envName + "). Initializing Env.Registration");
        try {
            registerEnvironment (_envName, domain);     
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            ARLGPSProviderUDP gpsProvider = new ARLGPSProviderUDP();
            gpsProvider.setEnvAgentUDP(this);
            gpsProvider.init();
            gpsProvider.start();

            BBNAdHocSNMPProviderUDP snmpProvider = new BBNAdHocSNMPProviderUDP();
            snmpProvider.setEnvAgentUDP(this);
            snmpProvider.init();
            snmpProvider.start();

            while (_running) {
                synchronized (this) {
                    wait(_waitInterval);
                }
            }
            deregisterEnvironment (_envName);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    //-------------------------- Private Methods --------------------------
    public synchronized void setGPS (String gpsInfo)
    {
        String smsg = "GPSINFO " + _envName + " " + _posUpdateCount++ + " " + gpsInfo;
        try {
            dispatchMessage (smsg);
        }
        catch (Exception e) {
            debugMsg ("Failed to dispatch GPS Coordinates (" + e.getMessage() + ")");
        }
    }

    //-------------------------- Private Methods --------------------------
    public synchronized void setEdgeUpdate (String edgeInfo)
    {
        String smsg = "EDGEINFO " + _envName + " " + _snmpUpdateCount++ + "\n" + edgeInfo;
        try {
            dispatchMessage (smsg);
        }
        catch (Exception e) {
            debugMsg ("Failed to dispatch GPS Coordinates (" + e.getMessage() + ")");
        }
    }

    //-------------------------- Private Methods --------------------------
    public String getName()
    {
        return (_envName);
    }

    //-------------------------- Private Methods --------------------------
    public void terminate()
    {
        synchronized (this) {
            _running = false;
            notifyAll();
        }
    }

    private synchronized void registerEnvironment (String envName, String domainList)
            throws Exception
    {
        try {
            envName = envName.replace(' ','_');
            String smsg = "REGISTER_ENVIRONMENT " + envName;
            dispatchMessage (smsg);
        }
        catch (Exception e) {
            debugMsg ("Failed to register environment: " + e.getMessage());
        }
    }

    private synchronized void deregisterEnvironment (String envName)
            throws Exception
    {
        try {
            envName = envName.replace(' ','_');
            String smsg = "DEREGISTER_ENVIRONMENT " + envName;
            dispatchMessage (smsg);
        }
        catch (Exception e) {
            debugMsg ("Failed to register environment: " + e.getMessage());
        }
    }

    //-------------------------- Private Methods --------------------------
    public void dispatchMessage (String msg)
            throws Exception
    {
        debugMsg ("Size of message is: " + msg.length());
        if (msg.length() > PACKET_SIZE) {
            debugMsg ("Unable to send Message. Size (" + msg.length() + ") exceeds upd buffer");
            return;
        }

        int packetlen = msg.length();
        debugMsg ("Packet length: " + packetlen);
        byte[] payload = _dgramPacket.getData();
        System.arraycopy (msg.getBytes(), 0, payload, 0, packetlen);
        _dgramPacket.setLength(packetlen);
        _dgramSock.send (_dgramPacket);
        _msgCount ++;
    }

    private String createRandomName ()
    {
        String hostname = null;
        try {
            hostname = "ENV" + generateRandomName();
        }
        catch (Exception e) {
            hostname = null;
            e.printStackTrace();
        }
        return hostname;
    }

    public long getMessageCount()
    {
        return (_msgCount);
    }

    //-------------------------- Private Methods --------------------------
    private String generateRandomName()
    {
        String name = "EA";
        Random rand = new Random();
        CRC32 crc32 = new CRC32();
        String sencode = rand.nextLong() + "." + System.currentTimeMillis() + "." + rand.nextDouble();
        crc32.update(sencode.getBytes());
        name = name + crc32.getValue();
        return (name);
    }

    //-------------------------- Private Methods --------------------------
    private String createNameBasedOnThirdOctet ()
    {
        String hostip = null;
        try {
            InetAddress[] allInets = InetAddress.getAllByName("localhost");
            InetAddress add = InetAddress.getByName (InetAddress.getLocalHost().getHostAddress());
            System.out.println(add.getHostAddress());
            if (!add.isLoopbackAddress()) {
                String inetAdd = add.getHostAddress();
                debugMsg ("Setting local Address to (" + inetAdd + ")");
                StringTokenizer st = new StringTokenizer (inetAdd, ".");
                if (st.countTokens() == 4) {
                    String oct1 = st.nextToken();
                    String oct2 = st.nextToken();
                    String oct3 = st.nextToken();
                    String oct4 = st.nextToken();
                    hostip = "N" + oct3;
                    debugMsg ("hostname (" + hostip + ")");
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return (hostip);
    }

    //-------------------------- Private Methods --------------------------
    private String createNameBasedOnHostname ()
    {
        String hostname = null;
        try {
            hostname = InetAddress.getLocalHost().getHostName();
            debugMsg ("Detected hostname: " + hostname);
        }
        catch (Exception e) {
            hostname = null;
            e.printStackTrace();
        }

        if (hostname == null) {
            hostname = generateRandomName();
        }
        return hostname;
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[EnvAgentUDP] " + msg);
        }
    }

    private static EnvAgentUDP _envAgentUDP;
    private long _snmpUpdateCount = 0;
    private long _posUpdateCount = 0;
    private static final int PACKET_SIZE = 65535;
    private DatagramPacket _dgramPacket;
    private DatagramSocket _dgramSock;
    private long _msgCount = 0;
    private String _envName;
    private boolean _debug = true;
    private boolean _running = true;
    private long _waitInterval = 15000;
}
