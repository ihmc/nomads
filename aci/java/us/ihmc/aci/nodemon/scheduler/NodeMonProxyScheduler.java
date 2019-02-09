package us.ihmc.aci.nodemon.scheduler;

import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyListener;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyServer;
import us.ihmc.aci.nodemon.proxy.NodeMonProxyServerChild;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * NodeMonProxyScheduler.java
 * <p/>
 * Class <code>NodeMonProxyScheduler</code> defines methods for a module responsible for scheduling message updates
 * about the
 * current WorldState to other nodes to the proxy server.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NodeMonProxyScheduler implements ProxyScheduler
{
    public NodeMonProxyScheduler (NodeMon nodeMon, NodeMonProxyServer proxyServer)
    {

        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _proxyServer = Objects.requireNonNull(proxyServer, "NodeMonProxyServer can't be null");
        _outgoing = new LinkedBlockingDeque<>();
        _isStopped = new AtomicBoolean(true);
        _proxyListeners = new ArrayList<>();
    }


    @Override
    public int getProxyListenersCount ()
    {
        return _proxyListeners.size();
    }

    @Override
    public void start ()
    {
        _isStopped.set(false);
        (new Thread(new OutgoingProxy(), "OutgoingProxy")).start();
    }

    @Override
    public void stop ()
    {
        _isStopped.set(true);
    }

    @Override
    public void addNodeMonProxyListener (NodeMonProxyListener listener)
    {
        synchronized (_proxyListeners) {
            if (!_proxyListeners.contains(listener)) {
                _proxyListeners.add(listener);
                log.debug("Added NodeMonProxyListener");
            }
        }
    }

    @Override
    public void removeNodeMonProxyListener (NodeMonProxyListener listener)
    {
        synchronized (_proxyListeners) {
            _proxyListeners.remove(listener);
            log.debug("Removed NodeMonProxyListener");
        }
    }

    class OutgoingProxy implements Runnable
    {

        @Override
        public void run ()
        {
            while (!_isStopped.get()) {

                try {
                    Container c = _outgoing.poll(Long.MAX_VALUE, TimeUnit.DAYS);
                    MessageType msgType = c.getMessageType();

                    if (msgType == null) {
                        throw new RuntimeException("Unable to deliver content to client without MessageType, stop");
                    }

                    Node n;
                    switch (msgType) {
                        case UPDATE_DATA:

                            log.debug(c.getTransportType() + " -> msg " + c.getDataType()
                                    + " of size: " + c.getSerializedSize()
                                    + " rfd: " + c.getDataNodeId()
                                    + " age: " + TimeUtil.distance(c.getTimestamp(), TimeUtil.getCurrentTime())
                                    .getSeconds() + " sec"
                                    + " QS: " + _outgoing.size() + "/N");
                            ProtoUtils.printDetails(c);

                            byte[] data = ProtoSerializer.serialize(c);
                            synchronized (_proxyListeners) {
                                for (NodeMonProxyListener listener : _proxyListeners) {

                                    //TODO evaluate whether changing it to dataNodeId
//                                    String nodeId = c.getSenderId();
//                                    if (nodeId == null || nodeId.equals("")) {
//                                        nodeId = c.getDataNodeId();
//                                    }

                                    listener.updateData(c.getDataNodeId(), data);
                                }
                            }
                            break;
                        case UPDATE_WORLD_STATE:
                            String clientId = c.getRecipientId();
                            if (clientId == null || clientId.equals("")) {
                                throw new RuntimeException("Unable to deliver content to client without client id, " +
                                        "stop");
                            }

                            NodeMonProxyServerChild child = _proxyServer.getServerChild(Short.valueOf(clientId));
                            if (child == null) {
                                throw new RuntimeException("NodeMonProxyServer child is null");
                            }
                            child.worldStateUpdate(_nodeMon.getWorldState().getNodesMapCopy().values());
                            break;
                        default:
                            throw new UnsupportedOperationException("Unsupported MessageType");
                    }
                }
                catch (Exception e) {
                    log.error("Interrupted exception while polling from outgoing queue ", e);
                }
            }
        }
    }


    @Override
    public boolean addOutgoingMessage (Container msg)
    {
        if (_proxyServer.getChildrenSize() == 0) {
            log.trace("No clients connected, dropping message");
            return false;
        }
        _outgoing.add(msg);
        log.trace("Added message to outgoing queue");
        return true;
    }

    @Override
    public boolean addIncomingMessage (Container msg)
    {
        throw new UnsupportedOperationException("Unable to add incoming message to proxy schedule");
    }

    private final BlockingDeque<Container> _outgoing;
    private final AtomicBoolean _isStopped;
    private final NodeMon _nodeMon;
    private final NodeMonProxyServer _proxyServer;
    private final List<NodeMonProxyListener> _proxyListeners;

    private static final Logger log = Logger.getLogger(NodeMonProxyScheduler.class);
}
