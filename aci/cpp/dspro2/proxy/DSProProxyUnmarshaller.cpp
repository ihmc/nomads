/**
 * DSProProxyUnmarshaller.cpp
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
 */

#include "DSProProxyUnmarshaller.h"
#include "DSProProxy.h"
#include "NodePath.h"

#include <DArray2.h>

using namespace IHMC_ACI;
using namespace IHMC_VOI;
using namespace NOMADSUtil;

const String DSProProxyUnmarshaller::SERVICE = "dspro";
const String DSProProxyUnmarshaller::VERSION = "20170206";

const String DSProProxyUnmarshaller::PING_METHOD = "ping";
const String DSProProxyUnmarshaller::REGISTER_PATH = "registerPath";
const String DSProProxyUnmarshaller::ADD_AREA_OF_INTEREST = "addAreaOfInterest";
const String DSProProxyUnmarshaller::ADD_CUSTUMS_POLICIES_AS_XML = "addCustumPoliciesAsXML";
const String DSProProxyUnmarshaller::ADD_MESSAGE = "addMessage";
const String DSProProxyUnmarshaller::ADD_MESSAGE_AV_LIST = "addMessage_AVList";
const String DSProProxyUnmarshaller::ADD_CHUNK = "addChunk";
const String DSProProxyUnmarshaller::ADD_CHUNK_AV_LIST = "addChunk_AVList";
const String DSProProxyUnmarshaller::ADD_ADDITIONAL_CHUNK = "addAdditionalChunk";
const String DSProProxyUnmarshaller::ADD_CHUNKED_MESSAGE = "addChunkedMessage";
const String DSProProxyUnmarshaller::ADD_CHUNKED_MESSAGE_AV_LIST = "addChunkedMessage_AVList";
const String DSProProxyUnmarshaller::ADD_USER_ID = "addUserId";
const String DSProProxyUnmarshaller::CANCEL = "cancel_str";
const String DSProProxyUnmarshaller::CANCEL_BY_OBJECT_INSTANCE_ID = "cancel_strs";
const String DSProProxyUnmarshaller::CHANGE_ENCRYPTION_KEY = "changeEncryptionKey";
const String DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE = "chunkAndAddMessage";
const String DSProProxyUnmarshaller::CHUNK_AND_ADD_MESSAGE_AV_LIST = "chunkAndAddMessage_AVList";
const String DSProProxyUnmarshaller::DISSEMINATE = "disseminateMessage";
const String DSProProxyUnmarshaller::DISSEMINATE_METADATA = "disseminateMessageMetadata";
const String DSProProxyUnmarshaller::GET_DATA = "getData";
const String DSProProxyUnmarshaller::GET_NODE_CONTEXT = "getNodeContext";
const String DSProProxyUnmarshaller::GET_NODE_ID = "getNodeId";
const String DSProProxyUnmarshaller::GET_PEER_MSG_COUNTS = "getPeerMsgCounts";
const String DSProProxyUnmarshaller::GET_SESSION_ID = "getSessionId";
const String DSProProxyUnmarshaller::RESET_TRANSMISSION_COUNTERS = "resetTransmissionCounters";
const String DSProProxyUnmarshaller::SEARCH = "search";
const String DSProProxyUnmarshaller::SEARCH_REPLY = "searchReply";
const String DSProProxyUnmarshaller::SET_CURRENT_PATH = "setCurrentPath";
const String DSProProxyUnmarshaller::SET_MISSION_ID = "setMissionId";
const String DSProProxyUnmarshaller::SET_DEFAULT_RANGE_OF_INFLUENCE = "setDefRangeOfInfluence";
const String DSProProxyUnmarshaller::SET_SELECTIVITY = "setSelectivity";
const String DSProProxyUnmarshaller::SET_RANGE_OF_INFLUENCE = "setRangeOfInfluence";
const String DSProProxyUnmarshaller::SET_ROLE = "setRole";
const String DSProProxyUnmarshaller::SET_TEAM_ID = "setTeamId";
const String DSProProxyUnmarshaller::SET_NODE_TYPE = "setNodeType";
const String DSProProxyUnmarshaller::SET_CURRENT_POSITION = "setCurrentPosition";
const String DSProProxyUnmarshaller::SET_DEFAULT_USEFUL_DISTANCE = "setDefUsefulDistance";
const String DSProProxyUnmarshaller::SET_USEFUL_DISTANCE = "setUsefulDistance";
const String DSProProxyUnmarshaller::SET_RANKING_WEIGHTS = "setRankingWeights";
const String DSProProxyUnmarshaller::SUBSCRIBE = "subscribe";

// Listener Registration
const String DSProProxyUnmarshaller::REGISTER_DSPRO_LISTENER_METHOD = "registerPathRegisteredCallback";
const String DSProProxyUnmarshaller::REGISTER_MATCHMAKING_LISTENER_METHOD = "registerMatchmakingLogCallback";
const String DSProProxyUnmarshaller::REGISTER_SEARCH_LISTENER_METHOD = "registerSearchCallback";

// Chunking Plugin Registration
const String DSProProxyUnmarshaller::REGISTER_CHUNK_FRAGMENTER = "registerChunkFragmenter";
const String DSProProxyUnmarshaller::REGISTER_CHUNK_REASSEMBLER = "registerChunkReassembler";

// Callbacks
const String DSProProxyUnmarshaller::METADATA_ARRIVED = "metadataArrivedCallback";
const String DSProProxyUnmarshaller::DATA_ARRIVED = "dataArrivedCallback";
const String DSProProxyUnmarshaller::DATA_AVAILABLE = "dataAvailableCallback";
const String DSProProxyUnmarshaller::SEARCH_ARRIVED = "searchArrivedCallback";
const String DSProProxyUnmarshaller::SEARCH_REPLY_ARRIVED = "searchReplyArrivedCallback";
const String DSProProxyUnmarshaller::VOLATILE_SEARCH_REPLY_ARRIVED = "volatileSearchReplyArrivedCallback";

namespace IHMC_ACI_DSPRO_PROXY_UNMARSHALLER
{
    //-------------------------------------------------------------------------
    // Commands
    //-------------------------------------------------------------------------

    bool doPing (uint16 ui16ClientId, DSPro * pDSPro, SimpleCommHelper2 * pCommHelper, SimpleCommHelper2::Error & error)
    {
        return false;
    }

    bool doMethodInvoked (uint16 ui16ClientId, const String & methodName, DSPro * pDSPro,
                          SimpleCommHelper2 * pCommHelper, SimpleCommHelper2::Error & error)
    {
        if (DSProProxyUnmarshaller::PING_METHOD == methodName) {
            return doPing (ui16ClientId, pDSPro, pCommHelper, error);
        }
        return false;
    }

    //-------------------------------------------------------------------------
    // Callbacks
    //-------------------------------------------------------------------------

    void readStrings (Reader * pReader, char ** ppszString, unsigned short usNString, SimpleCommHelper2::Error & error)
    {
        unsigned short i = 0;
        for (; i < usNString; i++) {
            char *pszId = nullptr;
            if (pReader->readString (&pszId) < 0) {
                error = SimpleCommHelper2::CommError;
                if (pszId != nullptr) {
                    free (pszId);
                    pszId = nullptr;
                }
                break;
            }
            ppszString[i] = pszId;
        }
        for (; i < usNString; i++) {
            ppszString[i] = nullptr;
        }
    }

    void deallocatedStrings (char ** ppszString, unsigned short usNString)
    {
        if (ppszString == nullptr) {
            return;
        }
        for (unsigned short i = 0; i < usNString; i++) {
            if (ppszString[i] != nullptr) {
                free (ppszString[i]);
            }
        }
    }

    bool doMetadatadArrived (uint16 ui16ClientId, DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();

        const unsigned short usNString = 7;
        char * strings[usNString];
        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }

        pDSProProxy->metadataArrived (strings[0], strings[1], strings[2], strings[3], strings[4], strings[5], strings[6]);

        deallocatedStrings (strings, usNString);

        return (error == SimpleCommHelper2::None);
    }

    bool doDatadArrived (uint16 ui16ClientId, DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();

        const unsigned short usNString = 6;
        char* strings[usNString];
        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }

        uint32 ui32DataLen = 0U;
        if (pReader->read32 (&ui32DataLen) < 0) {
            error = SimpleCommHelper2::CommError;
            deallocatedStrings (strings, usNString);
            return false;
        }
        void *pDataBuf = ui32DataLen == 0U ? nullptr : malloc (ui32DataLen);
        if (pDataBuf != nullptr) {
            pCommHelper->receiveBlob (pDataBuf, ui32DataLen, error);
            if (error != SimpleCommHelper2::None) {
                deallocatedStrings (strings, usNString);
                free (pDataBuf);
            }
        }

        uint8 ui8NChunks = 0;
        if (pReader->read8 (&ui8NChunks) < 0) {
            error = SimpleCommHelper2::CommError;
            deallocatedStrings (strings, usNString);
            if (pDataBuf != nullptr) free (pDataBuf);
            return false;
        }

        uint8 ui8TotNChunks = 0;
        if (pReader->read8 (&ui8TotNChunks) < 0) {
            error = SimpleCommHelper2::CommError;
            deallocatedStrings (strings, usNString);
            if (pDataBuf != nullptr) free (pDataBuf);
            return false;
        }

        char *pszCallbackParameter = nullptr;
        if (pReader->readString (&pszCallbackParameter) < 0) {
            error = SimpleCommHelper2::CommError;
            if (pszCallbackParameter != nullptr) free (pszCallbackParameter);
            deallocatedStrings (strings, usNString);
            if (pDataBuf != nullptr) free (pDataBuf);
            return false;
        }

        pDSProProxy->dataArrived (strings[0], strings[1], strings[2], strings[3], strings[4], strings[5],
                                  pDataBuf, ui32DataLen, ui8NChunks, ui8TotNChunks, pszCallbackParameter);

        if (pszCallbackParameter != nullptr) free (pszCallbackParameter);
        deallocatedStrings (strings, usNString);
        if (pDataBuf != nullptr) free (pDataBuf);

        return (error == SimpleCommHelper2::None);
    }

    bool doSearchArrived (uint16 ui16ClientId, DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();

        const unsigned short usNString = 5;
        char* strings[usNString];
        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }

        uint32 ui32QueryLen = 0U;
        if (pReader->read32 (&ui32QueryLen) < 0) {
            error = SimpleCommHelper2::CommError;
            deallocatedStrings (strings, usNString);
            return false;
        }
        void *pQuery = ui32QueryLen == 0U ? nullptr : malloc (ui32QueryLen);
        if (pQuery != nullptr) {
            pCommHelper->receiveBlock (pQuery, ui32QueryLen, error);
            if (error != SimpleCommHelper2::None) {
                deallocatedStrings (strings, usNString);
                free (pQuery);
            }
        }

        pDSProProxy->searchArrived (strings[0], strings[1], strings[2], strings[3],
                                    strings[4], pQuery, ui32QueryLen);

        deallocatedStrings (strings, usNString);
        if (pQuery != nullptr) free (pQuery);
        return (error == SimpleCommHelper2::None);
    }

    bool doSearchReplyArrived (uint16 ui16ClientId, DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef();

        const unsigned short usNString = 1;
        char* strings[usNString];
        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }
        const String queryId (strings[0]);
        free (strings[0]);
        strings[0] = nullptr;

        unsigned int i = 0;
        DArray2<String> ids;

        char buf[128];
        do {
            pCommHelper->receiveBlock (buf, 128, error);
            if (error != SimpleCommHelper2::None) {
                deallocatedStrings (strings, usNString);
                return false;
            }
            const String id (buf);
            if (id.length() > 0) {
                ids[i++] = id;
            }
            else {
                break;
            }
        } while (true);

        const char **ppszIds = static_cast<const char **>(calloc (ids.size () + 1, sizeof (char *)));
        if (ppszIds == nullptr) {
            return false;
        }
        for (unsigned int j = 0; j < ids.size (); j++) {
            ppszIds[j] = ids[j].c_str();
        }

        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }
        const String matchingNodeId (strings[0]);
        free (strings[0]);

        pDSProProxy->searchReplyArrived (queryId, ppszIds, matchingNodeId);
        return (error == SimpleCommHelper2::None);
    }

    bool doVolatileSearchReplyArrived (uint16 ui16ClientId, DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        SimpleCommHelper2::Error error = SimpleCommHelper2::None;
        Reader *pReader = pCommHelper->getReaderRef ();

        const unsigned short usNString = 1;
        char* strings[usNString];
        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }
        const String queryId (strings[0]);
        free (strings[0]);
        strings[0] = nullptr;

        char buf[1024];
        uint32 ui32BlockSize = pCommHelper->receiveBlock (buf, 1024, error);
        if (ui32BlockSize > 0xFFFF) {
            error = SimpleCommHelper2::ProtocolError;
            return false;
        }
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }

        readStrings (pReader, strings, usNString, error);
        if (error != SimpleCommHelper2::None) {
            deallocatedStrings (strings, usNString);
            return false;
        }
        const String matchingNodeId (strings[0]);
        free (strings[0]);

        pDSProProxy->volatileSearchReplyArrived (queryId, buf, (uint16)ui32BlockSize, matchingNodeId);
        return (error == SimpleCommHelper2::None);
    }

    bool doMethodArrived (uint16 ui16ClientId, const NOMADSUtil::String & methodName,
                          DSProProxy * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
    {
        if (pDSProProxy == nullptr) {
            checkAndLogMsg ("DSProProxyUnmarshaller", Logger::L_SevereError, " null DSProProxy.\n");
            return false;
        }
        if (DSProProxyUnmarshaller::METADATA_ARRIVED == methodName) {
            return doMetadatadArrived (ui16ClientId, pDSProProxy, pCommHelper);
        }
        if (DSProProxyUnmarshaller::DATA_ARRIVED == methodName) {
            return doSearchArrived (ui16ClientId, pDSProProxy, pCommHelper);
        }
        if (DSProProxyUnmarshaller::SEARCH_ARRIVED == methodName) {
            return doDatadArrived (ui16ClientId, pDSProProxy, pCommHelper);
        }
        if (DSProProxyUnmarshaller::SEARCH_REPLY_ARRIVED == methodName) {
            return doSearchReplyArrived (ui16ClientId, pDSProProxy, pCommHelper);
        }
        if (DSProProxyUnmarshaller::VOLATILE_SEARCH_REPLY_ARRIVED == methodName) {
            return doVolatileSearchReplyArrived (ui16ClientId, pDSProProxy, pCommHelper);
        }
        return false;
    }
}

using namespace IHMC_ACI_DSPRO_PROXY_UNMARSHALLER;

bool DSProProxyUnmarshaller::methodInvoked (uint16 ui16ClientId, const String & methodName, void * pDSPro,
                                            SimpleCommHelper2 * pCommHelper, SimpleCommHelper2::Error & error)
{
    return doMethodInvoked (ui16ClientId, methodName, static_cast<DSPro *>(pDSPro), pCommHelper, error);
}

bool DSProProxyUnmarshaller::methodArrived (uint16 ui16ClientId, const NOMADSUtil::String & methodName,
                                            Stub * pDSProProxy, NOMADSUtil::SimpleCommHelper2 * pCommHelper)
{
    return doMethodArrived (ui16ClientId, methodName, static_cast<DSProProxy *> (pDSProProxy), pCommHelper);
}

NodePath * DSProProxyUnmarshaller::readNodePath (Reader * pReader)
{
    if (pReader == nullptr) {
        return nullptr;
    }
    String pathId;
    uint16 ui16Len = 0;
    if (pReader->read16 (&ui16Len) < 0) {
        return nullptr;
    }
    if (ui16Len > 0) {
        char *pszString = (char *)calloc (ui16Len + 1, sizeof (char));
        if ((pszString == nullptr) || (pReader->readBytes (pszString, ui16Len) < 0)) {
            return nullptr;
        }
        pszString[ui16Len] = '\0';
        pathId = pszString;
        free (pszString);
    }
    float fPathProb;
    if (pReader->read32 (&fPathProb) < 0) {
        return nullptr;
    }
    int iPathType;
    if (pReader->read32 (&iPathType) < 0) {
        return nullptr;
    }
    int iPathLen;
    if (pReader->read32 (&iPathLen) < 0) {
        return nullptr;
    }

    NodePath *pNodePath = new NodePath (pathId, iPathType, fPathProb);
    if (pNodePath == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < iPathLen; i++) {

        float altitude, latitude, longitude;
        if (pReader->read32 (&altitude) < 0) {
            delete pNodePath;
            return nullptr;
        }
        if (pReader->read32 (&latitude) < 0) {
            delete pNodePath;
            return nullptr;
        }
        if (pReader->read32 (&longitude) < 0) {
            delete pNodePath;
            return nullptr;
        }

        String location;
        ui16Len = 0;
        if (pReader->read16 (&ui16Len) < 0) {
            delete pNodePath;
            return nullptr;
        }
        if (ui16Len > 0) {
            char *pszString = (char *)calloc (ui16Len + 1, sizeof (char));
            if ((pszString == nullptr) || (pReader->readBytes (pszString, ui16Len) < 0)) {
                delete pNodePath;
                return nullptr;
            }
            pszString[ui16Len] = '\0';
            location = pszString;
            free (pszString);
        }

        uint64 ui64Timestamp = 0;
        if (pReader->read64 (&ui64Timestamp) < 0) {
            delete pNodePath;
            return nullptr;
        }

        if (pNodePath->appendWayPoint (latitude, longitude, altitude,
            location, nullptr, ui64Timestamp) < 0) {
            delete pNodePath;
            return nullptr;
        }
    }
    return pNodePath;
}

int DSProProxyUnmarshaller::write (NodePath * pPath, NOMADSUtil::Writer * pWriter)
{
    if ((pWriter == nullptr) || (pPath == nullptr)) {
        return -1;
    }

    const String pathId (pPath->getPathId());
    uint16 ui16Len = (pathId.length () <= 0 ? 0 : pathId.length ());
    if (pWriter->write16 (&ui16Len) < 0) {
        return -4;
    }
    if ((ui16Len > 0) && (pWriter->writeBytes (pathId, ui16Len) < 0)) {
        return -5;
    }

    float fProb = pPath->getPathProbability();
    if (pWriter->write32 (&fProb) < 0) {
        return -6;
    }

    int iPathType = pPath->getPathType();
    if (pWriter->write32 (&iPathType) < 0) {
        return -7;
    }

    int iPathLen = pPath->getPathLength();
    if (iPathLen < 0) {
        return -8;
    }
    if (pWriter->write32 (&iPathLen) < 0) {
        return -9;
    }

    for (int i = 0; i < iPathLen; i++) {
        float fCoord = pPath->getAltitude (i);
        if (pWriter->write32 (&fCoord) < 0) {
            return -10;
        }
        fCoord = pPath->getLatitude (i);
        if (pWriter->write32 (&fCoord) < 0) {
            return -11;
        }
        fCoord = pPath->getLongitude (i);
        if (pWriter->write32 (&fCoord) < 0) {
            return -12;
        }
        String location (pPath->getLocation (i));
        ui16Len = (location.length () < 0 ? 0 : location.length ());
        if (pWriter->write16 (&ui16Len) < 0) {
            return -13;
        }
        if ((ui16Len > 0) && (pWriter->writeBytes (location, ui16Len) < 0)) {
            return -14;
        }
        uint64 ui64Timestamp = pPath->getTimeStamp (i);
        if (pWriter->write64 (&ui64Timestamp) < 0) {
            return -15;
        }
    }
    return 0;
}

