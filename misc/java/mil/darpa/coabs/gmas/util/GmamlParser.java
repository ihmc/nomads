package mil.darpa.coabs.gmas.util;

/**
 * GmamlParser.java Copyright 2000 Arne Grimstrup, Department of Computer
 * Science, Dartmouth College Use, modification and redistribution subject to
 * the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 * <p>
 * GmamlParser handles the parsing and conversion of GMAML-based messages into
 * their respective objects. The content of a GMAML message is described by its
 * Data Type Definition, gmaml.dtd, and can contain several different items
 * which are mirrored by Java classes. These can be either a GridAgentMetaData
 * (aka LaunchRequest), a LaunchRequestResponse, a GridAgentVariableState, or a
 * GmasMessage object. This class serves to parse the incoming XML and translate
 * it into the corresponding java objects. See (@link #walkDocumentTree(Element))
 * for a more detailed description of how this happens.
 * 
 * @author Arne Grimstrup
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version $Revision$ $Date$
 * @see AgentContainer
 */

import java.io.InputStream;
import java.io.StringReader;
import java.util.Vector;

import mil.darpa.coabs.gmas.messaging.GmasMessage;
import mil.darpa.coabs.gmas.mobility.AgentContainer;
import mil.darpa.coabs.gmas.mobility.GridAgentMetaData;
import mil.darpa.coabs.gmas.mobility.GridAgentVariableState;
import mil.darpa.coabs.gmas.mobility.LaunchRequestResponse;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import org.apache.xerces.parsers.DOMParser;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

import org.xml.sax.ErrorHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

public class GmamlParser {

    /**
     * Identify the embedded objects in the GMAML message passed in as an
     * InputStream. Convert each object to its appropriate Java representation
     * and include it in a returned vector.
     * 
     * @param xmlInputStream the input data stream
     * @return vector that contains the Java Objects described by the GMAML
     *         message.
     */
    public Vector extractMessageObjects (InputStream xmlInputStream)
    {
        return extractMessageObjects (new InputSource (xmlInputStream));
    }

    /**
     * Identify the embedded objects in the GMAML message passed in as an
     * String. Convert each object to its appropriate Java representation and
     * include it in a returned vector.
     * 
     * @param xmlString the input data message in GMAML
     * @return vector that contains the Java Objects described by the GMAML
     *         message.
     */
    public Vector extractMessageObjects (String xmlString)
    {
        return extractMessageObjects (new InputSource (new StringReader (xmlString)));
    }

    /**
     * Identify the embedded objects in the GMAML message passed in as an
     * String. Convert each object to its appropriate Java representation and
     * include it in a returned vector. Use the included classloader to load any
     * classes needed while converting the message elements to objects.
     * 
     * @param xmlString the input data message in GMAML
     * @param loader classloader to use in loading any classes needed while
     *            converting message elements to objects
     * @return vector that contains the Java Objects described by the GMAML
     *         message.
     */
    public Vector extractMessageObjects (String xmlString, ClassLoader loader)
    {
        _loader = loader;
        return extractMessageObjects (new InputSource (new StringReader (xmlString)));
    }

    /**
     * Extract the GmasMessage that is embedded in the GMAML message passed in
     * as a String. The GmasMessage is a MAS-neutral message object that is
     * utilized for inter agent messaging.
     * 
     * @param xmlString the input data message in GMAML
     * @return GmasMessage that contains the Java Objects described by the GMAML
     *         message.
     */
    public GmasMessage convertToMessage (String xmlString)
    {
        Vector msgObjects = extractMessageObjects (xmlString);
        if (msgObjects.size() == 1
                && msgObjects.elementAt (0) instanceof GmasMessage) {
            return (GmasMessage) msgObjects.elementAt (0);
        }
        else {
            _log.error ("Did not get message object when parsing: " + xmlString);
        }
        return null;
    }

    /**
     * Parse the XML message to rtn the type of message.
     * 
     * @param xmlString the XML message
     * @return String - type of GMAML msg primarily invoked from Launcher
     *         side(GmamlMessageHandlingService)
     */
    public String getMsgType (String xmlString)
    {
        NodeList nodeList = extractNodeList (xmlString);
        int size = nodeList.getLength();
        for (int i = 0; i < size; i++) {
            _log.debug ("node " + i + ":" + nodeList.item (i).getNodeName());
            if (!nodeList.item (i).getNodeName().equals ("#text")) {
                return (nodeList.item (i).getNodeName());
            }
        }
        return null;
    }

    /**
     * Parse the XML message to retrieve the token attribute fm the incoming
     * agents data.
     * 
     * @param istring the XML message
     * @return String the token originally issued by server on agent approval
     *         primarily invoked from Launcher side(GmamlMessageHandlingService)
     */
    public String getToken (String xmlString)
    {
        NodeList nodeList = extractNodeList (xmlString);
        int size = nodeList.getLength();
        for (int i = 0; i < size; i++) {
            if ((nodeList.item (i)).getNodeName().equals ("data")) {
                return (String) ((Element) nodeList.item (i)).getAttribute ("request");
            }
        }
        _log.error ("token not found");
        return null;
    }

    /**
     * Extract the agent descriptions from the GMAML launch requests.
     * 
     * @param istring the input data stream
     * @return a list of the agents to be launched
     */
    private NodeList extractNodeList (String xmlString)
    {
        Element root = null;
        DOMParser parser = new DOMParser();
        Errors errors = new Errors();
        parser.setErrorHandler (errors);

        try {
            parser.parse (new InputSource (new StringReader (xmlString)));
            Document document = parser.getDocument();
            root = document.getDocumentElement();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return root.getChildNodes();
    }

    /**
     * Identify the embedded objects in the GMAML message passed in as an
     * InputSource. Convert each object to its appropriate Java representation
     * and include it in a returned vector.
     * 
     * @param InputSource the SAX Input Source
     * @return vector that contains the Java Objects described by the GMAML
     *         message.
     */
    private Vector extractMessageObjects (InputSource inputSource)
    {
        DOMParser parser = new DOMParser();
        Errors errors = new Errors();
        Vector messageObjects = null;

        // The parser needs error handlers for the different type of errors it
        // detects so we establish the correct handlers using the Errors class.
        parser.setErrorHandler (errors);
        try {
            // Because we are using the DOM parser, the results of the parsing
            // of the input stream are stored in a tree structure.
            parser.parse (inputSource);
            Document document = parser.getDocument();
            messageObjects = walkDocumentTree (document.getDocumentElement());
        }
        catch (Exception e) {
            e.printStackTrace();
        }
        return messageObjects;
    }

    /**
     * Convert the object descriptions from DOM representation to the
     * appropriate objects. At the top level, this can contain either a launch
     * request, a response to a launch request, an agent description, or a
     * message.
     * <p>
     * The launch request contains the agents meta data, and is answered with a
     * launch request response, indicating whether or not the destination agent
     * system is willing to accept the incoming agent.
     * <p>
     * The agent description can contain either the agent's state, or its
     * serialized instance.
     * <p>
     * GMAS supports both self-serializable mobility, in which the agent is
     * responsible for transporting itself, and Java serialization, where the
     * actual instance of the agent is serialized and then transported (as a
     * Base64 encoded string) via the GMAML message. The former method of
     * mobility is represented by two objects - the variable state and the meta
     * data. The latter method is represented by the meta data and the instance.
     * <p>
     * The message is represented by the GMAS message object.
     * 
     * @param root the root node of the DOM tree.
     * @return a vector of corresponding objects
     */
    private Vector walkDocumentTree (Element root)
    {
        Vector<Object> messageObjects = new Vector<Object>();
        NodeList nodeList = root.getChildNodes();
        int size = nodeList.getLength();

        for (int i = 0; i < size; i++) {
            _log.debug ("Examining Node " + nodeList.item (i).getNodeName());
            if ((nodeList.item (i)).getNodeName().equals ("launchreq")) {
                NodeList childrensNodeList = nodeList.item (i).getChildNodes();
                int asize = childrensNodeList.getLength();
                for (int j = 0; j < asize; j++) {
                    if ((childrensNodeList.item (j)).getNodeName().equals ("gridmobileagent")) {
                        try {
                            _gamd = new GridAgentMetaData (childrensNodeList.item (j));
                            _log.info ("codebase URL is: "
                                    + _gamd.getCodeURL());
                            // add this to the vector to be passed back to the
                            // caller and store it internally so it's here for
                            // construction of agentContainer below
                            messageObjects.addElement (_gamd);
                        }
                        catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    else if ((childrensNodeList.item (j)).getNodeName().equals ("#text")) {
                        continue;
                    }
                    else {
                        _log.error ("cannot handle agent description "
                                + nodeList.item (i).getNodeName() + ".");
                    }
                }
            }
            else if ((nodeList.item (i)).getNodeName().equals ("response")) {
                messageObjects.addElement (new LaunchRequestResponse (nodeList.item (i)));
            }
            else if ((nodeList.item (i)).getNodeName().equals ("data")) {
                AgentContainer agentContainer = null;
                NodeList childrensNodeList = nodeList.item (i).getChildNodes();
                int asize = childrensNodeList.getLength();
                for (int j = 0; j < asize; j++) {
                    if ((childrensNodeList.item (j)).getNodeName().equals ("instance")) {
                        try {
                            if (_gamd == null) {
                                _log.error ("_gamd is null when parsing instance data");
                            }
                            // utilize _gamd object obtained via parsing of
                            // launch request previously
                            agentContainer = new AgentContainer (_gamd,
                                                                 childrensNodeList.item (j),
                                                                 _loader);
                            messageObjects.addElement (agentContainer);
                        }
                        catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    else if ((childrensNodeList.item (j)).getNodeName().equals ("state")) {
                        GridAgentVariableState gavs = new GridAgentVariableState (childrensNodeList.item (j));
                        // utilize _gamd object obtained via parsing of launch
                        // request previously
                        agentContainer = new AgentContainer (_gamd, gavs);
                        messageObjects.addElement (agentContainer);
                    }
                }
            }
            else if ((nodeList.item (i)).getNodeName().equals ("message")) {
                messageObjects.addElement (new GmasMessage (nodeList.item (i)));
            }
            else if ((nodeList.item (i)).getNodeName().equals ("#text")) {
                continue;
            }
            else {
                _log.debug ("cannot handle agent description "
                        + nodeList.item (i).getNodeName() + ".");
                continue;
            }
        }
        return messageObjects;
    }

    private ClassLoader _loader = null;
    private GridAgentMetaData _gamd = null;
    protected static Logger _log = LogInitializer.getLogger (GmamlParser.class.getName());
}

/**
 * This class provides the error handlers for the parser. At this point, I don't
 * understand enough about the XML parser to add more clever handling code, so
 * simple print statements will have to do. Arne.
 */
class Errors implements ErrorHandler {

    public void warning (SAXParseException ex)
    {
        _log.error ("Warning: " + ex.getMessage());
    }

    public void error (SAXParseException ex)
    {
        _log.error ("Error: " + ex.getMessage());
    }

    public void fatalError (SAXParseException ex)
            throws SAXException
    {
        _log.error ("Fatal Error: " + ex.getMessage());
    }

    protected static Logger _log = LogInitializer.getLogger (Errors.class.getName());
}
