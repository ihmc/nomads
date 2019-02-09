package us.ihmc.aci.nodemon.proxy;

import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.serialization.SerializationException;

import java.util.Objects;

/**
 * NodeMonProxyServerConnHandler.java
 * <p/>
 * Class <code>NodeMonProxyServerConnHandler</code> is a handler class for multiple connections to the
 * <code>BaseNodeMonProxyServer</code>.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonProxyServerConnHandler implements Runnable
{
    /**
     * Constructor
     * @param proxyServer reference to the <code>BaseNodeMonProxyServer</code> instance
     * @param commHelper reference to the <code>CommHelper</code> instance opened with the client
     */
    NodeMonProxyServerConnHandler (final BaseNodeMonProxyServer proxyServer, final CommHelper commHelper)
    {
        _proxyServer = Objects.requireNonNull (proxyServer, BaseNodeMonProxyServer.class.getSimpleName() + " cannot be null");
        _commHelper = Objects.requireNonNull (commHelper, CommHelper.class.getSimpleName() + " cannot be null");
    }

    /**
     * Starts the thread to accept the connections
     */
    void start()
    {
        Thread thread = new Thread (this);
        thread.setName ("ProxyServerConnHandlerThread");
        thread.start();
    }

    @Override
    public void run()
    {
        String[] request = null;
        try {
            request = _commHelper.receiveParsedSpecific ("1 1 1");
            Request rType = Request.valueOf (request[0]);
            short clientId = Short.valueOf (request[1]);
            String clientName = request[2];

            switch (rType) {
                case registerProxy:
                    doRegisterProxy (clientId, clientName);
                    break;
                case registerProxyCallback:
                    doRegisterProxyCallback (clientId);
                    break;
                default:
                    throw new ProtocolException ("Received " + rType + ", expected " + Request.registerProxy +
                            " or " + Request.registerProxyCallback);
            }
        }
        catch (ProtocolException e) {
            log.error (NodeMonProxyErrorHandler.getProtocolError());
            _commHelper.closeConnection();
        }
        catch (CommException e) {
            log.error (NodeMonProxyErrorHandler.getServerConnectionError (_proxyServer.getPort()), e);
            _commHelper.closeConnection();
        }
        catch (IllegalArgumentException e) {
            log.error (NodeMonProxyErrorHandler.getProtocolError (request[0]));
        }
        catch (SerializationException e) {
            log.error (NodeMonProxyErrorHandler.getSerializationError(), e);
        }
    }

    /**
     * Performs the proxy registration by creating a new <code>NodeMonProxyServerChild</code> for the client
     * @param clientId id of the client that wants to connect
     * @param clientName name of the client that wants to connect
     * @throws SerializationException if problems occur during the <code>DataSerializer</code> instance
     */
    private void doRegisterProxy (short clientId, String clientName) throws SerializationException
    {
        clientId = _proxyServer.getAvailableId (clientId);
        NodeMonProxyServerChild child = new NodeMonProxyServerChild (clientId, _commHelper, _proxyServer);
        _proxyServer.addServerChild (clientId, child);
        _proxyServer.addProxyName (clientId, clientName);

        try {
            _commHelper.sendLine (Request.confirm + " " + clientId);
            log.info ("Registered proxy connection with clientId: " + clientId);
            child.start();
        }
        catch (CommException e) {
            log.error (NodeMonProxyErrorHandler.getServerConnectionError (_proxyServer.getPort()), e);
        }
    }

    /**
     * Performs the proxy callback registration for the client
     * @param clientId id of the client that wants to connect
     */
    private void doRegisterProxyCallback (short clientId)
    {
        NodeMonProxyServerChild serverChild = _proxyServer.getServerChild (clientId);
        try {
            if (serverChild != null) {
                serverChild.setCallbackCommHelper (_commHelper);
                _commHelper.sendLine (Request.confirm.toString());
                log.info ("Registered proxy callback connection with client id: " + clientId);
            }
            else {
                String error = Request.error + ": proxy with id " + clientId + " not found";
                log.error (error);
                _commHelper.sendLine (error);
                _commHelper.closeConnection();
            }
        }
        catch (CommException e) {
            log.error (NodeMonProxyErrorHandler.getServerConnectionError (_proxyServer.getPort()), e);
        }
    }



    private final BaseNodeMonProxyServer _proxyServer;
    private final CommHelper _commHelper;

    private static final Logger log = Logger.getLogger (NodeMonProxyServerConnHandler.class);
}