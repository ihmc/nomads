/*
 * NetworkMessageServiceListener.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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

#ifndef INCL_NETWORK_MESSAGE_SERVICE_LISTENER_H
#define INCL_NETWORK_MESSAGE_SERVICE_LISTENER_H

#include "FTypes.h"

namespace NOMADSUtil
{
    class NetworkMessageServiceListener
    {
        public:
            explicit NetworkMessageServiceListener (uint16 ui16ApplicationId);
            virtual ~NetworkMessageServiceListener (void);

            /**
             * Callback function that is invoked when a message arrives on this node
             * The handler should return 0 if the message has been handled successfully
             * (which might include forwarding the message or dropping the message) or
             * a negative return code in case of error. The actions of the MessagePropagationService
             * in the event of a negative return code is not defined and could range from
             * doing nothing to logging the error or some other action.
             *
             * pszIncomingInterface - the interface on which this message was received
             *
             * ui32SourceIPAddress - source IP address
             *
             * ui8MsgType - an application-defined identifier that is used to demultiplex multiple
             *              applications and message types - same as the key used when registering
             *              the listener and when sending a message
             *
             * ui16MsgId - a unique identifier for this message
             *             NOTE: If the message is being forwarded and the desire is to maintain the id
             *             for the message, this id should be passed into broadcastMessage or transmitMessage
             *
             * ui8HopCount - the number of hops that this message has already traversed to arrive at this node
             *
             * ui8TTL - the time to live (in terms of number of hops) for this message, as originally defined
             *          by the sender (unless modified by an intermediate node, which is possible and allowed)
             *
             * pMsgMetaData - metadata for this message - as required by the component using the MessagePropagationService.
             *                Some potential data includes the path and link characteristics that the message has
             *                traversed to arrive at this node. The metadata may be modified as necessary (appended to,
             *                truncated, etc.) by the listener before forwarding the message on to the next hop.
             *                NOTE: The size of the metadata is limited to 65535 bytes, given that the size is
             *                specified using a 16-bit unsigned number.
             *
             * ui16MsgMetaDataLen - Size of the message metadata in bytes
             *
             * pMsg - the message itself
             *
             * ui16MsgLen - length of the message
             *
             */
            virtual int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL, bool bUnicast,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen, int64 i64Timestamp,
                                        uint64 ui64GroupMsgCount, uint64 ui64UnicastMsgCount) = 0;

            bool operator == (const NetworkMessageServiceListener &rhsCbackHandler) const;

        private:
	    friend class NetworkMessageServiceImpl;
            uint16 _ui16ApplicationId;
    };

    inline NetworkMessageServiceListener::NetworkMessageServiceListener (uint16 ui16ApplicationId)
        : _ui16ApplicationId (ui16ApplicationId)
    {
    }

    inline NetworkMessageServiceListener::~NetworkMessageServiceListener (void)
    {
    }

    inline bool NetworkMessageServiceListener::operator == (const NetworkMessageServiceListener &rhsCbackHandler) const
    {
        return (rhsCbackHandler._ui16ApplicationId == _ui16ApplicationId);
    }
}

#endif   // #ifndef INCL_NETWORK_MESSAGE_SERVICE_LISTENER_H
