package mil.darpa.coabs.gmas.mobility;

/**
 * AgentContainer is an object representation that mirrors the XML
 * representation of a GridMobileAgent. It can represent two distinct types of
 * agent: Type I or SelfSerializable agents are represented by the metadata and
 * their state variables. Type II or Serializable agents are represented by a
 * Base64 encoding of the serialized agent, along with a URL specifying the
 * location of the agent's .class file. In addition, AgentContainer provides
 * conversion methods to and from GMAML, the XML representation of this object.
 * 
 * @author Greg Hill
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version $Revision$ $Date$
 * @see GridAgentMetaData
 * @see Base64Transcoders
 * @see GridAgentVariableState
 */

import java.io.IOException;
import java.io.Serializable;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

import mil.darpa.coabs.gmas.util.Base64FormatException;
import mil.darpa.coabs.gmas.util.Base64Transcoders;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import org.w3c.dom.Node;

public class AgentContainer {

    /**
     * Constructor that takes a DOM(i.e. parsed XML) tree node representation of
     * the (incoming) agent. Passed in a classloader appropriate to load the
     * classes of the incoming agent and its support infrastructure, with the
     * location dictated by the codebase URL embedded in the agent's meta data.
     * Takes the Base64 encoded ascii string from the XML, converts it back to a
     * byte array, and then deserializes it back into the agent's java class
     * object.
     * 
     * @param gamd Agent's meta data
     * @param instance DOM tree node rep of incoming agent
     * @param loader ClassLoader with correct URL for retrieving requisite
     *            classes from agents codebase
     * @see mil.darpa.coabs.mobility.GridAgentMetaData
     * @see org.w3c.dom.Node
     */

    public AgentContainer (GridAgentMetaData gamd,
                           Node instance,
                           ClassLoader loader)
            throws ClassNotFoundException, IOException, Base64FormatException
    {
        _metaData = gamd;
        String bytes = instance.getFirstChild().getNodeValue();
        _loader = loader;
        _loader.loadClass (_metaData.getEntryClass());

        _agent = new Base64Transcoders().convertB64StringToObject (bytes,
                                                                    _loader);
    }

    /**
     * Constructs a new AgentContainer for an (outgoing) agent (Type II or
     * Serializable mobility).
     * 
     * @param gamd Agent's meta data
     * @param agent actual agent instance that implements Serializable
     */
    public AgentContainer (GridAgentMetaData gamd, Serializable agent)
    {
        _metaData = gamd;
        _agent = agent;
    }

    /**
     * Constructs a new AgentContainer for an agent with the given metadata and
     * the given state information (Type I or SelfSerializable mobility).
     * 
     * @param gamd object describing the agent's metadata
     * @param gavs object describing either the initial state of the agent or
     *            the current internal state of the agent
     */
    public AgentContainer (GridAgentMetaData gamd, GridAgentVariableState gavs)
    {
        _variableState = gavs;
        _metaData = gamd;
    }

    /**
     * Returns the agent's metadata description.
     * 
     * @return a GridAgentMetaData object containing the agent's metadata.
     */
    public GridAgentMetaData getMetaData()
    {
        return _metaData;
    }

    /**
     * Sets the agent's metadata into this AgentContainer.
     * 
     * @param gamd object containing the agent's metadata.
     */
    public void setMetaData (GridAgentMetaData gamd)
    {
        _metaData = gamd;
    }

    /**
     * Returns the classloader used to load classes from the codebase URL
     * provided with the incoming agent. It will be needed in other locations
     * where additional classes that sup- port this agent's infrastructure are
     * loaded.
     * 
     * @return a classloader for loading classes from agents codebase.
     */
    public ClassLoader getClassLoader()
    {
        return _loader;
    }

    /**
     * Returns the agent's state variables and their current values.
     * 
     * @return a GridAgentVariableState object containing the agent's state
     *         variables and their values.
     */
    public GridAgentVariableState getState()
    {
        return _variableState;
    }

    /**
     * Returns the agent object instance contained within this object.
     * 
     * @return agent object stored herein
     */
    public Serializable getObject()
    {
        return _agent;
    }

    /**
     * Sets the token recv'd from target agent system server into this. It will
     * be included in GMAML sent over with actual agent data.
     * 
     * @param tk string token received from destination server
     */
    public void setLaunchApprovalToken (String tk)
    {
        _launchApprovalToken = tk;
    }

    /**
     * Convert the AgentContainer to GMAML form. We assume that if we have an
     * agent instance internally, the mobility type is II, or Serializable.
     * 
     * @return String containing the equivalent GMAML form.
     */
    public String toExternalForm()
    {
        StringBuffer sb = new StringBuffer();

        sb.append ("<?xml version=\"1.0\" ?>\n");
        try {
            sb.append ("<!DOCTYPE gmas SYSTEM \"" + getGridCodebaseURL()
                    + "gmaml.dtd\">\n");
        }
        catch (GmasMobilityException e1) {
            e1.printStackTrace();
        }
        sb.append ("<gmas>\n");
        sb.append ("<data request=\"" + _launchApprovalToken + "\">\n");
        if (_agent == null)
            sb.append (_variableState.toExternalForm());
        else {
            sb.append ("<instance>\n");
            try {
                sb.append (new Base64Transcoders().convertObjectToB64String (_agent));
            }
            catch (Exception xcp) {
                xcp.printStackTrace();
            }
            sb.append ("\n</instance>\n");
        }
        sb.append ("</data>\n");
        sb.append ("</gmas>\n");
        return sb.toString();
    }

    /**
     * Extracts the URL from the first URL in java property java.rmi.server
     * .codebase, assuming this is set correctly, set to the active Grid
     * codebase where files pertinent to GMAS are stored for download. The
     * primary reference derived from this method is to the gmaml.dtd, the Data
     * Type Definition for the Grid Mobile Agent Markup Language.
     * 
     * @return String containing URL to the Grid codebase.
     */
    public static String getGridCodebaseURL()
            throws GmasMobilityException
    {

        String rmiCodebase = System.getProperty ("java.rmi.server.codebase");
        if (rmiCodebase == null) {
            throw new GmasMobilityException ("java.rmi.server.codebase not set");
        }
        String[] codebaseURLs = rmiCodebase.split (" ");
        URL gridCodebase = null;
        try {
            URL firstURL = new URL (codebaseURLs[0]);
            gridCodebase = new URL (firstURL.getProtocol(),
                                    firstURL.getHost(),
                                    firstURL.getPort(),
                                    "");
        }
        catch (MalformedURLException e) {
            e.printStackTrace();
        }
        _log.debug ("Got Grid codebase " + gridCodebase.toExternalForm());
        return gridCodebase.toExternalForm() + "/";
    }

    /**
     * Returns the agent's type as an integer value. We assume that if we have
     * an agent instance internally, the mobility type is II, or Serializable.
     * 
     * @return an enumerated type that corresponds to the agents mobility type.
     */
    public MobilityType getAgentType()
    {
        if (_agent != null) {
            return MobilityType.Serializable;
        }
        else {
            return MobilityType.SelfSerializable;
        }
    }

    /**
     * Display the object state. This method dumps the object state to stdout.
     * It is provided for debugging purposes.
     */
    public void dump()
    {
        if (_agent != null) {
            _log.info ("TYPE II Agent:");
            if (_metaData != null) {
                _metaData.dump();
            }
        }
        else {
            _log.info ("TYPE I Agent:");
            if (_metaData != null) {
                _metaData.dump();
            }
            if (_variableState != null) {
                _variableState.dump();
            }
        }
    }

    /**
     * Enumerated type that describes the different possible types of mobility
     * available to the GMAS Agent.
     */
    public enum MobilityType {
        SelfSerializable, Serializable
    }

    /**
     * Enumerated type that describes the different types of move operations
     * available to the GMAS agent.
     */
    public enum MoveOperationType {
        Clone, Launch
    }

    GridAgentMetaData _metaData;
    GridAgentVariableState _variableState;
    Serializable _agent = null;
    String _launchApprovalToken;
    ClassLoader _loader = null;
    static final Logger _log = LogInitializer.getLogger (AgentContainer.class.getName());
}
