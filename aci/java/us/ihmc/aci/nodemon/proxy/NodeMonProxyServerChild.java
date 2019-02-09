package us.ihmc.aci.nodemon.proxy;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.ddam.MessageType;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.util.serialization.SerializationException;

import java.util.Collection;
import java.util.Objects;

/**
 * NodeMonProxyServerChild.java
 * <p/>
 * Class <code>NodeMonProxyServerChild</code> handles a single connection with a proxy client.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonProxyServerChild implements Runnable, NodeMonProxyListener
{
    /**
     * Constructor
     *
     * @param clientId    id assigned to the client
     * @param commHelper  <code>CommHelper</code> instance used to receive requests from the client
     * @param proxyServer <code>BaseNodeMonProxyServer</code> instance
     * @throws SerializationException if problems occur during the <code>DataSerializer</code> instance
     */
    public NodeMonProxyServerChild (short clientId, CommHelper commHelper, BaseNodeMonProxyServer proxyServer)
            throws SerializationException
    {
        _clientId = clientId;
        _commHelper = Objects.requireNonNull(commHelper, CommHelper.class.getSimpleName() + " cannot be null");
        _proxyServer = Objects.requireNonNull(proxyServer, BaseNodeMonProxyServer.class.getSimpleName() + " cannot be" +
                " " +
                "null");
    }

    /**
     * Starts the main thread and the check alive thread
     */
    void start ()
    {
        _proxyServer.registerNodeMonProxyListener(_clientId);
        (new Thread(this, _clientId + "ProxyServerChildThread")).start();
        (new Thread(new CheckAlive(), _clientId + "ProxyServerChildCheckAliveThread")).start();
    }

    /**
     * Set the <code>CommHelper</code> instance associated to the the callback
     *
     * @param chCallback <code>CommHelper</code> instance associated to the the callback
     */
    public void setCallbackCommHelper (CommHelper chCallback)
    {
        _callbackCommHelper = chCallback;
    }

    @Override
    public void run ()
    {
        String request = null;
        while (true) {
            try {
                request = _commHelper.receiveLine();
                Request rType = Request.valueOf(request);
                boolean success;
                switch (rType) {
                    case worldState:
                        log.info("Received " + rType + " request");
                        success = doWorldState();
                        break;
                    default:
                        log.error(NodeMonProxyErrorHandler.getProtocolError(request));
                        success = false;
                }

                if (!success) {
                    log.error("Failed to execute the operation " + request + " on behalf of client " + _clientId);
                    disconnectChild();
                    break;
                }
            }
            catch (CommException e) {
                log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
                disconnectChild();
                break;
            }
            catch (IllegalArgumentException e) {
                log.error(NodeMonProxyErrorHandler.getProtocolError(request));
                sendErrorToClient();
            }
        }
    }

    /**
     * Executes the world state request.
     *
     * @return true if no problems occur
     */
    private boolean doWorldState ()
    {
        try {

            Container c = Container.newBuilder()
                    .setMessageType(MessageType.UPDATE_WORLD_STATE)
                    .setSenderId(_proxyServer.getNodeMon().getWorldState().getLocalNodeId())
                    .setRecipientId(String.valueOf(_clientId))
                    .setDataType(DataType.NODE)
                    .build();

            _proxyServer.getNodeMon().getProxyScheduler().addOutgoingMessage(c);
            _commHelper.sendLine(Request.confirm.toString());
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
            return false;
        }

        return true;
    }

    /**
     * Disconnects this <code>NodeMonProxyServerChild</code> instance
     */
    private void disconnectChild ()
    {
        _proxyServer.unregisterNodeMonProxyListener(_clientId);
        if (_commHelper.getSocket() != null) {
            _commHelper.closeConnection();
        }
        if (_callbackCommHelper.getSocket() != null) {
            _callbackCommHelper.closeConnection();
        }
    }

    /**
     * Sends an error message to the client
     */
    private void sendErrorToClient ()
    {
        try {
            _commHelper.sendLine(Request.error.toString());
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
        }
    }

    @Override
    public void worldStateUpdate (Collection<Node> worldState)
    {
        if (_callbackCommHelper == null) {
            log.error("Callback channel not registered");
            return;
        }

        if ((worldState == null) || (worldState.isEmpty())) {
            log.error("The world state collection is null or empty");
            return;
        }

        try {
            _callbackCommHelper.sendLine(Request.worldStateUpdate.toString());
            _callbackCommHelper.write32(worldState.size());
            log.debug("Requested world state update, sending size of world: " + worldState.size());


            for (Node n : worldState) {
                log.debug("Serializing Container with Node of size: " + n.getSerializedSize());
                Container c = ProtoUtils.toContainer(DataType.NODE, n.getId(), n);
                byte[] data = ProtoSerializer.serialize(c);
                _callbackCommHelper.write32(data.length);
                log.debug("Requested world state update, sending data of length: " + data.length);
                _callbackCommHelper.sendBlob(data);
            }
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
        }
    }

    @Override
    public void updateData (String nodeId, byte[] data)
    {
        if (_callbackCommHelper == null) {
            log.error("@@@ ERROR #### Callback channel not registered #### ERROR @@@");
            return;
        }

        if ((nodeId == null) || (data == null)) {
            log.error("@@@ ERROR #### Null input parameters for update data #### ERROR @@@");
            return;
        }

        try {
            _callbackCommHelper.sendLine(Request.updateData.toString());
            _callbackCommHelper.sendLine(nodeId);
            _callbackCommHelper.write32(data.length);
            _callbackCommHelper.sendBlob(data);
            log.trace("@@@ SENT data with length: " + data.length + " to proxy client");
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
        }
    }

//    @Override
//    public void newNode (Node n)
//    {
//        if (_callbackCommHelper == null) {
//            log.error("Callback channel not registered");
//            return;
//        }
//
//        if (n == null) {
//            log.error("Null node");
//            return;
//        }
//
//        try {
//            byte[] data = ProtoSerializer.serialize(n, NodeMonDataType.NODE);
//            if (data == null) {
//                throw new SerializationException("Null serialized object");
//            }
//
//            _callbackCommHelper.sendLine(Request.newNode.toString());
//            _callbackCommHelper.write32(data.length);
//            _callbackCommHelper.sendBlob(data);
//        }
//        catch (SerializationException e) {
//            log.error(NodeMonProxyErrorHandler.getSerializationError(_clientId), e);
//        }
//        catch (CommException e) {
//            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
//        }
//    }
//
//    @Override
//    public void deadNode (String nodeId)
//    {
//        if (_callbackCommHelper == null) {
//            log.error("Callback channel not registered");
//            return;
//        }
//
//        if (nodeId == null) {
//            log.error("Null node id");
//            return;
//        }
//
//        try {
//            _callbackCommHelper.sendLine(Request.deadNode.toString());
//            _callbackCommHelper.sendLine(nodeId);
//        }
//        catch (CommException e) {
//            log.error(NodeMonProxyErrorHandler.getServerConnectionError(_clientId), e);
//        }
//    }

    @Override
    public void connectionClosed ()
    {
    }

    /**
     * Thread which checks if the connection with the client is still alive
     */
    class CheckAlive implements Runnable
    {
        @Override
        public void run ()
        {
            while (true) {
                try {
                    Thread.sleep(2000);

                    if (!_commHelper.getSocket().isConnected()) {
                        log.warn("Lost connection between server and client " + _clientId + ", deregistering");
                        disconnectChild();
                        break;
                    }
                }
                catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }


    private final short _clientId;
    private final CommHelper _commHelper;
    private final BaseNodeMonProxyServer _proxyServer;
    private CommHelper _callbackCommHelper;

    private static final Logger log = Logger.getLogger(NodeMonProxyServerChild.class);
}