/*
 * NMSProxy.java
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS 
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

package us.ihmc.nms;

import com.google.common.net.InetAddresses;
import java.util.Collection;
import org.apache.log4j.Logger;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.proxy.CallbackHandlerFactory;
import us.ihmc.util.proxy.Stub;

/**
 *
 * @author @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class NMSProxy extends Stub
{
    private final Publisher _publisher;

    enum PropoagationMode
    {
        BROADCAST,
        MULTICAST
    }

    public NMSProxy (CallbackHandlerFactory handlerFactory, short applicationId, String host, int port) {
        super (handlerFactory, applicationId, host, port);
        _publisher = new Publisher (_commHelper, _isInitialized);
    }

    public void broadcastMessage (Byte msgType, Collection<String> outgoingInterfaces,
                                  String dstAddr, Short ui16MsgId, Byte ui8HopCount, Byte ui8TTL,
                                  Short ui16DelayTolerance, byte metadata, byte[] data,
                                  boolean bExpedited, String hints)
        throws CommException, ProtocolException
    {
        checkConcurrentModification (Protocol.BROADCAST_METHOD);

        synchronized (this) {
            _publisher.publish (Protocol.BROADCAST_METHOD, msgType, outgoingInterfaces,
                                dstAddr, ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                                metadata, data, bExpedited, hints);
        }
    }

    public void transmitMessage (Byte msgType, Collection<String> outgoingInterfaces,
                                 String dstAddr, Short ui16MsgId, Byte ui8HopCount, Byte ui8TTL,
                                 Short ui16DelayTolerance, byte metadata, byte[] data, String hints)
        throws CommException, ProtocolException
    {
        checkConcurrentModification (Protocol.TRANSMIT_METHOD);

        synchronized (this) {
            _publisher.publish (Protocol.TRANSMIT_METHOD, msgType, outgoingInterfaces,
                                dstAddr, ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                                metadata, data, hints);
        }
    }

    public void transmitReliableMessage (Byte msgType, Collection<String> outgoingInterfaces,
                                         String dstAddr, Short ui16MsgId, Byte ui8HopCount, Byte ui8TTL,
                                         Short ui16DelayTolerance, byte metadata, byte[] data, String hints)
        throws CommException, ProtocolException
    {
        checkConcurrentModification (Protocol.TRANSMIT_RELIABLE_METHOD);

        synchronized (this) {
            _publisher.publish (Protocol.TRANSMIT_RELIABLE_METHOD, msgType, outgoingInterfaces,
                                dstAddr, ui16MsgId, ui8HopCount, ui8TTL, ui16DelayTolerance,
                                metadata, data, hints);
        }
    }

    public void setRetransmissionTimeout (Integer timeout)
    {
        // TODO: implement this
    }

    public void setPrimaryInterface (String pszInterfaceAddr)
    {
        // TODO: implement this
    }

    public void startSvc()
    {
        // TODO: implement this
    }

    public void stopSvc()
    {
        // TODO: implement this
    }

    Short getMinMTU()
    {
        // TODO: implement this
        return (short)0;
    }

    Collection<String> getActiveNICsInfoAsString()
    {
        String addr = null;
        return getActiveNICsInfoAsStringForDestinationAddr (addr);
    }

    Collection<String> getActiveNICsInfoAsStringForDestinationAddr (String dstAddr)
    {
        // TODO: implement this
        return null;
    }

    Collection<String> getActiveNICsInfoAsStringForDestinationAddr (Integer senderRemoteIPv4Addr)
    {
        return getActiveNICsInfoAsStringForDestinationAddr (InetAddresses.fromInteger (senderRemoteIPv4Addr).getHostAddress());
    }

     
    PropoagationMode getPropagationMode()
    {
        // TODO: implement this
        return PropoagationMode.MULTICAST;
    }

    Integer getDeliveryQueueSize()
    {
        // TODO: implement this
        return 0;
    }

    Long getReceiveRate (String addr)
    {
        // TODO: implement this
        return (long) 0;
    }

    Integer getTransmissionQueueSize (String outgoingInterface)
    {
        // TODO: implement this
        return 0;
    }

    Byte getRescaledTransmissionQueueSize (String outgoingInterface)
    {
        // TODO: implement this
        return (byte) 0;
    }

    Integer getTransmissionQueueMaxSize (String outgoingInterface)    
    {
        // TODO: implement this
        return 0;
    }

    Integer getTransmitRateLimit (String iface)
    {
        // TODO: implement this
        return 0;
    }

    public void setTransmissionQueueMaxSize (String iface, Integer ui32MaxSize)
    {
        // TODO: implement this
    }

    public void setTransmitRateLimit (String iface, String dstAddr, Integer ui32RateLimit)
    {
        // TODO: implement this
    }

    public void setTransmitRateLimit (String dstAddr, Integer ui32RateLimit)
    {
        setTransmitRateLimit (null, dstAddr, ui32RateLimit);
    }

    public void setTransmitRateLimit (Integer ui32RateLimit)
    {
        setTransmitRateLimit (null, null, ui32RateLimit);
    }

    Integer getLinkCapacity (String iface)
    {
        // TODO: implement this
        return 0;
    }

    public void setLinkCapacity (String iface, Integer capacity)
    {
        // TODO: implement this
    }

    Byte getNeighborQueueLength (String iface, String remoteAddd)
    {
        int ipV4Addr = InetAddresses.coerceToInteger (InetAddresses.forString (remoteAddd));

        // TODO: implement this
        return (byte)0;
    }

    boolean clearToSend (String iface)
    {
        // TODO: implement this
        return false;
    }

    boolean clearToSendOnAllInterfaces()
    {
        return clearToSend (null);
    }

    public void ping ()
    {
        // TODO: implement this
    }

    // Callbacksi
    public void messageArrived (String incomingIface, String srcIPAddr, Byte msgType, Short msgId,
                         Byte hopCount, Byte ui8TTL, byte[] metadata, byte[] data)
    {
        // TODO: implement this
    }

    public void pong()            
    {

    }

    public void registerNetworkMessageServiceListener (NetworkMessageServiceListener listener)
    {
        // TODO: implement this
    }

    public void deregisterNetworkMessageServiceListener()            
    {
        // TODO: implement this
    }

}
