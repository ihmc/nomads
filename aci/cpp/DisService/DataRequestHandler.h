/*
 * DataRequestHandler.h
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
 * Created on: Mar 10, 2011
 * Author: Andrea Rossi      (arossi@ihmc.us)
 *         Giacomo Benincasa (gbenincasa@ihmc.us)
 */

#ifndef INCL_DATA_REQUEST_HANDLER_H
#define INCL_DATA_REQUEST_HANDLER_H

#include "DisServiceMsg.h"
#include "FTypes.h"
#include "LoggingMutex.h"
#include "LList.h"
#include "MessageId.h"

#include "ManageableThread.h"
#include "StringHashset.h"
#include "TimeIntervalAverage.h"

namespace IHMC_ACI
{
    class DisServiceDataReqMsg;
    class Message;
    class TransmissionService;
    class DisseminationService;

    class DataRequestHandler
    {
        public:
            virtual ~DataRequestHandler (void);

            static DataRequestHandler * getInstance (DisseminationService *pDisService,
                                                     TransmissionService *pTrSvc, int64 i64SleepTime,
                                                     int64 i64BaseTimeMs, int64 iMissingFragReqTimeout,
                                                     uint16 u16OffsetRange, float fReceiveRateThreshold,
                                                     bool bHandleRequests);

            /**
             * the DRH will take care of MFR. if it's configured to handle them,
             * it'll add the MFR to the list of received requests, to be served
             * later else, it'll pass it to DisseminationService without waiting
             */
            virtual void dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                                    const char *pszIncomingInterface) = 0;

            static int64 getRandomSleepTime (int64 i64SleepTime);
            static void registerWithListeners (DisseminationService *pDisService, DataRequestHandler *pDRH);
            static void halt (DataRequestHandler *pDRH);

        public:
            static const int64 DEFAULT_SLEEP_TIME;
            static const int64 DEFAULT_BASE_TIME;
            static const uint16 DEFAULT_OFFSET_RANGE;
            static const float DEFAULT_RECEIVE_RATE_THRESHOLD;

        protected:
            enum Type
            {
                SYNC,
                ASYNC
            };
            explicit DataRequestHandler (Type type, DisseminationService *pDisService);
            void handleDataRequestMessage (DisServiceDataReqMsg *pDSDRMsg, const char *pszIncomingInterface, int64 i64Timeout);
            void handleDataRequestMessage (const char *pszMsgId, DisServiceMsg::Range *pRange, bool bIsChunk, const char *pszTarget,
                                           unsigned int uiNumberOfActiveNeighbors, int64 i64RequestArrivalTime,
                                           const char **ppszOutgoingInterfaces, int64 i64Timeout);

        protected:
            const Type _type;
            int64 _i64DiscardTime;

            NOMADSUtil::LoggingMutex _mx;
            DisseminationService *_pDisService;
    };

    class AsynchronousDataRequestHandler: public DataRequestHandler, public NOMADSUtil::ManageableThread
    {
        public:
            AsynchronousDataRequestHandler (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                            int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                            float fReceiveRateThreshold);
            virtual ~AsynchronousDataRequestHandler (void);

            /**
             * The DRH will take care of MFR. if it's configured to handle them,
             * it'll add the MFR to the list of received requests, to be served
             * later else, it'll pass it to DisseminationService without waiting
             */
            void dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                            const char *pszIncomingInterface);

            static void registerWithListeners (DisseminationService *pDisService, DataRequestHandler *pDRH);

            void run (void);

        protected:
            // Data structure used to keep track of the received MFR and when
            // to serve them
            struct DataReqMsgWrapper
            {
                DataReqMsgWrapper (DisServiceDataReqMsg *pMessage, const char *pszIncomingInterface,
                                   int64 i64MessageServingTime);
                virtual ~DataReqMsgWrapper (void);

                DisServiceDataReqMsg *_pDataReqMsg; // the MissingFragmentRequest
                int64 _i64ServingTime;              // the instant in time after
                                                    // which the request can be served
                char *_pszIncomingInterface;         // the interface where it arrived
            };

            /*
             * This method returns one request for which the servingTime has
             * passed, so it's time to serve it right now it returns the first it
             * can find, but some kind of selection or priority can be added in
             * the future if there are more requests read, the method still
             * returns just one, so it should be called multiple times.
             * the search is done only on requests arrived on the specified pszInterfaces
             * requests made on other interfaces will be ignored.
             * if there are no requests ready, it returns NULL.
             */
            DataReqMsgWrapper * getRequestToServe (char **pszInterfaces);

            /*
             * This method returns an instant in time. when that instant has been
             * passed, the request can be given to disservice to be served
             * the time returned is an absolute value, not relative
             *
             * question: is it better if we add a relative time, like a countdown?
             *           no risk of overflow, but we have to update the counters
             *           (because they're all different) for every element @every
             *           cycle then.
             *           Perhaps a solution would be to put the time in module,
             *           so we set the timers with an absolute value capped at
             *           5 second (for example) and we keep track of the time
             *           with a counter. When the counter reaches 5 seconds, we
             *           set it back to 0 and update all the timers of the hash
             *           table. This way we update everything only 5 seconds
             *           instead of every time.
             */
            int64 calculateServingTime (void);

        protected:
            TransmissionService *_pTrSvc;
            int64 _i64SleepTime;
            int64 _i64BaseTimeMs;
            uint16 _u16OffsetRange;
            float _fReceiveRateThreshold;

            NOMADSUtil::LList<DataReqMsgWrapper*> _llreceivedDataReqMessages;
    };

    class AsynchronousDataRequestHandlerV2: public AsynchronousDataRequestHandler
    {
        public:
            AsynchronousDataRequestHandlerV2 (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                              int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                              float fReceiveRateThreshold);
            ~AsynchronousDataRequestHandlerV2 (void);

            /**
             * The DRH will take care of MFR. if it's configured to handle them,
             * it'll add the MFR to the list of received requests, to be served
             * later else, it'll pass it to DisseminationService without waiting
             */
            void dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                            const char *pszIncomingInterface);

            void display (FILE *pOutputFile);

            void run (void);

        private:
            /*
             * Return true if available interfaces were found, false otherwise
             */
            bool serveRequest (void);

        private:
            struct QueuedRequest
            {
                QueuedRequest (const char *pszMsgId, DisServiceMsg::Range &range, bool bIsChunk,
                               uint16 ui16MinimumNumberOfActiveNeighbors, const char *pszRequestingNodeId);
                ~QueuedRequest (void);

                bool operator > (const QueuedRequest &rhsQueuedReq) const;
                bool operator < (const QueuedRequest &rhsQueuedReq) const;
                int operator == (const QueuedRequest &rhsQueuedReq) const;
                QueuedRequest & operator += (QueuedRequest &rhsQueuedReq);

                const bool _bIsChunk;
                const int64 _i64ArrivalOfFirstRequestTimestamp;
                int64 _i64ArrivalOfLatestRequestTimestamp;
                const MessageId _messageId;
                NOMADSUtil::UInt32RangeDLList _requestedFragments;
                NOMADSUtil::StringHashset _requestingPeers;
                TimeIntervalAverage<uint16> _avgMinimumNumberOfActiveNeighbors;
            };

            struct RequestsByMessageIdList
            {
                RequestsByMessageIdList (void);
                ~RequestsByMessageIdList (void);

                NOMADSUtil::PtrLList<QueuedRequest> * getRequests (void);
                bool insertMerge (QueuedRequest *pReq);
                QueuedRequest * pop (uint32 ui32AgeThreshold);

                private:
                    mutable NOMADSUtil::Mutex _m;
                    NOMADSUtil::PtrLList<QueuedRequest> _queuedRequests;
            };

            struct RequestByInterface
            {
                RequestByInterface (const char *pszIncomingInterface);
                ~RequestByInterface (void);

                int operator == (const RequestByInterface &rhsReqsByIface) const;

                const NOMADSUtil::String _incomingInterface;
                RequestsByMessageIdList _reqByMsgId;
                mutable NOMADSUtil::Mutex _m;
            };

            struct RequestByInterfaceList
            {
                RequestByInterfaceList (void);
                ~RequestByInterfaceList (void);

                RequestByInterface * getFirst (void);
                RequestByInterface * getNext (void);

                RequestByInterface * insertUnique (RequestByInterface *pByIface);

                private:
                    mutable NOMADSUtil::Mutex _m;
                    NOMADSUtil::PtrLList<RequestByInterface> _requestsByIncomingInterface;
            };

        private:
            RequestByInterfaceList _requestsByIncomingInterface;
    };

    class AsynchronousDataRequestHandlerV4 : public AsynchronousDataRequestHandler
    {
        public:
            AsynchronousDataRequestHandlerV4 (DisseminationService *pDisService, TransmissionService *pTrSvc,
                                              int64 i64SleepTime, int64 i64BaseTimeMs, uint16 u16OffsetRange,
                                              float fReceiveRateThreshold);
            ~AsynchronousDataRequestHandlerV4 (void);

            void dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                            const char *pszIncomingInterface);

            void display (FILE *pOutputFile);

            void run (void);

        private:
            struct Ranges
            {
                explicit Ranges (bool bOriginatedLocally);
                ~Ranges (void);

                void reset (void);

                const bool _bOriginatedLocally;
                const int64 _i64FirstRequest;
                int64 _i64LatestRequest;
                NOMADSUtil::UInt32RangeDLList _ranges;
                NOMADSUtil::StringHashset _requestingPeers;
                TimeIntervalAverage<uint16> _avgMinimumNumberOfActiveNeighbors;
            };
            bool _bIsServingRequests;
            NOMADSUtil::Mutex _m;
            const NOMADSUtil::String _nodeId;
            typedef NOMADSUtil::StringHashtable<Ranges> RangesByMsgId;
            NOMADSUtil::StringHashtable<RangesByMsgId> *_pIncompleteMsgsByIface;
    };

    class SynchronousDataRequestHandler : public DataRequestHandler
    {
        public:
            SynchronousDataRequestHandler (DisseminationService *pDisService);
            virtual ~SynchronousDataRequestHandler (void);
            void dataRequestMessageArrived (DisServiceDataReqMsg *pDSDRMsg,
                                            const char *pszIncomingInterface);
    };

    //==========================================================================
    //  STRUCT DataReqMsgWrapper inline functions
    //==========================================================================
    inline AsynchronousDataRequestHandler::DataReqMsgWrapper::~DataReqMsgWrapper()
    {
    }

    //==========================================================================
    //  STRUCT RequestByInterface inline functions
    //==========================================================================

    inline int AsynchronousDataRequestHandlerV2::RequestByInterface::operator == (const AsynchronousDataRequestHandlerV2::RequestByInterface &rhsReqsByIface) const
    {
        return (_incomingInterface == rhsReqsByIface._incomingInterface);
    }

    //==========================================================================
    //  STRUCT RequestByMessageId inline functions
    //==========================================================================

    inline NOMADSUtil::PtrLList<AsynchronousDataRequestHandlerV2::QueuedRequest> * AsynchronousDataRequestHandlerV2::RequestsByMessageIdList::getRequests (void)
    {
        _m.lock();
        NOMADSUtil::PtrLList<AsynchronousDataRequestHandlerV2::QueuedRequest> *pReqs = new NOMADSUtil::PtrLList<AsynchronousDataRequestHandlerV2::QueuedRequest> (_queuedRequests);
        _m.unlock();
        return pReqs;
    }

    inline bool AsynchronousDataRequestHandlerV2::RequestsByMessageIdList::insertMerge (AsynchronousDataRequestHandlerV2::QueuedRequest *pReq)
    {
        if (pReq == NULL) {
            return false;
        }
        _m.lock();
        QueuedRequest *pOld = _queuedRequests.search (pReq);
        if (pOld == NULL) {
            _queuedRequests.insert (pReq);
            _m.unlock();
            return false;
        }
        else {
            *pOld += *pReq;
            _m.unlock();
            return true;
        }
    }
}

#endif // INCL_DATA_REQUEST_HANDLER_H

