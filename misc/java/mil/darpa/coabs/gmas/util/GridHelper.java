/**
 * GridHelper.java Provides a concise set of convenient routines with which to
 * interface to the CoABS Grid, and utilize the registration, lookup and
 * messaging services provided by its infrastructure. Copyright 2001 Institute
 * for Human and Machine Cognition, University of West Florida
 * 
 * @author Greg Hill
 * @author Niranjan Suri
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version $Revision$ $Date$
 */

package mil.darpa.coabs.gmas.util;

import java.io.IOException;
import java.io.Serializable;
import java.util.Vector;

import mil.darpa.coabs.gmas.GmasServiceException;
import mil.darpa.coabs.gmas.messaging.GmasMessagingException;
import mil.darpa.coabs.gmas.mobility.GmasMobilityException;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

import net.jini.core.entry.Entry;
import net.jini.core.lookup.ServiceID;

import com.globalinfotek.coabsgrid.AgentRegistrationHelper;
import com.globalinfotek.coabsgrid.BasicMessage;
import com.globalinfotek.coabsgrid.DataMessage;
import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.Directory;
import com.globalinfotek.coabsgrid.Message;
import com.globalinfotek.coabsgrid.MessageListener;
import com.globalinfotek.coabsgrid.MessageQueue;
import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

public class GridHelper implements MessageListener {

    /**
     * Constructor - parameters match those necessary to create a
     * CoABSAgentDescription, which it then registers with the Grid. See (@link
     * #register(String, Vector)).
     * 
     * @param name identification string to be visible on the Grid
     * @param description human understandable string describing registree
     * @param acls array of AgentCommunicationLanguages supported by the
     *            registree
     * @param architecture agent system descriptor
     * @param contentLanguages array of content languages that agent is in
     * @param displayIconURL URL with location of icon to be used as representa-
     *            tion of the registree in the GUI
     * @param documentationURL URL with location of documentation related to
     *            this agent and agent system
     * @param ontologies array of ontologies supported by the registree
     * @param organization org from which this agent was started
     * @throws GmasServiceException
     */
    public GridHelper (String name,
                       String description,
                       String[] acls,
                       String architecture,
                       String[] contentLanguages,
                       String displayIconURL,
                       String documentationURL,
                       String[] ontologies,
                       String organization) throws GmasServiceException
    {
        try {
            _directory = new Directory();
        }
        catch (Exception e) {
            e.printStackTrace();
            throw new GmasServiceException ("could not instantiate Directory object");
        }

        CoABSAgentDescription cadesc = new CoABSAgentDescription();

        cadesc.name = name;
        cadesc.description = description;
        cadesc.acls = acls;
        cadesc.architecture = architecture;
        cadesc.contentLanguages = contentLanguages;
        cadesc.displayIconURL = displayIconURL;
        cadesc.documentationURL = documentationURL;
        cadesc.ontologies = ontologies;
        cadesc.organization = organization;

        _name = name;
        _agentDescriptions = new Vector<CoABSAgentDescription> (1);
        _agentDescriptions.addElement (cadesc);

        register (name, _agentDescriptions);
    }

    /**
     * Registers an agent with the Grid. Set up necessary messagelistener in
     * order to retrieve any messages bound for this agent. Capture registration
     * confirmation messages so caller doesn't have to.
     * 
     * @param name The human readable name of the agent. Cannot be null.
     * @param agentDescriptions a vector of CoABSAgentDescription objects
     * @throws GmasServiceException
     */

    public void register (String name, Vector agentDescriptions)
            throws GmasServiceException
    {
        // check for null params
        if (name == null) {
            throw new NullPointerException ("name");
        }

        if (agentDescriptions == null) {
            throw new NullPointerException ("agentDescriptions");
        }

        _agentDescriptions = new Vector<CoABSAgentDescription> (agentDescriptions.size());

        // build up the list of COABSAgentDescriptions
        for (int i = 0; i < agentDescriptions.size(); i++) {
            CoABSAgentDescription cadesc = (CoABSAgentDescription) agentDescriptions.elementAt (i);
            _agentDescriptions.addElement (cadesc);
        }

        Entry[] entries = new Entry[_agentDescriptions.size()];
        _agentDescriptions.copyInto (entries);

        try {
            _agentRegHelper = new AgentRegistrationHelper (name);
        }
        catch (Exception x) {
            throw new GmasServiceException ("Could not create the GridHelper");
        }

        _msgQueue = _agentRegHelper.getMessageQueue();
        _agentRep = (DefaultAgentRep) _agentRegHelper.getAgentRep();
        _gridName = _agentRegHelper.getName();

        try {
            _agentRegHelper.registerAgent ("");
        }
        catch (Exception x) {
            x.printStackTrace();
            throw new GmasServiceException ("Could not register '" + name
                    + "' with grid " + _gridName);
        }

        _msgQueue.addMessageListener (this);

        try {
            Thread.sleep (1000);
        }
        catch (InterruptedException e) {
            e.printStackTrace();
        }

        Message registrationMsg = receiveMsgNonBlocking();
        if (registrationMsg == null) {
            _log.error ("Did not get a registration message returned");
        }
    }

    /**
     * Updates the grid registration just after an agent arrives.
     * 
     * @throws GmasServiceException
     */
    public void updateRegistration()
            throws GmasServiceException
    {
        try {
            _agentRegHelper.updateRegistration();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        Message registrationMsg = receiveMsgNonBlocking();
        if (registrationMsg == null) {
            _log.error ("Did not get a update registration message returned");
        }
    }

    /**
     * Updates the grid registration with the provided agent description just
     * after an agent arrives.
     * 
     * @param new agent description to utilize in place of what exists
     * @throws GmasServiceException
     */
    public void updateRegistration (CoABSAgentDescription agentDescription)
            throws GmasServiceException
    {
        try {
            _agentRegHelper.updateRegistration();
            _agentRegHelper.setCoABSAgentDescription (agentDescription);
        }
        catch (IOException e) {
            e.printStackTrace();
        }

        Message registrationMsg = receiveMsgNonBlocking();
        if (registrationMsg == null) {
            _log.error ("Did not get a update registration message returned");
        }
    }

    /**
     * Invoked by an agent in order to receive a message - will block until
     * receipt.
     * 
     * @return a Message object containing the message.
     */
    public Message receiveMsgBlocking()
    {
        Message msg = _msgQueue.pop();
        while (msg == null) {
            waitSync();
            msg = _msgQueue.pop();
        }
        return msg;
    }

    /**
     * Invoked by an agent in order to receive a message - will return
     * immediately whether or not message is in queue.
     * 
     * @return a Message object if one was available; null otherwise
     */
    public Message receiveMsgNonBlocking()
    {
        Message msg = _msgQueue.pop();

        if (msg == null) {
            return null;
        }

        return msg;
    }

    /**
     * Sends a text message to an agent specifed by an DefaultAgent Rep on the
     * Grid.
     * 
     * @param ari the AgentRep for the receiving agent
     * @param acl a String containing the agent communication language
     * @param msg a String containing the actual text message
     */
    public void sendMsg (DefaultAgentRep ari, String acl, String msg)
            throws GmasMessagingException
    {
        // check for null params
        if (ari == null) {
            throw new NullPointerException ("ari");
        }
        if (acl == null) {
            throw new NullPointerException ("acl");
        }
        if (msg == null) {
            throw new NullPointerException ("msg");
        }

        try {
            ari.addMessage (new BasicMessage (ari.getName(),
                                              _agentRep,
                                              acl,
                                              msg));
        }
        catch (Exception x) {
            throw new GmasMessagingException ("Could not send message to '"
                    + ari.getName() + "'");
        }
    }

    /**
     * Sends a data message, containing an Object, to a specified agent on the
     * Grid.
     * 
     * @param ari the AgentRep for the receiving agent
     * @param acl a String containing the agent communication language
     * @param msg a String containing the actual text message
     * @throws GmasMessagingException
     */

    public void sendDataMsg (DefaultAgentRep ari,
                             String acl,
                             String msg,
                             Object obj)
            throws GmasMessagingException
    {
        // check for null params
        if (ari == null) {
            throw new NullPointerException ("ari");
        }
        if (acl == null) {
            throw new NullPointerException ("acl");
        }
        if (msg == null) {
            throw new NullPointerException ("msg");
        }
        if (obj == null) {
            throw new NullPointerException ("obj");
        }

        try {
            ari.addMessage (new DataMessage (ari.getName(),
                                             _agentRep,
                                             acl,
                                             msg,
                                             (Serializable) obj));
        }
        catch (Exception x) {
            throw new GmasMessagingException ("Could not send message to '"
                    + ari.getName() + "'");
        }
    }

    /**
     * Attempts to find an agent with the specified name on the Grid.
     * 
     * @param agentName the name of the agent to be found on the grid
     * @return an AgentRep of the agent that was found; if no agent was found,
     *         null is returned
     * @throws GmasServiceException
     */

    public DefaultAgentRep findAgentByName (String agentName)
            throws GmasServiceException
    {
        // Check for null params
        if (agentName == null) {
            throw new NullPointerException ("agentName");
        }

        // Perform the grid lookup
        DefaultAgentRep ari = null;

        try {
            ari = (DefaultAgentRep) _directory.getAgentRepByName (agentName);
        }
        catch (Exception x) {
            throw new GmasServiceException ("Error while performing agent lookup on '"
                    + agentName + "' with grid " + _gridName);
        }

        if (ari == null) {
            _log.info ("Unable to find agent " + agentName + " with grid "
                    + _gridName);
            return null;
        }
        return ari;
    }

    /**
     * Deregisters the agent represented within this object from the grid.
     * 
     * @throws GmasServiceException
     */
    public void deregister()
            throws GmasServiceException
    {
        Entry[] entries = new Entry[_agentDescriptions.size()];

        _agentDescriptions.toArray (entries);

        try {
            _agentRegHelper.deregisterAgent ("");
        }
        catch (Exception xcp) {
            throw new GmasServiceException ("Could not deregister agent");
        }
        _msgQueue.removeMessageListener (this);

        _agentDescriptions = null;
        _agentRep = null;
        _agentRegHelper = null;
        _msgQueue = null;
        _name = null;
        _sid = null;
    }

    /**
     * Updates the grid registration just before agent leaves the grid; causes
     * the jini lease to be updated long enough for the agent to move.
     * 
     * @throws GmasServiceException
     */
    public void beforeMove()
            throws GmasMobilityException
    {
        Entry[] entries = new Entry[_agentDescriptions.size()];

        _agentDescriptions.toArray (entries);

        try {
            _agentRegHelper.updateRegistration ("");
        }
        catch (Exception x) {
            throw new GmasMobilityException ("Failed to update the agent's registration"
                    + " prior to moving. This agent may need to register with the"
                    + " new bridge instead of update its old registration.");
        }

        try {
            _sid = _agentRep.getServiceID();
        }
        catch (Exception x) {
            throw new GmasMobilityException ("Update registration succeeded but the"
                    + " serviceID is unavailable. The serviceID is needed to update"
                    + " the registration after the move completes. Without the"
                    + " serviceID you will need to register in the normal way after"
                    + " the move is finished.");
        }
    }

    /**
     * Updates the grid registration just after an agent arrives.
     * 
     * @throws GmasMobilityException
     */
    public void afterMove()
            throws GmasMobilityException
    {
        Entry[] entries = new Entry[_agentDescriptions.size()];
        _agentDescriptions.copyInto (entries);

        try {
            _agentRegHelper.updateRegistration ("");
        }
        catch (Exception x) {
            x.printStackTrace();
            throw new GmasMobilityException ("Could not update the agent's registration."
                    + _name + "'");
        }

        _msgQueue.addMessageListener (this);
    }

    /**
     * Notifies a waiting thread that message has been added to queue. This
     * method is implemented as a required callback method for the Message
     * Listener interface.
     * 
     * @param Message the message that has been added to the queue
     */

    public synchronized void messageAdded (Message msg)
    {
        _msgQueue = _agentRegHelper.getMessageQueue();
        if (_msgQueue != null) {
            notify();
        }
    }

    /**
     * Wrapper for Object::wait.&nbsp;So current thread will own monitor.
     */
    public synchronized void waitSync()
    {
        try {
            wait();
        }
        catch (Exception x) {/* intentionally left blank */
        }
    }

    public Vector<CoABSAgentDescription> _agentDescriptions = null;
    public DefaultAgentRep _agentRep = null;
    public AgentRegistrationHelper _agentRegHelper = null;
    public MessageQueue _msgQueue = null;
    private String _name = null;
    private String _gridName = null;
    private ServiceID _sid = null;
    private Directory _directory = null;
    static final Logger _log = LogInitializer.getLogger (GridHelper.class.getName());
}
