package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Vertex;

import java.awt.geom.Point2D;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class CircleVertexPositionTransformer implements Transformer<Vertex, Point2D>
{
    public CircleVertexPositionTransformer (double maxX, double maxY)
    {
        double minWidth = 10;
        double minHeight = 10;
        double maxWidth = maxX - 10;
        double maxHeight = maxY - 10;

        _xCenter = (maxWidth - minWidth)/2;
        _yCenter = (maxHeight - minHeight)/2;

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

        Point2D.Double point;
        if (_count.get() == 0) {
            point = new Point2D.Double (_xCenter, _yCenter);
            _radius.addAndGet (50);

        }
        else if ((_count.get() % 6) == 1) {
            int r = _radius.addAndGet (80);
            _angle.set (0);
            point = getPosition (_angle.get(), r, _xCenter, _yCenter);
        }
        else {
            _angle.addAndGet (45);
            point = getPosition (_angle.get(), _radius.get(), _xCenter, _yCenter);
        }

        _count.incrementAndGet();
        _locations.put (vertex.getId(), point);

        return point;
    }

    /**
     * Computes the position of the new vertex in the graph following information about a circumference
     * @param t angle value
     * @param r radius value
     * @param xCenter x coordinate of the circumference center
     * @param yCenter y coordinate of the circumference center
     * @return the point on the circumference
     */
    private Point2D.Double getPosition (double t, int r, double xCenter, double yCenter)
    {
        double x = r * Math.cos (t) + xCenter;
        double y = r * Math.sin (t) + yCenter;
        return new Point2D.Double (x, y);
    }

    private final double _xCenter;
    private final double _yCenter;

    private final AtomicInteger _count = new AtomicInteger (0);
    private final AtomicInteger _radius = new AtomicInteger (0);
    private final AtomicInteger _angle = new AtomicInteger (0);

    private final Map<String, Point2D.Double> _locations;
}
