/*
 * TCPConnHandler.h
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
 * Created on April 14, 2014, 5:58 PM
 */

#ifndef INCL_TCP_CONN_HANDLER_H
#define INCL_TCP_CONN_HANDLER_H

#include "ConnHandler.h"

namespace NOMADSUtil
{
    class Socket;
    class TCPSocket;
}

namespace IHMC_ACI
{
    class TCPConnHandler : public ConnHandler
    {
        public:
            TCPConnHandler (const AdaptorProperties &adptProp, const char *pszRemotePeerId,
                            CommAdaptorListener *pListener, NOMADSUtil::TCPSocket *pSocket,
                            const char *pszLocalPeerAdd);
            virtual ~TCPConnHandler (void);

            int init (void) ;
            bool isConnected (void);

            void requestTermination (void);
            void requestTerminationAndWait (void);
            void abortConnHandler (void);

            const char * getLocalPeerAddress (void) const;
            const char * getRemotePeerAddress (void) const;

            void resetTransmissionCounters (void);

            int send (const void *pBuf, uint32 ui32Len, uint8 ui8Priority);


            static int send (NOMADSUtil::Socket *pSocket, const void *pBuf, uint32 ui32Len, uint8 ui8Priority = 0);


        protected:
            int receive (BufferWrapper &bw, char **ppszRemotePeerAddr);


        private:
            static const uint16 MAX_MESSAGE_SIZE = 65000U;

            NOMADSUtil::TCPSocket *_pSocket;
            NOMADSUtil::String _localPeerAddr;
            NOMADSUtil::String _remotePeerAddr;
            char _buf[MAX_MESSAGE_SIZE];
    };


    inline TCPConnHandler::~TCPConnHandler (void) { }

    inline const char * TCPConnHandler::getLocalPeerAddress (void) const
    {
        return _localPeerAddr;
    }

    inline const char * TCPConnHandler::getRemotePeerAddress (void) const
    {
        return _remotePeerAddr;
    }

    inline void TCPConnHandler::resetTransmissionCounters (void) { }

}

#endif    /* INCL_TCP_CONN_HANDLER_H */
