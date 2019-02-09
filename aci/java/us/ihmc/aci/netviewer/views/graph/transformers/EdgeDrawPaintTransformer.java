package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Edge;

import java.awt.*;

/**
 * Manages the edge colors
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class EdgeDrawPaintTransformer implements Transformer<Edge, Paint>
{
    @Override
    public Paint transform (Edge edge)
    {
        if (edge.getColor() == null) {
            return Edge.DEFAULT_COLOR;
        }

        return edge.getColor();
    }
}
