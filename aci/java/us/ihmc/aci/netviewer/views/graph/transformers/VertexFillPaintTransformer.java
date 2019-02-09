package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Vertex;

import java.awt.*;

/**
 * Manages the vertex fill paint
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class VertexFillPaintTransformer implements Transformer<Vertex, Paint>
{
    @Override
    public Paint transform (Vertex vertex)
    {
        if (vertex.getColor() == null) {
            return Vertex.DEFAULT_COLOR;
        }

        return vertex.getColor();
    }
}
