/*
 * MessageRequestScheduler.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on February 20, 2010, 6:11 PM
 */

#ifndef INCL_MESSAGE_REQUEST_SCHEDULER_H
#define	INCL_MESSAGE_REQUEST_SCHEDULER_H

#include "DArray.h"
#include "FTypes.h"
#include "SetUniquePtrLList.h"
#include "DisServiceMsg.h"

namespace IHMC_ACI
{
    class MessageHeader;

    class MessageRequestScheduler
    {
        public:
            /**
             * bDeleteDuplicateRequest set on true means that the inserted
             * instances of DisServiceDataReqMsg::FragmentRequest will be
             * deleted in case of conflict (in case another instance equals
             * to the newly inserted one, already exists).
             */
            MessageRequestScheduler (float fDefaultReqProb, bool bDeleteDuplicateRequest=false);
            virtual ~MessageRequestScheduler (void);

            /**
             * Add a DisServiceDataReqMsg::FragmentRequest object to a priority-
             * ordered queue.  To more detail about the ordering of this queue
             * look at MessageRequestScheduler::FragReqWrapper::operator >()
             * implementation.
             *
             * NOTE: for efficiency reasons, addRequest does not make a copy of
             * pFragReq, thus the object must not be deleted.
             * If any of the elements in MessageRequestScheduler needs to be
             * deleted then MessageRequestScheduler object should be reset by
             * calling the reset() method first.
             *
             * NOTE: it is assumed that a message always has the same value
             * of priority. If it is not true, MessageRequestScheduler may
             * contain several entries for the same message but with different
             * values of priority. To avoid it, reset MessageRequestScheduler
             * when a change of priority happens.
             */
            void addRequest (DisServiceDataReqMsg::FragmentRequest *pFragReq, bool bSeq, bool bRel);
            DisServiceDataReqMsg::FragmentRequest * getRequest (void);

            bool isEmpty (void);

            /**
             * Remove and deallocates all the inserted elements.
             *
             * NOTE: check DisServiceDataReqMsg::FragmentRequest's destructor.
             */
            void reset (void);

            void resetGetRequest (void);

            /**
             * Set a mapping between the priority tag and the probability with
             * which fragments/messages market with such a tag will be requested
             */
            void setRequestProbability (uint8 ui8Priority, float lReqProb);
            float getRequestProbability (uint8 ui8Priority) const;

        private:
            friend class MessageReassembler;

            typedef DisServiceDataReqMsg::FragmentRequest FragmentRequest;
            enum FRAG_WRAPPER_TYPE {MSG_FW, CHK_FW};

            struct FragReqWrapper {
                /*
                 * NOTE: ~FragReqWrapper may call delete on the wrapped pFRequests
                 * depending on the value of bDelFragReq.
                 */
                virtual ~FragReqWrapper (void);

                const char * getGroupName (void) const;
                const char * getSenderNodeId (void) const;
                uint32 getMsgSeqId (void) const;
                virtual uint8 getPriority (void) const = 0;
                FragmentRequest * reliquishFragmentRequest (void);
                bool operator > (const FragReqWrapper &rhsReq) const;
                bool operator == (const FragReqWrapper &rhsReq) const;
                bool operator < (const FragReqWrapper &rhsReq) const;

                const bool bSequenced;
                const bool bReliable;
                const bool bDelFragReq;
                const uint8 ui8Priority;
                const FRAG_WRAPPER_TYPE type;
                FragmentRequest *pFRequests;

                protected:
                    /*
                     * - bSeq: the node subscribed the group of the message and this
                     *   subscription is sequenced
                     * - bRel: the node subscribed the group of the message and this
                     *   subscription is reliable
                     * - bDeleteFragmentRequest: if it is set on true, the wrapped
                     *   pFRequests will be deleted
                     */
                    FragReqWrapper (FragmentRequest *pFragReq, bool bSeq, bool bRel, bool bDeleteFragmentRequest,
                                    uint8 ui8Priority, const FRAG_WRAPPER_TYPE type);
            };

            struct MessageFragReqWrapper : public FragReqWrapper {
                MessageFragReqWrapper (FragmentRequest *pFragReq, bool bSeq, bool bRel, bool bDeleteFragmentRequest);
                virtual ~MessageFragReqWrapper (void);
                uint8 getPriority (void) const;
            };

            struct ChunkFragReqWrapper : public FragReqWrapper {
                ChunkFragReqWrapper (FragmentRequest *pFragReq, bool bSeq, bool bRel, bool bDeleteFragmentRequest);
                virtual ~ChunkFragReqWrapper (void);
                uint8 getPriority (void) const;
            };

            NOMADSUtil::SetUniquePtrLList<FragReqWrapper> * getRequests (void);
            void removeRequest (FragReqWrapper *pFragReq);

        private:
            FragReqWrapper *_pCurrReq;
            uint8 _ui8MaxNAttempts;
            uint8 _ui8NAttempts;
            bool _bAtLeastOnePick;
            bool _bDeleteDuplicateRequest;
            float _fDefaultReqProb;
            NOMADSUtil::DArray<float> _priorityToProb;
            NOMADSUtil::SetUniquePtrLList<FragReqWrapper> _requests;
    };

    inline NOMADSUtil::SetUniquePtrLList<MessageRequestScheduler::FragReqWrapper> * MessageRequestScheduler::getRequests (void)
    {
        return &_requests;
    }
}

#endif	// INCL_MESSAGE_REQUEST_SCHEDULER_H
