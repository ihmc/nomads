package us.ihmc.aci.netviewer.views.graph.menus;

import edu.uci.ics.jung.visualization.VisualizationViewer;
import us.ihmc.aci.netviewer.views.graph.Edge;
import us.ihmc.aci.netviewer.views.graph.Vertex;
import us.ihmc.aci.netviewer.tabs.LinkTabsListener;

/**
 * Interface used to set the edge and the graph viewer instance corresponding to a popup menu
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public interface EdgeMenuListener<E>
{
    /**
     * Associates a menu to a given edge and to a <code>LinkTabsListener</code>
     * @param edge graph edge which menu has been selected
     * @param viewer graph viewer instance
     * @param listener used to inform the links tabs panel about a new link to add or remove
     */
    void setEdgeAndView (E edge, VisualizationViewer<Vertex, Edge> viewer, LinkTabsListener listener);
}
