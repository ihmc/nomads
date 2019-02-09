package mil.darpa.coabs.gmas.mobility;
/**
 * GridAgentMetaData.java Copyright 2000 Arne Grimstrup, Department of Computer
 * Science, Dartmouth College Use, modification and redistribution subject to
 * the terms of the standard GNU public license at 
 * http://www.gnu.org/copyleft/gpl.html.
 * <p>
 * GridAgentMetaData manages the metadata of a Grid Mobile Agent. In addition,
 * GridAgentMetaData provides conversion methods to and from GMAML.
 *
 * @author Arne Grimstrup
 * @author Tom Cowin <tom.cowin@gmail.com> modifications to extend GMAS for 
 * messaging, additional platform support
 * @version $Revision$ $Date$
 * @see mil.darpa.coabs.mobility.GridMobileAgent
 * @see mil.darpa.coabs.mobility.GridMobilityService
 * @see mil.darpa.coabs.mobility.GmamlMessageHandlingService
 */

import java.io.Serializable;


import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class GridAgentMetaData implements Serializable {

    /**
     * Constructor. Create a new GridAgentMetaData for an agent with the given
     * description.
     * 
     * @param agentName agent name
     * @param agentUID agent identifier
     * @param agentMobilityType mobility type
     * @param agentMoveType mobility operation underway
     * @param agentURL URL containing the agent's code
     * @param agentLanguage agent's implementation language
     * @param launchHost host that launched the agent
     * @param agentParent parent agent
     * @param entryClass class that agent should begin execution
     * @param entryMethod method that agent should begin execution
     * @param reportToName name of agent that receives reports from this agent
     * @param reportToID id of agent that receives reports
     */
    public GridAgentMetaData (String agentName,
                              String agentUID,
                              AgentContainer.MobilityType mobilityType,
                              AgentContainer.MoveOperationType moveOperationType,
                              String codebaseURL,
                              String agentLanguage,
                              String launchHost,
                              String agentParent,
                              String entryClass,
                              String entryMethod,
                              String reportToName,
                              String reportToID)
    {
        _agentName = agentName;
        _agentUID = agentUID;
        _mobilityType = mobilityType;
        _moveOperationType = moveOperationType;
        _codebaseURL = codebaseURL;
        _agentLanguage = agentLanguage;
        _launchHost = launchHost;
        _agentParent = agentParent;
        _entryClass = entryClass;
        _entryMethod = entryMethod;
        _reportToName = reportToName;
        _reportToID = reportToID;
    }
    
    /**
     * Constructor. Create a new GridMobileAgentState from the GMAML
     * representation.
     * 
     * @param gmanode root node of the DOM tree
     * @see org.w3c.dom.Element
     * @see org.w3c.dom.Node
     */
    public GridAgentMetaData (Node gmanode)
    {

        // The gridmobileagent tag has no attributes so we can go immediately
        // to the component tags.
        try {
            _mobilityType = toMobilityType (((Element) gmanode).getAttribute ("type"));
            _moveOperationType = toAgentMoveType (((Element) gmanode).getAttribute ("op"));
            NodeList anl = gmanode.getChildNodes();
            int asize = anl.getLength();
            for (int j = 0; j < asize; j++) {
                if (anl.item (j).getNodeName().compareTo ("id") == 0) {
                    _agentName = ((Element) anl.item (j)).getAttribute ("name");
                    _agentUID = ((Element) anl.item (j)).getAttribute ("agentid");
                }
                else if (anl.item (j).getNodeName().compareTo ("url") == 0) {
                    _agentLanguage = ((Element) anl.item (j)).getAttribute ("lang");
                    _codebaseURL = anl.item (j).getFirstChild().getNodeValue();
                }
                else if (anl.item (j).getNodeName().compareTo ("launcher") == 0) {
                    _launchHost = ((Element) anl.item (j)).getAttribute ("name");
                    _agentParent = ((Element) anl.item (j)).getAttribute ("agent");
                }
                else if (anl.item (j).getNodeName().compareTo ("reportto") == 0) {
                    _reportToName = ((Element) anl.item (j)).getAttribute ("name");
                    _reportToID = ((Element) anl.item (j)).getAttribute ("agentid");
                }
                else if (anl.item (j).getNodeName().compareTo ("entry") == 0) {
                    _entryClass = ((Element) anl.item (j)).getAttribute ("class");
                    _entryMethod = ((Element) anl.item (j)).getAttribute ("method");
                }
            }
        } catch (Exception e) {
            _log.error ("Invalid GridAgentMetaData");
        }
    }

    /**
     * Convert string type received in text rep to the appropriate enumerated
     * type.
     * 
     * @param type string corresponding to mobility type
     * @return enumerated MobilityType corresponding to input string
     */
    public static AgentContainer.MobilityType toMobilityType (String type)
    {
        if (type.equals ("AgentContainer.MobilityType.SelfSerializable")
                || type.equals ("MobilityType.SelfSerializable") 
                || type.equals ("SelfSerializable")) {
            return AgentContainer.MobilityType.SelfSerializable;
        }
        else if (type.equals ("AgentContainer.MobilityType.Serializable")
                || type.equals ("MobilityType.Serializable") 
                || type.equals ("Serializable")) {
            return AgentContainer.MobilityType.Serializable;
        }
        return null;
    }

    /**
     * Convert string type received in text rep to the appropriate enumerated
     * type.
     * 
     * @param type string that corresponds to the Agent Move Type
     * @return enumerated agent move type
     */
    public static AgentContainer.MoveOperationType toAgentMoveType (String type)
    {
        if (type.equals ("AgentContainer.MoveOperationType.Clone")
                || type.equals ("MoveOperationType.Clone") 
                || type.equals ("Clone")) {
            return AgentContainer.MoveOperationType.Clone;
        }
        else if (type.equals ("AgentContainer.MoveOperationType.Launch")
                || type.equals ("MoveOperationType.Launch") 
                || type.equals ("Launch")) {
            return AgentContainer.MoveOperationType.Launch;
        }
        return null;
    }
    
    /** Agent Name retrieval method. */
    public String getName()
    {
        return _agentName;
    }

    /** Agent ID retrieval method. */
    public String getId()
    {
        return _agentUID;
    }

    /** Type retrieval method. */
    public AgentContainer.MobilityType getType()
    {
        return _mobilityType;
    }

    /** Operation retrieval method. */
    public AgentContainer.MoveOperationType getOperation()
    {
        return _moveOperationType;
    }

    /** Operation update method. */
    public void setOperation (AgentContainer.MoveOperationType newoperation)
    {
        _moveOperationType = newoperation;
    }

    /** Code URL retrieval method. */
    public String getCodeURL()
    {
        return _codebaseURL;
    }

    /** Code URL set method. */
    public void setCodeURL (String newURL)
    {
        _codebaseURL = newURL;
    }

    /** Code Language retrieval method. */
    public String getCodeLang()
    {
        return _agentLanguage;
    }

    /** Launching Host Name retrieval method. */
    public String getLaunchHost()
    {
        return _launchHost;
    }

    /** Launching Agent Name retrieval method. */
    public String getLaunchAgent()
    {
        return _agentParent;
    }

    /** Execution Entry Class Name retrieval method. */
    public String getEntryClass()
    {
        return _entryClass;
    }

    /** Execution Entry Method Name retrieval method. */
    public String getEntryMethod()
    {
        return _entryMethod;
    }

    /**
     * Execution Entry Class Name update method.
     * 
     * @param newclassname string containing the new entry class name.
     */
    public void setEntryClass (String newclassname)
    {
        _entryClass = newclassname;
    }

    /**
     * Execution Entry Method Name update method.
     * 
     * @param newentrymethod string containing the new entry method name.
     */
    public void setEntryMethod (String newentrymethod)
    {
        _entryMethod = newentrymethod;
    }

    /** Report Receiving Agent Name retrieval method. */
    public String getReportName()
    {
        return _reportToName;
    }

    /** Report Receiving Agent Id retrieval method. */
    public String getReportId()
    {
        return _reportToID;
    }

    /**
     * Convert the GridAgentMetaData to GMAML form. The only context in
     * which this is used in GMAML is as a request to launch an agent to
     * a GMAS enabled host, hence the 'launchreq' tags.
     * 
     * @return String containing the equivalent GMAML form.
     * @see mil.darpa.coabs.mobility.client.MobilityServiceImpl
     */
    public String toExternalForm()
    {
        StringBuffer sb = new StringBuffer();

        sb.append ("<launchreq>\n");
        sb.append ("<gridmobileagent type=\"" + _mobilityType + "\" op=\""
                + _moveOperationType + "\">\n");
        sb.append ("<id  name=\"" + _agentName + "\" agentid=\"" + _agentUID + "\"/>\n");
        sb.append ("<launcher name=\"" + _launchHost + "\" agent=\""
                + _agentParent + "\" />\n");
        sb.append ("<reportto name=\"" + _reportToName + "\" agentid=\""
                + _reportToID + "\" />\n");
        sb.append ("<url lang=\"" + _agentLanguage + "\">" + _codebaseURL + "</url>\n");
        sb.append ("<entry class=\"" + _entryClass + "\" method=\""
                + _entryMethod + "\" />\n");
        sb.append ("</gridmobileagent>\n");
        sb.append ("</launchreq>\n");
        
        return sb.toString();
    }

    /** 
     *  This method dumps the object state to the log.  It is provided for 
     *  debugging purposes.
     */
    public void dump()
    {
        _log.info ("Agent: " + _agentName + " Id: " + _agentUID + " Type: " + _mobilityType);
        _log.info ("Executing: " + _moveOperationType);
        _log.info ("Launch Host: " + _launchHost + " Agent: " + _agentParent);
        _log.info ("Report To Agent: " + _reportToName + " Id: " + _reportToID);
        _log.info ("Code Language: " + _agentLanguage + " URL: " + _codebaseURL);
        _log.info ("Entry Class: " + _entryClass + " Method: " + _entryMethod);
    }
    
    /** Name of the agent. */
    protected String _agentName;
    /** Identifier of the agent. */
    protected String _agentUID;
    /** Mobility type. Can be either SelfSerializable or (Java) Serializable */
    protected AgentContainer.MobilityType _mobilityType;
    /** Mobility operation. Can be either "clone" or "launch". */
    protected AgentContainer.MoveOperationType _moveOperationType;
    /** URL where the agent's code is located. */
    protected String _codebaseURL;
    /** Programming language the agent is implemented in. */
    protected String _agentLanguage;
    /** Host that originated the agent. */
    protected String _launchHost;
    /** Parent of this agent. */
    protected String _agentParent;
    /** Class where the agent should resume execution. */
    protected String _entryClass;
    /** Method that should be invoked when the agent resumes execution. */
    protected String _entryMethod;
    /** Name of the agent that reports should be sent. */
    protected String _reportToName;
    /** Identifier of the report receiving agent. */
    protected String _reportToID;

    static final long serialVersionUID = 0L;

    static final Logger _log = LogInitializer.getLogger (GridAgentMetaData.class.getName());
}
