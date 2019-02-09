package us.ihmc.aci.netSupervisor;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Link;
import us.ihmc.aci.ddam.NetworkHealth;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.netSupervisor.algorithms.ElectionAlgorithm;
import us.ihmc.aci.netSupervisor.information.InformationBroker;
import us.ihmc.aci.netSupervisor.link.LinkManager;
import us.ihmc.aci.netSupervisor.topology.TopologyManager;
import us.ihmc.aci.netSupervisor.topology.Subnetwork;
import us.ihmc.aci.netSupervisor.traffic.TrafficManager;
import us.ihmc.aci.netSupervisor.traffic.WorldStateSummary;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.*;

import static us.ihmc.aci.netSupervisor.algorithms.ElectionAlgorithm.*;
//import static us.ihmc.aci.nodemon.util.Utils.initLogger;


public class NetSupervisor
{
    /**
     * Constructor
     */
    public NetSupervisor (String netSupervisorPropertiesCfg) throws IOException
    {
        terminationRequested = false;

        _nodesMonitored     = new ArrayList<>();
        _remoteProxyIps     = new ArrayList<>();
        _topologyManager    = new TopologyManager();


        int timeWindow = Config.getIntegerValue(Conf.NodeMon.NETWORK_TIME_WINDOW_SIZE,
                DefaultValues.NodeMon.NETWORK_TIME_WINDOW_SIZE);
        //Load proprieties:
        if (readConfigurationFile(netSupervisorPropertiesCfg)) {
            startNetSupervisorThread();
        }
        else {
            log.error("NetSupervisor was not able to read its configuration file correctly");
            throw new IOException();
        }
        _trafficManager = new TrafficManager(_healtMessageUpdateTime, _sensorResolution, timeWindow);
        if(_remoteProxyIps.size() > 0) {_trafficManager.setRemoteNetproxyIps(_remoteProxyIps);}
        _trafficManager.enableInterconnectedPCalculation();
        _trafficManager.enableBackaulPCalculation();
        _trafficManager.enableLatenceCalculation();
        _trafficManager.enableSaturationCalculation();
        _trafficManager.enableLinkBWCalculation();
        _trafficManager.enableWithinBWCalculation();

    }

    private boolean readConfigurationFile(String cfgPath)
    {
        log.info("NetSupervisor cfg path: " + cfgPath);
        Properties props = new Properties();
        FileInputStream in;
        try {
            in = new FileInputStream(cfgPath);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        }
        try {
            props.load(in);
            in.close();

            //get the nodes we are interested in for the election algorithm
            if(props.containsKey("netsupervisor.ips.monitored")) {
                String[] ipMonitored = props.getProperty("netsupervisor.ips.monitored").split(" ");
                for (String nodeIp :
                        ipMonitored
                        ) {
                    log.debug("IP: " + nodeIp);
                    _nodesMonitored.add(nodeIp);
                }
            }
            else {
                log.warn("netsupervisor.ips.monitored not present, election algorithm is disabled");
            }

            if(props.containsKey("netsupervisor.force.remote.netproxy.ips")) {
                String[] remoteProxyIps = props.getProperty("netsupervisor.force.remote.netproxy.ips").split(" ");
                for (String remoteNetproxyIp :
                        remoteProxyIps
                        ) {
                    log.debug("Remote proxy IP: " + remoteNetproxyIp);
                    _remoteProxyIps.add(remoteNetproxyIp);
                }
            }
            else {
                log.info("No remote NetProxy IPs specified (netsupervisor.force.remote.netproxy.ips)," +
                        "NetSupervisor will try to get them from the Worldstates");
            }

            //Get the update period for the health message
            if(props.containsKey("netsupervisor.health.message.delivery.period")) {
                _healtMessageUpdateTime = Integer.parseInt(
                        props.getProperty("netsupervisor.health.message.delivery.period"));
                log.info("Health message period: " + _healtMessageUpdateTime);
            }
            else {
                _healtMessageUpdateTime = 10;
                log.warn("Health message period (netsupervisor.health.message.delivery.period): not specified" +
                "default value is: " + _healtMessageUpdateTime + "s");
            }

            if(props.containsKey("netsupervisor.sensor.resolution")) {
                _sensorResolution = Integer.parseInt(props.getProperty("netsupervisor.sensor.resolution"));
                log.info("Sensor resolution: " + _sensorResolution);
            }
            else {
                _sensorResolution = 5;
                log.warn("Health message period (netsupervisor.sensor.resolution): not specified" +
                        "default value is: " + _sensorResolution + "s");
            }

            //Get the parameters for the selection of the network type
            Double lowPerfBw;
            if(props.containsKey("netsupervisor.low.performance.network.bandwidth")) {
                lowPerfBw = Double.parseDouble(props.getProperty("netsupervisor.low.performance.network.bandwidth"));
                log.info("Low Performance Network Bandwidth set to: " + lowPerfBw);
            }
            else {
                lowPerfBw = 1000000d;
                log.warn("Low performance network bandwidth (netsupervisor.low.performance.network.bandwidth): not specified" +
                        "default value is: " + lowPerfBw + "B");
            }

            Double highPerfBw;
            if(props.containsKey("netsupervisor.high.performance.network.bandwidth")) {
                highPerfBw = Double.parseDouble(props.getProperty("netsupervisor.high.performance.network.bandwidth"));
                log.info("High performance network bandwidth: " + highPerfBw + "B");
            }
            else {
                highPerfBw = 1000000000d;
                log.warn("High performance network bandwidth (netsupervisor.high.performance.network.bandwidth): not specified" +
                        "default value is: " + highPerfBw + "B");
            }

            int lowPerfLat;
            if(props.containsKey("netsupervisor.low.performance.network.latency")) {
                lowPerfLat = Integer.parseInt(props.getProperty("netsupervisor.low.performance.network.latency"));
                log.info("Low performance network latency: " + lowPerfLat + "ms");
            }
            else {
                lowPerfLat = 400;
                log.warn("Low performance network latency (netsupervisor.low.performance.network.latency) " +
                        "not specified default value is: " + lowPerfLat + "ms");
            }

            int highPerfLat;
            if(props.containsKey("netsupervisor.high.performance.network.latency")) {
                highPerfLat = Integer.parseInt(props.getProperty("netsupervisor.high.performance.network.latency"));
                log.info("High performance network latency: " + highPerfLat + "ms");
            }
            else {
                highPerfLat = 100;
                log.warn("High performance network latency (netsupervisor.high.performance.network.latency) " +
                        "not specified default value is: " + highPerfLat + "ms");
            }

            if(props.containsKey("netsupervisor.election.algorithm.enabled")) {
                _electionAlgorithmEnabled = Boolean.parseBoolean(
                        props.getProperty("netsupervisor.election.algorithm.enabled"));
                log.info("Election algorithm: " + _electionAlgorithmEnabled);
            }
            else {
                _electionAlgorithmEnabled = true;
                log.warn("Election algorithm value (netsupervisor.election.algorithm.enabled) " +
                        "not specified default value is: " + _electionAlgorithmEnabled);
            }

            if(_electionAlgorithmEnabled) {
                Map<Integer,Integer> pointsGoodNetworkBandwidth;
                if(props.containsKey("netsupervisor.points.for.good.network.bandwidth")) {
                    pointsGoodNetworkBandwidth = readCollectionPointsFromProperties(
                            props.getProperty("netsupervisor.points.for.good.network.bandwidth").split(" "));
                    log.info("Points for good network : " + pointsGoodNetworkBandwidth);
                }
                else {
                    pointsGoodNetworkBandwidth = new HashMap<>();
                    pointsGoodNetworkBandwidth.put(0, 100);
                    pointsGoodNetworkBandwidth.put(1, 90);
                    pointsGoodNetworkBandwidth.put(2, 60);
                    pointsGoodNetworkBandwidth.put(3, 40);
                    log.warn("Points for good network not present(netsupervisor.points.for.good.network.bandwidth),"+
                            " using default: " + pointsGoodNetworkBandwidth);
                }

                Map<Integer,Integer> pointsBadNetworkBandwidth;
                if(props.containsKey("netsupervisor.points.for.bad.network.bandwidth")) {
                    pointsBadNetworkBandwidth = readCollectionPointsFromProperties(
                            props.getProperty("netsupervisor.points.for.bad.network.bandwidth").split(" "));
                    log.info("Points for bad network : " + pointsGoodNetworkBandwidth);
                }
                else {
                    pointsBadNetworkBandwidth = new HashMap<>();
                    pointsBadNetworkBandwidth.put(0, 100);
                    pointsBadNetworkBandwidth.put(1, 60);
                    pointsBadNetworkBandwidth.put(2, 40);
                    pointsBadNetworkBandwidth.put(3, -20);
                    log.warn("Points for bad network not present(netsupervisor.points.for.bad.network.bandwidth),"+
                            " using default: " + pointsBadNetworkBandwidth);
                }

                Map<Integer,Integer> pointsGoodNetworkLatency;
                if(props.containsKey("netsupervisor.points.for.good.network.latency")) {
                    pointsGoodNetworkLatency = readCollectionPointsFromProperties(
                            props.getProperty("netsupervisor.points.for.good.network.latency").split(" "));
                    log.info("Points for good network latency: " + pointsGoodNetworkLatency);
                }
                else {
                    pointsGoodNetworkLatency = new HashMap<>();
                    pointsGoodNetworkLatency.put(0, 100);
                    pointsGoodNetworkLatency.put(1, 90);
                    pointsGoodNetworkLatency.put(2, 90);
                    pointsGoodNetworkLatency.put(3, 50);
                    log.warn("Points for good network latency not present(netsupervisor.points.for.good.network.latency),"+
                            " using default: " + pointsGoodNetworkLatency);
                }

                Map<Integer,Integer> pointsBadNetworkLatency;
                if(props.containsKey("netsupervisor.points.for.bad.network.latency")) {
                    pointsBadNetworkLatency = readCollectionPointsFromProperties(
                            props.getProperty("netsupervisor.points.for.bad.network.latency").split(" "));
                    log.info("Points for bad network latency: " + pointsBadNetworkLatency);
                }
                else {
                    pointsBadNetworkLatency = new HashMap<>();
                    pointsBadNetworkLatency.put(0, 100);
                    pointsBadNetworkLatency.put(1, 60);
                    pointsBadNetworkLatency.put(2, 40);
                    pointsBadNetworkLatency.put(3, -20);
                    log.warn("Points for bad network latency not present(netsupervisor.points.for.good.network.latency)," +
                            " using default: " + pointsBadNetworkLatency);
                }

                log.info("Validity queried from nodemon cfg: " + _validity);

                //Setting the parameters
                /*
                _linkManager = new LinkManager(highPerfBw, lowPerfBw, highPerfLat, lowPerfLat, pointsGoodNetworkBandwidth,
                        pointsBadNetworkBandwidth, pointsGoodNetworkLatency, pointsBadNetworkLatency, _nodesMonitored, _validity);
                        */
            }
            else {
                log.warn("Election algorithm disabled");
            }
        }
        catch (IOException e) {
            log.error(e.toString());
            e.printStackTrace();
            return false;
        }
        return true;
    }

    /**
     * Utility to read more parameters from the same property (for an integer obj)
     * @param props array of parameters
     * @return a map<integer,integer> with keys from 1 to n (n = number of parameters)
     */
    private Map<Integer,Integer> readCollectionPointsFromProperties(String[] props){
        Map<Integer,Integer> pointsNetwork = new HashMap<>();
        int i = 1;
        for (String p : props) {
            pointsNetwork.put(i,Integer.parseInt(p));
            i++;
        }
        return pointsNetwork;
    }

    private boolean  generateHealthMessage()
    {
        String ntwName = _trafficManager.getLocalNetworkName();
        if (Objects.equals(ntwName, "")) {
            log.debug("Network Name not yet detected");
        }
        else {
            NetworkHealth msg = null;
            WorldStateSummary ws = null;
            msg = _informationBroker.getHealthMessage(ws);
            log.debug("Generating health message for segment: " + ntwName);
            if (ntwName != null && msg != null) {
                _informationBroker.enqueueHealthMessage(ntwName, msg);
            }
            return true;

        }
        return false;

    }


    public void startMainTimer()
    {
        _timeOfStart = System.currentTimeMillis();
    }

    public void printTimeElapsed()
    {
        log.debug("NetSupervisor Main cicle ended, time elapsed: " +
                (System.currentTimeMillis() - _timeOfStart) + "ms");
    }

    private void startNetSupervisorThread ()
    {
        log.debug("Starting Supervisor Main Thread");
        Thread thread = new Thread()
        {
            public void run() {
                while (!terminationRequested) {
                    try {
                        Thread.sleep(2000);
                        startMainTimer();
                        _informationBroker.update();
                        if (_informationBroker != null) {
                            _topologyManager.analizeTopology(_informationBroker, _nodesMonitored);
                            //_linkManager.analyzeWorldState(_informationBroker, _topologyManager);
                            //_trafficManager.analizeTraffic(_informationBroker,_topologyManager);
                            if(_trafficManager.healthMessageIsReady()) {generateHealthMessage();}
                        }
                        else {log.warn("Information Brooker is null");}
                        printTimeElapsed();
                    }
                    catch (InterruptedException e) { log.error(e.toString());}
                }
            }
        };
        thread.setName(this.getClass().getSimpleName() + "Thread");
        thread.start();
    }

    public static void main (String[] args) throws IOException
    {
        try {
            log.debug("Setting the launcher paths...\n");
            setConfigurationPaths(args);
        }
        catch (ArrayIndexOutOfBoundsException e) {
            log.error(e.toString());
            defaultConfigurationPaths();
        }
        //load log4j configuration
        log.info("Loading log4j configuration");

        //initLogger (_log4jConfPath, _logPath);

        log.info("Starting NetSupervisor");
        new NetSupervisor(_netSupervisorProperties);
    }

    //Set the default parameters
    private static void defaultConfigurationPaths()
    {
        _log4jConfPath = "../../conf/netsupervisor/netSupervisor.log4j.properties"; //Log4J conf
        _logPath = "../../log/"; //logs netsup path
        _netSupervisorProperties = "../../conf/netsupervisor/netSupervisor.properties"; //NetSup properties
    }

    //Set the parameters from the launcher input
    private static void setConfigurationPaths(String[] paths)
    {
        //_nodemonConf = paths[0]; // NodeMon conf
        //_groupManagerConf = paths[1]; // GroupManager conf
        _log4jConfPath = paths[2]; //Log4J conf
        _logPath = paths[3]; //logs netsup path
        _netSupervisorProperties = paths[4]; //NetSup properties
    }

    // <-------------------------------------------------------------------------------------------------------------->
    private static final Logger log = Logger.getLogger(NetSupervisor.class);
    private Collection<String> _nodesMonitored;
    private Collection<String> _remoteProxyIps;
    private int _healtMessageUpdateTime;
    private int _sensorResolution;
    private boolean terminationRequested;
    private static String _log4jConfPath, _logPath, _netSupervisorProperties;

    private LinkManager _linkManager;
    private ElectionAlgorithm _electionAlgorithm;
    private boolean _electionAlgorithmEnabled;
    private InformationBroker _informationBroker;

    double _timeOfStart;
    TopologyManager _topologyManager;
    TrafficManager _trafficManager;
    int _validity;

}
