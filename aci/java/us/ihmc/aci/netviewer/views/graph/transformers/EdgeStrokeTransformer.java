package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Edge;

import java.awt.*;

/**
 * Manages the edge stroke
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class EdgeStrokeTransformer implements Transformer<Edge, Stroke>
{
    @Override
    public Stroke transform (Edge edge)
    {
        return new BasicStroke (edge.getWidth());
    }
}
