/*
 * TransmissionService.h
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
 * Created on January 27, 2009, 9:04 PM
 */

#ifndef INCL_TRANSMISSION_SERVICE_H
#define INCL_TRANSMISSION_SERVICE_H

#include "NMSHelper.h"

#include "DisServiceMsg.h"
#include "RateEstimator.h"

#include "BufferWriter.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class ConfigManager;
    class NetworkMessageServiceInterface;
    class NICInfo;
}

namespace IHMC_ACI
{
    class DisServiceMsg;
    class MessageHeader;
    class TransmissionServiceListener;

    class TransmissionServiceLogger
    {
        public:
            TransmissionServiceLogger (void);
            virtual ~TransmissionServiceLogger (void);

            void init (const char *pszFileName);
            void log (DisServiceMsg::Type msgType, uint32 ui32MsgLen, const char *pszPurpose);
            bool enabled (void);

        private:
            FILE *_filePacketXMitLog;
    };

    class TransmissionService
    {
        public:
            enum TRANSMISSION_SVC_MODE
            {
                MULTICAST = 0x00,
                BROADCAST = 0x01,
                NORM = 0x02
            };

            struct TransmissionResults
            {
                public:
                    ~TransmissionResults (void);
                    void reset (void);

                public:
                    int rc;
                    char **ppszOutgoingInterfaces;

                private:
                    TransmissionResults (void);

                private:
                    friend class TransmissionService;
                    friend class RateEstimatingTransmissionService;
                    bool bDeallocateOutgoingInterfaces;
            };

            static const char * DEFAULT_DISSERVICE_MCAST_GROUP;
            static const uint16  DEFAULT_DISSERVICE_MCAST_PORT;

            static const char * DEFAULT_DISSERVICE_BCAST_ADDR;
            static const uint16 DEFAULT_DISSERVICE_BCAST_PORT;

            static const uint8 DEFAULT_DISSERVICE_TTL;

            static const bool DEFAULT_ASYNCHRONOUS_DELIVERY;
            static const bool DEFAULT_ASYNCHRONOUS_TRANSMISSION;

            static const uint32 DEFAULT_TRANSMIT_RATE_LIMIT;

            static const uint8 DEFAULT_MESSAGE_VERSION;

            virtual ~TransmissionService (void);

            static TransmissionService * getInstance (NOMADSUtil::ConfigManager *pCfgMgr, const char *pszNodeId, const char *pszSessionId);

            virtual int init (NOMADSUtil::ConfigManager *pCfgMgr,
                              const char **ppszBindingInterfaces = NULL,
                              const char **ppszIgnoredInterfaces = NULL,
                              const char **ppszAddedInterfaces = NULL);
            bool isInitialized (void);

            int registerWithListeners (DisseminationService *pDisService);

            /**
             * Broadcast a message using the specified interfaces. If no outgoing
             * interface is specified, all the available interfaces will be used.
             * Messages sent in broadcast are not fragmented by the Transmission
             * Service, therefore they must fit the MTU.
             *
             * NOTE: this method is not thread-safe.
             * NOTE: the method returns a class field that must not be deallocated,
             *       although it must be reset using the provided reset() method.
             */
            virtual TransmissionResults broadcast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                                   const char *pszPurpose, const char *pszTargetAddr, const char *pszHints);

            /**
             * Unicast a message using the specified interfaces. If no outgoing
             * interface is specified, all the available interfaces will be used.
             * Messages sent in unicast are fragmented by the Transmission Service.
             *
             * NOTE: this method is not thread-safe.
             * NOTE: the method returns a class field that must not be deallocated,
             *       although it must be reset using the provided reset() method.
             */
            TransmissionResults unicast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                         const char *pszPurpose, const char *pszTargetAddr, const char *pszHints,
                                         bool bReliable);

            /**
             * Start the underlying NetworkMessageService (which in-turn starts
             * the underlying NetworkInterfaces) in order to start receiving
             * incoming messages
             */
            int start (void);

            /**
             * Requests that any threads started by this class be terminated
             * Blocks until threads are terminated
             */
            void stop (void);

            void requestTermination (void);

            /**
             * Returns the NICInfo/Network interface IP Address of all the
             * network interfaces being used by the TransmissionService
             *
             * NOTE: the NICInfo objects must NOT be modified and must NOT be
             *       deallocated by the caller.
             *       the array returned by getActiveInterfaces and its elements
             *       must be deallocated by the caller.
             */
            char ** getActiveInterfacesAddress (void);

            bool getAsyncTransmission (void);

            /**
             * Returns true if the node communicates its outgoing queue size
             * with other nodes
             */
            bool getSharesQueueLength (void);

            /**
             * Returns the size of the delivery queue if the asynchronous
             * delivery is enabled, or 0 otherwise.
             */
            virtual uint32 getIncomingQueueSize (void);

            /**
             * Returns the network interfaces, among the ones being used by the
             * TransmissionService, that can reach the destination address
             *
             * NOTE: the pointer to the array and it elements should be
             *       deallocated, by the caller
             */
            char ** getInterfacesByDestinationAddress (const char *pszDestinationAddresses);
            char ** getInterfacesByDestinationAddress (uint32 ui32DestinationAddress);

            /**
             * Returns the network interfaces among the ones specified in ppNICs,
             * with receiving rate less than dPercRateThreshold (it's a percentage,
             * the methods accept threshold values between 0 and 1).
             *
             * If ppNICs is NULL all the available interfaces will be considered
             *
             * NOTE: the returned array must be deallocated by the caller
             */
            char ** getInterfacesByReceiveRate (float fPercRateThreshold);
            char ** getSilentInterfaces (uint32 ui32TimeThreshold);

            /**
             * Returns the network interfaces among the ones specified in ppNICs,
             * with queue length less than fPercLengthThreshold (it's a percentage,
             * the methods accept threshold values between 0 and 1).
             *
             * If ppNICs is NULL all the available interfaces will be considered
             *
             * NOTE: the returned array must be deallocated by the caller.
             */
            char ** getInterfacesByOutgoingQueueLength (float fPercLengthThreshold, char **ppszInterfaces=NULL);

            char ** getInterfacesByReceiveRateAndOutgoingQueueLenght (float fPercRateThreshold, float fPercLengthThreshold);

            int64 getLatestBroadcastTimeForInterface (const char *pszIface);

            uint16 getMTU (void);

            uint32 getReceiveRate (const char *pszAddr);

            uint32 getTransmitRateLimit (const char *pszAddr);

            uint32 getTransmitRateLimitCap (void);
            void setTransmitRateLimitCap (uint32 ui32Cap);

            /*
             * Get the capacity of the link used by the specified interface
             */
            uint32 getLinkCapacity (const char *pszInterface);

            /*
             * Set the capacity of the link used by the specified interface
             */
            void setLinkCapacity (const char *pszInterface, uint32 ui32Capacity);

            uint16 getMaxFragmentSize (void);

            void setMaxFragmentSize (uint16 ui16MaxFragmentSize);

            /**
             * Returns the size of the transmission queue, if asynchronous
             * transmission is enabled, or 0 otherwise
             */
            uint32 getTransmissionQueueSize (const char *pszInterface);

            /**
             * Returns the size of the transmission queue, rescaled to fit in
             * the interval [0, 255]
             */
            uint8 getRescaledTransmissionQueueSize (const char *pszInterface);

            /**
             * Returns the max size of the transmission queue
             */
            uint32 getTransmissionQueueMaxSize (const char *pszInterface);

            /**
             * Return the queue size of the specified neighbour
             */
            uint8 getNeighborQueueSize (const char *pszInterface, uint32 ui32Address);

            /**
             * Checks whether it is clear to send on the specified interface
             */
            bool clearToSend (const char *pszInterface);

            /**
             * Checks whether it is clear to send on all outgoing interfaces
             */
            bool clearToSendOnAllInterfaces (void);

            bool loggingEnabled (void);

            int registerHandlerCallback (uint8 ui8MsgType, TransmissionServiceListener *pListener);

            /**
             * Set the transmit rate limit for this socket
             * The rate limit is specified in bytes per second
             * A value of 0 turns off the transmit rate limit
             * This is a pass-through to the underlying socket - which may or may
             * not enforce the transmit rate limit.
             *
             * Returns 0 if successful or a negative value in case of error
             */
            int setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit);
            int setTransmitRateLimit (uint32 ui32RateLimit);

        protected:
            TransmissionService (TRANSMISSION_SVC_MODE mode, uint16 ui16NetworkMessageServicePort,
                                 const char* pszMcastGroup, uint8 ui8McastTTL,
                                 const char *pszNodeId, const char *pszSessionId,
                                 bool bAsyncDelivery = DEFAULT_ASYNCHRONOUS_DELIVERY,
                                 bool bAsyncTransmission = DEFAULT_ASYNCHRONOUS_TRANSMISSION,
                                 uint8 ui8MessageVersion = DEFAULT_MESSAGE_VERSION);

        private:
            char ** getInterfacesByReceiveRate (float fPercRateThreshold, char **ppszInterfaces);
            char ** getInterfacesByOutgoingQueueLengthInternal (float fPercLengthThreshold, const char **ppszInterfaces);
            bool checkAndPackMsg (DisServiceMsg *pDSMsg, NOMADSUtil::BufferWriter *pWriter, uint32 ui32MaxMsgSize);
            void updateLatestBcastTimeByIface (DisServiceMsg *pDSMsg, const char *pszOutgoingInterface);

        private:
            const TRANSMISSION_SVC_MODE _mode;
            const bool _bAsyncDelivery;
            const bool _bAsyncTransmission;
            uint8 _ui8McastTTL;
            uint8 _ui8MessageVersion;
            const uint16 _ui16Port;
            uint16 _ui16MaxFragmentSize;
            uint32 _ui32RateLimitCap;

            NOMADSUtil::NetworkMessageServiceInterface *_pMPS;
            NOMADSUtil::Mutex _m;
            const NOMADSUtil::String _dstAddr;
            NOMADSUtil::String _primaryIface;
            const NOMADSUtil::String _nodeId;
            const NOMADSUtil::String _sessionId;
            TransmissionServiceLogger _logger;
            NMSHelper _nmsHelper;
            NOMADSUtil::StringHashtable<int64> _latestBcastTimeByIface;
    };

    class RateEstimatingTransmissionService : public TransmissionService
    {
        public:
            ~RateEstimatingTransmissionService (void);

            TransmissionResults broadcast (DisServiceMsg *pDSMsg, const char **ppszOutgoingInterfaces,
                                           const char *pszPurpose, const char *pszTargetAddr, const char *pszHints);

            int init (NOMADSUtil::ConfigManager *pCfgMgr,
                      const char **ppszBindingInterfaces = NULL,
                      const char **ppszIgnoredInterfaces = NULL,
                      const char **ppszAddedInterfaces = NULL);

            int registerWithListeners (DisseminationService *pDisService);

        protected:
            friend class TransmissionService;
            RateEstimatingTransmissionService (TRANSMISSION_SVC_MODE mode, uint16 ui16NetworkMessageServicePort,
                                               const char* pszMcastGroup, uint8 ui8McastTTL,
                                               const char *pszNodeId, const char *pszSessionId,
                                               uint8 ui8RateEstimatorUpdateFactor, uint8 ui8RateEstimatorDecreaseFactor,
                                               uint32 ui32RateEstimatorStartingCapacity,
                                               bool bAsyncDelivery = DEFAULT_ASYNCHRONOUS_DELIVERY,
                                               bool bAsyncTransmission = DEFAULT_ASYNCHRONOUS_TRANSMISSION,
                                               uint8 ui8MessageVersion = DEFAULT_MESSAGE_VERSION);
        private:
            RateEstimator _rateEstimator;
    };

    class TransmissionServiceHelper
    {
        public:
            TransmissionServiceHelper (bool bUseRateEstimator);
            virtual ~TransmissionServiceHelper (void);

            uint32 computeMessageHeaderSize (const char *pszNodeId,
                                             const char *pszTargetNodeId,
                                             const char *pszSessionId,
                                             MessageHeader *pMH);

        private:
            const bool _bUseRateEstimator;
    };

    class TransmissionServiceConfReader
    {
        public:
            TransmissionServiceConfReader (NOMADSUtil::ConfigManager *pCfgMgr);
            virtual ~TransmissionServiceConfReader (void);

            TransmissionService::TRANSMISSION_SVC_MODE getMode (void);
            const char * getBcastAddr (void);
            const char * getMcastGroup (void);
            uint16 getPort (void);
            uint8 getMcastTTL (void);
            bool getAsyncDelivery (void);
            bool getAsyncTransmission (void);
            uint8 getNetworkMessageVersion (void);

        private:
            NOMADSUtil::ConfigManager *_pCfgMgr;
    };

    inline bool TransmissionServiceLogger::enabled (void)
    {
        return _filePacketXMitLog != NULL;
    }

    inline int64 TransmissionService::getLatestBroadcastTimeForInterface (const char *pszIface)
    {
        int64 *pI64Time = _latestBcastTimeByIface.get (pszIface);
        return (pI64Time == NULL ? 0U : *pI64Time);
    }

    inline bool TransmissionService::loggingEnabled (void)
    {
        return _logger.enabled();
    }
}

#endif  // INCL_TRANSMISSION_SERVICE_H
