/*
 * Protocol.java
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

/**
 *
 * @author Giacomo Benincasa    (gbenincasa@ihmc.us)
 */
public class Protocol {
    static final String BROADCAST_METHOD = "broadcast";
    static final String TRANSMIT_METHOD = "transmit";
    static final String TRANSMIT_RELIABLE_METHOD = "transmit-reliable";
    static final String SET_TRANSMISSION_TIMEOUT_METHOD = "setRetransmissionTimeout";
    static final String SET_PRIMARY_INTERFACE_METHOD = "setPrimaryInterface";
    static final String START_METHOD = "start";
    static final String STOP_METHOD = "stop";
    static final String GET_MIN_MTU_METHOD = "getMinMTU";
    static final String GET_ACTIVE_NIC_AS_STRING_METHOD = "getActiveNICsForDstAddr";
    static final String GET_PROPAGATION_MODE_METHOD = "getPropagationMode";
    static final String GET_DELIVERY_QUEUE_SIZE_METHOD = "getDeliveryQueueSize";
    static final String GET_ENCRYPTION_KEY_HASH_METHOD = "getEncryptionKeyHash";
    static final String GET_RECEIVE_RATE_METHOD = "getReceiveRate";
    static final String GET_TRANSMISSION_QUEUE_SIZE_METHOD = "getTransmissionQueueSize";
    static final String GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "getRescTransmissionQueueSize";
    static final String GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "getTransmissionQueueMaxSize";
    static final String GET_TRANSMISSION_RATE_LIMIT_METHOD = "getTransmitRateLimit";
    static final String SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "setTransmissionQueueMaxSize";
    static final String SET_TRANSMIT_RATE_LIMIT_METHOD = "setTransmitRateLimit";
    static final String GET_LINK_CAPACITY_METHOD = "getLinkCapacity";
    static final String SET_LINK_CAPACITY_METHOD = "setLinkCapacity";
    static final String GET_NEIGHBOR_QUEUE_LENGTH_METHOD = "getNeighborQueueLength";
    static final String CLEAR_TO_SEND_METHOD = "clearToSend";
    static final String PING_METHOD = "ping";
}
