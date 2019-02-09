package us.ihmc.aci.nodemon.data.process;

import org.snmp4j.TransportMapping;
import org.snmp4j.TransportStateReference;
import org.snmp4j.mp.PduHandle;
import org.snmp4j.mp.StateReference;
import org.snmp4j.smi.Address;

/**
 * SNMPContent.java
 * <p/>
 * Class <code>SNMPContent</code> is a container for statistics coming from the SNMP Sensor.
 *
 * @author Rita Lenzi (rlenzi@ihmc.us)
 */
public class SNMPContent extends ProcessContent
{
    public SNMPContent()
    {
        securityModel = -1;
        securityLevel = -1;
        maxSizeResponsePDU = 0;
        pduHandle = null;
        stateReference = null;
//        pdu = null;
        messageProcessingModel = -1;
        securityName = null;
        processed = false;
        peerAddress = null;
        transportMapping = null;
        tmStateReference = null;
    }

    public int getSecurityModel ()
    {
        return securityModel;
    }

    public void setSecurityModel (int securityModel)
    {
        this.securityModel = securityModel;
    }

    public int getSecurityLevel ()
    {
        return securityLevel;
    }

    public void setSecurityLevel (int securityLevel)
    {
        this.securityLevel = securityLevel;
    }

    public int getMaxSizeResponsePDU ()
    {
        return maxSizeResponsePDU;
    }

    public void setMaxSizeResponsePDU (int maxSizeResponsePDU)
    {
        this.maxSizeResponsePDU = maxSizeResponsePDU;
    }

    public PduHandle getPduHandle ()
    {
        return pduHandle;
    }

    public void setPduHandle (PduHandle pduHandle)
    {
        this.pduHandle = pduHandle;
    }

    public StateReference getStateReference ()
    {
        return stateReference;
    }

    public void setStateReference (StateReference stateReference)
    {
        this.stateReference = stateReference;
    }

//    public PDU getPdu ()
//    {
//        return pdu;
//    }
//
//    public void setPdu (PDU pdu)
//    {
//        this.pdu = pdu;
//    }

    public int getMessageProcessingModel ()
    {
        return messageProcessingModel;
    }

    public void setMessageProcessingModel (int messageProcessingModel)
    {
        this.messageProcessingModel = messageProcessingModel;
    }

    public byte[] getSecurityName ()
    {
        return securityName;
    }

    public void setSecurityName (byte[] securityName)
    {
        this.securityName = securityName;
    }

    public boolean isProcessed ()
    {
        return processed;
    }

    public void setProcessed (boolean processed)
    {
        this.processed = processed;
    }

    public Address getPeerAddress ()
    {
        return peerAddress;
    }

    public void setPeerAddress (Address peerAddress)
    {
        this.peerAddress = peerAddress;
    }

    public TransportMapping getTransportMapping ()
    {
        return transportMapping;
    }

    public void setTransportMapping (TransportMapping transportMapping)
    {
        this.transportMapping = transportMapping;
    }

    public TransportStateReference getTmStateReference ()
    {
        return tmStateReference;
    }

    public void setTmStateReference (TransportStateReference tmStateReference)
    {
        this.tmStateReference = tmStateReference;
    }

    // General SNMP Responder Event fields
    private int securityModel;
    private int securityLevel;
    private int maxSizeResponsePDU;
    private PduHandle pduHandle;
    private StateReference stateReference;
//    private PDU pdu;
    private int messageProcessingModel;
    private byte[] securityName;
    private boolean processed;
    private Address peerAddress;
    private transient TransportMapping transportMapping;
    private TransportStateReference tmStateReference;

    // Single PDU fields
}
