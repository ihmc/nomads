/* 
 * DisServiceMsgHelper.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * Created on May 25, 2011, 12:11 AM
 */

#ifndef INCL_DISSERVICE_MSG_HELPER_H
#define	INCL_DISSERVICE_MSG_HELPER_H

#include "StringHashset.h"

#include "StrClass.h"

namespace IHMC_ACI
{
    class DisServiceMsg;
    class DisServiceDataMsg;
    class DisServiceDataMsgFragmenter;
    class DisServiceDataReqMsg;
    class Message;
    class MessageHeader;

    class DisServiceMsgHelper
    {
        public:
            /**
             * Returns a copy of pDSMsg.  Returns NULL if an error occurs.
             *
             * NOTE: The object is copied by serializing and de-serializing it
             * again, thus this method may not be very efficient.
             */
            static DisServiceMsg * clone (DisServiceMsg *pDSMsg);

            /**
             * Deallocates pDSMsg. If pDSMsg is of DisServiceDataMsg type, it
             * calls deallocatedDisServiceDataMsg().
             */
            static void deallocatedDisServiceMsg (DisServiceMsg *pDSMsg);

            /**
             * Deallocates pDSDataMsg and its inner objects (including the ones
             * that would not be deallocated by DisServiceDataMsg's destructor).
             */
            static void deallocatedDisServiceDataMsg (DisServiceDataMsg *pDSDataMsg);

            /**
             * Deallocates pDSDataReqMsg and its inner objects (including the ones
             * that would not be deallocated by DisServiceDataReqMsg's destructor).
             */
            static void deallocatedDisServiceDataReqMsg (DisServiceDataReqMsg *pDSDataReqMsg);

            /**
             * Returns an instance of the class type derived from DisServiceMsg
             * that is specified in ui8Type.
             */
            static DisServiceMsg * getInstance (uint8 ui8Type);

            /**
             * De-serialize the needed portion of pMsgMetaData that contains the
             * DisServiceMsg type and assigns it to ui8MsgType.
             *
             * Returns a negative number if an error occurs, 0 otherwise.
             */
            static int getMessageType (const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                       uint8 &ui8MsgType);
            static const char * getMessageTypeAsString (uint8 ui8MsgType);

            /**
             * Returns true if ui8Type is DisseminationService's.
             */
            static bool isDisServiceMessage (uint8 ui8Type);

            /**
             * Return true is the target is not set, or if the message has a
             * target set and if the node is one of the targets.
             * Returns false otherwise.
             */
            static bool isTarget (const char *pszNodeId, DisServiceMsg *pDSMsg,
                                  bool &bTargetSpecified);

            static bool isInSession (const char *pszSessionId, DisServiceMsg *pDSMsg);
            static bool isInSession (const char *pszSessionId1, const char *pszSessionId2);

            /**
             * Concatenate a set of message ids of the nodes are targeted a
             * message.
             * NOTE: the caller should deallocate the returned string.
             */
            static NOMADSUtil::String getMultiNodeTarget (NOMADSUtil::StringHashset &targets);

            /**
             * Returns true if the given message type exists, false otherwise
             */
            static bool messageTypeExists (uint8 ui8MsgType);

            /**
             * Return true is the message pDSMsg was sent by pszNodeId, false
             * otherwise.
             *
             * NOTE: if the sender id was not set in pDSMsg, it returns false.
             */
            static bool sentBy (const char *pszNodeId, DisServiceMsg *pDSMsg);

        private:
            static const char TARGET_SEPARATOR;
    };

    //--------------------------------------------------------------------------

    class DisServiceDataMsgFragmenter
    {
        public:
            DisServiceDataMsgFragmenter (const char *pszNodeId);
            ~DisServiceDataMsgFragmenter (void);

            void init (DisServiceDataMsg *pDSDataMsg, uint16 ui16FragSize, uint32 ui32HeaderSize);

            /**
             * Return the next fragment if the message has not be completely
             * fragmented yet, return NULL otherwise.
             * NOTE: the returned pDSDataMsg must be deallocated by the caller.
             */
            DisServiceDataMsg * getNextFragment (void);

        private:
            uint16 _ui16FragSize;
            uint16 _ui32HeaderSize;
            uint32 _ui32CurrentOffset;
            uint32 _ui32StartOffset;
            uint32 _ui32StopOffset;
            MessageHeader *_pMH;
            DisServiceDataMsg *_pDSDataMsg;
            void *_pFragBuf;
            uint16 _ui16CurrFragBufLen;
            const NOMADSUtil::String _nodeId;
            Message *_pMsgFragment;
    };
}

#endif	// INCL_DISSERVICE_MSG_HELPER_H

