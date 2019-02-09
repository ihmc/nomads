package us.ihmc.aci.netviewer.views.graph.menus;

import edu.uci.ics.jung.algorithms.layout.GraphElementAccessor;
import edu.uci.ics.jung.visualization.VisualizationViewer;
import edu.uci.ics.jung.visualization.control.AbstractPopupGraphMousePlugin;
import us.ihmc.aci.netviewer.tabs.ChartTabsListener;
import us.ihmc.aci.netviewer.views.graph.Edge;
import us.ihmc.aci.netviewer.views.graph.Vertex;
import us.ihmc.aci.netviewer.tabs.LinkTabsListener;
import us.ihmc.aci.netviewer.tabs.NodeTabsListener;

import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseEvent;
import java.awt.geom.Point2D;

/**
 * Manages the creation of the popup menus in correspondence of the graph vertexes
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class PopupMenuMousePlugin extends AbstractPopupGraphMousePlugin
{
    /**
     * Constructor
     * @param vertexPopup popup menu for the vertex
     * @param edgePopup popup menu for the edge
     * @param ntListener nodes tabs listener
     * @param ltListener links tabs listener
     * @param ctListener charts tabs listener
     */
    public PopupMenuMousePlugin (JPopupMenu vertexPopup, JPopupMenu edgePopup, NodeTabsListener ntListener,
                                 LinkTabsListener ltListener, ChartTabsListener ctListener)
    {
        super (MouseEvent.BUTTON3_MASK);
        _vertexPopup = vertexPopup;
        _edgePopup = edgePopup;
        _nodeTabsListener = ntListener;
        _linkTabsListener = ltListener;
        _chartTabsListener = ctListener;
    }

    @Override
    protected void handlePopup (MouseEvent mouseEvent)
    {
        final VisualizationViewer<Vertex, Edge> viewer = (VisualizationViewer<Vertex, Edge>) mouseEvent.getSource();
        Point2D point = mouseEvent.getPoint();

        GraphElementAccessor<Vertex, Edge> pickSupport = viewer.getPickSupport();
        if (pickSupport != null) {
            final Vertex vertex = pickSupport.getVertex (viewer.getGraphLayout(), point.getX(), point.getY());
            if (vertex != null) {
                updateVertexMenu (vertex, viewer);
                _vertexPopup.show (viewer, mouseEvent.getX(), mouseEvent.getY());
                return;
            }

            double cx = point.getX();
            double cy = point.getY();
            for (double x=cx-EDGE_SELECTION_OFFSET; x<cx+EDGE_SELECTION_OFFSET; x++) {
                for (double y=cy-EDGE_SELECTION_OFFSET; y<cy+EDGE_SELECTION_OFFSET; y++) {
                    final Edge edge = pickSupport.getEdge (viewer.getGraphLayout(), x, y);
                    if (edge != null) {
                        updateEdgeMenu (edge, viewer);
                        _edgePopup.show (viewer, mouseEvent.getX(), mouseEvent.getY());
                        return;
                    }
                }
            }
        }
    }

    /**
     * Updates a vertex menu
     * @param vertex vertex which the menu is attached to
     * @param viewer graph viewer instance
     */
    private void updateVertexMenu (Vertex vertex, VisualizationViewer<Vertex, Edge> viewer)
    {
        if (_vertexPopup == null) {
            return;
        }

        Component[] menuComps = _vertexPopup.getComponents();
        for (Component comp : menuComps) {
            if (comp instanceof VertexMenuListener) {
                ((VertexMenuListener) comp).setVertexAndView (vertex, viewer, _nodeTabsListener, _chartTabsListener);
            }
        }
    }

    /**
     * Updates a edge menu
     * @param edge edge which the menu is attached to
     * @param viewer graph viewer instance
     */
    private void updateEdgeMenu (Edge edge, VisualizationViewer<Vertex, Edge> viewer)
    {
        if (_edgePopup == null) {
            System.out.println ("The edge popup is null");
            return;
        }

        Component[] menuComps = _edgePopup.getComponents();
        for (Component comp : menuComps) {
            if (comp instanceof EdgeMenuListener) {
                ((EdgeMenuListener) comp).setEdgeAndView (edge, viewer, _linkTabsListener);
            }
        }
    }


    private final JPopupMenu _vertexPopup;
    private final JPopupMenu _edgePopup;
    private final NodeTabsListener _nodeTabsListener;
    private final LinkTabsListener _linkTabsListener;
    private final ChartTabsListener _chartTabsListener;

    private static final int EDGE_SELECTION_OFFSET = 5;
}
