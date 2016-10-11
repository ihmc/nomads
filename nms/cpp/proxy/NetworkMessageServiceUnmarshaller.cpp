/* 
 * NetworkMessageServiceUnmarshaller.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on February 26, 2015, 9:51 PM
 */

#include "NetworkMessageServiceUnmarshaller.h"

#include "NetworkMessageService.h"
#include "NetworkMessageServiceProxy.h"
#include "SerializationUtil.h"

#include "CommHelper2.h"
#include "DArray.h"
#include "Writer.h"

using namespace NOMADSUtil;

namespace NOMADSUtil
{
    const int MAX_INT32 = 0x7FFFFFFF;
    const int MIN_INT32 = -0x7FFFFFFF;

    bool inInt32Range (int i)
    {
        return ((i > MIN_INT32) && (i <MAX_INT32));
    }

    int doCastMessage (const char *pszMethodName, NetworkMessageService *pNMS, SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        if ((pNMS == NULL) || (pCommHelper == NULL)) {
            return -1;
        }
        Reader *pReader = pCommHelper->getReaderRef();
        if (pReader == NULL) {
            return -2;
        }

        uint8 ui8MsgType;
        if (pReader->readUI8 (&ui8MsgType) < 0) {
            error = SimpleCommHelper2::CommError;
            return -3;
        }

        char **ppszOutgoingInterfaces = readStringArray (pReader, error);
        if (error != SimpleCommHelper2::None) {
            return -4;
        }

        uint32 ui32DstAddr;
        if (pReader->readUI32 (&ui32DstAddr) < 0) {
            error = SimpleCommHelper2::CommError;
            return -6;
        }

        uint16 ui16MsgId;
        if (pReader->readUI16 (&ui16MsgId) < 0) {
            error = SimpleCommHelper2::CommError;
            return -7;
        }

        uint8 ui8HopCount;
        if (pReader->readUI8 (&ui8HopCount) < 0) {
            error = SimpleCommHelper2::CommError;
            return -8;
        }

        uint8 ui8TTL;
        if (pReader->readUI8 (&ui8TTL) < 0) {
            error = SimpleCommHelper2::CommError;
            return -9;
        }

        uint16 ui16DelayTolerance;
        if (pReader->readUI16 (&ui16DelayTolerance) < 0) {
            error = SimpleCommHelper2::CommError;
            return -10;
        }

        uint16 ui16MsgMetaDataLen;
        if (pReader->readUI16 (&ui16MsgMetaDataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -11;
        }
        void *pMsgMetaData = calloc (ui16MsgMetaDataLen, 1);
        if (pMsgMetaData == NULL) {
            return -12;
        }
        if (pReader->readBytes (pMsgMetaData, ui16MsgMetaDataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -13;
        }

        uint16 ui16MsgLen;
        if (pReader->readUI16 (&ui16MsgLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -14;
        }
        void *pMsg = calloc (ui16MsgLen, 1);
        if (pMsg == NULL) {
            return -15;
        }
        if (pReader->readBytes (pMsg, ui16MsgLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return -16;
        }

        bool bExpedited = false;
        if (NetworkMessageServiceUnmarshaller::BROADCAST_METHOD == pszMethodName) {
            if (pReader->readBool (&bExpedited) < 0) {
                error = SimpleCommHelper2::CommError;
                return -17;
            }
        }

        char *pszHints = NULL;
        if (pReader->readString (&pszHints) < 0) {
            error = SimpleCommHelper2::CommError;
            return -18;
        }

        int rc = 0;
        int iSvcRc = 0;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (NetworkMessageServiceUnmarshaller::BROADCAST_METHOD == pszMethodName) {
            iSvcRc = pNMS->broadcastMessage (ui8MsgType, (const char **) ppszOutgoingInterfaces,
                                             ui32DstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                                             ui16DelayTolerance, pMsgMetaData, ui16MsgMetaDataLen,
                                            pMsg, ui16MsgLen, bExpedited, pszHints);
        }
        else if (NetworkMessageServiceUnmarshaller::TRANSMIT_METHOD == pszMethodName) {
            iSvcRc = pNMS->transmitMessage (ui8MsgType, (const char **) ppszOutgoingInterfaces,
                                            ui32DstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                                            ui16DelayTolerance, pMsgMetaData, ui16MsgMetaDataLen,
                                            pMsg, ui16MsgLen, pszHints);
        }
        else if (NetworkMessageServiceUnmarshaller::TRANSMIT_RELIABLE_METHOD == pszMethodName) {
            iSvcRc = pNMS->transmitReliableMessage (ui8MsgType, (const char **) ppszOutgoingInterfaces,
                                                    ui32DstAddr, ui16MsgId, ui8HopCount, ui8TTL,
                                                    ui16DelayTolerance, pMsgMetaData, ui16MsgMetaDataLen,
                                                    pMsg, ui16MsgLen, pszHints);
        }
        else {
            iSvcRc = -99;
            rc = -19;
        }

        if (ppszOutgoingInterfaces != NULL) {
            for (unsigned int i = 0; ppszOutgoingInterfaces[i] != NULL; i++) {
                free (ppszOutgoingInterfaces[i]);
            }
            free (ppszOutgoingInterfaces);
        }

        if (rc == 0) {
            rc = iSvcRc;
            assert (inInt32Range (iSvcRc));
            int32 i32 = (int32) iSvcRc;
            if (pWriter->write32 (&i32) < 0) {
                error = SimpleCommHelper2::CommError;
                return -20;
            }
        }

        return rc;
    }

    bool doBroadcast (uint16 ui16ClientId, NetworkMessageService *pNMS, SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        return (0 == doCastMessage (NetworkMessageServiceUnmarshaller::BROADCAST_METHOD, pNMS, pCommHelper, error));
    }

    bool doTransmit (uint16 ui16ClientId, NetworkMessageService *pNMS, SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        return (0 == doCastMessage (NetworkMessageServiceUnmarshaller::TRANSMIT_METHOD, pNMS, pCommHelper, error));
    }

    bool doTransmitReliable (uint16 ui16ClientId, NetworkMessageService *pNMS, SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        return (0 == doCastMessage (NetworkMessageServiceUnmarshaller::TRANSMIT_RELIABLE_METHOD, pNMS, pCommHelper, error));
    }

    bool doSetRetransmissionTimeout (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                     SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        if ((pNMS == NULL) || (pCommHelper == NULL)) {
            return false;
        }
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        uint32 ui32RetransmisisonTimeout;
        if (pReader->readUI32 (&ui32RetransmisisonTimeout) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        int rc = pNMS->setRetransmissionTimeout (ui32RetransmisisonTimeout);
        assert (inInt32Range (rc));
        int32 i32 = (int32) rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doSetPrimaryInterface (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        char *pszPrimaryIface = NULL;
        if (pReader->readString (&pszPrimaryIface) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszPrimaryIface != NULL) {
                free (pszPrimaryIface);
            }
            return false;
        }
        int rc = pNMS->setPrimaryInterface (pszPrimaryIface);
        assert (inInt32Range (rc));
        int32 i32 = (int32) rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doStart (uint16 ui16ClientId, NetworkMessageService *pNMS,
                  SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return false;
        }
        int rc = pNMS->start();
        assert (inInt32Range (rc));
        int32 i32 = (int32) rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doStop (uint16 ui16ClientId, NetworkMessageService *pNMS,
                 SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return false;
        }
        int rc = pNMS->stop();
        assert (inInt32Range (rc));
        int32 i32 = (int32) rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetMinMTU (uint16 ui16ClientId, NetworkMessageService *pNMS,
                      SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return false;
        }
        uint16 ui16MinMTU = pNMS->getMinMTU();
        if (pWriter->write16 (&ui16MinMTU) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetActiveNICsInfoAsStringForDestination (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                                    SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        char *pszGetDstAddr = NULL;
        Reader *pReader = pCommHelper->getReaderRef();
        if (pReader->readString (&pszGetDstAddr) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        char **ppszAddresses = NULL;
        if (pszGetDstAddr == NULL) {
            ppszAddresses = pNMS->getActiveNICsInfoAsString();
        }
        else {
            ppszAddresses = pNMS->getActiveNICsInfoAsStringForDestinationAddr (pszGetDstAddr);
            free (pszGetDstAddr);
            pszGetDstAddr = NULL;
        }

        if (writeStringArray (pCommHelper->getWriterRef(), (const char **) ppszAddresses, error) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        if (ppszAddresses != NULL) {
            for (unsigned int i = 0; ppszAddresses[i] != NULL; i++) {
                free (ppszAddresses[i]);
            }
            free (ppszAddresses);
        }
        return true;
    }

    bool goGetPropagationMode (uint16 ui16ClientId, NetworkMessageService *pNMS,
                               SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return false;
        }
        PROPAGATION_MODE mode = pNMS->getPropagationMode();
        assert (mode < 0xFF);
        uint8 ui8PropagationMode = (uint8) mode;
        if (pWriter->write8 (&ui8PropagationMode) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetDeliveryQueueSize (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                 SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Writer *pWriter = pCommHelper->getWriterRef();
        if (pWriter == NULL) {
            return false;
        }
        uint32 ui32DeliveryQueueSize = pNMS->getDeliveryQueueSize();
        if (pWriter->write32 (&ui32DeliveryQueueSize) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetReceiveRate (uint16 ui16ClientId, NetworkMessageService *pNMS,
                           SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszAddr = NULL;
        if (pReader->readString (&pszAddr) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszAddr != NULL) {
                free (pszAddr);
            }
            return false;
        }

        int64 i64 = pNMS->getReceiveRate (pszAddr);
        free (pszAddr); pszAddr = NULL;
        if (pWriter->write64 (&i64) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetTransmissionQueueSize (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                     SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszOutgoingInterface = NULL;
        if (pReader->readString (&pszOutgoingInterface) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszOutgoingInterface != NULL) {
                free (pszOutgoingInterface);
            }
            return false;
        }

        uint32 ui32TrQueueSize = pNMS->getTransmissionQueueSize (pszOutgoingInterface);
        free (pszOutgoingInterface); pszOutgoingInterface = NULL;
        if (pWriter->writeUI32 (&ui32TrQueueSize) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetRescaledTransmissionQueueSize (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                             SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszAddr = NULL;
        if (pReader->readString (&pszAddr) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszAddr != NULL) {
                free (pszAddr);
            }
            return false;
        }

        uint8 ui8RescTrQueueSize = pNMS->getRescaledTransmissionQueueSize (pszAddr);
        free (pszAddr); pszAddr = NULL;
        if (pWriter->writeUI8 (&ui8RescTrQueueSize) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetTransmissionQueueMaxSize (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                        SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszOutgoingInterface = NULL;
        if (pReader->readString (&pszOutgoingInterface) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszOutgoingInterface != NULL) {
                free (pszOutgoingInterface);
            }
            return false;
        }

        uint32 ui32TrQueueMaxSize = pNMS->getTransmissionQueueMaxSize (pszOutgoingInterface);
        free (pszOutgoingInterface); pszOutgoingInterface = NULL;
        if (pWriter->writeUI32 (&ui32TrQueueMaxSize) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetTransmitRateLimit (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                 SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszOutgoingInterface = NULL;
        if (pReader->readString (&pszOutgoingInterface) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszOutgoingInterface != NULL) {
                free (pszOutgoingInterface);
            }
            return false;
        }

        uint32 ui3TrRateLimit = pNMS->getTransmitRateLimit (pszOutgoingInterface);
        free (pszOutgoingInterface); pszOutgoingInterface = NULL;
        if (pWriter->writeUI32 (&ui3TrRateLimit) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doSetTransmissionQueueMaxSize (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                        SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }

        char *pszOutgoingInterface = NULL;
        uint32 ui32MaxSize = 0;
        if ((pReader->readString (&pszOutgoingInterface) < 0) || (pReader->readUI32 (&ui32MaxSize) < 0)) {
            error = SimpleCommHelper2::CommError;
            if (pszOutgoingInterface != NULL) {
                free (pszOutgoingInterface);
            }
            return false;
        }
        if (ui32MaxSize == 0) {
            error = SimpleCommHelper2::ProtocolError;
            if (pszOutgoingInterface != NULL) {
                free (pszOutgoingInterface);
            }
            return false;
        }

        int rc = pNMS->setTransmissionQueueMaxSize (pszOutgoingInterface, ui32MaxSize);
        assert (inInt32Range (rc));
        int32 i32 = rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doSetTransmitRateLimit (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                 SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        char *pszInterface = NULL;
        char *pszDestinationAddress = NULL;
        uint32 ui32RateLimit = 0;
        if ((pReader->readString (&pszInterface) < 0) || (pReader->readString (&pszDestinationAddress) < 0) || (pReader->readUI32 (&ui32RateLimit) < 0)) {
            error = SimpleCommHelper2::CommError;
            if (pszInterface != NULL) {
                free (pszInterface);
            }
            if (pszDestinationAddress != NULL) {
                free (pszDestinationAddress);
            }
            return false;
        }

        if (ui32RateLimit == 0) {
            if (pszInterface != NULL) {
                free (pszInterface);
            }
            if (pszDestinationAddress != NULL) {
                free (pszDestinationAddress);
            }
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }

        int rc = 0;
        if (pszInterface != NULL) {
            if (pszDestinationAddress != NULL) {
                rc = pNMS->setTransmitRateLimit (pszInterface, pszDestinationAddress, ui32RateLimit);
            }
            else {
                if (pszInterface != NULL) {
                    free (pszInterface);
                }
                if (pszDestinationAddress != NULL) {
                    free (pszDestinationAddress);
                }
                error = SimpleCommHelper2::ProtocolError;
                return false;
            }
        }
        else if (pszDestinationAddress != NULL) {
            rc = pNMS->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
        }
        else {
            rc = pNMS->setTransmitRateLimit (ui32RateLimit);
        }

        assert (inInt32Range (rc));
        int32 i32 = (int32) rc;
        if (pWriter->write32 (&i32) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doGetLinkCapacity (uint16 ui16ClientId, NetworkMessageService *pNMS,
                            SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        char *pszInterface = NULL;
        if ((pReader->readString (&pszInterface) < 0)) {
            error = SimpleCommHelper2::CommError;
            if (pszInterface != NULL) {
                free (pszInterface);
            }
            return false;
        }
        uint32 ui32Capacity = pNMS->getLinkCapacity (pszInterface);
        free (pszInterface);
        pszInterface = NULL;
        if (pWriter->writeUI32 (&ui32Capacity) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doSetLinkCapacity (uint16 ui16ClientId, NetworkMessageService *pNMS,
                            SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        uint32 ui32Capacity = 0;
        char *pszInterface = NULL;
        if ((pReader->readString (&pszInterface) < 0) || (pReader->readUI32 (&ui32Capacity) < 0)) {
            error = SimpleCommHelper2::CommError;
            if (pszInterface != NULL) {
                free (pszInterface);
            }
            return false;
        }
        pNMS->setLinkCapacity (pszInterface, ui32Capacity);
        free (pszInterface);
        pszInterface = NULL;
        return true;
    }

    bool doGetNeighborQueueLength (uint16 ui16ClientId, NetworkMessageService *pNMS,
                                   SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        uint32 ui32SenderRemoteAddr = 0;
        char *pchIncomingInterface = NULL;
        if ((pReader->readString (&pchIncomingInterface) < 0) || (pReader->readUI32 (&ui32SenderRemoteAddr) < 0)) {
            error = SimpleCommHelper2::CommError;
            if (pchIncomingInterface != NULL) {
                free (pchIncomingInterface);
            }
            return false;
        }
        uint8 ui8QueueLen = pNMS->getNeighborQueueLength (pchIncomingInterface, ui32SenderRemoteAddr);
        free (pchIncomingInterface);
        pchIncomingInterface = NULL;
        if (pWriter->writeUI8 (&ui8QueueLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doClearToSend (uint16 ui16ClientId, NetworkMessageService *pNMS,
                        SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        Writer *pWriter = pCommHelper->getWriterRef();
        if ((pReader == NULL) || (pWriter == NULL)) {
            return false;
        }
        char *pszInterface = NULL;
        if (pReader->readString (&pszInterface) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszInterface != NULL) {
                free (pszInterface);
            }
            return false;
        }
        const bool bClearToSend = pszInterface == NULL ? pNMS->clearToSendOnAllInterfaces() : pNMS->clearToSend (pszInterface);
        free (pszInterface);
        pszInterface = NULL;
        if (pWriter->writeBool (&bClearToSend) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }
        return true;
    }

    bool doPing (uint16 ui16ClientId, NetworkMessageService *pNMS, SimpleCommHelper2 *pCommHelper, SimpleCommHelper2::Error &error)
    {
        pCommHelper->sendLine (error, "pong");
        return true;
    }

    //--------------------------------------------------------------------------
    // Callbacks
    //--------------------------------------------------------------------------

    bool doMessageArrived (uint16 ui16ClientId, NetworkMessageServiceProxy *pNMSProxy, SimpleCommHelper2 *pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();
        char *psztmp = NULL;
        if (pReader->readString (&psztmp) < 0) {
            error = SimpleCommHelper2::CommError;
            if (psztmp != NULL) {
                free (psztmp);
            }
            return false;
        }
        String incomingIface = psztmp;

        uint32 ui32SourceIPAddress = 0U;
        if (pReader->readUI32 (&ui32SourceIPAddress) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        uint8 ui8MsgType = 0U;
        if (pReader->readUI8 (&ui8MsgType) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        uint16 ui16MsgId = 0U;
        if (pReader->readUI16 (&ui16MsgId) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        uint8 ui8HopCount = 0U;
        if (pReader->readUI8 (&ui8HopCount) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        uint8 ui8TTL = 0U;
        if (pReader->readUI8 (&ui8TTL) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        uint16 ui16MsgMetaDataLen = 0U;
        if (pReader->readUI16 (&ui16MsgMetaDataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            return false;
        }

        void *pMetadata = NULL;
        if (ui16MsgMetaDataLen > 0) {
            pMetadata = calloc (ui16MsgMetaDataLen, 1);
            if (pReader->readBytes (pMetadata, ui16MsgMetaDataLen) < 0) {
                error = SimpleCommHelper2::CommError;
                if (pMetadata != NULL) {
                    free (pMetadata);
                }
                return false;
            }
        }

        uint16 ui16MsgLen = 0U;
        if (pReader->readUI16 (&ui16MsgLen) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pMetadata != NULL) {
                free (pMetadata);
            }
            return false;
        }

        void *pMsg = NULL;
        if (ui16MsgLen > 0) {
            pMsg = calloc (ui16MsgLen, 1);
            if (pReader->readBytes (pMsg, ui16MsgLen) < 0) {
                error = SimpleCommHelper2::CommError;
                if (pMetadata != NULL) {
                    free (pMetadata);
                }
                if (pMsg != NULL) {
                    free (pMsg);
                }
                return false;
            }
        }

        int64 i64Timestamp = 0U;
        if (pReader->read64 (&i64Timestamp) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pMetadata != NULL) {
                free (pMetadata);
            }
            if (pMsg != NULL) {
                free (pMsg);
            }
            return false;
        }

        pNMSProxy->messageArrived (incomingIface, ui32SourceIPAddress, ui8MsgType, ui16MsgId,
                                   ui8HopCount, ui8TTL, pMetadata, ui16MsgMetaDataLen,
                                   pMsg, ui16MsgLen, i64Timestamp);
        if (pMetadata != NULL) {
            free (pMetadata);
        }
        if (pMsg != NULL) {
            free (pMsg);
        }
        return (error == SimpleCommHelper2::None);
    }

    bool doPong (uint16 ui16ClientId, NetworkMessageServiceProxy *pNMSProxy, SimpleCommHelper2 *pCommHelper)
    {
        pNMSProxy->pong();
        return false;
    }
}

const String NetworkMessageServiceUnmarshaller::SERVICE = "nms";
const String NetworkMessageServiceUnmarshaller::VERSION = "20150619";

const String NetworkMessageServiceUnmarshaller::BROADCAST_METHOD = "broadcast";
const String NetworkMessageServiceUnmarshaller::TRANSMIT_METHOD = "transmit";
const String NetworkMessageServiceUnmarshaller::TRANSMIT_RELIABLE_METHOD = "transmit-reliable";
const String NetworkMessageServiceUnmarshaller::SET_TRANSMISSION_TIMEOUT_METHOD = "setRetransmissionTimeout";
const String NetworkMessageServiceUnmarshaller::SET_PRIMARY_INTERFACE_METHOD = "setPrimaryInterface";
const String NetworkMessageServiceUnmarshaller::START_METHOD = "start";
const String NetworkMessageServiceUnmarshaller::STOP_METHOD = "stop";
const String NetworkMessageServiceUnmarshaller::GET_MIN_MTU_METHOD = "getMinMTU";
const String NetworkMessageServiceUnmarshaller::GET_ACTIVE_NIC_AS_STRING_METHOD = "getActiveNICsForDstAddr";
const String NetworkMessageServiceUnmarshaller::GET_PROPAGATION_MODE_METHOD = "getPropagationMode";
const String NetworkMessageServiceUnmarshaller::GET_DELIVERY_QUEUE_SIZE_METHOD = "getDeliveryQueueSize";
const String NetworkMessageServiceUnmarshaller::GET_RECEIVE_RATE_METHOD = "getReceiveRate";
const String NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_QUEUE_SIZE_METHOD = "getTransmissionQueueSize";
const String NetworkMessageServiceUnmarshaller::GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "getRescTransmissionQueueSize";
const String NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "getTransmissionQueueMaxSize";
const String NetworkMessageServiceUnmarshaller::GET_TRANSMISSION_RATE_LIMIT_METHOD = "getTransmitRateLimit";
const String NetworkMessageServiceUnmarshaller::SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD = "setTransmissionQueueMaxSize";
const String NetworkMessageServiceUnmarshaller::SET_TRANSMIT_RATE_LIMIT_METHOD = "setTransmitRateLimit";
const String NetworkMessageServiceUnmarshaller::GET_LINK_CAPACITY_METHOD = "getLinkCapacity";
const String NetworkMessageServiceUnmarshaller::SET_LINK_CAPACITY_METHOD = "setLinkCapacity";
const String NetworkMessageServiceUnmarshaller::GET_NEIGHBOR_QUEUE_LENGTH_METHOD = "getNeighborQueueLength";
const String NetworkMessageServiceUnmarshaller::CLEAR_TO_SEND_METHOD = "clearToSend";
const String NetworkMessageServiceUnmarshaller::PING_METHOD = "ping";

const String NetworkMessageServiceUnmarshaller::REGISTER_LISTENER_METHOD = "registerListener";

const String NetworkMessageServiceUnmarshaller::MESSAGE_ARRIVED = "messageArrived";
const String NetworkMessageServiceUnmarshaller::PONG_ARRIVED = "pong";

bool NetworkMessageServiceUnmarshaller::methodInvoked (uint16 ui16ClientId, const String &methodName,
                                                       void *pNMS, SimpleCommHelper2 *pCommHelper,
                                                       SimpleCommHelper2::Error &error)
{
    if (BROADCAST_METHOD == methodName) {
        return doBroadcast (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (TRANSMIT_METHOD == methodName) {
        return doTransmit (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (TRANSMIT_RELIABLE_METHOD == methodName) {
        return doTransmitReliable (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (SET_TRANSMISSION_TIMEOUT_METHOD == methodName) {
        return doSetRetransmissionTimeout (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (SET_PRIMARY_INTERFACE_METHOD == methodName) {
        return doSetPrimaryInterface (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (START_METHOD == methodName) {
        return doStart (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (STOP_METHOD == methodName) {
        return doStop (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_MIN_MTU_METHOD == methodName) {
        return doGetMinMTU (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_ACTIVE_NIC_AS_STRING_METHOD == methodName) {
        return doGetActiveNICsInfoAsStringForDestination (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_PROPAGATION_MODE_METHOD == methodName) {
        return goGetPropagationMode (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_DELIVERY_QUEUE_SIZE_METHOD == methodName) {
        return doGetDeliveryQueueSize (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_RECEIVE_RATE_METHOD == methodName) {
        return doGetReceiveRate (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_TRANSMISSION_QUEUE_SIZE_METHOD == methodName) {
        return doGetTransmissionQueueSize (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_RESCALED_TRANSMISSION_QUEUE_MAX_SIZE_METHOD == methodName) {
        return doGetRescaledTransmissionQueueSize (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD == methodName) {
        return doGetTransmissionQueueMaxSize (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_TRANSMISSION_RATE_LIMIT_METHOD == methodName) {
        return doGetTransmitRateLimit (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (SET_TRANSMISSION_QUEUE_MAX_SIZE_METHOD == methodName) {
        return doSetTransmissionQueueMaxSize (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (SET_TRANSMIT_RATE_LIMIT_METHOD == methodName) {
        return doSetTransmitRateLimit (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_LINK_CAPACITY_METHOD == methodName) {
        return doGetLinkCapacity (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (SET_LINK_CAPACITY_METHOD == methodName) {
        return doSetLinkCapacity (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (GET_NEIGHBOR_QUEUE_LENGTH_METHOD == methodName) {
        return doGetNeighborQueueLength (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (CLEAR_TO_SEND_METHOD == methodName) {
        return doClearToSend (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else if (PING_METHOD == methodName) {
        return doPing (ui16ClientId, (NetworkMessageService *) pNMS, pCommHelper, error);
    }
    else {
        return false;
    }
    return false;
}

bool NetworkMessageServiceUnmarshaller::methodArrived (uint16 ui16ClientId, const String &methodName,
                                                       void *pNMSProxy, SimpleCommHelper2 *pCommHelper)
{
    if (MESSAGE_ARRIVED == methodName) {
        return doMessageArrived (ui16ClientId, (NetworkMessageServiceProxy *) pNMSProxy, pCommHelper);
    }
    else if (PONG_ARRIVED == methodName) {
        return true;
    }
    else {
        return false;
    }
}

