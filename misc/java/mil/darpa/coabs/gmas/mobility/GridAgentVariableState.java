package mil.darpa.coabs.gmas.mobility;

/**
 * GridAgentVariableState.java Copyright 2000 Arne Grimstrup, Department of
 * Computer Science, Dartmouth College Use, modification and redistribution
 * subject to the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 */

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

import org.w3c.dom.*;
import java.lang.reflect.*;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * GridAgentVariableState manages the attribute state of a Grid Mobile agent. In
 * addition, GridAgentVariableState provides conversion methods to and from
 * GMAML.
 * 
 * @author Arne Grimstrup
 * @version $Revision$ $Date$
 * @see mil.darpa.coabs.gmas.mobility.AgentContainer
 */
public class GridAgentVariableState {

    /** Default Constructor. */
    public GridAgentVariableState()
    {
        statevars = new Hashtable();
    }

    /**
     * Constructor. Create a new GridMobileAgentState from the GMAML
     * representation.
     * 
     * @param gmanode root node of the DOM tree
     */
    public GridAgentVariableState (Node gmanode)
    {

        // The gridmobileagent tag has no attributes so we can go immediately
        // to the component tags.
        try {
            if (gmanode.getNodeName().compareTo ("state") == 0) {
                NodeList vnl = gmanode.getChildNodes();
                int vsize = vnl.getLength();
                String vname, vtype, vvalue;

                if (vsize > 0) {
                    statevars = new Hashtable();
                    for (int k = 0; k < vsize; k++) {
                        if (vnl.item (k).getNodeName().compareTo ("var") == 0) {
                            vname = ((Element) vnl.item (k)).getAttribute ("name");
                            vtype = ((Element) vnl.item (k)).getAttribute ("type");
                            vvalue = ((Element) vnl.item (k)).getAttribute ("value");

                            // Since no primitive types are allowed in the agent
                            // state, we can use Java reflection to reconstitute
                            // the individual attributes. The Character type
                            // is a special case since there is no constructor
                            // that accepts a String type.
                            if (vtype.compareTo ("java.lang.Character") == 0)
                                put (vname, new Character (vvalue.charAt (0)));
                            else {
                                Class parms[] = new Class[1];
                                Constructor vobj;
                                // Class cl;

                                parms[0] = Class.forName ("java.lang.String");
                                vobj = ((Class) Class.forName (vtype)).getConstructor (parms);
                                put (vname,
                                     vobj.newInstance (new Object[] {vvalue}));
                            }
                        }
                    }
                }
            }
        }
        catch (Exception e) {
            _log.info ("Invalid GridAgentVariableState");
        }
    }

    /**
     * Attribute value retrieval method.
     * 
     * @param variablename String containing the attribute name.
     * @return Object containing the value.
     */
    public Object get (String variablename)
    {
        return statevars.get (variablename);
    }

    /**
     * Attribute value update method.
     * 
     * @param variablename String containing the attribute name.
     * @param obj Object reference to the wrapper instance holding the attribute .
     * @return 0 if successful, 1 if the key is currently being used for an
     *         instance of a different class.
     */
    public int put (String variablename, Object obj)
    {
        if (statevars.containsKey (variablename) == false)
            statevars.put (variablename, obj);
        else {
            Object tobj = statevars.get (variablename);
            if (tobj.getClass() != obj.getClass())
                return 1;
            else
                statevars.put (variablename, obj);
        }
        return 0;
    }

    /**
     * Convert the GridAgentVariableState to GMAML form.
     * 
     * @return String containing the equivalent GMAML form.
     */
    public String toExternalForm()
    {
        Enumeration e;
        Object o;
        String key;
        StringBuffer sb = new StringBuffer();

        // The state requires special handling since we need to include type
        // information
        // in order to reconstitute the correct objects at the receiving end.
        sb.append ("<state>\n");
        if (statevars != null && statevars.isEmpty() == false) {
            e = statevars.keys();
            while (e.hasMoreElements()) {
                key = (String) e.nextElement();
                o = statevars.get (key);
                sb.append ("<var name=\"" + key + "\" type=\""
                        + o.getClass().getName() + "\" value=\""
                        + o.toString() + "\" />\n");
            }
        }
        sb.append ("</state>\n");
        return sb.toString();
    }

    /** 
     *  This method dumps the object state to stdout.  It is provided for debugging 
     *  purposes.
     */
    public void dump()
    {
        Enumeration e;
        Object o;
        String key;

        if (statevars != null && statevars.isEmpty() == false) {
            e = statevars.keys();
            while (e.hasMoreElements()) {
                key = (String) e.nextElement();
                o = statevars.get (key);
                _log.info ("Variable Name: " + key + " Type: "
                        + o.getClass().getName() + " Value: " + o.toString());
            }
        }
    }

    /** Storage for the agent's attributes */
    protected Hashtable statevars;
    private static final long serialVersionUID = 1;
    static final Logger _log = LogInitializer.getLogger (GridAgentVariableState.class.getName());
}
