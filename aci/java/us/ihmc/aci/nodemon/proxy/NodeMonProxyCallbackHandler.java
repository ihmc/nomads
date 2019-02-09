package us.ihmc.aci.nodemon.proxy;

import com.google.protobuf.InvalidProtocolBufferException;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.comm.CommException;
import us.ihmc.comm.CommHelper;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.serialization.SerializationException;

import java.util.HashSet;
import java.util.Set;

/**
 * NodeMonProxyCallbackHandler.java
 * <p/>
 * Class <code>NodeMonProxyCallbackHandler</code> handles messages received from the
 * <code>NodeMonProxyServerChild</code>
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class NodeMonProxyCallbackHandler implements Runnable
{
    /**
     * @param proxy
     * @param commHelper
     * @throws SerializationException if problems occur during the <code>DataSerializer</code> instance
     */
    NodeMonProxyCallbackHandler (NodeMonProxy proxy, CommHelper commHelper) throws SerializationException
    {
        _proxy = proxy;
        _commHelper = commHelper;
    }

    @Override
    public void run ()
    {
        Thread.currentThread().setPriority(Thread.MAX_PRIORITY);

        while (true) {
            String requestCallback = null;
            try {
                requestCallback = _commHelper.receiveLine();
                log.debug("Request callback is: " + requestCallback);
                Request request = Request.valueOf(requestCallback);

                switch (request) {
                    case worldStateUpdate:
                        doWorldStateUpdate();
                        break;
                    case updateData:
                        doUpdateData();
                        break;
                }
            }
            catch (CommException e) {
                log.error(NodeMonProxyErrorHandler.getConnectionError(), e);
                if (_proxy instanceof BaseNodeMonProxy) {
                    ((BaseNodeMonProxy) _proxy).notifyCallbackHandlerStop();
                }
                break; //terminate
            }
            catch (IllegalArgumentException e) {
                if (requestCallback != null) {
                    log.error(NodeMonProxyErrorHandler.getProtocolError(requestCallback));
                }
                else {
                    log.error(NodeMonProxyErrorHandler.getProtocolError("receiveParsed()"));
                }
                if (_proxy instanceof BaseNodeMonProxy) ((BaseNodeMonProxy) _proxy).notifyCallbackHandlerStop();
                break; //terminate
            }
        }

    }

    /**
     * Performs the worldStateUpdate callback by receiving the messages containing the <code>NODE</code>
     * from the <code>NodeMonProxyServerChild</code> and then sending them to the client
     */
    private void doWorldStateUpdate ()
    {
        try {
            int nNodes = _commHelper.read32();
            log.debug("Number of nodes: " + nNodes);
            Set<Node> worldState = new HashSet<>();
            for (int i = 0; i < nNodes; i++) {
                int dataSize = _commHelper.read32();
                log.debug("Data size: " + dataSize);
                byte[] data = _commHelper.receiveBlob(dataSize);
                Container c = ProtoSerializer.deserialize(data);
                if (c == null) {
                    continue;
                }
                Node n = c.getNode();
                if (n == null) {
                    continue;
                }

                worldState.add(n);
            }

            if (_proxy instanceof BaseNodeMonProxy) {
                ((BaseNodeMonProxy) _proxy).worldStateUpdate(worldState);
            }
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getConnectionError(), e);
        }
        catch (ProtocolException e) {
            log.error(NodeMonProxyErrorHandler.getProtocolError(), e);
        }
        catch (InvalidProtocolBufferException e) {
            log.error(NodeMonProxyErrorHandler.getSerializationError(), e);
        }
    }

    /**
     * Performs the updateData callback by receiving the messages containing the updated data
     * from the <code>NodeMonProxyServerChild</code> and then sending them to the client
     */
    private void doUpdateData ()
    {
        try {
            String nodeId = _commHelper.receiveLine();
            int dataLength = _commHelper.read32();
            byte[] data = _commHelper.receiveBlob(dataLength);

            if (_proxy instanceof BaseNodeMonProxy) {
                ((BaseNodeMonProxy) _proxy).updateData(nodeId, data);
            }
        }
        catch (CommException e) {
            log.error(NodeMonProxyErrorHandler.getConnectionError(), e);
        }
        catch (ProtocolException e) {
            log.error(NodeMonProxyErrorHandler.getProtocolError(), e);
        }

    }


    private final NodeMonProxy _proxy;
    private final CommHelper _commHelper;

    private static final Logger log = Logger.getLogger(NodeMonProxyCallbackHandler.class);
}
