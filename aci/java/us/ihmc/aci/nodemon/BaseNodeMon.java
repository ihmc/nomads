package us.ihmc.aci.nodemon;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.ddam.TransportType;
import us.ihmc.aci.nodemon.controllers.delivery.*;
import us.ihmc.aci.nodemon.controllers.throughput.ThroughputController;
import us.ihmc.aci.nodemon.delivery.UnicastService;
import us.ihmc.aci.nodemon.discovery.DiscoveryService;
import us.ihmc.aci.nodemon.discovery.DiscoveryType;
import us.ihmc.aci.nodemon.proxy.BaseNodeMonProxyServer;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyServer;
import us.ihmc.aci.nodemon.scheduler.*;
import us.ihmc.aci.nodemon.sensors.PolledNodeSensor;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.aci.nodemon.util.Strings;
import us.ihmc.aci.nodemon.util.Utils;
import us.ihmc.util.Config;
import us.ihmc.util.serialization.SerializationException;

import java.io.*;
import java.util.*;
import java.util.concurrent.TimeUnit;

/**
 * BaseNodeMon.java
 * <p/>
 * Class <code>BaseNodeMon</code> is the core of the application. It takes care of initializing
 * the service's discovery, messaging and low level monitoring functions.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class BaseNodeMon implements NodeMon, Runnable
{

    public BaseNodeMon ()
    {
        //assertRequiredConfig(GroupManagerService.getDefaultGroupManagerConfigFile()); //REMOVE THIS
        _worldState = BaseWorldState.getInstance();
        _sensors = new HashSet<>();
    }

    public void init () throws IOException, SerializationException
    {

        //verify if forcing unicast
        boolean isForceUnicast = Config.getBooleanValue(Conf.NodeMon.NETWORK_UNICAST_FORCE_ENABLE,
                DefaultValues.NodeMon.NETWORK_UNICAST_FORCE_ENABLE);
        if (isForceUnicast) {
            int unicastPort = Config.getIntegerValue(Conf.NodeMon.NETWORK_UNICAST_PORT,
                    DefaultValues.NodeMon.NETWORK_UNICAST_PORT);
            _unicastService = new UnicastService(this, unicastPort);
            _scheduler = new NodeMonScheduler(this, _unicastService);
        }
        else {
            _discoveryService = new DiscoveryService(this, DiscoveryType.GROUP_MANAGER);
            _scheduler = new NodeMonScheduler(this, _discoveryService);
        }

        _proxyServer = BaseNodeMonProxyServer.getInstance(this);
        _proxyScheduler = new NodeMonProxyScheduler(this, _proxyServer);
        _worldState.init(
                this,
                Config.getStringValue(Conf.GroupManager.NODEUUID, DefaultValues.GroupManager.NODEUUID),
                Config.getStringValue(Conf.GroupManager.NODENAME, DefaultValues.GroupManager.NODENAME),
                _NICIPAddress);

        _nodeToNodeThroughputController = new ThroughputController(
                this,
                _scheduler,
                isForceUnicast ? TransportType.UDP_UNICAST : TransportType.UDP_MULTICAST,
                Config.getIntegerValue(Conf.NodeMon.NETWORK_THROUGHPUT_MTU,
                        DefaultValues.NodeMon.NETWORK_THROUGHPUT_MTU),
                Config.getIntegerValue(Conf.NodeMon.NETWORK_QUEUE_SIZE,
                        DefaultValues.NodeMon.NETWORK_QUEUE_SIZE));
        _proxyThroughputController = new ThroughputController(
                this,
                _proxyScheduler,
                TransportType.TCP,
                Config.getIntegerValue(Conf.NodeMon.PROXY_THROUGHPUT_MAX,
                        DefaultValues.NodeMon.PROXY_THROUGHPUT_MAX),
                50000); //TODO make a config key

        List<ThroughputController> throughputControllers = new ArrayList<>();
        throughputControllers.add(_nodeToNodeThroughputController);
        throughputControllers.add(_proxyThroughputController);
        //TODO group all these controller in one DeliveryManager
        _infoDeliveryController = new InfoDeliveryController(this, throughputControllers);
        _groupDeliveryController = new GroupDeliveryController(this, throughputControllers);
        _topologyDeliveryController = new TopologyDeliveryController(this, throughputControllers);
        _linkDeliveryController = new LinkDeliveryController(this, throughputControllers);
        _worldStateInjector = new WorldStateInjector(this);

        //assert required config before starting threads
        assertRequiredConfig();


        _scheduler.start();
        _proxyScheduler.start();
        _proxyServer.init();
        _proxyServer.start();
        _worldStateInjector.start();
        if (isForceUnicast) _unicastService.start();
        if (!isForceUnicast) _discoveryService.init();
        _nodeToNodeThroughputController.start();
        _proxyThroughputController.start();
        _infoDeliveryController.start();
        _groupDeliveryController.start();
        _topologyDeliveryController.start();
        _linkDeliveryController.start();
        (new Thread(this, "NodeMon")).start();
    }

    private void assertRequiredConfig ()
    {
        boolean isAutoNetworkConfig = Config.getBooleanValue(Conf.NodeMon.NETWORK_CONFIG_AUTO_ENABLE,
                DefaultValues.NodeMon.NETWORK_CONFIG_AUTO_ENABLE);


        String netIFsMode = Config.getStringValue(Conf.GroupManager.NETIFS,
                DefaultValues.GroupManager.NETIFS);
        _NICIPAddress = netIFsMode.split("/")[0];

        Utils.printAllNetworkInterfaces();

        if (isAutoNetworkConfig) {
            String detectedIP = Utils.getPrimaryNetworkInterfaceIP();
            if (detectedIP != null) {
                _NICIPAddress = detectedIP;
                Config.setValue(Conf.GroupManager.NETIFS,
                        _NICIPAddress + "/" + Strings.Symbols.GROUP_MANAGER_UDP_MULTICAST_NORELAY);
            }

            //TODO make this less engineered
            String discoveryConfigAutoFile = Utils.getAutoConfigFile(Config.getStringValue(Conf.NodeMon.CONF_FILE));
            log.info("Writing auto detected network conf to: " + discoveryConfigAutoFile);
            try {
                Config.getProperties().store(new FileOutputStream(new File(discoveryConfigAutoFile)), Strings.Messages
                        .AUTO_CONFIG_WARNING);
            }
            catch (FileNotFoundException e) {
                log.error("Unable to write file: " + discoveryConfigAutoFile);
            }
            catch (IOException e) {
                e.printStackTrace();
            }
        }

        if (_NICIPAddress == null || _NICIPAddress.equals("127.0.0.1")) {
            String err = "Primary NIC not valid: " + _NICIPAddress + ". Configure NodeMon accordingly";
            log.error(err);
            throw new RuntimeException(err);
        }

        printConfig();
    }

    private void printConfig ()
    {
        log.debug(" <====== NodeMon will run with following config =====>");

        Enumeration keys = Config.getProperties().keys();
        while (keys.hasMoreElements()) {
            String key = (String) keys.nextElement();
            String value = (String) Config.getProperties().get(key);
            log.debug("     " + key + ": " + value + "    ");
        }

        log.debug(" <===================================================> ");
    }

    @Override
    public void run ()
    {
        //initial NodeInfo update


        while (!_terminate) {

            updateAllSensors(DataType.INFO);
            updateAllSensors(DataType.LINK);
            updateAllSensors(DataType.TOPOLOGY);


            try {
                TimeUnit.MILLISECONDS.sleep(Config.getIntegerValue(Conf.NodeMon.SENSORS_POLL_INTERVAL,
                        DefaultValues.NodeMon.SENSORS_POLL_INTERVAL));
            }
            catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void updateAllSensors (DataType type)
    {
        for (PolledNodeSensor p : _sensors) {
            p.update(type);
        }
    }

    @Override
    public WorldState getWorldState ()
    {
        return _worldState;
    }

    @Override
    public Scheduler getScheduler ()
    {
        return _scheduler;
    }

    @Override
    public ProxyScheduler getProxyScheduler ()
    {
        return _proxyScheduler;
    }

    @Override
    public void addNodeSensor (PolledNodeSensor nodeSensor)
    {
        _sensors.add(nodeSensor);
        log.debug("Added sensor " + nodeSensor.getClass().getSimpleName());
    }

    @Override
    public void updateData (String nodeId, Container c)
    {
        log.trace("updateData for type " + c.getDataType() + " about nodeId: " + nodeId);

        boolean isDirectLink = Config.getBooleanValue(Conf.NodeMon.PROXY_DIRECT_LINK_ENABLED,
                DefaultValues.NodeMon.PROXY_DIRECT_LINK_ENABLED);
        if (isDirectLink && c.getDataType().equals(DataType.LINK)) {
            //update clients directly if direct link is enabled
            _worldState.updateClients(nodeId, c);
        }

        _worldState.updateData(nodeId, c);
    }


    private DiscoveryService _discoveryService;
    private UnicastService _unicastService;
    private final Set<PolledNodeSensor> _sensors;
    private final WorldState _worldState;

    private Controller _infoDeliveryController;
    private Controller _groupDeliveryController;
    private Controller _topologyDeliveryController;
    private Controller _linkDeliveryController;
    private Controller _worldStateInjector;
    private ThroughputController _nodeToNodeThroughputController;
    private ThroughputController _proxyThroughputController;
    private Scheduler _scheduler;
    private NodeMonProxyServer _proxyServer;
    private ProxyScheduler _proxyScheduler;

    private String _NICIPAddress;
    private volatile boolean _terminate;
    private static final Logger log = Logger.getLogger(BaseNodeMon.class);
}