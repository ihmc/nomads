package mil.darpa.coabs.gmas;

import mil.darpa.coabs.gmas.messaging.GmasMessagingException;

/**
 * MessagingService defines the messaging service interface to be implemented by any GMAS 
 * implementor. A basic implementation is provided and should be extended.
 * (@link mil.darpa.coabs.gmas.messaging.MessagingServiceImpl)
 * <p>
 * Copyright 2000 Arne Grimstrup, Department of
 * Computer Science, Dartmouth College Use, modification and redistribution
 * subject to the terms of the standard GNU public license at
 * http://www.gnu.org/copyleft/gpl.html.
 * 
 * @author Arne Grimstrup
 * @author Greg Hill
 * @author Tom Cowin <tom.cowin@gmail.com>
 */

public interface MessagingService {

    /**
     * Send the message provided to the destination specified by dest, 
     * asynchronously, i.e., without waiting for an answer.
     * 
     * @param dest destination of message - typically agent's Grid visible name
     * @param message text of message to be sent
     * @throws GmasMessagingException
     */
    public void sendMsgAsync (String dest, String message)
            throws GmasMessagingException;

    /**
     * Send the message provided to the destination specified by dest, 
     * synchronously, waiting for an answer for a max of timeout seconds.
     * 
     * @param dest destination of message - typically agent's Grid visible name
     * @param message text of message to be sent
     * @param timeout time in seconds to wait for response
     * @return plaintext representation of the message received, or a null.
     * @throws GmasMessagingException
     */
    public String sendMsgSync (String dest, String message, int timeout)
            throws GmasMessagingException;

    /**
     * Send the message provided in the given agent communication language, to 
     * the destination specified by dest, asynchronously.
     * 
     * @param dest destination of message - typically agent's Grid visible name
     * @param message text of message to be sent
     * @param acl agent communication language(s) that this msg is in
     * @throws GmasMessagingException
     */
    public void sendMsgAsync (String dest, String message, String acl)
            throws GmasMessagingException;

    /**
     * Send the message provided in the given agent communication language, to 
     * the destination specified by dest, synchronously, waiting for an answer 
     * for a max of timeout seconds.
     * 
     * @param dest destination of message - typically agent's Grid visible name
     * @param message text of message to be sent
     * @param timeout time in seconds to wait for response
     * @param acl agent communication language(s) that this msg is in
     * @return
     * @throws GmasMessagingException
     */
    public String sendMsgSync (String dest,
                               String message,
                               int timeout,
                               String acl)
            throws GmasMessagingException;

    /**
     * Receive any message waiting for this entity, if there is no message
     * available, return a null.
     * 
     * @return plaintext representation of the message received, or a null.
     * @throws GmasMessagingException
     */
    public String receiveMsgAsync()
            throws GmasMessagingException;

    /**
     * Receive any message for this entity, waiting a max of timeout seconds
     * for a message to arrive. If there is no message
     * available, return a null.
     * 
     * @param timeout time in seconds to wait for response
     * @return plaintext representation of the message received, or a null.
     * @throws GmasMessagingException
     */
    public String receiveMsgSync (int timeout)
            throws GmasMessagingException;
}
