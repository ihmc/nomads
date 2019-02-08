/*
 * NetworkMessageServiceUnmarshaller.h
 *
 * This file is part of the IHMC Network Message Service Library
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 26, 2015, 9:51 PM
 */

#ifndef INCL_NETWORK_MESSAGE_SERVICE_UNMARSHALLER_H
#define    INCL_NETWORK_MESSAGE_SERVICE_UNMARSHALLER_H

#include "StrClass.h"
#include "SimpleCommHelper2.h"

namespace NOMADSUtil
{
    class Stub;
    class NetworkMessageService;
    class NetworkMessageServiceProxy;
    class SimpleCommHelper2;

    class NetworkMessageServiceUnmarshaller
    {
        public:
            static const String SERVICE;
            static const String VERSION;

            // List of invokable methods
            static const String BROADCAST_METHOD;
            static const String TRANSMIT_METHOD;
            static const String TRANSMIT_RELIABLE_METHOD;
            static const String SET_TRANSMISSION_TIMEOUT_METHOD;
            static const String SET_PRIMARY_INTERFACE_METHOD;
            static const String START_METHOD;
            static const String STOP_METHOD;
            static const String GET_MIN_MTU_METHOD;
            static const String GET_ACTIVE_NIC_AS_STRING_METHOD;
            static const String GET_PROPAGATION_MODE_METHOD;
            static const String GET_DELIVERY_QUEUE_SIZE_METHOD;
            static const String GET_RECEIVE_RATE_METHOD;
            static const String GET_TRANSMISSION_QUEUE_SIZE_METHOD;
            static const String GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD;
            static const String GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD;
            static const String GET_TRANSMISSION_RATE_LIMIT_METHOD;
            static const String SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD;
            static const String SET_TRANSMIT_RATE_LIMIT_METHOD;
            static const String GET_LINK_CAPACITY_METHOD;
            static const String SET_LINK_CAPACITY_METHOD;
            static const String GET_NEIGHBOR_QUEUE_LENGTH_METHOD;
            static const String CLEAR_TO_SEND_METHOD;
            static const String GET_ENCRYPTION_KEY_HASH;
            static const String CHANGE_KEY_METHOD;
            static const String PING_METHOD;

            static const String REGISTER_LISTENER_METHOD;

            // Callbacks
            static const String MESSAGE_ARRIVED;
            static const String PONG_ARRIVED;

            static bool methodInvoked (uint16 ui16ClientId, const String &methodName,
                                       void *pNMS, SimpleCommHelper2 *pCommHelper,
                                       SimpleCommHelper2::Error &error);

            static bool methodArrived (uint16 ui16ClientId, const String &methodName,
                                       Stub *pNMSProxy, SimpleCommHelper2 *pCommHelper);
    };
}

#endif    /* INCL_NETWORK_MESSAGE_SERVICE_UNMARSHALLER_H */

