package us.ihmc.aci.netviewer.views.graph;

import us.ihmc.aci.netviewer.util.*;
import us.ihmc.aci.netviewer.util.Label;
import us.ihmc.aci.nodemon.data.traffic.BaseTrafficParticle;
import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;

import java.awt.*;
import java.util.HashMap;
import java.util.Map;
import java.util.List;

/**
 * Defines a vertex of the graph representing a node
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Vertex
{
    /**
     * Constructor
     */
    Vertex()
    {
        _singleInTraffic = new HashMap<>();
        _singleOutTraffic = new HashMap<>();
    }

    /**
     * Define a given vertex with the node information
     * @param id node id
     * @param name node name
     * @param ips list of ip addresses associated to the node
     */
    void define (String id, String name, List<String> ips)
    {
        define (id, name, ips, DEFAULT_COLOR);
    }

    /**
     * Define a given vertex with the node information
     * @param id node id
     * @param name node name
     * @param ips list of ip addresses associated to the node
     * @param color node color in the graph
     */
    void define (String id, String name, List<String> ips, Color color)
    {
        _id = id;
        _name = name;
        _ips = ips;
        setColor (color);
        _aggrInTraffic = new Stats (_name);
        _aggrOutTraffic = new Stats (_name);
    }

    /**
     * Gets the node id
     * @return node id
     */
    public String getId()
    {
        return _id;
    }

    /**
     * Gets the node name
     * @return node name
     */
    public String getName()
    {
        return _name;
    }

    /**
     * Gets the list of ip addresses associated to the node
     * @return the list of ip addresses associated to the node
     */
    public List<String> getIPs()
    {
        return _ips;
    }

    /**
     * Sets the node color
     * @param color node color
     */
    void setColor (Color color)
    {
        _color = color;
    }

    /**
     * Gets the node color
     * @return the node color
     */
    public Color getColor()
    {
        return _color;
    }

    /**
     * Updates the aggregated value of incoming traffic
     * @param value new aggregated value of incoming traffic
     */
    public void updateAggInTraffic (TrafficParticle value)
    {
        if (value == null) {
            return;
        }

        _aggrInTraffic.value = value;
    }

    /**
     * Enable or disable the aggregated incoming traffic in the graph
     * @param b true if the aggregated incoming traffic has to be shown
     */
    public void showAggrInTraffic (boolean b)
    {
        _aggrInTraffic.show = b;
    }

    /**
     * Says whether the aggregated incoming traffic
     * @return true if the aggregated incoming traffic
     */
    public synchronized boolean isInTrafficShown()
    {
        return _aggrInTraffic.show;
    }

    /**
     * Updates the aggregated value of outgoing traffic
     * @param value new aggregated value of outgoing traffic
     */
    public void updateAggOutTraffic (TrafficParticle value)
    {
        if (value == null) {
            return;
        }

        _aggrOutTraffic.value = value;
    }

    /**
     * Enable or disable the aggregated outgoing traffic in the graph
     * @param b true if the aggregated outgoing traffic has to be shown
     */
    public void showAggrOutTraffic (boolean b)
    {
        _aggrOutTraffic.show = b;
    }

    /**
     * Says whether the aggregated outgoing traffic
     * @return true if the aggregated outgoing traffic
     */
    public synchronized boolean isOutTrafficShown()
    {
        return _aggrOutTraffic.show;
    }

    /**
     * Updates the value of incoming traffic received from the given node
     * @param nodeId node id to look for
     * @param nodeName node name
     * @param value updated value of incoming traffic
     */
    public synchronized void updateInTraffic (String nodeId, String nodeName, TrafficParticle value)
    {
        if ((nodeId == null) || (nodeName == null)) {
            return;
        }

        if (value == null) {
            return;
        }


        if (!_singleInTraffic.containsKey (nodeId)) {
            _singleInTraffic.put (nodeId, new Stats (nodeName));
        }
        _singleInTraffic.get (nodeId).value = value;
    }

    /**
     * Enable or disable the incoming traffic in the graph from a specific node
     * @param nodeId node id to look for
     * @param nodeName node name
     * @param b true if the traffic has to be shown
     */
    public synchronized void showInTraffic (String nodeId, String nodeName, boolean b)
    {
        if ((nodeId == null) || (nodeName == null)) {
            return;
        }

        if (!_singleInTraffic.containsKey (nodeId)) {
            _singleInTraffic.put (nodeId, new Stats (nodeName));
        }

        _singleInTraffic.get (nodeId).show = b;
    }

    /**
     * Says whether the incoming traffic from the given node needs to be displayed on the graph
     * @param nodeId node id to look for
     * @return true if the incoming traffic from the specific node needs to be displayed on the graph
     */
    public synchronized boolean isInTrafficShown (String nodeId)
    {
        if ((nodeId == null) || (!_singleInTraffic.containsKey (nodeId))) {
            return false;
        }

        return _singleInTraffic.get (nodeId).show;
    }

    /**
     * Updates the value of outgoing traffic received from the given node
     * @param nodeId node id to look for
     * @param nodeName node name
     * @param value updated value of outgoing traffic
     */
    public synchronized void updateOutTraffic (String nodeId, String nodeName, TrafficParticle value)
    {
        if ((nodeId == null) || (nodeName == null)) {
            return;
        }

        if (value == null) {
            return;
        }


        if (!_singleOutTraffic.containsKey (nodeId)) {
            _singleOutTraffic.put (nodeId, new Stats (nodeName));
        }

        _singleOutTraffic.get (nodeId).value = value;
    }

    /**
     * Enable or disable the outgoing traffic in the graph to a specific node
     * @param nodeId node id to look for
     * @param nodeName node name
     * @param b true if the traffic has to be shown
     */
    public synchronized void showOutTraffic (String nodeId, String nodeName, boolean b)
    {
        if ((nodeId == null) || (nodeName == null)) {
            return;
        }

        if (!_singleOutTraffic.containsKey (nodeId)) {
            _singleOutTraffic.put (nodeId, new Stats (nodeName));
        }

        _singleOutTraffic.get (nodeId).show = b;
    }

    /**
     * Says whether the outgoing traffic from the given node needs to be displayed on the graph
     * @param nodeId node id to look for
     * @return true if the outgoing traffic from the specific node needs to be displayed on the graph
     */
    public synchronized boolean isOutTrafficShown (String nodeId)
    {
        if ((nodeId == null) || (!_singleOutTraffic.containsKey (nodeId))) {
            return false;
        }

        return _singleOutTraffic.get (nodeId).show;
    }

    /**
     * Creates the label containing the node name and the selected traffic info
     * @return the label containing the node name and the selected traffic info that can be displayed on the graph
     */
    public synchronized String createTrafficLabel()
    {
        StringBuilder sb = new StringBuilder();
        sb.append ("<html><div style=\"text-align: left;\">");

        sb.append (_name);
        sb.append ("<br>");

        if (_aggrInTraffic.show) {
            sb.append (Label.left_red_arrow_fives.get());    // Leftwards arrow
            sb.append (" ");
            sb.append (_aggrInTraffic.value.getFives());
            sb.append (" Bytes<br>");
            sb.append (Label.left_red_arrow_min.get());
            sb.append (" ");
            sb.append (_aggrInTraffic.value.getMin());
            sb.append (" Bytes");
        }

        if (_aggrOutTraffic.show) {
            if (!sb.toString().endsWith (">")){
                sb.append ("<br>");
            }
            sb.append (Label.right_green_arrow_fives.get());    // Rightwards arrow
            sb.append (" ");
            sb.append (_aggrOutTraffic.value.getFives());
            sb.append (" Bytes<br>");
            sb.append (Label.right_green_arrow_min.get());
            sb.append (" ");
            sb.append (_aggrOutTraffic.value.getMin());
            sb.append (" Bytes");
        }

        if (!_singleInTraffic.keySet().isEmpty()) {
            if (!sb.toString().endsWith (">")){
                sb.append ("<br>");
            }
        }
        for (String nodeId : _singleInTraffic.keySet()) {
            if (_singleInTraffic.get (nodeId).show) {
                if (sb.toString().endsWith (">")){
                    sb.append (Label.left_blue_arrow_fives.get());   // Leftwards arrow
                    sb.append (" ");
                }
                else {
                    sb.append (", ");
                }
                sb.append (_singleInTraffic.get (nodeId).node);
                sb.append (": ");
                sb.append (_singleInTraffic.get (nodeId).value.getFives());
                sb.append (" Bytes");
            }
        }
        if (!_singleInTraffic.keySet().isEmpty()) {
            if (!sb.toString().endsWith (">")){
                sb.append ("<br>");
            }
        }
        for (String nodeId : _singleInTraffic.keySet()) {
            if (_singleInTraffic.get (nodeId).show) {
                if (sb.toString().endsWith (">")){
                    sb.append (Label.left_blue_arrow_min.get());   // Leftwards arrow
                    sb.append (" ");
                }
                else {
                    sb.append (", ");
                }
                sb.append (_singleInTraffic.get (nodeId).node);
                sb.append (": ");
                sb.append (_singleInTraffic.get (nodeId).value.getMin());
                sb.append (" Bytes");
            }
        }

        if (!_singleOutTraffic.keySet().isEmpty()) {
            if (!sb.toString().endsWith (">")){
                sb.append ("<br>");
            }
        }
        for (String nodeId : _singleOutTraffic.keySet()) {
            if (_singleOutTraffic.get (nodeId).show) {
                if (sb.toString().endsWith (">")){
                    sb.append (Label.right_orange_arrow_fives.get());   // Rightwards arrow
                    sb.append (" ");
                }
                else {
                    sb.append (", ");
                }
                sb.append (_singleOutTraffic.get (nodeId).node);
                sb.append (": ");
                sb.append (_singleOutTraffic.get (nodeId).value.getFives());
                sb.append (" Bytes");
            }
        }
        if (!_singleOutTraffic.keySet().isEmpty()) {
            if (!sb.toString().endsWith (">")){
                sb.append ("<br>");
            }
        }
        for (String nodeId : _singleOutTraffic.keySet()) {
            if (_singleOutTraffic.get (nodeId).show) {
                if (sb.toString().endsWith (">")){
                    sb.append (Label.right_orange_arrow_min.get());   // Rightwards arrow
                    sb.append (" ");
                }
                else {
                    sb.append (", ");
                }
                sb.append (_singleOutTraffic.get (nodeId).node);
                sb.append (": ");
                sb.append (_singleOutTraffic.get (nodeId).value.getMin());
                sb.append (" Bytes");
            }
        }

        sb.append ("</html>");
        return (sb.toString());
    }


    private String _id;
    private String _name;
    private List<String> _ips;
    private Color _color;
    private Stats _aggrInTraffic;
    private Stats _aggrOutTraffic;
    private final Map<String, Stats> _singleInTraffic;
    private final Map<String, Stats> _singleOutTraffic;

    public static Color DEFAULT_COLOR = Color.GREEN;

    /**
     * Collects the traffic info
     */
    private class Stats
    {
        /**
         * Constructor
         * @param node node name
         */
        Stats (String node)
        {
            this.node = node;
            value.setFives (String.valueOf (0));
            value.setMin (String.valueOf (0));
        }

        boolean show = false;
        TrafficParticle value = new BaseTrafficParticle();
        final String node;
    }
}
