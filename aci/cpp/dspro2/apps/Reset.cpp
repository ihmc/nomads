#include "Reset.h"

#include "Defs.h"
#include "MetaData.h"
#include "MetadataInterface.h"
#include "MetadataHelper.h"

#include "SessionId.h"

#include "Base64Transcoders.h"
#include "Json.h"
#include "Logger.h"
#include "StrClass.h"

#include <memory>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

Reset::Reset (void)
    : _i64Timestamp (0)
{
}

Reset::~Reset (void)
{
}

bool Reset::pathRegistered (IHMC_VOI::NodePath *pNodePath, const char *pszNodeId,
    const char *pszTeam, const char *pszMission)
{
    return true;
}

bool Reset::positionUpdated (float fLatitude, float fLongitude, float fAltitude, const char *pszNodeId)
{
    return true;
}

void Reset::newPeer (const char *pszPeerNodeId)
{
}

void Reset::deadPeer (const char *pszDeadPeer)
{
}

int Reset::dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                        const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                        const void *pBuf, uint32 ui32Len, uint8 ui8ChunkIndex, uint8 ui8TotNChunks,
                        const char *pszCallbackParameter)
{
    return 0;
}

int Reset::metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                            const char *pszReferredDataInstanceId, const char *pszJsonMetadada,
                            const char *pszReferredDataId, const char *pszQueryId)
{
    if (pszJsonMetadada == nullptr) {
        return -1;
    }
    const char *pszMethodName = "Reset::metadataArrived";
    MetaData metadata;
    if (metadata.fromString (pszJsonMetadada) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse metadata: %s.\n", pszJsonMetadada);
        return -2;
    }

    String mimeType;
    metadata.getFieldValue (MetaData::DATA_FORMAT, mimeType);

    if (mimeType ^= SessionId::MIME_TYPE) {
        String payload;
        metadata.getFieldValue (MetaData::APPLICATION_METADATA, payload);
        unsigned int len = payload.length() < 0 ? 0U : payload.length();
        if (len == 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "message does not contain session key.\n");
        }
        else {
            void *pDecodedJson = Base64Transcoders::decode (payload, &len);
            if ((pDecodedJson == nullptr) || (len == 0U)) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "base64 decoding error.\n");
            }
            else {
                String sDecodedJson ((const char *)pDecodedJson, len);
                free (pDecodedJson);
                JsonObject json (sDecodedJson);
                handle (json);
            }
        }
    }

    return 0;
}

int Reset::dataAvailable (const char *pszId, const char *pszGroupName,
                          const char *pszReferredDataObjectId, const char *pszReferredDataInstanceId,
                          const char *pszReferredDataId, const char *pszMimeType,
                          const void *pMetadata, uint32 ui32MetadataLength,
                          const char *pszQueryId)
{
    return 0;
}

void Reset::handle (NOMADSUtil::JsonObject &json)
{
    String step;
    json.getString ("step", step);
    int64 i64Timestamp;
    json.getNumber ("timestamp", i64Timestamp);

    if (i64Timestamp < _i64Timestamp) {
        // Drop obsolete packet
        return;
    }

    if (step ^= "stop_sending") {

    }
    else if (step ^= "reset") {

    }
    else if (step ^= "change_session_key") {
        String sessionKey;
        json.getString ("sessionKey", sessionKey);
        SessionId::getInstance()->setSessionId (sessionKey, i64Timestamp);
    }
    else if (step ^= "start") {

    }

    std::lock_guard<std::mutex> lock (_m);
    _i64Timestamp = i64Timestamp;
}

