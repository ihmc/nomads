package mil.darpa.coabs.gmas.messaging;

import mil.darpa.coabs.gmas.BasicGridServiceImpl;
import mil.darpa.coabs.gmas.MessagingService;

import com.globalinfotek.coabsgrid.DefaultAgentRep;
import com.globalinfotek.coabsgrid.Message;
import com.globalinfotek.coabsgrid.entry.CoABSAgentDescription;

/**
 * The implementation of the BasicGridService for GMAS Messaging Services. Its
 * parent class uses the GridHelper class to communicate over the Grid. If the 
 * user of this service wants to exchange messages with some other agent on 
 * the Grid, there is no need for the user to explicitly register with the Grid. 
 * The service object itself will register with the Grid in order to perform 
 * the messaging operations.
 * <p>
 * This does not prevent the user from doing his/her own registration, and pre-
 * senting his/her own presence on the Grid, and then performing the same 
 * messaging operations.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com> modifications to extend GMAS for
 *         messaging, additional platform support
 * @version $Revision$ $Date$
 * @see mil.darpa.coabs.mobility.client.BasicGridService
 */

abstract public class MessagingServiceImpl extends
        BasicGridServiceImpl implements MessagingService {

    /**
     * 
     */
    public MessagingServiceImpl()
    {
        super();
    }

    /**
     * Send the string contained in message to the Grid entity whose ID matches
     * dest. This ID is the name in the CoABSAgentDescription. Send this message
     * asynchronously, i.e., do not expect a response. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @param dest name or ID of recipient
     * @param message string message to be sent to recipient
     * @throws GmasMessagingException
     */
    public void sendMsgAsync (String dest, String message)
            throws GmasMessagingException
    {
        sendMsgAsync (dest, message, "NaturalLanguage");
    }

    /**
     * Send the string contained in message to the Grid entity whose ID matches
     * dest. This ID is the name in the CoABSAgentDescription. Send this message
     * asynchronously, i.e., do not expect a response. Specify that the message
     * is encoded in the specified agent language. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @param dest name or ID of recipient
     * @param message string message to be sent to recipient
     * @param agentLanguage language of the message - GMAML, NaturalLanguage
     * @throws GmasMessagingException
     */
    public void sendMsgAsync (String dest, String message, String agentLanguage)
            throws GmasMessagingException
    {
        try {
            DefaultAgentRep gridDestination = findServerOnGrid (new CoABSAgentDescription (dest));
            _gridHelper.sendMsg (gridDestination, agentLanguage, message);
        }
        catch (Exception xcp) {
            xcp.printStackTrace();
            throw new GmasMessagingException ("Failed to send message to grid location ");
        }
    }

    /**
     * Send the string contained in message to the Grid entity whose ID matches
     * dest. This ID is the name in the CoABSAgentDescription. Send this message
     * synchronously, i.e., expect and return the response. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @param dest name or ID of recipient
     * @param message string message to be sent to recipient
     * @param timeout time in seconds to wait for a reponse
     * @throws GmasMessagingException
     */
    public String sendMsgSync (String dest, String message, int timeout)
            throws GmasMessagingException
    {
        return sendMsgSync (dest, message, timeout, "NaturalLanguage");
    }

    /**
     * Send the string contained in message to the Grid entity whose ID matches
     * dest. This ID is the name in the CoABSAgentDescription. Send this message
     * synchronously, i.e., expect and return the response. Wait the specified
     * period of time in seconds for the return message. If a message does not
     * appear in that time, a null is returned. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @param dest name of destination grid agent to which we are sending the
     *            msg
     * @param message message content as a string
     * @param timeout time in seconds to wait for reply message
     * @param agentLanguage agent communication language
     * @return string containing raw text of returned Coabs Grid Message.
     * @throws GmasMessagingException
     */
    public String sendMsgSync (String dest,
                               String message,
                               int timeout,
                               String agentLanguage)
            throws GmasMessagingException
    {
        Message msg = null;
        try {
            DefaultAgentRep gridDestination = findServerOnGrid (new CoABSAgentDescription (dest));
            _gridHelper.sendMsg (gridDestination, agentLanguage, message);
            long startTime = System.currentTimeMillis();
            do {
                msg = _gridHelper.receiveMsgNonBlocking();
            } while (msg == null
                    && System.currentTimeMillis() < startTime
                            + (timeout * 1000));
            if (msg != null) {
                _log.debug ("recv'd msg: " + msg.toString());
            }
            else {
                _log.debug ("recv'd null msg after sendMsgSync");
            }

        }
        catch (Exception xcp) {
            xcp.printStackTrace();
            throw new GmasMessagingException ("Failed to send message to grid location ");
        }
        return (msg == null ? null : msg.getRawText());
    }
    
    /**
     * Receives a message from the Grid Asynchronously. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @return string with raw text from Coabs Grid Message, null if no msg
     * @throws GmasMessagingException
     */
    public String receiveMsgAsync()
            throws GmasMessagingException
    {
        try {
            registerWithGrid();
        }
        catch (Exception xcp) {
            throw new GmasMessagingException ("Failed to register with the Grid.");
        }
        Message msg = _gridHelper.receiveMsgNonBlocking();
        return (msg == null ? null : msg.getRawText());
    }
    
    /**
     * Receives a message from the Grid synchronously, waiting for the specified
     * period in seconds for the message to appear. Will register with the 
     * Grid if this object had not already registered.
     * 
     * @param timeout int value in seconds of time to wait for a message
     * @return string with raw text from Coabs Grid Message, null if no msg
     * @throws GmasMessagingException
     */
    public String receiveMsgSync (int timeout)
            throws GmasMessagingException
    {
        try {
            registerWithGrid();
        }
        catch (Exception xcp) {
            throw new GmasMessagingException ("Failed to register with the Grid.");
        }
        Message msg = null;
        long startTime = System.currentTimeMillis();
        do {
            msg = _gridHelper.receiveMsgNonBlocking();
        } while (msg == null
                && System.currentTimeMillis() < startTime
                        + (timeout * 1000));
        
        return (msg == null ? null : msg.getRawText());
    }
}
