package us.ihmc.aci.nodemon.proxy;

import org.apache.log4j.Logger;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.Config;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;

/**
 * <code>BaseNodeMonProxyServer.java</code>
 * <p/>
 * Class <code>BaseNodeMonProxyServer</code> handles connections with (potentially) multiple proxy clients.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class BaseNodeMonProxyServer implements Runnable, NodeMonProxyServer
{
    /**
     * Constructor
     *
     * @param nodeMon <code>NodMon</code> instance
     */
    private BaseNodeMonProxyServer (NodeMon nodeMon)
    {
        _port = Config.getIntegerValue(Conf.NodeMon.PROXY_PORT, DefaultValues.NodeMon.PROXY_PORT);
        _proxies = new ConcurrentHashMap<>();
        _proxiesNames = new ConcurrentHashMap<>();
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon instance cannot be null");
    }

    /**
     * Creates or retrieves the <code>BaseNodeMonProxyServer</code> instance
     *
     * @param nodeMon <code>ProxyScheduler</code> instance
     * @return the <code>BaseNodeMonProxyServer</code> instance
     */
    public static BaseNodeMonProxyServer getInstance (NodeMon nodeMon)
    {
        if (INSTANCE == null) {
            INSTANCE = new BaseNodeMonProxyServer(nodeMon);
        }

        return INSTANCE;
    }

    /**
     * Initializes the server socket.
     *
     * @return true if the initialization ended without any error
     */
    @Override
    public void init () throws IOException
    {
        try {
            log.info("Initializing node monitor proxy server on port " + _port);
            _serverSocket = new ServerSocket(_port);
        }
        catch (IOException e) {
            log.error(NodeMonProxyErrorHandler.getBoundError(_port), e);
            throw new IOException(e);
        }
    }

    /**
     * Starts the main threads that accepts clients connections
     */
    @Override
    public void start ()
    {
        Thread federationProxyServerThread = new Thread(this);
        federationProxyServerThread.setName("ProxyServerThread");
        federationProxyServerThread.start();
    }

    @Override
    public void run ()
    {
        while (!Thread.currentThread().isInterrupted()) {
            if (_serverSocket == null) {
                log.error("The server socket is still null");
                continue;
            }

            try {
                Socket socket = _serverSocket.accept();
                socket.setTcpNoDelay(true);
                log.info("Connection accepted from client");
                NodeMonProxyServerConnHandler connHandler = new NodeMonProxyServerConnHandler(this, new CommHelper
                        (socket));
                connHandler.start();
            }
            catch (IOException e) {
                log.error(NodeMonProxyErrorHandler.getServerConnectionError(_port), e);
            }
        }
    }

    /**
     * Gets the current <code>ProxyScheduler</code>
     *
     * @return an instance of the <code>ProxyScheduler</code>.
     */
    NodeMon getNodeMon ()
    {
        return _nodeMon;
    }

    /**
     * Gets the binding port
     *
     * @return binding port value
     */
    int getPort ()
    {
        return _port;
    }

    /**
     * Gets the first available client id starting from the suggested one
     *
     * @param suggestedId suggested client id
     * @return the client id
     */
    synchronized short getAvailableId (short suggestedId)
    {
        short clientId = suggestedId;
        while (_proxies.containsKey(clientId)) {
            clientId++;
        }

        return clientId;
    }

    /**
     * Adds a new proxy server child
     *
     * @param clientId client id
     * @param child    proxy server child instance
     */
    synchronized void addServerChild (short clientId, NodeMonProxyServerChild child)
    {
        _proxies.put(clientId, child);
    }

    /**
     * Adds a new proxy server child name
     *
     * @param clientId   client id
     * @param clientName client name
     */
    synchronized void addProxyName (short clientId, String clientName)
    {
        if (!_proxiesNames.containsKey(clientId)) {
            _proxiesNames.put(clientId, clientName);
        }
        else {
            log.warn("The map _proxiesNames already contains the client id " + clientId);
        }
    }

    /**
     * Retrieves the <code>NodeMonProxyServerChild</code> associated to the given client id
     *
     * @param clientId client id to look for
     * @return the <code>NodeMonProxyServerChild</code> associated to the client
     */
    @Override
    public NodeMonProxyServerChild getServerChild (short clientId)
    {
        return _proxies.get(clientId);
    }

    /**
     * Retrieves the size (number) of the current children.
     *
     * @return the number of current children.
     */
    @Override
    public int getChildrenSize ()
    {
        return _proxies.size();
    }

    /**
     * Registers a server child to receive updates
     *
     * @param clientId client id
     */
    void registerNodeMonProxyListener (short clientId)
    {
        _nodeMon.getProxyScheduler().addNodeMonProxyListener(_proxies.get(clientId));
    }

    /**
     * Unregisters a server child
     *
     * @param clientId client id
     */
    synchronized void unregisterNodeMonProxyListener (short clientId)
    {
        _nodeMon.getProxyScheduler().removeNodeMonProxyListener(_proxies.get(clientId));
        _proxiesNames.remove(clientId);
        _proxies.remove(clientId);
    }


    private static BaseNodeMonProxyServer INSTANCE;
    //private static final int DEFAULT_PORT = 12345;
    private final int _port;
    private final Map<Short, NodeMonProxyServerChild> _proxies;
    private final Map<Short, String> _proxiesNames;
    private final NodeMon _nodeMon;
    private ServerSocket _serverSocket;

    private static final Logger log = Logger.getLogger(BaseNodeMonProxyServer.class);
}
