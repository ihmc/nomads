package mil.darpa.coabs.gmas.messaging;

/**
 * GmasMessageListener.java This defines an interface that acts to listen for
 * and subsequently disposition incoming GMAS messages received by the
 * Grid-attached GmasMessageHandler.
 * 
 * @author tcowin <tcowin@ihmc.us>
 * @version $Revision$ $Date$
 */
public interface GmasMessageListener {

    /**
     * Receive and disposition the message msg received from sender. The string
     * sender is a CoABS Grid identifiable entity.
     * 
     * @param msg Gmas message received
     * @param sender name of sender - name from CoABSAgentDescription
     * @throws GmasMessagingException
     */
    public void receiveMessage (GmasMessage msg, String sender)
            throws GmasMessagingException;

    /**
     * Get the name of this listener. This is a callback method for the 
     * registering entity to obtain an identifying name of this registree.
     * 
     * @return string with identifying process name
     */
    public String getProcessName ();

}
