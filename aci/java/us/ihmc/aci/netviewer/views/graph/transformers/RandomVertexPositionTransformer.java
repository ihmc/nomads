package us.ihmc.aci.netviewer.views.graph.transformers;

import us.ihmc.aci.netviewer.views.graph.Vertex;

import java.awt.geom.Point2D;
import java.util.HashMap;
import java.util.Map;

/**
 * Randomly computes the vertexes position on the graph
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class RandomVertexPositionTransformer
        implements org.apache.commons.collections15.Transformer<Vertex, java.awt.geom.Point2D>
{
    public RandomVertexPositionTransformer (double maxX, double maxY)
    {
        _minWidth = 10;
        _minHeight = 10;
        _maxWidth = maxX - 10;
        _maxHeight = maxY - 10;

        _locations = new HashMap<>();
    }

    @Override
    public Point2D transform (Vertex vertex)
    {
        if (vertex == null) {
            return new Point2D.Double (-1, -1);
        }

        if (_locations.containsKey (vertex.getId())) {
            return _locations.get (vertex.getId());
        }

        double r = Math.random();
        double x = r * (_maxWidth - _minWidth) + _minWidth;
        r = Math.random();
        double y = r * (_maxHeight - _minHeight) + _minHeight;
        Point2D.Double point = new Point2D.Double (x, y);
        _locations.put (vertex.getId(), point);

        return point;
    }

    private final double _minWidth;
    private final double _minHeight;
    private final double _maxWidth;
    private final double _maxHeight;
    private final Map<String, Point2D.Double> _locations;
}
