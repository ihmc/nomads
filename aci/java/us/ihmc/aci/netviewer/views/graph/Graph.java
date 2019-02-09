package us.ihmc.aci.netviewer.views.graph;

import edu.uci.ics.jung.graph.SparseMultigraph;
import edu.uci.ics.jung.graph.util.EdgeType;
import us.ihmc.aci.netviewer.proxy.NeighborLinkInfo;
import us.ihmc.aci.nodemon.data.traffic.TrafficParticle;

import java.util.*;

/**
 * Manages the graph of the connections
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
class Graph extends SparseMultigraph<Vertex, Edge>
{
    /**
     * Constructor
     */
    Graph()
    {
        super();
        _vertexFactory = new VertexFactory();
        _edgeFactory = new EdgeFactory();
        _nodeNames = new HashMap<>();
    }

    /**
     * Adds a new vertex to the graph
     * @param id node id
     * @param name vertex name
     * @param ips list of ip addresses associated to the node
     */
    void addVertex (String id, String name, List<String> ips)
    {
        if (name == null) {
            return;
        }

        addVertex (_vertexFactory.create (id, name, ips));
        _nodeNames.put (id, name);
    }

    /**
     * Looks for a given vertex in the graph
     * @param id id of the vertex to look for
     * @return the <code>Vertex</code> instance if any, null otherwise
     */
    Vertex getVertex (String id)
    {
        if (id == null) {
            return null;
        }

        for (Vertex vertex : getVertices()) {
            if ((vertex.getId() != null) && (vertex.getId().equals (id))) {
                return vertex;
            }
        }

        return null;
    }

    /**
     * Removes a vertex from the graph
     * @param id vertex id
     */
    void removeVertex (String id)
    {
        if (id == null) {
            return;
        }

        Vertex removingVertex = null;
        for (Vertex vertex : getVertices()) {
            if ((vertex.getId() != null) && (vertex.getId().equals (id))) {
                removingVertex = vertex;
                break;
            }
        }
        if (removingVertex != null) {
            removeVertex (removingVertex);
        }
    }

    /**
     * Adds a new edge to the graph
     * @param origId edge origin id
     * @param destId edge destination id
     * @param type edge type
     */
    void addEdge (String origId, String destId, EdgeType type)
    {
        if (type == null) {
            return;
        }

        for (Edge edge : getEdges()) {
            switch (type) {
                case DIRECTED:
                    if (edge.compareId (origId, destId)) {
                        return;
                    }
                    break;
                case UNDIRECTED:
                    if ((edge.compareId (origId, destId)) || (edge.compareId (destId, origId))) {
                        return;
                    }
                    break;
            }
        }

        Vertex vOrig = getVertex (origId);
        if (vOrig == null) {
            return;
        }

        Vertex vDest = getVertex (destId);
        if (vDest == null) {
            return;
        }

        addEdge (_edgeFactory.create (origId, vOrig.getName(), destId, vDest.getName(), type.equals (EdgeType.DIRECTED)),
                vOrig, vDest, type);
    }

    /**
     * Gets ids of the edges whose origin or destination id correspond to the parameter
     * @param id vertex id to look for in the edges id
     * @return the list of ids of the edges whose origin or destination id correspond to the parameter
     */
    List<String> getEdgesIds (String id)
    {
        List<String> ids = new ArrayList<>();

        if (id == null) {
            return ids;
        }

        for (Edge edge : getEdges()) {
            if (edge.compareId (id)) {
                ids.add (edge.getId());
            }
        }

        return ids;
    }

    /**
     * Updates the aggregated traffic values
     * @param id node id
     * @param aggrInTraffic aggregated incoming traffic statistics
     * @param aggrOutTraffic aggregated outgoing traffic statistics
     */
    void updateAggrTraffic (String id, TrafficParticle aggrInTraffic, TrafficParticle aggrOutTraffic)
    {
        if (id == null) {
            return;
        }

        for (Vertex vertex : getVertices()) {
            if (vertex.getId().equals (id)) {
                vertex.updateAggInTraffic (aggrInTraffic);
                vertex.updateAggOutTraffic (aggrOutTraffic);
                return;
            }
        }
    }

    /**
     * Updates traffic information for the connections of a specific node
     * @param nodeId id of the node displaying the traffic info
     * @param linksInfo traffic info for all the connections
     */
    void updateSingleTraffic (String nodeId, Map<String, NeighborLinkInfo> linksInfo)
    {
        if ((nodeId == null) || (linksInfo == null)) {
            return;
        }

        for (Vertex vertex : getVertices()) {
            if (vertex.getId().equals (nodeId)) {
                for (String neighId : linksInfo.keySet()) {
                    // TODO
//                    vertex.updateInTraffic (neighId, _nodeNames.get (neighId),
//                            linksInfo.get (neighId).getInTraffic());
//                    vertex.updateOutTraffic (neighId, _nodeNames.get (neighId),
//                            linksInfo.get (neighId).getOutTraffic());
                }
                return;
            }
        }
    }

    /**
     * Removes all the vertexes in the graph
     */
    public void clean()
    {
        Collection<Vertex> vertices = new ArrayList<>();
        vertices.addAll (getVertices());
        for (Vertex v : vertices) {
            removeVertex (v);
        }
    }


    private final VertexFactory _vertexFactory;
    private final EdgeFactory _edgeFactory;
    private final Map<String, String> _nodeNames;   // map containing <nodeId, nodeName>
}
