/**
 * GridMobileAgent.java Copyright 2000 Arne Grimstrup, Department of Computer
 * Science, Dartmouth College Use, modification and redistribution subject to
 * the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 */

package mil.darpa.coabs.gmas.mobility;

/**
 * Interface to allow access to the Grid mobile agent's descriptive information.
 * 
 * @author Arne Grimstrup
 * @version $Revision$ $Date$
 * @see GridAgentMetaData
 * @see GridAgentVariableState
 */

public interface GridMobileAgent {

    /** Set the Agent's state */
    public void setInitialState (GridAgentVariableState as);

    /** Set the Agent's metadata */
    public void setMetaData (GridAgentMetaData ad);

    /** Retrieve the Agent's metadata */
    public GridAgentMetaData getMetaData();
}
