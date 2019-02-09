/**
 * SelfSerializable.java Copyright 2000 Arne Grimstrup, Department of Computer
 * Science, Dartmouth College Use, modification and redistribution subject to
 * the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 * <p>
 * Interface to allow access to the Grid mobile agent's variable collection.
 * 
 * @author Arne Grimstrup
 * @version $Revision$ $Date$
 * @see GridAgentVariableState
 */

package mil.darpa.coabs.gmas.mobility;

public interface SelfSerializable {

    /** Set the Agent's state */
    public void setAgentState (GridAgentVariableState as);

    /** Retrieve the Agent's state */
    public GridAgentVariableState getAgentState();
}
