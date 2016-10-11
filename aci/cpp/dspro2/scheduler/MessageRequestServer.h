/* 
 * MessageRequestServer.h
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
 *
 * Created on November 1, 2012, 9:50 PM
 */

#ifndef INCL_MESSAGE_REQUEST_SERVER_H
#define	INCL_MESSAGE_REQUEST_SERVER_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class DataStore;
    class Message;

    class MessageRequestServer 
    {
        public:
            MessageRequestServer (DataStore *pDataStore);
            ~MessageRequestServer (void);

            /**
             * Return the message matching pszRequestedMsgId, if any - it returns
             * null otherwise.
             * If pszRequestedMsgId is the ID of a message that was chunked,
             * getRequestReply() returns a chunk that is not in pChunkIdFilters,
             * if any - it returns null otherwise.
             *
             * NOTE: the caller should deallocate the returned message.
             */
            Message * getRequestReply (const char *pszRequestedMsgId, uint8 *pChunkIdFilters,
                                       uint8 ui8ChunkIdFiltersLen);

        private:
            Message * getAnyMatchingChunk (const char *pszRequestedMsgId, uint8 *pChunkIdFilters,
                                           uint8 ui8ChunkIdFiltersLen);

            DataStore * const _pDataStore;
    };
}

#endif	/* INCL_MESSAGE_REQUEST_SERVER_H */

