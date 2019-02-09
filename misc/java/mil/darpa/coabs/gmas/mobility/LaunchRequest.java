/**
 * LaunchRequest consists of the information sent to a destination GMAS host to
 * request permission to launch or clone a given agent from a source host. This
 * class contains the functionality to also convert this request into the GMAML
 * XML description. 
 * 
 * $author Tom Cowin <tom.cowin@gmail.com> Mar 29, 2005
 * $version
 */
package mil.darpa.coabs.gmas.mobility;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

public class LaunchRequest {

    public LaunchRequest (GridAgentMetaData metaData)
    {
        _metaData = metaData;
    }

    /**
     * Convert the LaunchRequest to GMAML form.
     * 
     * @return String containing the equivalent GMAML form.
     */
    public String toExternalForm()
    {
        StringBuffer sb = new StringBuffer();

        sb.append ("<?xml version=\"1.0\" ?>\n");
        try {
            sb.append ("<!DOCTYPE gmas SYSTEM \""
                    + AgentContainer.getGridCodebaseURL() + "gmaml.dtd\">\n");
        }
        catch (GmasMobilityException e1) {
            e1.printStackTrace();
        }
        sb.append ("<gmas>\n");
        sb.append (_metaData.toExternalForm());
        sb.append ("</gmas>\n");
        return sb.toString();
    }

    static final Logger _log = LogInitializer.getLogger (LaunchRequest.class.getName());
    GridAgentMetaData _metaData = null;
}
