package us.ihmc.aci.nodemon.util;

/**
 * Conf.java
 * <p/>
 * Class <code>Conf</code> contains configuration properties useful throughout the <code>NodeMon</code> context.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class Conf
{

    public interface DDAM
    {

    }

    public interface GroupManager
    {
        String NODEUUID = "aci.groupmanager.nodeUUID";
        String PORT = "aci.groupmanager.port";
        String NETIFS = "aci.groupmanager.netIFs";
        String NODENAME = "aci.groupmanager.nodeName";
        String ACIGROUP_NAME = "aci.groupmanager.acigroup.name";
        String ACIGROUP_PASSWORD = "aci.groupmanager.acigroup.password";
        String PING_INTERVAL = "aci.groupmanager.ping.interval";
        String INFO_INTERVAL = "aci.groupmanager.info.interval";
        String PING_HOPCOUNT = "aci.groupmanager.ping.hopcount";
        String PEERSEARCH_HOPCOUNT = "aci.groupmanager.peersearch.hopcount";
        String PEERGROUPDATA_UPDATEINTERVAL = "aci.groupmanager.peergroupdata.updateinterval";
    }

    public interface NodeMon
    {
        String CONF_FILE = "aci.nodemon.conf.file";
        String LOG_DIR = "aci.nodemon.log.dir";
        String NETWORK_CONFIG_AUTO_ENABLE = "aci.nodemon.network.config.auto.enable";
        String NETWORK_UNICAST_FORCE_ENABLE = "aci.nodemon.network.unicast.force.enable";
        String NETWORK_UNICAST_PORT = "aci.nodemon.network.unicast.port";
        String NETWORK_QUEUE_SIZE = "aci.nodemon.network.queue.size";
        String NETWORK_THROUGHPUT_MTU = "aci.nodemon.network.throughput.mtu";
        String NETWORK_THROUGHPUT_INTERVAL = "aci.nodemon.network.throughput.interval";
        String NETWORK_LOCAL_ENABLE = "aci.nodemon.network.local.enable";
        String NETWORK_MASTERS_ENABLE = "aci.nodemon.network.masters.enable";
        String NETWORK_MASTERS_FORWARD_LOCAL_ENABLE = "aci.nodemon.network.masters.forward.local.enable";
        String NETWORK_TIME_WINDOW_SIZE = "aci.nodemon.network.time.window.size";
        String SUPERVISOR_ENABLE = "aci.nodemon.supervisor.enable";
        String DISCOVERY_TYPE = "aci.nodemon.discovery.type";   //GroupManager
        String TOPOLOGY_CONFIG_ENABLE = "aci.nodemon.topology.config.enable";
        String TOPOLOGY_CONFIG_DIR = "aci.nodemon.topology.config.dir";
        String WORLDSTATE_PRELOAD_ENABLE = "aci.nodemon.worldstate.preload.enable";
        String WORLDSTATE_PRELOAD_DIR = "aci.nodemon.worldstate.preload.dir";
        String WORLDSTATE_OUTPUT_ENABLE = "aci.nodemon.worldstate.output.enable";
        String WORLDSTATE_OUTPUT_DIR = "aci.nodemon.worldstate.output.dir";
        String WORLDSTATE_MULTICAST_ADDRESSES = "aci.nodemon.worldstate.multicast.addresses";
        String GROUPS_CONFIG_ENABLE = "aci.nodemon.groups.config.enable";
        String GROUPS_CONFIG_DIR = "aci.nodemon.groups.config.dir";
        String GROUPS_LOCAL = "aci.nodemon.groups.local";
        String GROUPS_MASTERS = "aci.nodemon.groups.masters";
        String GROUPS_IS_MASTER = "aci.nodemon.groups.isMaster";
        String SENSORS_DISSERVICE_ENABLE = "aci.nodemon.sensors.disservice.enable";
        String SENSORS_NETSENSOR_ENABLE = "aci.nodemon.sensors.netsensor.enable";
        String SENSORS_NETSENSOR_QUEUE_SIZE = "aci.nodemon.sensors.netsensor.queue.size";
        String SENSORS_MOCKETS_ENABLE = "aci.nodemon.sensors.mockets.enable";
        String SENSORS_NETPROXY_ENABLE = "aci.nodemon.sensors.netproxy.enable";
        String SENSORS_SNMP_ENABLE = "aci.nodemon.sensors.snmp.enable";
        String SENSORS_DISSERVICE_PORT = "aci.nodemon.sensors.disservice.port";
        String SENSORS_NETSENSOR_PORT = "aci.nodemon.sensors.netsensor.port";
        String SENSORS_NETSENSOR_RESOLUTION = "aci.nodemon.sensors.netsensor.resolution";
        String SENSORS_MOCKETS_PORT = "aci.nodemon.sensors.mockets.port";
        String SENSORS_NETPROXY_PORT = "aci.nodemon.sensors.netproxy.port";
        String SENSORS_SNMP_PORT = "aci.nodemon.sensors.snmp.port";
        String SENSORS_POLL_INTERVAL = "aci.nodemon.sensors.poll.interval";
        String MESSENGER_TIMEOUT = "aci.nodemon.messenger.timeout";
        String PROXY_PORT = "aci.nodemon.proxy.port";
        String PROXY_THROUGHPUT_MAX = "aci.nodemon.proxy.throughput.max";
        String PROXY_THROUGHPUT_INTERVAL = "aci.nodemon.proxy.throughput.interval";
        String PROXY_DIRECT_LINK_ENABLED = "aci.nodemon.proxy.direct.link.enabled";
        String AGGREGATION_INTERVAL = "aci.nodemon.aggregation.interval";
        String DELIVERY_GROUP_INTERVAL = "aci.nodemon.delivery.group.interval";
        String DELIVERY_TOPOLOGY_INTERVAL = "aci.nodemon.delivery.topology.interval";
        String DELIVERY_INFO_INTERVAL = "aci.nodemon.delivery.info.interval";
        String DELIVERY_LINK_INTERVAL = "aci.nodemon.delivery.link.interval";
    }
}
