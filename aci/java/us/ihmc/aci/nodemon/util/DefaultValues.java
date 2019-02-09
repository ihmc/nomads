package us.ihmc.aci.nodemon.util;

/**
 * DefaultValues.java
 * <p/>
 * Class <code>DefaultValues</code> holds default values for class <code>Conf</code> keys.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class DefaultValues
{
    public interface DDAM
    {

    }

    public interface GroupManager
    {
        String NODEUUID = "defaultuuid";
        int PORT = 8500;
        String NETIFS = "127.0.0.1/UDP_MULTICAST";
        String NODENAME = "defaultname";
        String ACIGROUP_NAME = "us.ihmc";
        String ACIGROUP_PASSWORD = "nomads";
        int PING_INTERVAL = 1000;
        int INFO_INTERVAL = 10000;
        int PING_HOPCOUNT = 3;
        int PEERSEARCH_HOPCOUNT = 1;
        int PEERGROUPDATA_UPDATEINTERVAL = 10000;
    }

    public interface NodeMon
    {
        boolean NETWORK_CONFIG_AUTO_ENABLE = true;
        boolean NETWORK_UNICAST_FORCE_ENABLE = false;
        int NETWORK_UNICAST_PORT = 8501;
        boolean NETWORK_LOCAL_ENABLE = true;
        boolean NETWORK_MASTERS_ENABLE = true;
        boolean NETWORK_MASTERS_FORWARD_LOCAL_ENABLE = true;
        int NETWORK_THROUGHPUT_MTU = 1500;
        int NETWORK_THROUGHPUT_INTERVAL = 1000;
        int NETWORK_QUEUE_SIZE = 10000;
        int NETWORK_TIME_WINDOW_SIZE = 600;
        boolean SUPERVISOR_ENABLE = false;
        String DISCOVERY_TYPE = "GroupManager";   //GroupManager
        boolean TOPOLOGY_CONFIG_ENABLE = false;
        String TOPOLOGY_CONFIG_DIR = "topology/config";
        boolean WORLDSTATE_PRELOAD_ENABLE = false;
        String WORLDSTATE_PRELOAD_DIR = "worldstate/preload";
        boolean WORLDSTATE_OUTPUT_ENABLE = true;
        String WORLDSTATE_OUTPUT_DIR = "worldstate/output";
        String WORLDSTATE_MULTICAST_ADDRESSES = "225.0.0.1, 224.0.1.1, 239.0.0.239"; //nodemon, disservice
        boolean GROUPS_CONFIG_ENABLE = false;
        String GROUPS_CONFIG_DIR = "groups/config";
        String GROUPS_LOCAL = "local";
        String GROUPS_MASTERS = "masters";
        boolean GROUPS_IS_MASTER = false;
        boolean SENSORS_DISSERVICE_ENABLE = false;
        boolean SENSORS_NETSENSOR_ENABLE = false;
        int SENSORS_NETSENSOR_QUEUE_SIZE = 10000;
        int SENSORS_NETSENSOR_RESOLUTION = 3000;
        boolean SENSORS_MOCKETS_ENABLE = false;
        boolean SENSORS_NETPROXY_ENABLE = false;
        boolean SENSORS_SNMP_ENABLE = false;
        int SENSORS_DISSERVICE_PORT = 2400;
        int SENSORS_NETSENSOR_PORT = 7777;
        int SENSORS_MOCKETS_PORT = 1400;
        int SENSORS_NETPROXY_PORT = 8755;
        int SENSORS_SNMP_PORT = 162;
        int SENSORS_POLL_INTERVAL = 5000;
        int DELIVERY_INFO_INTERVAL = 10000;
        int DELIVERY_LINK_INTERVAL = 5000;
        int DELIVERY_GROUP_INTERVAL = 5000;
        int DELIVERY_TOPOLOGY_INTERVAL = 5000;
        int MESSENGER_TIMEOUT = 5000;
        int PROXY_PORT = 12345;
        int PROXY_THROUGHPUT_MAX = 32768;
        boolean PROXY_DIRECT_LINK_ENABLED = true;
        int PROXY_THROUGHPUT_INTERVAL = 1000;
        int AGGREGATION_INTERVAL = 3000;
    }
}
