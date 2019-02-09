package us.ihmc.aci.netviewer.conf;

/**
 * List of properties and default values used to configure the node monitor proxy
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonProxyConf
{
    public static final String SERVER_HOST = "nodemon.proxy.server.host";
    public static final String SERVER_PORT = "nodemon.proxy.server.port";
    public static final String CLIENT_NAME = "client.name";

    // Default values
    public static final String DEFAULT_SERVER_HOST = "127.0.0.1";
    public static final int DEFAULT_SERVER_PORT = 12345;
    public static final String DEFAULT_CLIENT_NAME = "NetViewer";

    /**
     * Constructor
     */
    private NodeMonProxyConf()
    {
        throw new AssertionError();
    }
}
