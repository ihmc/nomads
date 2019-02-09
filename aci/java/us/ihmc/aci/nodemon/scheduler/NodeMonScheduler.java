package us.ihmc.aci.nodemon.scheduler;

import com.google.protobuf.util.TimeUtil;
import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.msg.Messenger;
import us.ihmc.aci.nodemon.proto.ProtoSerializer;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.util.Objects;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * NodeMonScheduler.java
 * <p/>
 * Class <code>NodeMonScheduler</code> defines methods for a module responsible for scheduling message updates about the
 * current WorldState to other nodes.
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class NodeMonScheduler implements Scheduler
{
    public NodeMonScheduler (NodeMon nodeMon, Messenger messenger)
    {
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _messenger = Objects.requireNonNull(messenger, "Messenger can't be null");
        _incoming = new LinkedBlockingDeque<>();
        _isStopped = new AtomicBoolean(false);
        QUEUE_SIZE = Config.getIntegerValue(Conf.NodeMon.NETWORK_QUEUE_SIZE,
                DefaultValues.NodeMon.NETWORK_QUEUE_SIZE);
        MTU = Config.getIntegerValue(Conf.NodeMon.NETWORK_THROUGHPUT_MTU,
                DefaultValues.NodeMon.NETWORK_THROUGHPUT_MTU);
        THROUGHPUT_INTERVAL = Config.getIntegerValue(Conf.NodeMon.NETWORK_THROUGHPUT_INTERVAL,
                DefaultValues.NodeMon.NETWORK_THROUGHPUT_INTERVAL);
        _outgoing = new ArrayBlockingQueue<>(QUEUE_SIZE);
    }

    @Override
    public void start ()
    {
        (new Thread(new Incoming(), "Incoming")).start();
        (new Thread(new Outgoing(), "Outgoing")).start();
    }

    @Override
    public void stop ()
    {
        _isStopped.set(true);
    }

    @Override
    public boolean addOutgoingMessage (Container msg)
    {
        _outgoing.add(msg);
        log.trace("Added message to outgoing queue");
        return true;
    }

    @Override
    public boolean addIncomingMessage (Container msg)
    {
        _incoming.add(msg);
        log.trace("Added message to incoming queue");
        return true;
    }

    class Incoming implements Runnable
    {

        @Override
        public void run ()
        {
            while (!_isStopped.get()) {
                try {
                    Container c = _incoming.poll(Long.MAX_VALUE, TimeUnit.DAYS);
                    log.debug("Received message of size: " + c.getSerializedSize() + " and type: " + c.getDataType());
                    _nodeMon.updateData(c.getDataNodeId(), c);
                }
                catch (InterruptedException e) {
                    log.error("Interrupted exception while polling from incoming queue ", e);
                }
                catch (IndexOutOfBoundsException e) {
                    log.error("Error while deserializing incoming object", e);
                }
            }
        }
    }

    class Outgoing implements Runnable
    {

        @Override
        public void run ()
        {
            int sentDataSize = 0;

            while (!_isStopped.get()) {
                try {

                    if (sentDataSize >= MTU) {
                        TimeUnit.MILLISECONDS.sleep(THROUGHPUT_INTERVAL);
                        sentDataSize = 0;
                    }

                    Container c = _outgoing.poll(Long.MAX_VALUE, TimeUnit.DAYS);

                    if (c.getDataType() == null) {
                        log.warn("Found Container with null DataType, unable to serialize");
                        throw new UnsupportedOperationException();
                    }

                    //rewrite data
                    byte[] data = ProtoSerializer.serialize(c);
                    log.trace("###### Serialized " + c.getDataType() + ". Size: " + data.length);

                    switch (c.getTransportType()) {
                        case UDP_UNICAST:
                            _messenger.broadcastMessage(c.getRecipientId(), data);
                            log.trace("Sent message to destination node: " + c.getRecipientId());
                            break;
                        case UDP_MULTICAST:
                            _messenger.broadcastMessage(c.getRecipientId(), data);
                            log.trace("Broadcast message to destination group: " + c.getRecipientId());
                            break;
                    }

                    log.debug(c.getTransportType() + " -> msg " + c.getDataType()
                            + " of size: " + c.getSerializedSize()
                            + " rfd: " + c.getDataNodeId()
                            + " age: " + TimeUtil.distance(c.getTimestamp(), TimeUtil.getCurrentTime())
                            .getSeconds() + " sec"
                            + " to ->: " + c.getRecipientId()
                            + " QS: " + _outgoing.size() + "/" + QUEUE_SIZE);
                    ProtoUtils.printDetails(c);

                    sentDataSize += data.length;
                }
                catch (InterruptedException e) {
                    log.error("Interrupted exception while polling from outgoing queue ", e);
                }
            }

        }
    }

    private final BlockingQueue<Container> _outgoing;
    private final BlockingDeque<Container> _incoming;
    private final AtomicBoolean _isStopped;
    private final NodeMon _nodeMon;
    private final Messenger _messenger;
    private final int MTU;
    private final int THROUGHPUT_INTERVAL;
    private final int QUEUE_SIZE;

    private static final Logger log = Logger.getLogger(NodeMonScheduler.class);

}
