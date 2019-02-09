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

import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import us.ihmc.comm.CommException;
import us.ihmc.comm.ProtocolException;
import us.ihmc.util.proxy.CallbackHandlerFactory;
import us.ihmc.util.proxy.Stub;

/**
 * @author @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 * @author Filippo Poltronieri (fpoltronieri@ihmc.us)
 */
public class NMSProxy extends Stub {
    private static Logger _logger = LoggerFactory.getLogger(NMSProxy.class);

    public static final int DEFAULT_PORT = 56488;
    public static final String SERVICE = "nms";
    public static final String VERSION = "20150619";
    private final Publisher _publisher;
    private final Collection<NetworkMessageServiceListener> _listeners = new ArrayList<>();

    enum PropagationMode {
        BROADCAST,
        MULTICAST,
        NORM
    }

    public NMSProxy() {
        this("127.0.0.1", DEFAULT_PORT);
    }

    public NMSProxy(String host) {
        this(host, DEFAULT_PORT);
    }

    public NMSProxy(String host, int port) {
        this((short) 0, host, port);
    }

    public NMSProxy(short applicationId, String host, int port) {
        this(new us.ihmc.nms.CallbackHandlerFactory(), applicationId, host, port);
    }

    private NMSProxy(CallbackHandlerFactory handlerFactory, short applicationId, String host, int port) {
        super(handlerFactory, applicationId, host, port, SERVICE, VERSION);
        _publisher = new Publisher(_commHelper, _isInitialized);
    }

    public void broadcastMessage(byte msgType, Collection<String> outgoingInterfaces,
                                 String dstAddr, short ui16MsgId, byte ui8HopCount, byte ui8TTL,
                                 short ui16DelayTolerance, byte[] metadata, byte[] data,
                                 boolean bExpedited, String hints)
            throws CommException, ProtocolException {

        final String method = Protocol.BROADCAST_METHOD;
        checkConcurrentModification(method);
        synchronized (this) {
            _publisher.publish(method, msgType, outgoingInterfaces, dstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                    ui16DelayTolerance, metadata, data, bExpedited, hints);
        }
    }

    public void transmitMessage(byte msgType, Collection<String> outgoingInterfaces,
                                String dstAddr, short ui16MsgId, byte ui8HopCount, byte ui8TTL,
                                short ui16DelayTolerance, byte[] metadata, byte[] data, String hints)
            throws CommException, ProtocolException {

        final String method = Protocol.TRANSMIT_METHOD;
        checkConcurrentModification(method);
        synchronized (this) {
            _publisher.publish(method, msgType, outgoingInterfaces, dstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                    ui16DelayTolerance, metadata, data, hints);
        }
    }

    public void transmitReliableMessage(byte msgType, Collection<String> outgoingInterfaces,
                                        String dstAddr, short ui16MsgId, byte ui8HopCount, byte ui8TTL,
                                        short ui16DelayTolerance, byte[] metadata, byte[] data, String hints)
            throws CommException, ProtocolException {

        final String method = Protocol.TRANSMIT_RELIABLE_METHOD;
        checkConcurrentModification(method);
        synchronized (this) {
            _publisher.publish(method, msgType, outgoingInterfaces, dstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                    ui16DelayTolerance, metadata, data, hints);
        }
    }

    //-------------------------------------------------------------------------

    public void setRetransmissionTimeout(Integer timeout)
            throws CommException, ProtocolException {
        setRetransmissionTimeout(Protocol.SET_TRANSMISSION_TIMEOUT_METHOD, timeout);
    }

    private void setRetransmissionTimeout(String method, Integer timeout)
            throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.writeUI32(timeout.intValue());
                _commHelper.read32();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void setPrimaryInterface(String pszInterfaceAddr) throws CommException, ProtocolException {
        setPrimaryInterface(Protocol.SET_TRANSMISSION_TIMEOUT_METHOD, pszInterfaceAddr);
    }

    private void setPrimaryInterface(String method, String pszInterfaceAddr) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(pszInterfaceAddr);
                _commHelper.read32();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void startSvc() throws CommException, ProtocolException {
        singleString(Protocol.START_METHOD);
    }

    public void stopSvc() throws CommException, ProtocolException {
        singleString(Protocol.STOP_METHOD);
    }

    private void singleString(String method)
            throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.read32();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public short getMinMTU() throws CommException, ProtocolException {
        return getMinMTU(Protocol.GET_MIN_MTU_METHOD);
    }

    private short getMinMTU(String method) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                short minMtu = _commHelper.read16();
                return minMtu;
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public Collection<String> getActiveNICsInfoAsString(String addr) throws CommException, ProtocolException {
        return getActiveNICsInfoAsStringForDestinationAddr(Protocol.GET_ACTIVE_NIC_AS_STRING_METHOD, addr);
    }

    private Collection<String> getActiveNICsInfoAsStringForDestinationAddr(String method, String dstAddr)
            throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(dstAddr);
                String line = _commHelper.receiveString();
                ArrayList<String> networkInterfaceList = new ArrayList<>();
                while (line.length() > 0) {
                    networkInterfaceList.add(line);
                    line = _commHelper.receiveString();
                }
                return networkInterfaceList;

            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public PropagationMode getPropagationMode() throws CommException, ProtocolException {
        return getPropagationMode(Protocol.GET_PROPAGATION_MODE_METHOD);
    }

    private PropagationMode getPropagationMode(String method) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                Byte propagationMode = _commHelper.read8();
                if (propagationMode.equals(0x00)) {
                    return PropagationMode.BROADCAST;
                } else if (propagationMode.equals(0x01)) {
                    return PropagationMode.MULTICAST;
                } else {
                    return PropagationMode.NORM;
                }
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public int getDeliveryQueueSize() throws CommException, ProtocolException {
        return getDeliveryQueueSize(Protocol.GET_DELIVERY_QUEUE_SIZE_METHOD);
    }

    private int getDeliveryQueueSize(String method) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                //int can be safely use to read uint32
                int deliveryQueueSize = (int) _commHelper.readUI32();
                return Integer.valueOf(deliveryQueueSize);
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public long getReceiveRate(String addr) throws CommException, ProtocolException {
        return getReceiveRate(Protocol.GET_RECEIVE_RATE_METHOD, addr);
    }

    private long getReceiveRate(String method, String addr) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(addr);
                return _commHelper.readI64();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public int getTransmissionQueueSize(String outgoingInterface) throws CommException, ProtocolException {
        return getTransmissionInfo(Protocol.GET_TRANSMISSION_QUEUE_SIZE_METHOD, outgoingInterface);
    }

    public int getTransmissionQueueMaxSize(String outgoingInterface) throws CommException, ProtocolException {
        return getTransmissionInfo(Protocol.GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, outgoingInterface);
    }

    public int getTransmitRateLimit(String iface) throws CommException, ProtocolException {
        return getTransmissionInfo(Protocol.GET_TRANSMISSION_RATE_LIMIT_METHOD, iface);
    }

    private int getTransmissionInfo(String method, String outgoingInterface) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(outgoingInterface);
                int tranmissionQueueSize = (int) _commHelper.readUI32();
                return Integer.valueOf(tranmissionQueueSize);
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public byte getRescaledTransmissionQueueSize(String outgoingInterface) throws CommException, ProtocolException {
        return getRescaledTransmissionQueueSize(Protocol.GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, outgoingInterface);
    }

    private byte getRescaledTransmissionQueueSize(String method, String outgoingInterface)
            throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(outgoingInterface);
                return _commHelper.read8();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void setTransmissionQueueMaxSize(String iface, Integer ui32MaxSize) throws CommException, ProtocolException {
        setTransmissionQueueMaxSize(Protocol.SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD, iface, ui32MaxSize);
    }

    private void setTransmissionQueueMaxSize(String method, String iface, Integer ui32MaxSize) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                _commHelper.writeUI32(ui32MaxSize);
                _commHelper.read32();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void setTransmitRateLimit(Integer ui32RateLimit) throws CommException, ProtocolException {
        setTransmitRateLimit(null, null, ui32RateLimit);
    }

    public void setTransmitRateLimit(String iface, String dstAddr, Integer ui32RateLimit)
            throws CommException, ProtocolException {
        setTransmitRateLimit(Protocol.SET_TRANSMIT_RATE_LIMIT_METHOD, iface, dstAddr, ui32RateLimit);
    }

    private void setTransmitRateLimit(String method, String iface, String dstAddr, Integer ui32RateLimit)
            throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                _commHelper.sendStringBlock(dstAddr);
                _commHelper.writeUI32(ui32RateLimit);
                _commHelper.read32();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public int getLinkCapacity(String iface) throws CommException, ProtocolException {
        return getLinkCapacity(Protocol.GET_LINK_CAPACITY_METHOD, iface);
    }

    private int getLinkCapacity(String method, String iface) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                return Integer.valueOf((int) _commHelper.readUI32());
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void setLinkCapacity(String iface, Integer capacity) throws CommException, ProtocolException {
        setLinkCapacity(Protocol.SET_LINK_CAPACITY_METHOD, iface, capacity);
    }

    private void setLinkCapacity(String method, String iface, Integer capacity) throws CommException, ProtocolException {

        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                _commHelper.writeUI32(capacity);
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public byte getNeighborQueueLength(String iface, String remoteAddd) throws CommException, ProtocolException {
        int ipV4Addr = InetAddresses.coerceToInteger(InetAddresses.forString(remoteAddd));
        return getNeighborQueueLength(Protocol.GET_NEIGHBOR_QUEUE_LENGTH_METHOD, iface, ipV4Addr);
    }

    private byte getNeighborQueueLength(String method, String iface, int remoteAddress)
            throws CommException, ProtocolException {

        checkConcurrentModification(Protocol.GET_NEIGHBOR_QUEUE_LENGTH_METHOD);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                _commHelper.writeUI32(remoteAddress);
                return _commHelper.read8();
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public String getEncryptionKeyHash() throws CommException, ProtocolException {

        final String method = Protocol.GET_ENCRYPTION_KEY_HASH_METHOD;

        checkConcurrentModification(method);
        synchronized (this) {
            _commHelper.sendLine(method);
            long idLen = _commHelper.readUI32();
            if(idLen > 32) {
                // It's a 32-bit hash...
                _logger.warn("Received an encryption key length of " + idLen + " while max length should be 32 at most");
                idLen = 32;
            }
            final int safeIdLen = safeToInt(idLen);
            byte[] buf = new byte[safeIdLen];
            _commHelper.receiveBlob (buf, 0, safeIdLen);
            _commHelper.receiveMatch("OK");
            if (buf == null || buf.length == 0) {
                return "";
            }
            return new String(buf, Charset.defaultCharset());
        }
    }

    private static int safeToInt(long value) {
        if ((int)value != value) {
            throw new ArithmeticException("integer overflow");
        }
        return (int)value;
    }

    //-------------------------------------------------------------------------

    public boolean clearToSendOnAllInterfaces() throws CommException, ProtocolException {
        return clearToSend(null);
    }

    public boolean clearToSend(String iface) throws CommException, ProtocolException {
        return clearToSend(Protocol.CLEAR_TO_SEND_METHOD, iface);
    }

    private boolean clearToSend(String method, String iface) throws CommException, ProtocolException {
        checkConcurrentModification(method);
        synchronized (this) {
            try {
                _commHelper.sendLine(method);
                _commHelper.sendStringBlock(iface);
                short clearToSend = _commHelper.read8();
                return (clearToSend == 1);
            } catch (CommException e) {
                _isInitialized.set(false);
                throw e;
            }
        }
    }

    //-------------------------------------------------------------------------

    public void ping() {
        // TODO: implement this
    }

    // Callback
    public void messageArrived(String incomingIface, String srcIPAddr, byte msgType, short msgId,
                               byte hopCount, byte ui8TTL, boolean isUnicast, byte[] metadata, byte[] data,
                               long timestamp, Long groupMsgCount, Long unicastMsgCount) {

        MessageArrived cback = new MessageArrived(incomingIface, srcIPAddr, msgType, msgId,
                hopCount, ui8TTL, isUnicast, metadata, data, timestamp, groupMsgCount, unicastMsgCount);
        for (NetworkMessageServiceListener l : _listeners) {
            l.messageArrived(cback);
        }
    }

    public void pong() {}

    public void registerNetworkMessageServiceListener(byte msgType, NetworkMessageServiceListener listener)
            throws CommException, ProtocolException {

        _commHelper.sendLine("registerListener");
        _commHelper.write8(msgType);
        _commHelper.receiveMatch("OK");
        _listeners.add(listener);
    }

    public void deregisterNetworkMessageServiceListener() {
        // TODO: implement this
    }
}
