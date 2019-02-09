package us.ihmc.aci.netviewer.views.graph.menus;

import edu.uci.ics.jung.visualization.VisualizationViewer;
import us.ihmc.aci.netviewer.tabs.ChartTabsListener;
import us.ihmc.aci.netviewer.views.graph.Edge;
import us.ihmc.aci.netviewer.views.graph.Vertex;
import us.ihmc.aci.netviewer.tabs.LinkTabsListener;
import us.ihmc.aci.netviewer.tabs.NodeTabsListener;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * Manages the menus to interact with the graph
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class MouseMenus
{
    /**
     * Manages the menus associated to the graph vertexes
     */
    public static class VertexMenu extends JPopupMenu
    {
        /**
         * Constructor for <code>VertexMenu</code>
         */
        public VertexMenu()
        {
            super("Vertex Menu");

//            add (new OpenDashbordMenuItem<Vertex>);
            add (new AddNodeMenuItem<Vertex>());
            add (new RemoveNodeMenuItem<Vertex>());
            addSeparator();
            add (new AddChartMenuItem<Vertex>());
            add (new RemoveChartMenuItem<Vertex>());
            addSeparator();
            add (new TrafficMenuItem<Vertex>("Show aggregated incoming traffic", TrafficType.incoming_total));
            add (new TrafficMenuItem<Vertex>("Show aggregated outgoing traffic", TrafficType.outgoing_total));
            add (new ShowIncomingTrafficMenuItem<Vertex>());
            add (new ShowOutgoingTrafficMenuItem<Vertex>());
        }
    }

    /**
     * Manages the menus associated to the graph edges
     */
    public static class EdgeMenu extends JPopupMenu
    {
        /**
         * Constructor for <code>VertexMenu</code>
         */
        public EdgeMenu()
        {
            super("Edge Menu");
            add (new AddLinkMenuItem<Edge>());
            addSeparator();
            add (new RemoveLinkMenuItem<Edge>());
        }
    }

    /**
     * Manages the menu item to add a node info tab
     * @param <V> selected vertex
     */
    static class AddNodeMenuItem<V> extends JMenuItem implements VertexMenuListener<Vertex>
    {
        /**
         * Constructor
         */
        AddNodeMenuItem()
        {
            super (_label);

            addActionListener(new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ntListener == null) {
                        return;
                    }

                    _ntListener.addNodeTab (_vertex.getId(), _vertex.getName());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            _vertex = vertex;
            _viewer = viewer;
            _ntListener = ntListener;
            setText (_label);
        }

        private Vertex _vertex;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private NodeTabsListener _ntListener;
        private static final String _label = "Add node info tab";
    }

    /**
     * Manages the menu item to delete a node info tab
     * @param <V> selected vertex
     */
    static class RemoveNodeMenuItem<V> extends JMenuItem implements VertexMenuListener<Vertex>
    {
        /**
         * Constructor
         */
        RemoveNodeMenuItem()
        {
            super (_label);
            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ntListener == null) {
                        return;
                    }

                    _ntListener.removeNodeTab(_vertex.getId());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            _vertex = vertex;
            _viewer = viewer;
            _ntListener = ntListener;
            setText (_label);
        }

        private Vertex _vertex;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private NodeTabsListener _ntListener;
        private static final String _label = "Delete node info tab";
    }

    /**
     * Manages the menu item to add a node chart tab
     * @param <V> selected vertex
     */
    static class AddChartMenuItem<V> extends JMenuItem implements VertexMenuListener<Vertex>
    {
        /**
         * Constructor
         */
        AddChartMenuItem()
        {
            super (_label);

            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ctListener == null) {
                        return;
                    }

                    _ctListener.addChartTab(_vertex.getId(), _vertex.getName());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            _vertex = vertex;
            _viewer = viewer;
            _ctListener = ctListener;
            setText (_label);
        }

        private Vertex _vertex;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private ChartTabsListener _ctListener;
        private static final String _label = "Add node charts tab";
    }

    /**
     * Manages the menu item to delete a node chart tab
     * @param <V> selected vertex
     */
    static class RemoveChartMenuItem<V> extends JMenuItem implements VertexMenuListener<Vertex>
    {
        /**
         * Constructor
         */
        RemoveChartMenuItem()
        {
            super (_label);
            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ctListener == null) {
                        return;
                    }

                    _ctListener.removeChartTab (_vertex.getId());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            _vertex = vertex;
            _viewer = viewer;
            _ctListener = ctListener;
            setText (_label);
        }

        private Vertex _vertex;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private ChartTabsListener _ctListener;
        private static final String _label = "Delete node charts tab";
    }

    /**
     * Manages the menu to select the neighbors generating incoming traffic that needs to be displayed
     * @param <V> selected vertex
     */
    static class ShowIncomingTrafficMenuItem<V> extends JMenu implements VertexMenuListener<Vertex>
    {
        ShowIncomingTrafficMenuItem()
        {
            super ("Show incoming traffic from");
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            removeAll();
            for (Edge edge : viewer.getGraphLayout().getGraph().getEdges()) {
                String neighbor;
                String id;
                if (edge.getOriginId().equals (vertex.getId())) {
                    neighbor = edge.getDestination();
                    id = edge.getDestinationId();
                }
                else if (edge.getDestinationId().equals (vertex.getId())) {
                    neighbor = edge.getOrigin();
                    id = edge.getOriginId();
                }
                else {
                    continue;
                }

                TrafficMenuItem menuItem = new TrafficMenuItem (neighbor, id, TrafficType.incoming_from);
                menuItem.setSelected (vertex.isInTrafficShown (id));
                add (menuItem);
                menuItem.setVertexAndView (vertex, viewer, ntListener, ctListener);
            }
        }
    }

    /**
     * Manages the menu to select the neighbors generating outgoing traffic that needs to be displayed
     * @param <V> selected vertex
     */
    static class ShowOutgoingTrafficMenuItem<V> extends JMenu implements VertexMenuListener<Vertex>
    {
        ShowOutgoingTrafficMenuItem()
        {
            super ("Show outgoing traffic to");
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            removeAll();
            for (Edge edge : viewer.getGraphLayout().getGraph().getEdges()) {
                String neighbor;
                String id;
                if (edge.getOriginId().equals (vertex.getId())) {
                    neighbor = edge.getDestination();
                    id = edge.getDestinationId();
                }
                else if (edge.getDestinationId().equals (vertex.getId())) {
                    neighbor = edge.getOrigin();
                    id = edge.getOriginId();
                }
                else {
                    continue;
                }

                TrafficMenuItem menuItem = new TrafficMenuItem (neighbor, id, TrafficType.outgoing_to);
                menuItem.setSelected (vertex.isOutTrafficShown (id));
                add (menuItem);
                menuItem.setVertexAndView (vertex, viewer, ntListener, ctListener);
            }
        }
    }

    /**
     * Manages the menu item to select whether the traffic with a given neighbor needs to be displayed
     * @param <V> selected vertex
     */
    static class TrafficMenuItem<V> extends JCheckBoxMenuItem implements VertexMenuListener<Vertex>
    {
        /**
         * Constructor
         * @param label label to be displayed on the menu
         * @param trafficType traffic type
         */
        TrafficMenuItem (String label, TrafficType trafficType)
        {
            this (label, null, trafficType);
        }

        /**
         * Constructor
         * @param label label to be displayed on the menu
         * @param id neighbor id
         * @param trafficType traffic type
         */
        TrafficMenuItem (String label, final String id, TrafficType trafficType)
        {
            super (label);

            _label = label;
            _trafficType = trafficType;
            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    switch (_trafficType) {
                        case incoming_total:
                            _vertex.showAggrInTraffic (isSelected());
                            break;
                        case outgoing_total:
                            _vertex.showAggrOutTraffic (isSelected());
                            break;
                        case incoming_from:
                            _vertex.showInTraffic (id, _label, isSelected());
                            break;
                        case outgoing_to:
                            _vertex.showOutTraffic (id, _label, isSelected());
                            break;
                    }
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setVertexAndView (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                                      ChartTabsListener ctListener)
        {
            _vertex = vertex;
            _viewer = viewer;
            _ntListener = ntListener;
            switch (_trafficType) {
                case incoming_total:
                    setSelected (vertex.isInTrafficShown());
                    break;
                case outgoing_total:
                    setSelected (vertex.isOutTrafficShown());
            }
            setText (_label);
        }

        private Vertex _vertex;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private NodeTabsListener _ntListener;
        private final String _label;
        private final TrafficType _trafficType;
    }

    /**
     * Defines all the possible network traffic types
     */
    static enum TrafficType {
        incoming_total,
        outgoing_total,
        incoming_from,
        outgoing_to
    };

    /**
     * Manages the menu item to add a link
     * @param <E> selected vertex
     */
    static class AddLinkMenuItem<E> extends JMenuItem implements EdgeMenuListener<Edge>
    {
        /**
         * Constructor
         */
        AddLinkMenuItem()
        {
            super (_label);
            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ltListener == null) {
                        return;
                    }

                    _ltListener.addLinkTab(_edge.getId(), _edge.getName());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setEdgeAndView (Edge edge, VisualizationViewer<Vertex, Edge> viewer, LinkTabsListener listener)
        {
            _edge = edge;
            _viewer = viewer;
            _ltListener = listener;
            setText (_label);
        }

        private Edge _edge;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private LinkTabsListener _ltListener;
        private static final String _label = "Add link info tab";
    }

    /**
     * Manages the menu item to delete a link
     * @param <E> selected vertex
     */
    static class RemoveLinkMenuItem<E> extends JMenuItem implements EdgeMenuListener<Edge>
    {
        /**
         * Constructor
         */
        RemoveLinkMenuItem()
        {
            super (_label);
            addActionListener (new ActionListener()
            {
                @Override
                public void actionPerformed (ActionEvent e)
                {
                    if (_ltListener == null) {
                        return;
                    }

                    _ltListener.removeLinkTab(_edge.getId());
                    _viewer.repaint();
                }
            });
        }

        @Override
        public void setEdgeAndView (Edge edge, VisualizationViewer<Vertex, Edge> viewer, LinkTabsListener listener)
        {
            _edge = edge;
            _viewer = viewer;
            _ltListener = listener;
            setText (_label);
        }

        private Edge _edge;
        private VisualizationViewer<Vertex, Edge> _viewer;
        private LinkTabsListener _ltListener;
        private static final String _label = "Remove link info tab";
    }
}
