package mil.darpa.coabs.gmas.messaging;

/** GmasMessage. This is designed to be a MAS-neutral message format that can
 *  be used by any MAS adapting to GMAS to enable the transformation of a 
 *  proprietary msg format to this format. The utilities exist then to transfer
 *  this GMASMessage via normal GMAS methods - i.e., converting it to XML 
 *  according to the defined GMAML (Grid Mobile Agent Markup Language), which
 *  can then be transferred as ASCII Text to any other GMAS-enabled host.
 *  
 * @author Tom Cowin <tom.cowin@gmail.com>
 * @version
 * Created on Jan 25, 2005
 */

import java.io.IOException;
import java.util.Enumeration;

import mil.darpa.coabs.gmas.mobility.AgentContainer;
import mil.darpa.coabs.gmas.mobility.GmasMobilityException;
import mil.darpa.coabs.gmas.util.Base64FormatException;
import mil.darpa.coabs.gmas.util.Base64Transcoders;

import mil.darpa.coabs.gmas.log.LogInitializer;
import mil.darpa.coabs.gmas.log.Logger;
import org.w3c.dom.DOMException;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class GmasMessage {

    /**
     * Creates a new blank GmasMessage
     */
    public GmasMessage()
    {
        _recipient = new AgentAddress();
        _sender = new AgentAddress();
        _timestamp = System.currentTimeMillis();
    }

    /**
     * Construct a new GmasMessage given a core set of message components.
     * 
     * @param recipientName agents plain text string name
     * @param recipientUID agents unique identifier
     * @param recipientSystemType type of agent system 
     * @param recipientSystemHostname hostname where agent is resident
     * @param recipientSystemIdentifier identifying characteristic - perhaps port
     * @param senderName sending agent's plain text string name
     * @param senderUID sending agent's unique identifier
     * @param senderSystemType type of agent system
     * @param recipientSystemHostname hostname where agent is resident
     * @param recipientSystemIdentifier identifying characteristic - perhaps port
     * @param contentType type of content contained in message
     * @param messageType type of message, relating to purpose
     * @param messageText actual message
     */
    public GmasMessage (String recipientName,
                        String recipientUID,
                        AgentSystemType recipientSystemType,
                        String recipientSystemHostname,
                        String recipientSystemIdentifier,
                        String senderName,
                        String senderUID,
                        AgentSystemType senderSystemType,
                        String senderSystemHostname,
                        String senderSystemIdentifier,
                        MessageContentType contentType,
                        MessageType messageType,
                        byte[] messageText)
    {
        _recipient = new AgentAddress (recipientName,
                                       recipientUID,
                                       "",
                                       recipientSystemType,
                                       recipientSystemHostname,
                                       recipientSystemIdentifier);
        _sender = new AgentAddress (senderName,
                                    senderUID,
                                    "",
                                    senderSystemType,
                                    senderSystemHostname,
                                    senderSystemIdentifier);

        _contentType = contentType;
        _messageType = messageType;
        _messageText = messageText;
        _timestamp = System.currentTimeMillis();
    }


    /**
     * Creates a GmasMessage from the GMAML XML representation received via GMAS
     * within a Grid message. It is parsing the W3C's Document Object Model 
     * representation.
     * 
     * @param n DOM Node containing the GMAML rep of a GmasMessage
     * @see org.w3c.dom.Node
     */

    public GmasMessage (Node n)
    {
        _recipient = new AgentAddress();
        _sender = new AgentAddress();
        NodeList anl = n.getChildNodes();
        for (int j = 0; j < anl.getLength(); j++) {
            if ((anl.item (j)).getNodeName().equals ("header")) {
                setMessageID (((Element) anl.item (j)).getAttribute ("message-id"));
                setTimestamp (new Long (((Element) anl.item (j)).getAttribute ("timestamp")));
                setContentType (toContentType (((Element) anl.item (j)).getAttribute ("content-type")));
                setMessageType (toMessageType (((Element) anl.item (j)).getAttribute ("message-type")));
                NodeList bnl = anl.item (j).getChildNodes();
                for (int k = 0; k < bnl.getLength(); k++) {
                    if ((bnl.item (k)).getNodeName().equals ("sender")) {
                        setSenderAgentName (((Element) bnl.item (k)).getAttribute ("name"));
                        setSenderAgentUID (((Element) bnl.item (k)).getAttribute ("uid"));
                        setSenderAgentClassName (((Element) bnl.item (k)).getAttribute ("classname"));
                        NodeList cnl = bnl.item (k).getChildNodes();
                        for (int l = 0; l < cnl.getLength(); l++) {
                            if ((cnl.item (l)).getNodeName().equals ("agentsystem")) {
                                setSenderAgentSystemType (toAgentSystemType (((Element) cnl.item (l)).getAttribute("type")));
                                NodeList dnl = cnl.item (l).getChildNodes();
                                for (int m = 0; m < dnl.getLength(); m++) {
                                    if ((dnl.item (m)).getNodeName().equals ("instance")) {
                                        setSenderAgentSystemHostname (((Element) dnl.item (m)).getAttribute ("hostname"));
                                        setSenderAgentSystemIdentifier (((Element) dnl.item (m)).getAttribute ("environment"));
                                    }
                                }
                            }
                        }
                    }
                    else if ((bnl.item (k)).getNodeName().equals ("recipient")) {
                        setRecipientAgentName (((Element) bnl.item (k)).getAttribute ("name"));
                        setRecipientAgentUID (((Element) bnl.item (k)).getAttribute ("uid"));
                        setRecipientAgentClassName (((Element) bnl.item (k)).getAttribute ("classname"));
                        NodeList cnl = bnl.item (k).getChildNodes();
                        for (int l = 0; l < cnl.getLength(); l++) {
                            if ((cnl.item (l)).getNodeName().equals ("agentsystem")) {
                                setRecipientAgentSystemType (toAgentSystemType (((Element) cnl.item (l)).getAttribute("type")));
                                NodeList dnl = cnl.item (l).getChildNodes();
                                for (int m = 0; m < dnl.getLength(); m++) {
                                    if ((dnl.item (m)).getNodeName().equals ("instance")) {
                                        setRecipientAgentSystemHostname (((Element) dnl.item (m)).getAttribute ("hostname"));
                                        setRecipientAgentSystemIdentifier (((Element) dnl.item (m)).getAttribute ("environment"));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if ((anl.item (j)).getNodeName().equals ("body")) {
                try {
                    if (anl.item (j).getFirstChild().getNodeValue() == null) {
                    }
                    else {
                        setMessageText (Base64Transcoders.convertB64StringToByteArray (anl.item (j).getFirstChild().getNodeValue()));
                    }
                } catch (DOMException e) {

                    e.printStackTrace();
                } catch (IOException e) {

                    e.printStackTrace();
                } catch (Base64FormatException e) {

                    e.printStackTrace();
                }
            }
            else if ((anl.item (j)).getNodeName().equals ("attachment")) {
                try {
                    setAttachment (Base64Transcoders.convertB64StringToByteArray (anl.item (j).getFirstChild().getNodeValue()));
                } catch (DOMException e) {

                    e.printStackTrace();
                } catch (IOException e) {

                    e.printStackTrace();
                } catch (Base64FormatException e) {

                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * Create a reply message to the given message, turning sender and recipient
     * around and making other appropriate modifications.
     * 
     * @param gmasMsg message to create a reply to
     * @return new message constructed as reply
     */
    public GmasMessage createReplyTo (GmasMessage gmasMsg)
    {
        GmasMessage reply = new GmasMessage();
        _recipient = new AgentAddress (gmasMsg._sender);
        _sender = new AgentAddress (gmasMsg._recipient);
        _contentType = gmasMsg.getContentType();
        _messageID = gmasMsg.getMessageID();
        _timestamp = System.currentTimeMillis();
        if (gmasMsg.getMessageType() == MessageType.sync) {
            _messageType = MessageType.syncReply;
        }
        else {
            _messageType = gmasMsg.getMessageType();
        }

        return reply;
    }
    
    /**
     * Turns the message into a descriptive string.
     * 
     * @return string containing message info
     */
    public String toString()
    {
        StringBuffer buffer = new StringBuffer();
        buffer.append ("\nSender: \n");
        buffer.append ("\t agentName: " + _sender.getName() + "\n");
        buffer.append ("\t agentUID: " + _sender.getUID() + "\n");
        buffer.append ("\t hostname: " + _sender.getMasHostname() + "\n");
        buffer.append ("\t identifier: " + _sender.getMasIdentifier() + "\n");
        buffer.append ("\t agentSystemType: " + _sender.getMasType() + "\n");
        buffer.append ("Recipient:\n");
        buffer.append ("\t agentName: " + _recipient.getName() + "\n");
        buffer.append ("\t agentUID: " + _recipient.getUID() + "\n");
        buffer.append ("\t hostname: " + _recipient.getMasHostname() + "\n");
        buffer.append ("\t identifier: " + _recipient.getMasIdentifier()
                + "\n");
        buffer.append ("\t agentSystemType: " + _recipient.getMasType() + "\n");
        buffer.append ("\t _messageType: " + _messageType + "\n");
        if (_messageText != null) {
            buffer.append ("\t _messageText: " + new String (_messageText)
                    + "\n");
        }
        return buffer.toString();
    }

    /**
     * @return Returns the _messageID.
     */
    public String getMessageID()
    {
        return _messageID;
    }

    /**
     * @param _messageid The _messageID to set.
     */
    public void setMessageID (String messageid)
    {
        _messageID = messageid;
    }

    /**
     * @return Returns the Recipient's AgentClassName.
     */
    public String getRecipientAgentClassName()
    {
        return _recipient.getClassName();
    }

    /**
     * @param agentClassName Recipient's AgentClassName to set.
     */
    public void setRecipientAgentClassName (String agentClassName)
    {
        _recipient.setClassName (agentClassName);
    }

    /**
     * @return Returns the Sender's AgentClassName.
     */
    public String getSenderAgentClassName()
    {
        return _sender.getClassName();
    }

    /**
     * @param agentClassName Sender's AgentClassName to set.
     */
    public void setSenderAgentClassName (String agentClassName)
    {
        _sender.setClassName (agentClassName);
    }

    /**
     * @return Returns the Recipient's AgentName.
     */
    public String getRecipientAgentName()
    {
        return _recipient.getName();
    }

    /**
     * @param agentName Recipient's AgentName to set.
     */
    public void setRecipientAgentName (String recipientAgentName)
    {
        _recipient.setName (recipientAgentName);
    }

    /**
     * @return Returns the Recipient's AgentUID
     */
    public String getRecipientAgentUID()
    {
        return _recipient.getUID();
    }

    /**
     * @param agentUID Recipient's AgentUID to set.
     */
    public void setRecipientAgentUID (String UID)
    {
        _recipient.setUID (UID);
    }

    /**
     * @return Returns the Recipient's AgentSystemIdentifier.
     */
    public String getRecipientAgentSystemIdentifier()
    {
        return _recipient.getMasIdentifier();
    }

    /**
     * @param agentSystemIdentifier Recipient's AgentSystemIdentifier to set.
     */
    public void setRecipientAgentSystemIdentifier (String agentSystemIdentifier)
    {
        _recipient.setMasIdentifier (agentSystemIdentifier);
    }

    /**
     * @return Returns the Recipient's AgentSystemHostname.
     */
    public String getRecipientAgentSystemHostname()
    {
        return _recipient.getMasHostname();
    }

    /**
     * @param agentSystemHostname Recipient's AgentSystemHostname to set.
     */
    public void setRecipientAgentSystemHostname (String agentSystemHostname)
    {
        _recipient.setMasHostname (agentSystemHostname);
    }

    /**
     * @return Returns the Recipient's AgentSystemType.
     */
    public AgentSystemType getRecipientAgentSystemType()
    {
        return _recipient.getMasType();
    }

    /**
     * @param agentSystemType Recipient's AgentSystemType to set.
     */
    public void setRecipientAgentSystemType (AgentSystemType agentSystemType)
    {
        _recipient.setMasType (agentSystemType);
    }

    /**
     * @return Returns the Sender's AgentName.
     */
    public String getSenderAgentName()
    {
        return _sender.getName();
    }

    /**
     * @param agentName Sender's AgentName to set.
     */
    public void setSenderAgentName (String agentName)
    {
        _sender.setName (agentName);
    }

    /**
     * @return Returns the Sender's AgentUID.
     */
    public String getSenderAgentUID()
    {
        return _sender.getUID();
    }

    /**
     * @param agentUID The Sender's AgentUID to set.
     */
    public void setSenderAgentUID (String UID)
    {
        _sender .setUID (UID);
    }

    /**
     * @return Returns the Sender's AgentSystemIdentifier.
     */
    public String getSenderAgentSystemIdentifier()
    {
        return _sender.getMasIdentifier();
    }

    /**
     * @param agentSystemIdentifier The Sender's AgentSystemIdentifier to set.
     */
    public void setSenderAgentSystemIdentifier (String agentSystemIdentifier)
    {
        _sender.setMasIdentifier (agentSystemIdentifier);
    }

    /**
     * @return Returns the Sender's AgentSystemHostname.
     */
    public String getSenderAgentSystemHostname()
    {
        return _sender.getMasHostname();
    }

    /**
     * @param agentSystemHostname The Sender's AgentSystemHostname to set.
     */
    public void setSenderAgentSystemHostname (String agentSystemHostname)
    {
        _sender.setMasHostname (agentSystemHostname);
    }

    /**
     * @return Returns the Sender's AgentSystemType.
     */
    public AgentSystemType getSenderAgentSystemType()
    {
        return _sender.getMasType();
    }

    /**
     * @param agentSystemType Sender's AgentSystemType to set.
     */
    public void setSenderAgentSystemType (AgentSystemType agentSystemType)
    {
        _sender.setMasType (agentSystemType);
    }

    /**
     * @return Returns the attachment.
     */
    public byte[] getAttachment()
    {
        return _attachment;
    }

    /**
     * @param attachment The attachment to set.
     */
    public void setAttachment (byte[] attachment)
    {
        _attachment = attachment;
    }

    /**
     * @return Returns the contentType.
     */
    public MessageContentType getContentType()
    {
        return _contentType;
    }

    /**
     * @param contentType The contentType to set.
     */
    public void setContentType (MessageContentType contentType)
    {
        _contentType = contentType;
    }

    /**
     * @return Returns the messageText.
     */
    public byte[] getMessageText()
    {
        return _messageText;
    }

    /**
     * @param messageText The messageText to set.
     */
    public void setMessageText (byte[] messageText)
    {

        if (messageText == (byte[]) null) {
            _messageText = null;
        }
        else {
            _messageText = new byte[messageText.length];
            System.arraycopy (messageText,
                              0,
                              _messageText,
                              0,
                              messageText.length);
        }
    }

    /**
     * @return Returns the messageType.
     */
    public MessageType getMessageType()
    {
        return _messageType;
    }

    /**
     * @param messageType The messageType to set.
     */
    public void setMessageType (MessageType messageType)
    {
        _messageType = messageType;
    }

    /**
     * @return Returns the timestamp.
     */
    public long getTimestamp()
    {
        return _timestamp;
    }

    /**
     * @param timestamp The timestamp to set.
     */
    public void setTimestamp (long timestamp)
    {
        _timestamp = timestamp;
    }

    /**
     * Put the GMASMessage into GMAML format suitable for sending across the Grid
     * embedded in a Grid message.
     * 
     * @return string containing GMAML rep of GmasMessage
     */
    public String toExternalForm()
    {
        StringBuffer sb = new StringBuffer();

        sb.append ("<?xml version=\"1.0\" ?>\n");
        try {
            sb.append ("<!DOCTYPE gmas SYSTEM \"" + AgentContainer.getGridCodebaseURL()
                    + "gmaml.dtd\">\n");
        } catch (GmasMobilityException e1) {
            e1.printStackTrace();
        }
        sb.append ("<gmas>\n");
        sb.append ("<message>\n");
        sb.append ("<header message-id=\"" + _messageID + "\" timestamp=\""
                + _timestamp + "\" content-type=\"" + _contentType
                + "\" message-type=\"" + _messageType + "\">\n");
        sb.append ("<sender name=\"" + _sender.getName() + "\" uid=\""
                + _sender.getUID() + "\" classname=\""
                + _sender.getClassName() + "\">\n");
        sb.append ("<agentsystem type=\"" + _sender.getMasType() + "\">\n");
        sb.append ("<instance hostname=\"" + _sender.getMasHostname()
                + "\" identifier=\"" + _sender.getMasIdentifier() + "\" />\n");
        sb.append ("</agentsystem>\n");
        sb.append ("</sender>\n");
        sb.append ("<recipient name=\"" + _recipient.getName() + "\" uid=\""
                + _recipient.getUID() + "\" classname=\""
                + _recipient.getClassName() + "\">\n");
        sb.append ("<agentsystem type=\"" + _recipient.getMasType() + "\">\n");
        sb.append ("<instance hostname=\"" + _recipient.getMasHostname()
                + "\" identifier=\"" + _recipient.getMasIdentifier()
                + "\" />\n");
        sb.append ("</agentsystem>\n");
        sb.append ("</recipient>\n");
        sb.append ("</header>\n");
        if (_messageText != null) {
            sb.append ("<body>\n");
            try {
                sb.append (Base64Transcoders.convertByteArrayToB64String (_messageText));
            } catch (IOException e1) {

                e1.printStackTrace();
            }
            sb.append ("\n</body>\n");
        }
        if (_attachment != null) {
            sb.append ("<attachment>\n");
            try {
                sb.append (Base64Transcoders.convertByteArrayToB64String (_attachment));
            } catch (IOException e2) {

                e2.printStackTrace();
            }
            sb.append ("\n</attachment>\n");
        }
        sb.append ("</message>\n");
        sb.append ("</gmas>\n");
        return sb.toString();
    }

    /**
     * Class containing the necessary elements to address a given agent on a 
     * given agent system.
     * 
     * @author Tom Cowin <tom.cowin@gmail.com>
     *
     */
    protected class AgentAddress {

        /**
         * Construct a new AgentAddress object with identifying elements.
         * 
         * @param name plaintext string with agent name
         * @param UID unique agent ID
         * @param className agent's class name
         * @param type agent's native system type
         * @param hostname where agent resides
         * @param identifier ID of agent system instance on host - perhaps port
         */
        public AgentAddress (String name,
                             String UID,
                             String className,
                             AgentSystemType type,
                             String hostname,
                             String identifier)
        {
            _name = name;
            _UID = UID;
            _className = className;
            _masType = type;
            _masHostname = hostname;
            _masIdentifier = identifier;
        }

        public AgentAddress (AgentAddress copy)
        {
            _name = copy.getName();
            _className = copy.getClassName();
            _masType = copy.getMasType();
            _masHostname = copy.getMasHostname();
            _masIdentifier = copy.getMasIdentifier();
        }

        public AgentAddress()
        {
        }

        /**
         * @return Returns the _className.
         */
        public String getClassName()
        {
            return _className;
        }

        /**
         * @param name The _className to set.
         */
        public void setClassName (String name)
        {
            _className = name;
        }

        /**
         * @return Returns the _masHostname.
         */
        public String getMasHostname()
        {
            return _masHostname;
        }

        /**
         * @param hostname The _masHostname to set.
         */
        public void setMasHostname (String hostname)
        {
            _masHostname = hostname;
        }

        /**
         * @return Returns the _masIdentifier.
         */
        public String getMasIdentifier()
        {
            return _masIdentifier;
        }

        /**
         * @param identifier The _masIdentifier to set.
         */
        public void setMasIdentifier (String identifier)
        {
            _masIdentifier = identifier;
        }

        /**
         * @return Returns the _masType.
         */
        public AgentSystemType getMasType()
        {
            return _masType;
        }

        /**
         * @param type The _masType to set.
         */
        public void setMasType (AgentSystemType type)
        {
            _masType = type;
        }

        /**
         * @return Returns the _name.
         */
        public String getName()
        {
            return _name;
        }

        /**
         * @param _name The _name to set.
         */
        public void setName (String name)
        {
            _name = name;
        }

        /**
         * @return Returns the _UID.
         */
        public String getUID()
        {
            return _UID;
        }

        /**
         * @param _uid The _UID to set.
         */
        public void setUID (String UID)
        {
            _UID = UID;
        }

        private String _name = null;
        private String _UID = null;
        private String _className = null;
        private AgentSystemType _masType = AgentSystemType.unknown;
        private String _masHostname = null;
        private String _masIdentifier = null;
    }

    /**
     * Convert string to appropriate instance of the enumerated type 
     * AgentSystemType by doing string comparison. If no match is found,
     * set it to 'unknown'
     * 
     * @param type string instance that is thought to match an instance of 
     *             AgentSystemType
     * @return enumerated type AgentSystemType matching the input string
     */
    public static AgentSystemType toAgentSystemType (String type)
    {
        if (type == null) {
            return AgentSystemType.unknown;
        }
        else if (type.equals ("AgentSystemType.aglets")
                || type.equals ("aglets")) {
            return AgentSystemType.aglets;
        }
        else if (type.equals ("AgentSystemType.nomads")
                || type.equals ("nomads")) {
            return AgentSystemType.nomads;
        }
        else if (type.equals ("AgentSystemType.dagents")
                || type.equals ("dagents")) {
            return AgentSystemType.dagents;
        }
        else if (type.equals ("AgentSystemType.gmasrefimpl")
                || type.equals ("gmasrefimpl")) {
            return AgentSystemType.gmasrefimpl;
        }
        return AgentSystemType.unknown;
    }

    /**
     * Convert string to appropriate instance of the enumerated type 
     * MessageType by doing string comparison. If no match is found,
     * a null is returned.
     * 
     * @param type string instance that is thought to match an instance of 
     *             MessageType
     * @return enumerated type MessageType matching the input string
     */
    public static MessageType toMessageType (String type)
    {
        if (type == null || type.equals ("MessageType.async")
                || type.equals ("async")) {
            return MessageType.async;
        }
        else if (type.equals ("MessageType.syncReply")
                || type.equals ("syncReply")) {
            return MessageType.syncReply;
        }
        else if (type.equals ("MessageType.sync") || type.equals ("sync")) {
            return MessageType.sync;
        }
        else if (type.equals ("MessageType.finger") || type.equals ("finger")) {
            return MessageType.finger;
        }
        else if (type.equals ("MessageType.error") || type.equals ("error")) {
            return MessageType.error;
        }
        else if (type.equals ("MessageType.confirmReceipt")
                || type.equals ("confirmReceipt")) {
            return MessageType.confirmReceipt;
        }
        return null;
    }

    /**
     * Convert string to appropriate instance of the enumerated type 
     * MessageContentType by doing string comparison. If no match is found,
     * a null is returned.
     * 
     * @param type string instance that is thought to match an instance of 
     *             MessageContentType
     * @return enumerated type MessageContentType matching the input string
     */
    public static MessageContentType toContentType (String type)
    {
        if (type == null || type.equals ("MessageContentType.rawText")
                || type.equals ("rawText")) {
            return MessageContentType.rawText;
        }
        else if (type.equals ("MessageContentType.encapNomads")
                || type.equals ("encapNomads")) {
            return MessageContentType.encapNomads;
        }
        else if (type.equals ("MessageContentType.encapAglets")
                || type.equals ("encapAglets")) {
            return MessageContentType.encapAglets;
        }
        return null;
    }

    /**
     * Enumerated type to describe the various types of message content that might
     * be utilized in GMAS. encapNomads, for instance, means an encapsulated Nomads
     * message. Since the GMAS implementor controls both how this content is placed
     * into the GmasMessage, and then how it is retrieved, he/she has complete 
     * control over what can be done with it.
     */
    public enum MessageContentType {
        rawText, encapNomads, encapAglets
    };
    
    /**
     * Enumerated type describing the possible types of messages in GMAS Messaging.
     */
    public enum MessageType {
        sync, syncReply, async, finger, error, confirmReceipt
    };

    /**
     * Enumerated type describing the possible types of agent systems in GMAS.
     */
    public enum AgentSystemType {
        aglets, nomads, dagents, gmasrefimpl, unknown
    };

    private AgentAddress _sender;
    private AgentAddress _recipient;
    private long _timestamp = 0L;
    private MessageContentType _contentType = MessageContentType.rawText;
    private MessageType _messageType = MessageType.sync;

    private byte[] _messageText = null;
    private String _messageID = null;
    private byte[] _attachment = null;

    static final Logger _log = LogInitializer.getLogger (GmasMessage.class.getName());
}
