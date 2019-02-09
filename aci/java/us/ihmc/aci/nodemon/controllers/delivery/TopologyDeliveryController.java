package us.ihmc.aci.nodemon.controllers.delivery;

import org.apache.log4j.Logger;
import us.ihmc.aci.ddam.*;
import us.ihmc.aci.nodemon.NodeMon;
import us.ihmc.aci.nodemon.controllers.throughput.ThroughputController;
import us.ihmc.aci.nodemon.proto.ProtoUtils;
import us.ihmc.aci.nodemon.util.Conf;
import us.ihmc.aci.nodemon.util.DefaultValues;
import us.ihmc.util.Config;

import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * TopologyDeliveryController.java
 */
public class TopologyDeliveryController extends DeliveryController
{
    public TopologyDeliveryController (NodeMon nodeMon, List<ThroughputController> controllers)
    {
        _clazz = TopologyDeliveryController.class;
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _throughputControllers = Objects.requireNonNull(controllers, "ThroughputControllers can't be null");
        log = Logger.getLogger(_clazz);
        _isRunning = new AtomicBoolean(false);
        _deliveryInterval = Config.getIntegerValue(Conf.NodeMon.DELIVERY_TOPOLOGY_INTERVAL,
                DefaultValues.NodeMon.DELIVERY_TOPOLOGY_INTERVAL);
        _isMaster = Config.getBooleanValue(Conf.NodeMon.GROUPS_IS_MASTER,
                DefaultValues.NodeMon.GROUPS_IS_MASTER);
        _localGroup = Config.getStringValue(Conf.NodeMon.GROUPS_LOCAL,
                DefaultValues.NodeMon.GROUPS_LOCAL);
        _mastersGroup = Config.getStringValue(Conf.NodeMon.GROUPS_MASTERS,
                DefaultValues.NodeMon.GROUPS_MASTERS);
        _isMasterForwardingLocals = Config.getBooleanValue(Conf.NodeMon.NETWORK_MASTERS_FORWARD_LOCAL_ENABLE,
                DefaultValues.NodeMon.NETWORK_MASTERS_FORWARD_LOCAL_ENABLE);
    }

    @Override
    protected Collection<Container> getData (Node node)
    {
        // equivalent to this in JDK 8
        // List<GeneratedMessage> elements = node.getTopology().values().stream().collect(Collectors.toList());
        //

        List<Container> elements = new ArrayList<>();
        if (_isMaster && _isMasterForwardingLocals) {
            //deliver topology of the other local nodes as well
            Group localGroup = node.getGrump().getGroups().get(_localGroup);
            if (localGroup == null) {
                String error = "Local group is null, check group configuration";
                log.error(error);
                throw new RuntimeException(error);
            }

            Map<String, Node> nodeMap = _nodeMon.getWorldState().getNodesMapCopy();
            for (String nodeId : localGroup.getMembers().keySet()) {
                Node currentNode = nodeMap.get(nodeId);
                if (currentNode == null) {
                    continue;
                }
                for (Topology t : currentNode.getTopology().values()) {
                    Container cm = ProtoUtils.toContainer(getType(), currentNode.getId(), t);
                    elements.add(cm);
                }
            }
        }
        else {
            for (Topology t : node.getTopology().values()) {
                Container cm = ProtoUtils.toContainer(getType(), node.getId(), t);
                elements.add(cm);
            }
        }

        return elements;
    }

    @Override
    protected DataType getType ()
    {
        return DataType.TOPOLOGY;
    }
}
