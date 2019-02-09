package us.ihmc.aci.netviewer.views.graph;

import us.ihmc.aci.netviewer.util.Utils;

import java.awt.*;

/**
 * Defines an edge of the graph representing the network link between two nodes
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class Edge
{
    /**
     * Constructor
     * @param originId vertex id where the edge origins from
     * @param origin vertex the edge origins from
     * @param destinationId vertex id where the edge ends into
     * @param destination vertex the edge ends into
     * @param directed true if the edge has a direction
     */
    void define (String originId, String origin, String destinationId, String destination, boolean directed)
    {
        define (originId, origin, destinationId, destination, directed, DEFAULT_COLOR, DEFAULT_WIDTH);
    }

    /**
     * Constructor
     * @param originId vertex id where the edge origins from
     * @param origin vertex the edge origins from
     * @param destinationId vertex id where the edge ends into
     * @param destination vertex the edge ends into
     * @param directed true if the edge has a direction
     * @param color color to use to draw the edge
     * @param width edge width
     */
    void define (String originId, String origin, String destinationId, String destination, boolean directed, Color color,
                 float width)
    {
        _originId = originId;
        _origin = origin;
        _destinationId = destinationId;
        _destination = destination;
        _directed = directed;
        _color = color;
        _width = width;
    }

    /**
     * Gets the edge origin vertex id
     * @return the edge origin vertex id
     */
    public String getOriginId()
    {
        return _originId;
    }

    /**
     * Gets the edge origin vertex
     * @return the edge origin vertex
     */
    public String getOrigin()
    {
        return _origin;
    }

    /**
     * Gets the edge destination vertex id
     * @return the edge destination vertex id
     */
    public String getDestinationId()
    {
        return _destinationId;
    }

    /**
     * Gets the edge destination vertex
     * @return the edge destination vertex
     */
    public String getDestination()
    {
        return _destination;
    }

    /**
     * Creates the id for the edge
     * @return the edge id
     */
    public String getId()
    {
        return Utils.buildId (_originId, _destinationId);
    }

    /**
     * Compares the local id with the id built using the two strings passed as parameters
     * @param s1 first string
     * @param s2 second string
     * @return true if the local id is equal to the id built using the two strings passed as parameters
     */
    public boolean compareId (String s1, String s2)
    {
        return getId().equals (Utils.buildId (s1, s2));
    }

    /**
     * Returns true if and only if the local id contains the specified sequence of char values
     * @param s the sequence to search for
     * @return true if the local id contains <code>s</code>, false otherwise
     */
    public boolean compareId (String s)
    {
        if (s == null) {
            return false;
        }

        return getId().contains (s);
    }

    /**
     * Creates the name for the edge
     * @return the edge name
     */
    public String getName()
    {
        return _origin + "-" + _destination;
    }

    /**
     * Says if the edge has a direction
     * @return true if the edge has a direction
     */
    public boolean isDirected()
    {
        return _directed;
    }

    /**
     * Gets the edge color
     * @return the edge color
     */
    public Color getColor()
    {
        return _color;
    }

    /**
     * Ges the edge width
     * @return the edge width
     */
    public float getWidth()
    {
        return _width;
    }


    private String _originId;
    private String _origin;
    private String _destinationId;
    private String _destination;
    private boolean _directed;
    private Color _color = DEFAULT_COLOR;
    private float _width = DEFAULT_WIDTH;

    public static final Color DEFAULT_COLOR = Color.BLACK;
    public static final float DEFAULT_WIDTH = 1.0f;
}
