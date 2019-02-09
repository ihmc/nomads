package us.ihmc.aci.netviewer.views.graph.menus;

import edu.uci.ics.jung.visualization.VisualizationViewer;
import us.ihmc.aci.netviewer.tabs.ChartTabsListener;
import us.ihmc.aci.netviewer.views.graph.Edge;
import us.ihmc.aci.netviewer.views.graph.Vertex;
import us.ihmc.aci.netviewer.tabs.NodeTabsListener;

/**
 * Interface used to set the vertex, edge lists and the graph viewer instance corresponding to a popup menu
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface VertexMenuListener<V>
{
    /**
     * Associates a menu to a given vertex and to a <code>NodeTabsListener</code>
     * @param vertex graph vertex which menu has been selected
     * @param viewer graph viewer instance
     * @param ntListener used to inform the nodes tabs panel about a new node to add or remove
     * @param ctListener used to inform the charts tabs panel about a new node to add or remove
     */
    void setVertexAndView (V vertex, VisualizationViewer<Vertex, Edge> viewer, NodeTabsListener ntListener,
                           ChartTabsListener ctListener);
}
