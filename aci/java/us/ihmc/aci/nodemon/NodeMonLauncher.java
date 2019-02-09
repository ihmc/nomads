package us.ihmc.aci.nodemon;

import org.apache.log4j.Logger;
import us.ihmc.aci.nodemon.sensors.custom.DisServicePolledNodeSensor;
import us.ihmc.aci.nodemon.sensors.custom.MocketsPolledNodeSensor;
import us.ihmc.aci.nodemon.sensors.custom.NetSensorPolledNodeSensor;
import us.ihmc.aci.nodemon.sensors.custom.SnmpPolledNodeSensor;
import us.ihmc.aci.nodemon.sensors.info.SigarPolledNodeSensor;
import us.ihmc.aci.nodemon.util.*;
import us.ihmc.util.Config;
import us.ihmc.util.serialization.SerializationException;

import java.io.File;
import java.io.IOException;
import java.net.SocketException;

/**
 * NodeMonLauncher.java
 * <p/>
 * Class <code>NodeMonLauncher</code> takes care of launching the <code>NodeMon</code> service.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NodeMonLauncher
{
    public NodeMonLauncher (String nodeMonConf, String log4jConf, String logDir) throws
            IOException
    {
        if (nodeMonConf == null) {
            throw new IOException("NodeMon configuration file not found");
        }

        _nodeMonConf = nodeMonConf;
        _log4JConf = log4jConf;
        _logDir = logDir;

        Config.loadConfig(_nodeMonConf);
        Config.setValue(Conf.NodeMon.LOG_DIR, _logDir);
        Config.setValue(Conf.NodeMon.CONF_FILE, _nodeMonConf);

        ///DETECT architecture (e.g., possible values: armv71, i686, arch64, x86_64)
        String arch = System.getProperty("os.arch");
        boolean isAndroid = arch.contains("arm");
        if (!isAndroid) {
            Utils.initLogger(_log4JConf, _logDir);
        }
        log.info("+++ System architecture is: " + arch + " +++");

        _nodemon = new BaseNodeMon();

        try {
            //SENSORS
            boolean isDisServiceEnabled = Config.getBooleanValue(Conf.NodeMon.SENSORS_DISSERVICE_ENABLE,
                    DefaultValues.NodeMon.SENSORS_DISSERVICE_ENABLE);
            boolean isNetSensorEnabled = Config.getBooleanValue(Conf.NodeMon.SENSORS_NETSENSOR_ENABLE,
                    DefaultValues.NodeMon.SENSORS_NETSENSOR_ENABLE);
            boolean isMocketsEnabled = Config.getBooleanValue(Conf.NodeMon.SENSORS_MOCKETS_ENABLE,
                    DefaultValues.NodeMon.SENSORS_MOCKETS_ENABLE);
            boolean isNetProxyEnabled = Config.getBooleanValue(Conf.NodeMon.SENSORS_NETPROXY_ENABLE,
                    DefaultValues.NodeMon.SENSORS_NETPROXY_ENABLE);
            boolean isSnmpEnabled = Config.getBooleanValue(Conf.NodeMon.SENSORS_SNMP_ENABLE,
                    DefaultValues.NodeMon.SENSORS_SNMP_ENABLE);

            if (!isAndroid) {
                _nodemon.addNodeSensor(new SigarPolledNodeSensor(_nodemon));
                log.info("Added Sigar Hardware sensor to NodeMon");
            }

            //TODO temporarily disabled, add libraries
            if (isDisServiceEnabled) {
                _nodemon.addNodeSensor(new DisServicePolledNodeSensor(_nodemon,
                        Config.getIntegerValue(Conf.NodeMon.SENSORS_DISSERVICE_PORT,
                                DefaultValues.NodeMon.SENSORS_DISSERVICE_PORT)));
                log.info("Added DisService sensor to NodeMon");
            }

            if (isNetSensorEnabled) {
                _nodemon.addNodeSensor(new NetSensorPolledNodeSensor(_nodemon,
                        Config.getIntegerValue(Conf.NodeMon.SENSORS_NETSENSOR_PORT,
                                DefaultValues.NodeMon.SENSORS_NETSENSOR_PORT)));
                log.info("Added NetSensor sensor to NodeMon");
            }

            if (isMocketsEnabled) {
                _nodemon.addNodeSensor(new MocketsPolledNodeSensor(_nodemon,
                        Config.getIntegerValue(Conf.NodeMon.SENSORS_MOCKETS_PORT,
                                DefaultValues.NodeMon.SENSORS_MOCKETS_PORT)));
                log.info("Added Mockets sensor to NodeMon");
            }

            if (isSnmpEnabled) {
                _nodemon.addNodeSensor(new SnmpPolledNodeSensor(_nodemon,
                        Config.getIntegerValue(Conf.NodeMon.SENSORS_SNMP_PORT,
                                DefaultValues.NodeMon.SENSORS_SNMP_PORT)));
                log.info("Added SNMP sensor to NodeMon");
            }
        }
        catch (SocketException e) {
            log.error("Unable to initialize NodeMon's custom sensors", e);
        }
        catch (IOException e) {
            log.error("Unable to initialize NodeMon's discovery service.", e);
        }
    }

    public NodeMon getNodeMon ()
    {
        return _nodemon;
    }


    public static void main (String[] args)
    {
        String nodemonConf;
        String log4Jconf;
        String logDir;
        try {
            nodemonConf = args[0]; // NodeMon conf
            log4Jconf = args[1]; //Log4J conf
            logDir = args[2]; //logs dir

        }
        catch (ArrayIndexOutOfBoundsException e) {
            log.error("Configuration file is missing\n", e);
            return;
        }

        NodeMonLauncher launcher = null;
        try {
            launcher = new NodeMonLauncher(nodemonConf, log4Jconf, logDir);
            launcher.startup();
            addShutdownHook(launcher.getNodeMon());
        }
        catch (SocketException e) {
            log.error("Unable to initialize NodeMon's custom sensors", e);
        }
        catch (IOException e) {
            log.error("Unable to initialize NodeMon's discovery service.", e);
        }
    }

    public void startup ()
    {
        try {
            _nodemon.init();    //only GroupManager
            // supported
            // for now
        }
        catch (IOException e) {
            log.error("Unable to initialize NodeMon's discovery service.", e);
        }
        catch (SerializationException e) {
            log.error("Unable to initialize NodeMon's serializer.", e);
        }
    }

    public void stop (String logDir)
    {
        log.warn("Writing WorldState to JSON...");
        Utils.writeWorldStateToJSON(_nodemon,
                logDir + File.separator + Config.getStringValue(
                        Conf.NodeMon.WORLDSTATE_OUTPUT_DIR,
                        DefaultValues.NodeMon.WORLDSTATE_OUTPUT_DIR));
        log.warn("Exiting NodeMon...");
        System.exit(0);
    }

    public static void addShutdownHook (final NodeMon nodeMon)
    {
        Runtime.getRuntime().addShutdownHook(new Thread(new Runnable()
        {
            @Override
            public void run ()
            {
                log.debug("Dumping world state to JSON");

                //String world = ((BaseWorldState) nodeMon.getWorldState()).toJSON();
                Utils.printWorldState(nodeMon);

                if (Config.getBooleanValue(
                        Conf.NodeMon.WORLDSTATE_OUTPUT_ENABLE,
                        DefaultValues.NodeMon.WORLDSTATE_OUTPUT_ENABLE)) {

                    Utils.writeWorldStateToJSON(nodeMon,
                            _logDir + File.separator + Config.getStringValue(
                                    Conf.NodeMon.WORLDSTATE_OUTPUT_DIR,
                                    DefaultValues.NodeMon.WORLDSTATE_OUTPUT_DIR));

                    Utils.writeWorldStateToFile(nodeMon,
                            _logDir + File.separator + Config.getStringValue(
                                    Conf.NodeMon.WORLDSTATE_OUTPUT_DIR,
                                    DefaultValues.NodeMon.WORLDSTATE_OUTPUT_DIR));
                }
            }
        }));
    }

    private final NodeMon _nodemon;
    private static String _nodeMonConf = null;
    private static String _log4JConf = null;
    private static String _logDir = null;

    private static final Logger log = Logger.getLogger(NodeMonLauncher.class);
}
