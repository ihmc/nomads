/* 
 * ConnHandler.h
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
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 * Created on August 19, 2012, 4:14 PM
 */

#ifndef INCL_CONNECTON_HANDLER_H
#define	INCL_CONNECTON_HANDLER_H

#include  "AdaptorProperties.h"
#include "MessageHeaders.h"

#include "BufferReader.h"
#include "ManageableThread.h"
#include "StrClass.h"

namespace IHMC_ACI
{
    class CommAdaptorListener;
    class ConnEndPoint;

    class ConnHandler : public NOMADSUtil::ManageableThread
    {
        public:
            struct HandshakeResult
            {
                HandshakeResult (const NOMADSUtil::String remotePeerId,
                                 const NOMADSUtil::String localConnectionIfaceAddr);
                HandshakeResult (const HandshakeResult &res);
                ~HandshakeResult (void);

                const NOMADSUtil::String _remotePeerId;
                const NOMADSUtil::String _localConnectionIfaceAddr;
            };

            virtual ~ConnHandler (void);

            virtual int init (void) = 0;
            virtual bool isConnected (void) = 0;
            void run (void);

            virtual const char * getLocalPeerAddress (void) const = 0;
            virtual const char * getRemotePeerAddress (void) const = 0;
            uint16 getRemotePeerAddressPort (void) const;
            const char * getRemotePeerNodeId (void) const;

            virtual void resetTransmissionCounters (void) = 0;
            virtual int send (const void *pBuf, uint32 ui32Len, uint8 ui8Priority) = 0;

            /**
             * Returns the ID of the connected remote peer if successful, an empty string otherwise
             */
            static HandshakeResult doHandshake (ConnEndPoint *pEndPoint, const char *pszNodeId,
                                                const char *pszSessionId, CommAdaptorListener *pListener);

        protected:
            ConnHandler (const AdaptorProperties &adptProp, const char *pszRemotePeerId,
                         uint16 ui16RemotePeerPort, CommAdaptorListener *pListener);

            struct BufferWrapper
            {
                BufferWrapper (void);
                ~BufferWrapper (void);

                int init (void *pBuf, uint32 ui32BufSize, bool bDeleteWhenDone);

                const void * getBuffer (void);
                uint32 getBufferLength (void);

                private:
                    bool _bDeleteWhenDone;
                    void *_pBuf;
                    uint32 _ui32BufSize;
            };

            // NOTE: ppRemotePeerAddr must be deallocated by the caller
            virtual int receive (BufferWrapper &bw, char **ppszRemotePeerAddr) = 0;

            virtual int processPacket (const void *pBuf, uint32 ui32BufSize,
                                       char *pRemotePeerAddr);

        private:
            int doChunkMessageRequest (const char *pszPublisherId, NOMADSUtil::Reader *pReader);
            int doMessageRequest (const char *pszPublisherId, NOMADSUtil::Reader *pReader);
            int doSearchReply (NOMADSUtil::Reader *pReader);
            int doVolatileSearchReply (NOMADSUtil::Reader *pReader);

            int processCtrlPacket (MessageHeaders::MsgType type, NOMADSUtil::BufferReader &br,
                                   const void *pBuf, uint32 ui32BufSize, unsigned int &uiShift);
            int processDataPacket (MessageHeaders::MsgType type, NOMADSUtil::BufferReader &br,
                                   uint32 ui32BufSize);

        protected:
            const AdaptorProperties _adptProp;
            CommAdaptorListener *_pListener;

        private:
            const uint16 _ui16RemotePeerPort;
            const NOMADSUtil::String _remotePeerId;
    };

    inline const void * ConnHandler::BufferWrapper::getBuffer (void)
    {
        return _pBuf;
    }

    inline uint32 ConnHandler::BufferWrapper::getBufferLength (void)
    {
        return _ui32BufSize;
    }

    inline uint16 ConnHandler::getRemotePeerAddressPort (void) const
    {
        return _ui16RemotePeerPort;
    }

    inline const char * ConnHandler::getRemotePeerNodeId (void) const
    {
        return _remotePeerId.c_str();
    }

    inline ConnHandler::HandshakeResult::HandshakeResult (const NOMADSUtil::String remotePeerId,
                                                          const NOMADSUtil::String localConnectionIfaceAddr)
        : _remotePeerId (remotePeerId),
          _localConnectionIfaceAddr (localConnectionIfaceAddr)
    {
    }

    inline ConnHandler::HandshakeResult::HandshakeResult (const ConnHandler::HandshakeResult &res)
        : _remotePeerId (res._remotePeerId),
          _localConnectionIfaceAddr (res._localConnectionIfaceAddr)
    {
    }

    inline ConnHandler::HandshakeResult::~HandshakeResult (void)
    {
    }
}

#endif  // INCL_CONNECTON_HANDLER_H

