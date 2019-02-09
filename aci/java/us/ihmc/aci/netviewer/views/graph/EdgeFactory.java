package us.ihmc.aci.netviewer.views.graph;

import org.apache.commons.collections15.Factory;

import java.awt.*;

/**
 * Creates a new edge
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class EdgeFactory implements Factory<Edge>
{
    @Override
    public Edge create()
    {
        return new Edge();
    }

    /**
     * Creates an edge with specific values
     * @param originId vertex id where the edge origins from
     * @param origin vertex the edge origins from
     * @param destinationId vertex id where the edge ends into
     * @param destination vertex the edge ends into
     * @param isDirected true if the edge has a direction
     */
    public Edge create (String originId, String origin, String destinationId, String destination, boolean isDirected)
    {
        Edge edge = create();
        edge.define (originId, origin, destinationId, destination, isDirected);
        return edge;
    }

    /**
     * Creates an edge with specific values
     * @param originId vertex id where the edge origins from
     * @param origin vertex the edge origins from
     * @param destinationId vertex id where the edge ends into
     * @param destination vertex the edge ends into
     * @param isDirected true if the edge has a direction
     * @param color color to use to draw the edge
     * @param width edge width
     */
    public Edge create (String originId, String origin, String destinationId, String destination, boolean isDirected,
                        Color color, float width)
    {
        Edge edge = create();
        edge.define (originId, origin, destinationId, destination, isDirected, color, width);
        return edge;
    }
}
