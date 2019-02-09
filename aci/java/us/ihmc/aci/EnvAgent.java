package us.ihmc.aci;

import us.ihmc.nomads.Agent;
import us.ihmc.nomads.Environment;
import us.ihmc.aci.gss.GSSClient;
import us.ihmc.aci.gss.EnvironmentalMonitorAdaptor;
import us.ihmc.aci.envMonitor.EnvironmentalMonitor;
import us.ihmc.ds.fgraph.FGraph;
import us.ihmc.util.ConfigLoader;

import java.util.*;
import java.util.zip.CRC32;
import java.net.URI;
import java.net.InetAddress;

/**
 * EnvAgent
 *
 * @author Marco Carvalho (mcarvalho@ihmc.us)
 * @version $Revision$
 *          Created on Jul 22, 2004 at 4:30:02 PM
 *          $Date$
 *          Copyright (c) 2004, The Institute for Human and Machine Cognition (www.ihmc.us)
 */
public class EnvAgent extends Agent
{
    public EnvAgent()
    {
        /*
        try {
            //starting the MANET ROUTING SERVICE....
            MANETRoutingService routingService = new MANETRoutingService();
            routingService.start();
        }
        catch (Exception e) {
            //Ignore if this is a BindException - the router might be running already...
            if (!(e instanceof java.net.BindException)) {
                e.printStackTrace();
            }
        }
        */

        try {
            ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();
            _initFFAciManager = cloader.hasProperty("init.ffaci.manager");
            _gatewayIP = cloader.getProperty("adhoc.gateway.ip");
            _gssClient = new GSSClient();
            //_envID = createNameBasedOnThirdOctet();
            //_envID = createNameBasedOnHostname();
            //_envID = createRandomName();
            _envID = Environment.getEnvInfoService().getDefaultEnvURI().getHost() + ":" +
                     Environment.getEnvInfoService().getDefaultEnvURI().getPort();
            _envAgent = this;
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static EnvAgent getInstance() throws Exception
    {
        if (_envAgent == null) {
            throw new Exception ("aci's Environmental Agent is not Running. Unable to proceed!\n");
        }
        return (_envAgent);
    }

    public void start(String[] args)
    {

        String domain = null;
        for (int i=0; i<args.length; i++) {
            if (args[i].compareToIgnoreCase("-envid")==0 && i<(args.length-1)) {
                _envID = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-envname")==0 && i<(args.length-1)) {
                _envName = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-name")==0 && i<(args.length-1)) {
                _agentName = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-domain")==0 && i<(args.length-1)) {
                domain = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-latGPS")==0 && i<(args.length-1)) {
                _latGPS = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-longGPS")==0 && i<(args.length-1)) {
                _longGPS = args[i+1];
            }
            if (args[i].compareToIgnoreCase("-mobile")==0) {
                _mobile = true;
            }

        }
        if (_agentName == null) {
            _agentName = _envID + ".agent";
        }

        debugMsg ("Parsed arguments (EnvID: " + _envID + ", EnvName: " + _envName +
                  ", AgentName: " + _agentName + ")");
        try {
            registerEnvironment (_envID, domain);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            _gssClient.registerAgent(_agentName);
            _gssClient.bindAgentToEnvironment(_agentName, _envID);
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        try {
            EnvironmentalMonitorAdaptor envMonAdaptor = new EnvironmentalMonitorAdaptor(_envID);
            _envMonitor = new EnvironmentalMonitor(_envID);
            _envMonitor.init();
            _envMonitor.addChangeListener(envMonAdaptor);
            while (_running) {
                synchronized (this) {
                    wait(_updateInterval);
                    _envMonitor.update();
                }
            }
            deregisterEnvironment (_envID);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    public long getUpdateInterval ()
    {
        return (_updateInterval);
    }

    public void setUpdateInterval (long updateInterval)
    {
        _updateInterval = updateInterval;
    }

    public GSSClient getGSSClient ()
    {
        return _gssClient;
    }

    public FGraph getFGraphClient ()
    {
        return _gssClient.getFGraph();
    }

    public EnvironmentalMonitor getEnvironmentalMonitor ()
    {
        return _envMonitor;
    }

    public String getEnvUUID ()
    {
        return (_envID);
    }

    public String getName()
    {
        return (_envID);
    }

    public void terminate()
    {
        synchronized (this) {
            _running = false;
            notifyAll();
        }
    }

    //-------------------------- Private Methods --------------------------
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
        String hostip = _gatewayIP;
        try {
            if (hostip == null) {
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
            else {
                debugMsg ("Setting local Address to (" + hostip + ")");
                StringTokenizer st = new StringTokenizer (hostip, ".");
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


    private synchronized void registerEnvironment (String envName, String domainList)
            throws Exception
    {
        _envID = Environment.getEnvInfoService().getUUID();
        if (envName != null) {
            _envID = envName;
        }
//        _envURI = new URI(Environment.getEnvInfoService().getDefaultEnvURI().toExternalForm());
        _envURI = new URI(Environment.getEnvInfoService().getDefaultEnvURI().toString());
        Hashtable attrib = new Hashtable ();
        if (domainList != null) {
            attrib.put(AttributeList.NODE_DOMAIN, domainList);
        }
        if (_envName != null) {
            attrib.put(AttributeList.NODE_NAME, _envName);
        }

        String configname = System.getProperty("cfile");
        if (configname == null) {
            configname = "ffaci.properties";
        }
        ConfigLoader cloader = ConfigLoader.getDefaultConfigLoader();

        double  transfCost = cloader.getPropertyAsDouble("transformation.cost", 1.0);
        Double DtransfCost = new Double (transfCost);
        attrib.put(AttributeList.NODE_TRANSFORM_COST, DtransfCost);

        if (_mobile) {
            attrib.put(AttributeList.NODE_MOBLIE,"TRUE");
        }

        double  relayCost = cloader.getPropertyAsDouble("relay.cost", 1.0);
        Double DrelayCost = new Double (relayCost);
        attrib.put(AttributeList.NODE_RELAY_COST, DrelayCost);
        if (_latGPS != null) {
            Double dLat = Double.valueOf(_latGPS);
            attrib.put(AttributeList.NODE_GPS_LATITUDE, dLat);
        }
        if (_longGPS != null) {
            Double dLong = Double.valueOf(_longGPS);
            attrib.put(AttributeList.NODE_GPS_LONGITUDE, dLong);
        }

        attrib.put(AttributeList.ENVIRONMENT_URI, _envURI.toASCIIString());
        try {
            _gssClient.registerEnvironmentWithAttributes(_envID, attrib);
        }
        catch (Exception e) {
            throw e;
        }
    }

    private synchronized void deregisterEnvironment (String envName)
            throws Exception
    {
        try {
            Enumeration connAgents = _gssClient.getAgentsConnectedToEnvironment(envName);
            while (connAgents.hasMoreElements()) {
                String agentID = (String) connAgents.nextElement();
                _gssClient.deregister(agentID);
            }
            _gssClient.deregister(envName);
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void debugMsg (String msg)
    {
        if (_debug) {
            System.out.println("[ACI.ENV.AGENT] " + msg);
        }
    }

    private static EnvAgent _envAgent;
    private String _envID;
    private String _envName;
    private String _agentName;
    private String _latGPS;
    private String _longGPS;
    private String _gatewayIP = null;
    private boolean _initFFAciManager;
    private boolean _debug = true;
    private boolean _running = true;
    private boolean _mobile = false;
    private long _updateInterval = 5000;
    private URI _envURI;
    private GSSClient _gssClient;
    private EnvironmentalMonitor _envMonitor;
}
