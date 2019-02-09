package us.ihmc.aci.nodemon.controllers.delivery;

import com.google.protobuf.Duration;
import com.google.protobuf.util.TimeUtil;
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
 * LinkDeliveryController.java
 *
 * @author Enrico Casini (ecasini@ihmc.us)
 */
public class LinkDeliveryController extends DeliveryController
{
    public LinkDeliveryController (NodeMon nodeMon, List<ThroughputController> controllers)
    {
        _clazz = LinkDeliveryController.class;
        _nodeMon = Objects.requireNonNull(nodeMon, "NodeMon can't be null");
        _throughputControllers = Objects.requireNonNull(controllers, "ThroughputControllers can't be null");
        log = Logger.getLogger(_clazz);
        _isRunning = new AtomicBoolean(false);
        _deliveryInterval = Config.getIntegerValue(Conf.NodeMon.DELIVERY_LINK_INTERVAL,
                DefaultValues.NodeMon.DELIVERY_LINK_INTERVAL);
        _isMaster = Config.getBooleanValue(Conf.NodeMon.GROUPS_IS_MASTER,
                DefaultValues.NodeMon.GROUPS_IS_MASTER);
        _localGroup = Config.getStringValue(Conf.NodeMon.GROUPS_LOCAL,
                DefaultValues.NodeMon.GROUPS_LOCAL);
        _mastersGroup = Config.getStringValue(Conf.NodeMon.GROUPS_MASTERS,
                DefaultValues.NodeMon.GROUPS_MASTERS);
        _isMasterForwardingLocals = Config.getBooleanValue(Conf.NodeMon.NETWORK_MASTERS_FORWARD_LOCAL_ENABLE,
                DefaultValues.NodeMon.NETWORK_MASTERS_FORWARD_LOCAL_ENABLE);
        _timeWindow = Config.getIntegerValue(Conf.NodeMon.NETWORK_TIME_WINDOW_SIZE,
                DefaultValues.NodeMon.NETWORK_TIME_WINDOW_SIZE);
    }

    @Override
    protected Collection<Container> getData (Node node)
    {
        List<Container> elements = new ArrayList<>();
        if (_isMaster && _isMasterForwardingLocals) {
            //deliver topology of the other local nodes as well
            Group localGroup = node.getGrump().getGroups().get(_localGroup);
            if (localGroup == null) {
                String error = "Local group is null, check group configuration";
                log.error(error);
                throw new RuntimeException(error);
            }

            //improve this, now it's O(n^3)
            Map<String, Node> nodeMap = _nodeMon.getWorldState().getNodesMapCopy();
            for (String nodeId : localGroup.getMembers().keySet()) {
                Node currentNode = nodeMap.get(nodeId);
                if (currentNode == null) {
                    continue;
                }
                Traffic currentTraffic = currentNode.getTraffic();
                if (currentTraffic == null) {
                    continue;
                }

                elements = getValidElements(currentNode);
            }
        }
        else {
            elements = getValidElements(node);
        }

        return elements;
    }

    private List<Container> getValidElements (Node node)
    {

        List<Link> validList = new ArrayList<>();
        for (int sourceIp : node.getTraffic().getSources().keySet()) {
            for (Link l : node.getTraffic().getSources().get(sourceIp).getDestinations().values()) {

                if (l == null
                        || l.getStatsList() == null
                        || l.getStatsList().size() == 0) {
                    continue;
                }

                //check timestamp
                if (TimeUtil.distance(l.getTimestamp(), TimeUtil.getCurrentTime()).getSeconds() > _timeWindow) {
                    continue;
                }

                validList.add(l);
            }
        }
        List<Container> elements = new ArrayList<>();
        Container cm = ProtoUtils.toContainer(node.getId(), validList);
        elements.add(cm);

        return elements;
    }

    @Override
    protected DataType getType ()
    {
        return DataType.LINK;
    }
}
