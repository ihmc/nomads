package mil.darpa.coabs.gmas.messaging;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

import mil.darpa.coabs.gmas.GmamlMessageHandlingService;
import mil.darpa.coabs.gmas.GridMobileAgentSystem;
import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;

/**
 * GmasMessageListenerImpl
 * 
 * This class acts as a listener for incoming messages bound for locally
 * resident Agents. It attachs to the local implementation of the Gmaml
 * MessageHandlingService, which is the CoABS Grid attached agent for
 * the local GMAS system. Once it receives a message, it will attempt to
 * find the intended recipient agent, and then deliver the message locally.
 * If the agent is not found, it will return an agentNotFound message to the
 * sender.
 * <p>
 * Messages sent by agents are sent by using the appropriate (@link mil.darpa.coabs.gmas.MessagingService)
 * and immediate responses to sync messages, or simple error messages indicat-
 * ing 'agent not found', or simple confirmReceipt messages will be sent 
 * directly back to that MessagingService, instead of to the primary GMAS
 * server - the GmamlMessageHandlingService.
 * 
 * @author Tom Cowin <tom.cowin@gmail.com> Adapted for GMAS February 2005
 * @version 1.10 $Date$
 * @see mil.darpa.coabs.mobility.util.GmasMessage
 */
abstract public class GmasMessageListenerImpl implements GmasMessageListener {


    /**
     * Register with the GmasMessageHandling Service as a listener. Expect
     * that the object passed in is a handle to the local agent environment
     * which has been created within the context of the local agent system
     * to provide agent support services.
     * 
     * @param obj AgletContext - local aglet environment
     */
    public GmasMessageListenerImpl (Object agentEnvironmentHandle)
    {
        GmamlMessageHandlingService gmhs = GridMobileAgentSystem.getGmamlMessageHandlingService (agentEnvironmentHandle);
        gmhs.register (this);
    }
    
    /**
     * Receives the incoming GmasMessage and dispositions is according to its'
     * type. Types sync, async and syncReply messages will get an error msg
     * returned to the sender directly if the recipient agent is not found.
     * A finger type message will return a status of whether or not the 
     * desired recipient agent is locally resident. Otherwise a local delivery
     * of the message will be attempted.
     * <p>
     * This method is invoked from the GmamlMessageHandlingService since this
     * object is registered as a listener to it.
     */
    public void receiveMessage (GmasMessage msg, String sender)
            throws GmasMessagingException
    {
        Object agentHandle = null;

        if ((agentHandle = getRecipientAgentHandle (msg)) != null) {
            try {
                switch (msg.getMessageType()) {
                    case finger :
                        respondWithAgentStatus (sender, msg, true);
                        break;
                    case sync :
                        Object obj = deliverSyncMessageLocally (agentHandle, msg);
                        replyToSyncMessage (sender, msg, obj);
                        break;
                    case async :
                        deliverAsyncMessageLocally (agentHandle, msg);
                        confirmReceipt (sender, msg);
                        break;
                    case syncReply :
                        deliverAsyncMessageLocally (agentHandle, msg);
                        break;
                    default :
                        _log.info ("Got unexpected message type: " + msg.getMessageType());
                        break;
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        else {// error - agent not found
            switch (msg.getMessageType()) {
                case sync :
                case async :
                case syncReply :
                    respondWithAgentStatus (sender, msg, false);
                    break;
            } 
        }
    }
    
    /**
     * Returns an identifying string that provides unique identification for
     * this object. This is used primarily by the GmamlMessageHandlingService
     * to register this object as a listener. The implementor should not need
     * to override this method.
     * 
     * @return identifying string of this object - class name without package
     */
    public String getProcessName()
    {
        String[] fullClassName = this.getClass().getName().split ("\\.");
        return fullClassName[fullClassName.length - 1];
    }
    
    /**
     * Deliver the message to the locally identified agent. It is anticipated
     * that the GmasMessage will be converted to the message type of the local
     * agent system type. The agentHandle has been predetermined by invocation
     * of (@link #getRecipientAgentHandle(GmasMessage)). See (@link
     * #receiveMessage(GmasMessage, String)). As this is asynchronous, no
     * immediate return message is handled.
     * 
     * @param agentHandle reference to the intended recipient
     * @param msg subject message (GmasMessage) to be delivered
     * @throws GmasMessagingException
     */
    abstract protected void deliverAsyncMessageLocally (Object agentHandle,
                                                        GmasMessage msg)
            throws GmasMessagingException;
    
    /**
     * Deliver the message to the locally identified agent. It is anticipated
     * that the GmasMessage will be converted to the message of the local agent
     * system type. The agentHandle has been predetermined by invocation of
     * (@link #getRecipientAgentHandle(GmasMessage)). See (@link
     * #receiveMessage(GmasMessage, String)). This method is synchronous, and
     * expects that some sort of object will be returned.
     * 
     * @param agentHandle reference to the intended recipient
     * @param msg subject message (GmasMessage) to be delivered
     * @return object an object to be returned to the original sender.
     * @throws GmasMessagingException
     */
    abstract protected Object deliverSyncMessageLocally (Object agentHandle,
                                                         GmasMessage msg)
            throws GmasMessagingException;

    /**
     * Search the local agent environment for the agent by its ID.
     * 
     * @param agentName as a Base64 string extracted directly from the 
     *        incoming message.
     * @return Object the agent if found, otherwise a null
     */
    abstract protected Object findAgentByID (String agentName);

    /**
     * Search the local agent environment (Context) for the agent by its Name.
     * 
     * @param agentName as a string extracted directly from the 
     *        incoming message.
     * @return Object the agent if found, otherwise a null
     */
    abstract protected Object findAgentByName (String agentName);
    
    /**
     * Find the local agent specified as the recipient agent in the msg and
     * return a handle to it if it exists. Otherwise return a null. This method
     * typically uses (@link #findAgentByID(String)) and/or 
     * (@link #findAgentByName(String)).
     * 
     * @param msg intended for recipient containing recipient addressing info
     * @return object that is primary handle to the recipient agent 
     */
    abstract protected Object getRecipientAgentHandle (GmasMessage msg);
    
    /**
     * Construct reply message to the received msg object, placing returnObj 
     * into it as appropriate to its type. Then send this reply to the sender
     * via (@link #sendReplyMessage(String, GmasMessage)).
     * 
     * @param sender original sender of message, to whom we are replying
     * @param msg original msg received from sender
     * @param returnObj object that is to be returned with the reply
     */
    protected void replyToSyncMessage (String sender, GmasMessage msg, Object returnObj) 
    {
        GmasMessage reply = new GmasMessage();
        reply.createReplyTo (msg);
        if (returnObj == null) {
            reply.setMessageText (null);
        }
        else if (returnObj instanceof java.lang.String) {
            reply.setMessageText (((String) returnObj).getBytes());
        }
        else {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            try {
                ObjectOutputStream oos = new ObjectOutputStream (baos);
                oos.writeObject (returnObj);
            } catch (IOException e3) {
                e3.printStackTrace();
            }
            reply.setMessageText (baos.toByteArray());
        }
        sendReplyMessage (sender, reply);
    }
    
    /**
     * Construct a reply to confirm receipt of the supplied message and return
     * this reply to the sender.
     * 
     * @param sender name of the sender - Grid identifiable
     * @param msg original message to which we are constructing a reply
     */
    protected void confirmReceipt (String sender, GmasMessage msg) 
    {
        GmasMessage reply = new GmasMessage();
        reply.createReplyTo (msg);
        reply.setMessageType (GmasMessage.MessageType.confirmReceipt);
        sendReplyMessage (sender, reply);
    }
    
    /**
     * Send the provided message back to the sender as indicated.
     * 
     * @param sender name of the sender - Grid identifiable
     * @param reply message spefically constructed as a reply to an originally
     *              received message
     */
    protected void sendReplyMessage (String sender, GmasMessage reply)
    {
        _log.info ("sending a confirmation message back to: " + sender);
        try {
            GridMobileAgentSystem.getGridMessagingService().sendMsgAsync (sender,
                                                                          reply.toExternalForm(),
                                                                          "GMAML");
        }
        catch (GmasMessagingException e1) {
            e1.printStackTrace ();
        }
    }

    /**
     * Respond to the sender with a status message indicating whether or not
     * the recipient agent was located.
     * 
     * @param sender name of the sender - Grid identifiable
     * @param msg original message that contains info identifying recipient agent
     * @param agentFound boolean indicating whether or not agent has been located
     */
    protected void respondWithAgentStatus (String sender, GmasMessage msg, boolean agentFound)
    {
        GmasMessage reply = new GmasMessage();
        reply.createReplyTo (msg);
        
        if (agentFound) {
            reply.setMessageText ("agentfound".getBytes());
        }
        else {
            reply.setMessageType (GmasMessage.MessageType.error);
            reply.setMessageText ("agentnotfound".getBytes());
        }
        sendReplyMessage (sender, reply);
    }

    protected Logger _log = LogInitializer.getLogger (this.getClass().getName());
}