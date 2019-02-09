package us.ihmc.aci.netviewer.views.graph.transformers;

import org.apache.commons.collections15.Transformer;
import us.ihmc.aci.netviewer.views.graph.Vertex;

import java.util.List;

/**
 * Manages the vertex tool tip
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class VertexToolTipTransformer implements Transformer<Vertex, String>
{
    @Override
    public String transform (Vertex vertex)
    {
        StringBuilder sb = new StringBuilder();
        sb.append ("<html>Node name: ");
        sb.append (vertex.getName());

        List<String> ips = vertex.getIPs();
        if (ips != null) {
            if (ips.size() == 0) {
                sb.append ("<br>");
                sb.append ("Network interface: ");
                sb.append (ips.get(0));
            }
            else {
                sb.append ("<br>");
                sb.append ("Network interfaces: ");
                for (String ip : ips) {
                    sb.append ("<br>-&nbsp;");
                    sb.append (ip);
                }
            }
        }
        sb.append ("</html>");

        return sb.toString();
    }
}
