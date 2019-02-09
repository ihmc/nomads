package us.ihmc.aci.netviewer.views.graph;

import edu.uci.ics.jung.algorithms.layout.CircleLayout;
import edu.uci.ics.jung.algorithms.layout.Layout;
import edu.uci.ics.jung.algorithms.layout.StaticLayout;
import edu.uci.ics.jung.graph.util.EdgeType;
import edu.uci.ics.jung.visualization.VisualizationViewer;
import edu.uci.ics.jung.visualization.control.EditingModalGraphMouse;
import edu.uci.ics.jung.visualization.control.ModalGraphMouse;
import us.ihmc.aci.netviewer.proxy.NeighborLinkInfo;
import us.ihmc.aci.netviewer.tabs.ChartTabsListener;
import us.ihmc.aci.netviewer.tabs.LinkTabsListener;
import us.ihmc.aci.netviewer.views.graph.menus.MouseMenus;
import us.ihmc.aci.netviewer.tabs.NodeTabsListener;
import us.ihmc.aci.netviewer.views.graph.menus.PopupMenuMousePlugin;
import us.ihmc.aci.netviewer.views.graph.transformers.*;
import us.ihmc.aci.netviewer.util.Label;
import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;

import javax.swing.*;
import java.awt.*;
import java.util.List;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Map;

/**
 * Creates an instance of a <code>VisualizationViewer</code> container where the whole graph can
 * be edited using the jung library
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class GContainer extends JPanel implements ActionListener
{
    /**
     * Constructor
     * @param ntListener used to inform the nodes tabs panel about a new node to add or remove
     * @param ltListener used to inform the links tabs panel about a new link to add or remove
     * @param ctListener used to inform the charts tabs panel about a new link to add or remove
     */
    public GContainer (NodeTabsListener ntListener, LinkTabsListener ltListener, ChartTabsListener ctListener)
    {
        setLayout (new BorderLayout());

        _graph = new Graph();

        Layout<Vertex, Edge> layout;
        switch (LAYOUT_TYPE) {
            case circular:
                layout = new CircleLayout<> (_graph);
                break;
            case static_random:
                layout = new StaticLayout<> (_graph);
                layout.setInitializer (new RandomVertexPositionTransformer (LAYOUT_WIDTH, LAYOUT_HEIGHT));
                break;
            case static_circle:
                layout = new StaticLayout<> (_graph);
                layout.setInitializer (new CircleVertexPositionTransformer (LAYOUT_WIDTH, LAYOUT_HEIGHT));
                break;
            default:
                layout = new CircleLayout<> (_graph);
                break;
        }

        layout.setSize (new Dimension (LAYOUT_WIDTH, LAYOUT_HEIGHT));

        _viewer = new VisualizationViewer<> (layout);
        _viewer.setPreferredSize (new Dimension (VIEWER_WIDTH, VIEWER_HEIGHT));
        _viewer.getRenderContext().setVertexFillPaintTransformer (new VertexFillPaintTransformer());
        _viewer.getRenderContext().setVertexLabelTransformer (new VertexLabelTransformer());
        _viewer.setVertexToolTipTransformer (new VertexToolTipTransformer());
        _viewer.getRenderContext().setEdgeDrawPaintTransformer (new EdgeDrawPaintTransformer());
        _viewer.getRenderContext().setEdgeStrokeTransformer (new EdgeStrokeTransformer());
        _viewer.getRenderContext().setLabelOffset (50);
        _viewer.getRenderContext().getEdgeLabelRenderer().setRotateEdgeLabels (false);
        _viewer.getRenderContext().setEdgeLabelTransformer (new EdgeLabelTransformer());
        _viewer.getRenderContext().setLabelOffset (100);

        EditingModalGraphMouse<Vertex, Edge> gm =
                new EditingModalGraphMouse<> (_viewer.getRenderContext(), new VertexFactory(), new EdgeFactory());
        gm.setMode(ModalGraphMouse.Mode.TRANSFORMING);

        PopupMenuMousePlugin plugin = new PopupMenuMousePlugin (new MouseMenus.VertexMenu(), new MouseMenus.EdgeMenu(),
                ntListener, ltListener, ctListener);
        gm.remove (gm.getPopupEditingPlugin());
        gm.add (plugin);

        _viewer.setGraphMouse(gm);

//        _viewer.setBorder (BorderFactory.createLineBorder (Color.black));
        JScrollPane viewerScrollPane = new JScrollPane (_viewer);
        viewerScrollPane.setVerticalScrollBarPolicy (JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        viewerScrollPane.setHorizontalScrollBarPolicy (JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);

        JRadioButton rbTransforming = new JRadioButton (ModalGraphMouseSelection.transforming.getActionCommand());
        rbTransforming.setActionCommand (ModalGraphMouseSelection.transforming.getActionCommand());
        rbTransforming.setSelected (true);
        rbTransforming.addActionListener (this);

        JRadioButton rbPicking = new JRadioButton (ModalGraphMouseSelection.picking.getActionCommand());
        rbPicking.setActionCommand(ModalGraphMouseSelection.picking.getActionCommand());
        rbPicking.addActionListener (this);

        ButtonGroup group = new ButtonGroup();
        group.add (rbTransforming);
        group.add (rbPicking);

        JPanel rbPanel = new JPanel();
        rbPanel.setLayout (new BoxLayout (rbPanel, BoxLayout.X_AXIS));
        rbPanel.add (rbTransforming);
        rbPanel.add (rbPicking);

        JPanel leftFieldsetPanel = new JPanel();
        leftFieldsetPanel.setLayout (new BoxLayout (leftFieldsetPanel, BoxLayout.PAGE_AXIS));
        leftFieldsetPanel.add (new JLabel ("<html><b><i>5 seconds stats update</i></b></html>"));
        leftFieldsetPanel.add (new JLabel ("<html>" + Label.left_red_arrow_fives.get() + " Aggregated incoming traffic rate</html>"));
        leftFieldsetPanel.add (new JLabel ("<html>" + Label.right_green_arrow_fives.get() + " Aggregated outgoing traffic rate</html>"));
        leftFieldsetPanel.add (new JLabel ("<html>" + Label.left_blue_arrow_fives.get() + " Incoming traffic rate from specific node</html>"));
        leftFieldsetPanel.add (new JLabel ("<html>" + Label.right_orange_arrow_fives.get() + " Outgoing traffic rate to specific node</html>"));

        JPanel rightFieldsetPanel = new JPanel();
        rightFieldsetPanel.setLayout (new BoxLayout (rightFieldsetPanel, BoxLayout.PAGE_AXIS));
        rightFieldsetPanel.add (new JLabel ("<html><b><i>1 minute stats update</i></b></html>"));
        rightFieldsetPanel.add (new JLabel ("<html>" + Label.left_red_arrow_min.get() + " Aggregated incoming traffic rate</html>"));
        rightFieldsetPanel.add (new JLabel ("<html>" + Label.right_green_arrow_min.get() + " Aggregated outgoing traffic rate</html>"));
        rightFieldsetPanel.add (new JLabel ("<html>" + Label.left_blue_arrow_min.get() + " Incoming traffic rate from specific node</html>"));
        rightFieldsetPanel.add (new JLabel ("<html>" + Label.right_orange_arrow_min.get() + " Outgoing traffic rate to specific node</html>"));

        JPanel fieldsetPanel = new JPanel();
        fieldsetPanel.setLayout (new BoxLayout (fieldsetPanel, BoxLayout.LINE_AXIS));
        fieldsetPanel.setBorder(BorderFactory.createEmptyBorder(30, 0, 30, 0));
        fieldsetPanel.add (leftFieldsetPanel);
        fieldsetPanel.add (rightFieldsetPanel);

        setLayout (new BoxLayout (this, BoxLayout.PAGE_AXIS));
        rbPanel.setAlignmentX (Component.LEFT_ALIGNMENT);
        fieldsetPanel.setAlignmentX (Component.LEFT_ALIGNMENT);
        add (rbPanel);
//        add (_viewer);
        add (viewerScrollPane);
        add (fieldsetPanel);
    }

    /**
     * Gets the graph container ready to be added to the <code>JPanel</code>
     * @return the container ready to be added to the <code>JPanel</code>
     */
    public Component getComponent()
    {
        return this;
    }

    /**
     * Repaint the graph
     */
    private void repaintGraph()
    {
        if (_viewer == null) {
            return;
        }

        _viewer.repaint();
    }

    /**
     * Adds a new vertex to the graph
     * @param id vertex id
     * @param name vertex name
     * @param ips list of ip addresses associated to the node
     */
    public void addVertex (String id, String name, List<String> ips)
    {
        _graph.addVertex (id, name, ips);
        repaintGraph();
    }

    /**
     * Adds a new edge to the graph
     * @param origId edge origin id
     * @param destId edge destination id
     */
    public void addEdge (String origId, String destId)
    {
        addEdge (origId, destId, false);
    }

    /**
     * Adds a new edge to the graph
     * @param origId edge origin id
     * @param destId edge destination id
     * @param direct edge type
     */
    public void addEdge (String origId, String destId, boolean direct)
    {
        _graph.addEdge (origId, destId, (direct ? EdgeType.DIRECTED : EdgeType.UNDIRECTED));
        repaintGraph();
    }

    /**
     * Removes a vertex from the graph
     * @param id vertex id
     */
    public void removeVertex (String id)
    {
        _graph.removeVertex (id);
        repaintGraph();
    }

    /**
     * Gets ids of the edges whose origin or destination id correspond to the parameter
     * @param id vertex id to look for in the edges id
     * @return the list of ids of the edges whose origin or destination id correspond to the parameter
     */
    public List<String> getEdgesIds (String id)
    {
        return _graph.getEdgesIds (id);
    }

    /**
     * Updates the aggregated traffic values
     * @param id node id
     * @param aggrInTraffic aggregated incoming traffic statistics
     * @param aggrOutTraffic aggregated outgoing traffic statistics
     */
    public void updateAggrTraffic (String id, TrafficParticle aggrInTraffic, TrafficParticle aggrOutTraffic)
    {
        _graph.updateAggrTraffic (id, aggrInTraffic, aggrOutTraffic);
        repaintGraph();
    }

    /**
     * Updates traffic information for the connections of a specific node
     * @param nodeId id of the node displaying the traffic info
     * @param linksInfo traffic info for all the connections
     */
    public void updateSingleTraffic (String nodeId, Map<String, NeighborLinkInfo> linksInfo)
    {
        _graph.updateSingleTraffic (nodeId, linksInfo);
        repaintGraph();
    }

    /**
     * Removes all the vertexes in the graph
     */
    public void clean()
    {
        _graph.clean();
        repaintGraph();
    }


    // <-------------------------------------------------------------------------------------------------------------->
    // <-- Methods implementing ActionListener                                                                      -->
    // <-------------------------------------------------------------------------------------------------------------->

    @Override
    public void actionPerformed (ActionEvent e)
    {
        ModalGraphMouse.Mode mode = ModalGraphMouseSelection.getMode(e.getActionCommand());
        ((EditingModalGraphMouse<Vertex, Edge>) _viewer.getGraphMouse()).setMode (mode);
        repaint();
    }

    // <-------------------------------------------------------------------------------------------------------------->


    private final VisualizationViewer<Vertex, Edge> _viewer;
    private final Graph _graph;

    public static final int LAYOUT_WIDTH = 500;
    public static final int LAYOUT_HEIGHT = 350;
    public static final int VIEWER_WIDTH = 530;
    public static final int VIEWER_HEIGHT = 400;

    public static final LayoutType LAYOUT_TYPE = LayoutType.static_circle;
}
