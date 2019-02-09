package mil.darpa.coabs.gmas;

import java.net.InetAddress;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.Hashtable;
import java.util.Random;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import mil.darpa.coabs.gmas.messaging.GmasMessage;
import mil.darpa.coabs.gmas.messaging.GmasMessageListener;
import mil.darpa.coabs.gmas.mobility.AgentContainer;
import mil.darpa.coabs.gmas.mobility.GmasMobilityException;
import mil.darpa.coabs.gmas.mobility.GridAgentMetaData;
import mil.darpa.coabs.gmas.mobility.LaunchRequestResponse;
import mil.darpa.coabs.gmas.util.GmamlParser;
import mil.darpa.coabs.gmas.util.GridHelper;
import mil.darpa.coabs.gmas.util.MutableURLClassLoader;
import mil.darpa.coabs.gmas.util.URIEncoder;

import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.Message;

/**
 * The GmamlMessageHandlingService is an interface that grew out of the
 * NOMADSGMASLauncher and the Aglets equivalent. This was designed to register
 * with the GRID with a default name of "GMASHandler.hostname". It then
 * acts as a service, listening for requests from GRID attached GMAS entities
 * that want to launch or clone an agent to this particular agent platform. The
 * requester should encapsulate his request as an XML encoding of a launch
 * request. This request (will be) is authenticated to see if the target
 * enviroment is willing to accept this agent. If so, a Response XML message is
 * formed and returned to the Agent. The Agent then sends it's state in either a
 * Variable State object (GMAS Type 1) or a Java Serialized form (GMAS Type 2).
 * This was extended to include messaging, and provide a way for heterogeneous
 * agents to send messages to one another utilizing their respective native
 * messaging APIs.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version $Revision$ $Date$
 */
public abstract class GmamlMessageHandlingService implements Runnable {

    /**
     * Constructs an instance of this service, setting the string by which it
     * will be visible on the grid to the plain hostname, without any domain
     * suffixes.
     * <p>
     * This will typically be started by the agent system when initializing 
     * GMAS services. This is the primary GMAS support service, and the only
     * service that will persist on the Grid.
     */
    public GmamlMessageHandlingService()
    {
        String hostname = null;
        try {
            hostname = InetAddress.getLocalHost().getHostName();
        }
        catch (UnknownHostException e) {
            e.printStackTrace();
        }
        String[] results = hostname.split ("\\.");
        _gridVisibleAgentName = "GMASHandler." + results[0];

        _listeners = new Hashtable<String, GmasMessageListener>();
    }
    
    /**
     * This method must be implemented in this fashion by any subclasses. This
     * is a static method meant to provide access to the single instance of the
     * primary GMAS Service, implemented in this class. If the service is not
     * already instantiated, it will invoke the Constructor, passing in the
     * handle to the agent system environment. Not all implementations will need
     * to have access to this handle at this point, so it may invoke the empty
     * Constructor. See also (@link #start()) This service is meant to be access-
     * ible from anywhere within the GMAS implementation, via:
     * (@link mil.darpa.coabs.gmas.GridMobileAgentSystem#getGmamlMessageHandlingService())
     * 
     * @return GmamlMessageHandlingService    
     * @see us.ihmc.nomads.coabs.gmas.SpringGmamlMessageHandlingService
     * public static GmamlMessageHandlingService getGmamlMessageHandlingService (Object agentSystemHandle)
     {
        if (_gmamlMessageHandlingService == null) {
            _gmamlMessageHandlingService = new <agent system name>GmamlMessageHandlingService ({(AgletContext) agentSystemHandle});
            _gmamlMessageHandlingService.start();
        }
        
        return _gmamlMessageHandlingService;
     }
     */
    
    /**
     * Given that this class will be instantiated from the method 
     * getGmamlMessageHandlingService in the subclass and that we implement
     * Runnable here, this method is called from the static instance retrieval
     * method in the subclass. 
     * <p>
     * Suggestions for better ways to handle this, given the constraints, are
     * welcome.
     */
    public void start()
    {
        if (_gmhsThread == null) {
            _gmhsThread = new Thread (this, "GmamlMessageHandlingService");
            _gmhsThread.start();
        }
    }

    /**
     * Retrieve the object containing the incoming GMAS agent. When an agent 
     * is launched into the local agent platform, the inconsistencies in agent
     * parameters have forced us to move to this model, where the GridAgentExecutor
     * of each agent system must pull the agent down from this server process
     * once it is ready for it. 
     * 
     * @see us.ihmc.nomads.coabs.gmas.server.GridAgentExecutor
     * @return
     */
    public AgentContainer getIncomingAgent()
    {
        return _agentContainer;
    }

    /**
     * Registers a process as a listener for messages with the server. Upon
     * registration, the server returns its Name which it has used to register
     * with the Grid. The process registering as a listener will then receive 
     * incoming messages that may be appropriate for it, given the agent
     * system name that begins its process name.
     * 
     * @param listener process to register as a message listener
     * @return Grid visible Name of this object
     * @throws NullPointerException
     * @see #handleReceivedMessage(GmasMessage, String)
     */
    public synchronized String register (GmasMessageListener listener)
            throws NullPointerException
    {
        if (_gridVisibleAgentName == null) {
            throw new NullPointerException ("_gridVisibleAgentName is null");
        }

        if (listener != null && listener.getProcessName() != null) {
            _listeners.put (listener.getProcessName(), listener);
            notify();
            _log.debug ("Registered: " + listener.getProcessName()
                    + " as a Message Listener");
        }
        else {
            throw new NullPointerException (listener == null ? "Null Listener"
                    : "Null Listener Name");
        }
        return _gridVisibleAgentName;
    }

    /**
     * Deregisters a process as a listener for messages with the server.
     * 
     * @see #register(GmasMessageListener)
     * @param listener process to deregister as a message listener
     */
    public synchronized void deregister (GmasMessageListener listener)
            throws NullPointerException
    {
        if (listener != null && listener.getProcessName() != null) {
            _listeners.remove (listener.getProcessName());
        }
        else {
            throw new NullPointerException (listener == null ? "Null Listener"
                    : "Null Listener Name");
        }
    }

    /**
     * This is the primary loop for this service. It will register with the
     * Grid and listen for incoming messages. Upon receipt of a message, it
     * will be appropriately parsed and dispositioned. Launch Requests will 
     * be responded to positively, incoming agents will be launched - provided
     * that they were preceded by a launch request, and messages will be 
     * forwarded to any appropriate listener. Establish a class loader up 
     * at this level so that repeated references to the same codebase by
     * subsequent visiting GMAS agents will not require reloading of classes
     * nor re-initializing of micro environments.
     * <p>
     * for the client side of the agent transmission, reference
     * (@link mil.darpa.coabs.mobility.client.MobilityServiceImpl#mobilizeAgent(DefaultAgentRep, AgentContainer))
     * 
     */
    public void run()
    {
        Hashtable<Integer, GridAgentMetaData> incomingAgentsTable = new Hashtable<Integer, GridAgentMetaData>();
        Message gridMsg = null;

        try {
            registerWithGrid();
            GmamlParser parser = new GmamlParser();
            while (true) {
                // Check for a new message.
                gridMsg = _gridHelper.receiveMsgBlocking();
                DefaultAgentRep senderAgentRep = (DefaultAgentRep) gridMsg.getSenderAgentRep();

                if (senderAgentRep == null) {
                    _log.error ("senderAgentRep is null");
                    continue;
                }
                else {
                    _log.debug ("senderAgentRep.name:"
                            + senderAgentRep.getName());
                }

                String msgText = URIEncoder.includeSpecialChars (gridMsg.getRawText());
                // peek into msg to get type
                String gmasMsgType = parser.getMsgType (msgText);
                _log.debug ("msgType is " + gmasMsgType);
                if (gmasMsgType.equals ("launchreq")) {
                    for (Object messageObject : parser.extractMessageObjects (msgText)) {
                        if (messageObject.getClass().getName().endsWith ("GridAgentMetaData")) {
                            GridAgentMetaData gamd = (GridAgentMetaData) messageObject;
                            _log.info ("Received launch request from: "
                                    + gamd.getLaunchHost());
                            _codebaseURL = new URL (gamd.getCodeURL());
                            
                            if (_loader == null) {
                                _loader = new MutableURLClassLoader (new URL[] {_codebaseURL});
                                /*ClassLoader systemCL = this.getClass().getClassLoader();
                                _loader = new GmasURLClassLoader (new URL[] {_codebaseURL}, systemCL);
                                */
                                // set this classloader as the context classloader 
                                // as the GMAS impl of the arriving agent system 
                                // should check for this in loading its classes
                                // from the codebaseURL, so it can avoid possible
                                // ClassNotFound or ClassCast exceptions that
                                // arise when deserializing and/or casting 
                                // classes retrieved via a new URLClassLoader.
                                // This is important in Aglets, as Aglets 
                                // defines and uses its own classloader, and
                                // only has access to this classloader via
                                // Thread.currentThread().getContextClassLoader()
                                Thread.currentThread().setContextClassLoader (_loader);
                            }
                            else {
                                _loader.addURL (_codebaseURL);
                            }
                            
                            // need to eventually check with agent system
                            // environment to see if approved
                            // (authenticate userid/passwd or hostname?)
                            // presently just sending with errorcode of 0

                            Integer randomToken = new Integer (new Random().nextInt (32767));
                            // store token & metadata in hashtable
                            // form response msg with appropriate errorcode,
                            incomingAgentsTable.put (randomToken, gamd);
                            LaunchRequestResponse responseMsg = new LaunchRequestResponse (0,
                                                                                           randomToken.toString(),
                                                                                           "Welcome",
                                                                                           null);
                            try {
                                // send response back to requesting agent
                                if (senderAgentRep != null
                                        && senderAgentRep.isReachable()) {
                                    _log.info ("Sending positive Response msg back on Launch Request");
                                    _log.debug ("Sending Response msg back to : \n"
                                            + senderAgentRep.getServiceID().toString()
                                            + "\n"
                                            + responseMsg.toExternalForm());
                                    _gridHelper.sendMsg (senderAgentRep,
                                                         "GMAML",
                                                         responseMsg.toExternalForm());
                                }
                                else {
                                    throw new GmasMobilityException ("cannot reach requesting agent to return response");
                                }
                            }
                            catch (Exception xcp) {
                                xcp.printStackTrace();
                            }
                        }
                        else {
                            throw new GmasMobilityException ("not implemented yet.");
                        }
                    }
                }
                else if (gmasMsgType.equals ("response")) {
                    throw new GmasMobilityException ("not expecting a response"
                            + " GMAML msg in the GridMessageHandler");
                    //a LaunchRequestResponse is expected only in the GridMobility
                    //Service, in reponse to a LaunchRequest
                }
                else if (gmasMsgType.equals ("data")) {
                    _log.debug ("Received incoming agent");
                    Integer numToken = new Integer (parser.getToken (msgText));
                    if (incomingAgentsTable.containsKey (numToken)) {
                        // agent has been approved for launching
                        for (Object msgObject : parser.extractMessageObjects (msgText, _loader)) {
                            if (msgObject.getClass().getName().endsWith ("AgentContainer")) {
                                _agentContainer = (AgentContainer) msgObject;
                                _agentContainer.setMetaData (incomingAgentsTable.get (numToken));
                            }
                            else {
                                _log.debug ("Can only handle Generic Grid agents, not "
                                        + msgObject.getClass().getName());
                            }
                        }
                        try {
                            _log.debug ("Trying to launch agent");
                            launchGridAgentNatively();
                        }
                        catch (Exception xcp) {
                            xcp.printStackTrace();
                            throw new GmasMobilityException ("communication failure\n");
                        }
                    }
                    else {
                        // ignore request as agent was not approved for entry
                        _log.error ("Agent's token does not match any in hashtable\n");
                    }
                }
                else if (gmasMsgType.equals ("message")) {
                    GmasMessage msg = parser.convertToMessage (msgText);
                    _log.info ("Received GMAS Message of type: " 
                               + msg.getMessageType());
                    handleReceivedMessage (msg, senderAgentRep.getName());
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }
    
    /**
     * Invoked when we receive a GMAML msg that contains a GmasMessage. It will
     * invoke the method GmasMessageListener#receiveMessage upon registered
     * message listeners. It will deliver the message provided that the message
     * is intended either for a generic GMAS Reference Implementation installa-
     * tion or if the message is intended for an agent system type that matches
     * the prefix of the classname registered as a listener(eg., if the agent 
     * system type it is intended for is 'aglets' and the message listener is 
     * named AgletsGmasMessageListenerImpl)
     * <p>
     * In some instances, messages may arrive for an agent before the agents
     * environment and support services have been set up. If we cannot find any
     * listeners, we will block until one becomes available. If we cannot find
     * an appropriate listener, we'll wait for 30 seconds, periodically re-
     * attempting to find the right listener. The architecture should not allow
     * this to present a deadlock, but if it presents extra delay, we may want
     * to place it in its own thread.
     * 
     * @param gmasMsg message just received that this method will handle
     * @param sender Grid agent that sent the message - should be name from 
     *               CoABSAgentDescription
     * @see mil.darpa.coabs.mobility.util.GmasMessageListener#receiveMessage(GmasMessage,
     */
    protected synchronized void handleReceivedMessage (GmasMessage gmasMsg, String sender)
    {
        boolean matchedOnListener = false;
        int iterations = 0;
        try {
            _log.debug ("there are " + _listeners.size()
                    + " GmasMessageListeners");
            if (_listeners.size() == 0) {
                wait();
            }
            if (gmasMsg != null) {
                GmasMessage.AgentSystemType agentSystemType = gmasMsg.getRecipientAgentSystemType();
                do {
                    iterations++;
                    for (GmasMessageListener gml : _listeners.values()) {
                        if (agentSystemType == GmasMessage.AgentSystemType.gmasrefimpl ) { 
                            _log.debug ("Calling receiveMessage on a GmasMessageListener: " + gml.getClass().getName());
                            matchedOnListener = true;
                            gml.receiveMessage (gmasMsg, sender);
                        }
                        else {
                            String[] classNameElements = gml.getClass().getName().split ("\\.");
                            String className = classNameElements[classNameElements.length - 1].toLowerCase();
                            int endIndex = className.indexOf ("gmas");
                            if (endIndex != -1) {//try to match on specific agent system
                                if (agentSystemType == GmasMessage.toAgentSystemType (className.substring (0, endIndex))) {
                                    _log.debug ("Calling receiveMessage on a GmasMessageListener: " + gml.getClass().getName());
                                    matchedOnListener = true;
                                    gml.receiveMessage (gmasMsg, sender);
                                }
                                else {
                                    _log.debug ("Did not match " + agentSystemType + " to GmasMessageListener: " + gml.getClass().getName());
                                }
                            }
                        }
                    }
                    if (matchedOnListener == false) {
                        wait();
                    }
                } while (matchedOnListener == false && iterations < 2);
            }
            else {
                throw new GmasMobilityException ("Unable to deliver the message locally:"
                        + gmasMsg == null ? "Message is null"
                        : "Destination is unknown." + gmasMsg.toString());
            }
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /**
     * Launch the special agent in the local agent system. This special agent
     * then prepares and executes the GridMobileAgent just received via GMAS. 
     * 
     * @see us.ihmc.nomads.coabs.gmas.mobility.GridAgentExecutor
     * @see com.ibm.maf.gmas.mobility.GridAgentExecutor
     * @see #run()
     */
    protected abstract void launchGridAgentNatively();
    
    /**
     * Registers this Message Handling Service with the CoABS Grid, if not 
     * previously registered. Establishes a gridHelper object to provide basic
     * Grid services.
     */
    void registerWithGrid()
            throws GmasMobilityException
    {
        if (_gridHelper != null) {
            // already registered.
            return;
        }
        
        String description = "GMAS Message Handler";
        String[] agentCommunicationLanguages = new String[] {
                "NaturalLanguage", "GMASMessaging"};
        String agentSystemArchitecture = "GMAS";
        String[] contentLanguages = new String[] {"Java"};
        String displayIconURL = "http://iconurl";
        String documentationURL = "http://docurl";
        String[] ontologies = new String[] {};
        String organization = "unknown";
        
        try {
            _gridHelper = new GridHelper (_gridVisibleAgentName,
                                          description,
                                          agentCommunicationLanguages,
                                          agentSystemArchitecture,
                                          contentLanguages,
                                          displayIconURL,
                                          documentationURL,
                                          ontologies,
                                          organization);
        }
        catch (Exception xcp) {
            xcp.printStackTrace();
            throw new GmasMobilityException ("failed to register with grid");
        }
    }


    
    protected static GmamlMessageHandlingService _gmamlMessageHandlingService = null;
    protected String _gridVisibleAgentName = null;
    protected Hashtable<String, GmasMessageListener> _listeners = null;
    protected URL _codebaseURL = null;
    protected Logger _log = LogInitializer.getLogger (this.getClass().getName());
    
    Thread _gmhsThread = null;
    GridHelper _gridHelper = null;
    AgentContainer _agentContainer = null;
    MutableURLClassLoader _loader = null;
}


