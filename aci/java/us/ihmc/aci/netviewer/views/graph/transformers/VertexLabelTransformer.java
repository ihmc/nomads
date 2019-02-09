package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Vertex;

/**
 * Manages the vertex labels
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class VertexLabelTransformer implements Transformer<Vertex, String>
{
    @Override
    public String transform (Vertex vertex)
    {
        return vertex.createTrafficLabel();
    }
}
