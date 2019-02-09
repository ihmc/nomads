package us.ihmc.aci.nodemon.controllers.delivery;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.Container;
import us.ihmc.aci.ddam.DataType;
import us.ihmc.aci.ddam.Group;
import us.ihmc.aci.ddam.Node;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.controllers.throughput.ThroughputController;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * GroupDeliveryController.java
 */
public class GroupDeliveryController extends DeliveryController
{
    public GroupDeliveryController (NodeMon nodeMon, List<ThroughputController> controllers)
    {

        _clazz = GroupDeliveryController.class;
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _throughputControllers = Objects.requireNonNull(controllers, "ThroughputControllers can't be null");
        log = Logger.getLogger(_clazz);
        _isRunning = new AtomicBoolean(false);
        _deliveryInterval = Config.getIntegerValue(Conf.NodeMon.DELIVERY_GROUP_INTERVAL,
                DefaultValues.NodeMon.DELIVERY_GROUP_INTERVAL);
    }

    @Override
    protected Collection<Container> getData (Node node)
    {
        List<Container> elements = new ArrayList<>();
        for (Group group : node.getGrump().getGroups().values()) {

            if (group.getMembers().isEmpty()) {
                //empy info, don't send
                log.warn("!!! Found empty GROUP, not sending");
                continue;
            }

            elements.add(ProtoUtils.toContainer(getType(), node.getId(), group));
        }

        return elements;
    }

    @Override
    protected DataType getType ()
    {
        return DataType.GROUP;
    }
}
