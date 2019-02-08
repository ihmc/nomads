/*
 * MocketsConnHandler.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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
 * Created on August 19, 2012, 4:14 PM
 */

#ifndef INCL_MOCKETS_CONNECTON_HANDLER_H
#define INCL_MOCKETS_CONNECTON_HANDLER_H

#include "ConnHandler.h"

#include "Mutex.h"

class Mocket;


namespace IHMC_ACI
{
    class MocketConnHandler : public ConnHandler
    {
        public:
            MocketConnHandler (const AdaptorProperties & adptProp, const char * pszRemotePeerId,
                               CommAdaptorListener * pListener, Mocket * pMocket);
            ~MocketConnHandler (void);

            int init (void);
            bool isConnected (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);
            void abortConnHandler (void);

            bool isRemotePeerActive (void) const;
            void setActiveRemotePeer (void);
            void setInactiveRemotePeer (void);
            const char * getLocalPeerAddress (void) const;
            const char * getRemotePeerAddress (void) const;

            void resetTransmissionCounters (void);

            int send (const void * pBuf, uint32 ui32BufSize, uint8 ui8Priority);

            int sendDataMessage (const void * pBuf, uint32 ui32BufSize, uint8 ui8Priority);
            int sendVersionMessage (const void * pBuf, uint32 ui32BufSize, uint8 ui8Priority);
            int sendWaypointMessage (const void * pBuf, uint32 ui32BufSize, uint8 ui8Priority,
                                     const char * pszPublisherNodeId);


       protected:
            int receive (BufferWrapper & bw, char ** ppszRemotePeerAddr);
            int cancelPreviousAndSend (bool bReliableTransmission, bool bSequencedTransmission,
                                       const void * pBuf, uint32 ui32BufSize, uint16 ui16Tag,
                                       uint8 ui8Priority);
            int replace (bool bReliableTransmission, bool bSequencedTransmission,
                         const void * pBuf, uint32 ui32Len, uint16 ui16Tag, uint8 ui8Priority);
            int send (bool bReliableTransmission, bool bSequencedTransmission,
                      const void * pBuf, uint32 ui32Len, uint16 ui16Tag, uint8 ui8Priority);


            static bool callNewPeer (void * pArg, unsigned long ulTimeSinceLastContact);
            static bool callDeadPeer (void * pArg, unsigned long ulTimeSinceLastContact);


        private:
            static const uint16 MAX_MESSAGE_SIZE = 65000U;

            const bool _bReliableTransmission;
            const bool _bSequencedTransmission;
            bool _bActiveRemotePeer;
            const uint32 _ui32EnqueueTimeout;
            const uint32 _ui32RetryTimeout;
            unsigned long _ui32UncreachablePeerTimeout;

            Mocket * _pMocket;
            const NOMADSUtil::String _localPeerAddr;
            const NOMADSUtil::String _remotePeerAddr;

            NOMADSUtil::Mutex _m;
            char _buf[MAX_MESSAGE_SIZE];
    };


    inline bool MocketConnHandler::isRemotePeerActive (void) const
    {
        return _bActiveRemotePeer;
    }

    inline void MocketConnHandler::setActiveRemotePeer (void)
    {
        _bActiveRemotePeer = true;
    }

    inline void MocketConnHandler::setInactiveRemotePeer (void)
    {
        _bActiveRemotePeer = false;
    }

    inline const char * MocketConnHandler::getLocalPeerAddress (void) const
    {
        return _localPeerAddr.c_str();
    }

    inline const char * MocketConnHandler::getRemotePeerAddress (void) const
    {
        return _remotePeerAddr.c_str();
    }

}

#endif  // INCL_MOCKETS_CONNECTON_HANDLER_H
