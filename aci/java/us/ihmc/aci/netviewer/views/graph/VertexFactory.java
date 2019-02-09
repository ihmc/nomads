package us.ihmc.aci.netviewer.views.graph;

import org.apache.commons.collections15.Factory;

import java.awt.Color;
import java.util.List;

/**
 * Creates a new vertex
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class VertexFactory implements Factory<Vertex>
{
    @Override
    public Vertex create()
    {
        return new Vertex();
    }

    /**
     * Creates a vertex with specific values
     * @param id node id
     * @param name vertex name
     * @param ips list of ip addresses associated to the node
     * @return the new vertex instance
     */
    public Vertex create (String id, String name, List<String> ips)
    {
        Vertex vertex = create();
        vertex.define (id, name, ips);
        return vertex;
    }

    /**
     * Creates a vertex with specific values
     * @param id node id
     * @param name vertex name
     * @param ips list of ip addresses associated to the node
     * @param color vertex color
     * @return the new vertex instance
     */
    public Vertex create (String id, String name, List<String> ips, Color color)
    {
        Vertex vertex = create();
        vertex.define (id, name, ips, color);
        return vertex;
    }
}
