#ifndef INCL_MESSAGE_SENDER_H
#define INCL_MESSAGE_SENDER_H

/*
 * MessageSender.h
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
 * MessageSender
 *
 * The class used to send messages
 * Obtained by calling getSender() on a mocket
 * Note - message may also be sent using the Mocket class directly
 * This class makes it a little more convenient by having default or configurable values for some of the parameters
 */

#include "FTypes.h"


class Mocket;

class MessageSender
{
    public:
        class Params
        {
            public:
                Params (uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);
                uint16 getTag (void);
                uint8 getPriority (void);
                uint32 getEnqueueTimeout (void);
                uint32 getRetryTimeout (void);
            private:
                uint16 _ui16Tag;
                uint8 _ui8Priority;
                uint32 _ui32EnqueueTimeout;
                uint32 _ui32RetryTimeout;
        };

        MessageSender (const MessageSender &src);

        // Send (enqueue for transmission) data to the remote endpoint
        // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
        // A tag value of 0 and a priority of 0 are used for the data, along with the default values for
        //     the enqueue and retry timeouts
        // Returns 0 if successful or a negative value in case of error
        int send (const void *pBuf, uint32 ui32BufSize);

        // Gather write version of send
        // Caller can pass in any number of buffer and buffer size pairs
        // NOTE: The last argument, after all buffer and buffer size pairs, must be nullptr
        int gsend (const void *pBuf1, uint32 ui32BufSize1, ...);

        // Send (enqueue for transmission) data to the remote endpoint
        // The data is tagged with the specified tag value and sent using the specified priority
        // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
        // The default values for the enqueue and retry timeouts are used
        // Returns 0 if successful or a negative value in case of error
        int send (const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority);

        // Send (enqueue for transmission) data to the remote endpoint
        // The tag, priority, enqueue timeout, and retry timeout values are specified via the Params object
        // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
        // Returns 0 if successful or a negative value in case of error
        int send (const void *pBuf, uint32 ui32BufSize, Params *pParams);

        // Send (enqueue for transmission) data to the remote endpoint
        // The data is tagged with the specified tag value and sent using the specified priority, using the
        //     specified enqueue and retry timeout values
        // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
        // Returns 0 if successful or a negative value in case of error
        int send (const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                  uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

        // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
        // and then transmits the new message using the specified parameters
        // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
        // See documentation for cancel() and send() for more details
        // Returns 0 if successful or a negative value in case of error
        int replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag);

        // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
        // and then transmits the new message using the specified parameters
        // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
        // See documentation for cancel() and send() for more details
        // Returns 0 if successful or a negative value in case of error
        int replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, Params *pParams);

        // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
        // and then transmits the new message using the specified parameters
        // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
        // See documentation for cancel() and send() for more details
        // Returns 0 if successful or a negative value in case of error
        int replace (const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                     uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

        // Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
        // Note that the messages may be pending transmission (which applies to all flows)
        // or may have already been transmitted but not yet acknowledged (which only applies to
        // reliable flows)
        int cancel (uint16 ui16TagId);

        // Set the default timeout for enqueuing data into the outgoing buffer
        // If the outgoing buffer is full, a call to send will block until the timeout expires
        // Specifying a timeout value of 0 disables the timeout
        void setDefaultEnqueueTimeout (uint32 ui32EnqueueTimeout);

        // Set the default timeout for retransmission of reliable packets that have not been
        // acknowledged
        // Specifying a timeout value of 0 disables the timeout
        // NOTE: Setting a timeout would imply that packets may not be reliable!
        void setDefaultRetryTimeout (uint32 ui32RetryTimeout);

        static const uint8 DEFAULT_PRIORITY = 5;

    private:
        friend class Mocket;
        MessageSender (Mocket *pMocket, bool bReliable, bool bSequenced);

    private:
        Mocket *_pMocket;
        bool _bReliable;
        bool _bSequenced;
        uint32 _ui32DefaultEnqueueTimeout;
        uint32 _ui32DefaultRetryTimeout;
};

inline MessageSender::MessageSender (Mocket *pMocket, bool bReliable, bool bSequenced)
{
    _pMocket = pMocket;
    _bReliable = bReliable;
    _bSequenced = bSequenced;
    _ui32DefaultEnqueueTimeout = 0;
    _ui32DefaultRetryTimeout = 0;
}

inline void MessageSender::setDefaultEnqueueTimeout (uint32 ui32EnqueueTimeout)
{
    _ui32DefaultEnqueueTimeout = ui32EnqueueTimeout;
}

inline void MessageSender::setDefaultRetryTimeout (uint32 ui32RetryTimeout)
{
    _ui32DefaultRetryTimeout = ui32RetryTimeout;
}

inline MessageSender::Params::Params (uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    _ui16Tag = ui16Tag;
    _ui8Priority = ui8Priority;
    _ui32EnqueueTimeout = ui32EnqueueTimeout;
    _ui32RetryTimeout = ui32RetryTimeout;
}

inline uint16 MessageSender::Params::getTag (void)
{
    return _ui16Tag;
}

inline uint8 MessageSender::Params::getPriority (void)
{
    return _ui8Priority;
}

inline uint32 MessageSender::Params::getEnqueueTimeout (void)
{
    return _ui32EnqueueTimeout;
}

inline uint32 MessageSender::Params::getRetryTimeout (void)
{
    return _ui32RetryTimeout;
}

#endif   //#ifndef INCL_MESSAGE_SENDER_H
