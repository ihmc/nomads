#include "AMTDictator.h"

#include "Base64Transcoders.h"
#include "BoundingBox.h"
#include "ConditionVariable.h"
#include "CustomPolicies.h"
#include "DSProImpl.h"
#include "FIFOQueue.h"
#include "Json.h"
#include "ManageableThread.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

#define mistAssignmentMsgParsingError checkAndLogMsg (pszMethodName, Logger::L_Warning, "parsing error.\n")

namespace AMT_DICTATOR
{
    static String PHOENIX_JSON_BASE64_MIME_TYPE = "Phoenix_Base64";
    static String SOIGEN2_JSON_BASE64_MIME_TYPE = "Soigen2_Json_Base64";
    static String PHOENIX_MIST_JSON_BASE64_MIME_TYPE = "x-dspro/x-phoenix-mist";
    static String JSON_MIME_TYPE = "application/json";

    static String MIST_MISSION_ASSIGNMENT = "mistMissionAssignment";
    static String MIST_USEFUL_DISTANCE_ASSIGNMENT = "mistUsefulDistanceAssignment";
    static String MIST_RANGE_OF_INFLUENCE_ASSIGNMENT = "mistRangeOfInfluenceAssignment";
    static String MIST_AREA_OF_INTEREST_ASSIGNMENT = "mistAreaOfInterestAssignment";
    static String MIST_CUSTUM_POLICY_ASSIGNMENT = "mistCustomPolicyAssignment";

    void missionAssignmentArrived (DSProImpl *pDSPro, JsonObject &json)
    {
        /*
        {
            "messageType": "mistMissionAssignment",
            "callsign" : "callsign567",
            "mission" : "Rescue101",
            "role" : "MEDEVAC",
            "defaultUsefulDistance" : 10
        }
        */

        String mission;
        if (json.getString ("mission", mission) == 0) {
            pDSPro->setMissionId (mission);
        }
        String role;
        if (json.getString ("role", role) == 0) {
            pDSPro->setRole (role);
        }
        double dUsefulDistance = 0.0f;
        if (json.getNumber ("defaultUsefulDistance", dUsefulDistance) == 0) {
            pDSPro->setDefaultUsefulDistance (dUsefulDistance);
        }
    }

    void usefulDistanceAssignmentArrived (DSProImpl *pDSPro, JsonObject &json)
    {
        const char *pszMethodName = "AMTDictator::usefulDistanceAssignmentArrived";
        /*
        {
          "usefulDistanceData": [
            {
              "usefulDistance": 654,
              "mimeType": "mime34"
            },
            {
              "usefulDistance": 654,
              "mimeType": "mime567"
            },
            {
              "usefulDistance": 654,
              "mimeType": "mime3"
            },
            {
              "usefulDistance": 654,
              "mimeType": "mime876"
            },
            {
              "usefulDistance": 654,
              "mimeType": "mime764"
            }
          ],
          "messageType": "mistUsefulDistanceAssignment",
          "callsign": "nodeID67980"
        }
        */

        const JsonArray *pArray = json.getArray ("usefulDistanceData");
        if (pArray == nullptr) {
            return;
        }
        for (int i = 0; i < pArray->getSize(); i++) {
            JsonObject *pAssignment = pArray->getObject (i);
            if (pAssignment == nullptr) {
                mistAssignmentMsgParsingError;
            }
            else {
                String mimeType;
                double dUsefulDistance = 0.0f;
                if ((pAssignment->getString ("mimeType", mimeType) == 0) && (pAssignment->getNumber ("usefulDistance", dUsefulDistance) == 0)) {
                    pDSPro->setUsefulDistance (mimeType, (uint32) dUsefulDistance);
                }
                else {
                    mistAssignmentMsgParsingError;
                }
                delete pAssignment;
            }
        }
        delete pArray;
    }

    void rangeOfInfluenceAssignmentArrived (DSProImpl *pDSPro, JsonObject &json)
    {
        const char *pszMethodName = "AMTDictator::rangeOfInfluenceAssignmentArrived";
        /*
        {
            "messageType": "mistRangeOfInfluenceAssignment",
            "rangeOfInfluenceData": [
                {
                "symbolId": "symbolId45",
                "rangeOfInfluence": 10
                },
                {
                "symbolId": "symbolId78",
                "rangeOfInfluence": 34
                },
                {
                "symbolId": "symbolId55",
                "rangeOfInfluence": 10
                }
            ],
            "callsign": "nodeID67980"
        }
        */
        const JsonArray *pArray = json.getArray ("rangeOfInfluenceData");
        if (pArray == nullptr) {
            return;
        }
        for (int i = 0; i < pArray->getSize(); i++) {
            JsonObject *pAssignment = pArray->getObject (i);
            if (pAssignment == nullptr) {
                mistAssignmentMsgParsingError;
            }
            else {
                String symbolId;
                double rangeOfInfluence = 0.0f;
                if ((pAssignment->getString ("symbolId", symbolId) == 0) && (pAssignment->getNumber ("rangeOfInfluence", rangeOfInfluence) == 0)) {
                    pDSPro->setRangeOfInfluence (symbolId, (uint32) rangeOfInfluence);
                }
                else {
                    mistAssignmentMsgParsingError;
                }
                delete pAssignment;
            }
        }
        delete pArray;
    }

    void areaOfInterestAssignmentArrived (DSProImpl *pDSPro, JsonObject &json)
    {
        //const char *pszMethodName = "AMTDictator::rangeOfInfluenceAssignmentArrived";
        /*
          "areaOfInterest": {
            "name": "areaName",
            "area": {
              "maxLat": 14.3,
              "minLat": 45.2,
              "maxLon": 14.3
              "minLon": 45.2
           },
           "time": {
             "start": 1234455,
             "end": 435775
           }
        }
        */
        JsonObject *pInterest = json.getObject ("areaOfInterest");
        if (pInterest == nullptr) {
            return;
        }
        String name;
        if (pInterest->getString ("name", name) < 0) {
            delete pInterest;
            return;
        }
        JsonObject *pArea = pInterest->getObject ("area");
        if (pArea == nullptr) {
            delete pInterest;
            return;
        }
        double maxLat, minLat, maxLon, minLon;
        if (pArea->hasObject ("maxLat") && pArea->hasObject ("minLat") && pArea->hasObject ("maxLon") && pArea->hasObject ("minLon")) {
            pArea->getNumber ("maxLat", maxLat);
            pArea->getNumber ("minLat", minLat);
            pArea->getNumber ("maxLon", maxLon);
            pArea->getNumber ("minLon", minLon);
            NOMADSUtil::BoundingBox bbox ((float) maxLat, (float) minLon, (float) minLat, (float) maxLon);
            if (!bbox.isValid()) {
                delete pInterest;
                delete pArea;
                return;
            }

            int64 i64Start = getTimeInMilliseconds();
            int64 i64End = 0x7FFFFFFFFFFFFFFF;
            JsonObject *pTime = pInterest->getObject ("time");
            if (pTime != nullptr) {
                pTime->getNumber ("start", i64Start);
                pTime->getNumber ("end", i64End);
                delete pTime;
            }

            pDSPro->addAreaOfInterest (name, bbox, i64Start, i64End);
        }
        delete pInterest;
        delete pArea;
    }

    void customPolicyAssignmentArrived (DSProImpl *pDSPro, JsonObject &json)
    {
        /*
    {
        "customPolicies": {
            "policies": [
            {
                "attribute": "Tag",
                    "type" : 1,
                    "weight" : 100,
                    "matches" : [
                {
                    "value": "bomb",
                        "rank" : 7
                },
                {
                    "value": "prisoner",
                    "rank" : 8
                },
                {
                    "value": "IED",
                    "rank" : 10
                }
                    ]
            },
            {
                "attribute": "Tag",
                "type" : 1,
                "weight" : 2,
                "matches" : [
                    {
                        "value": "vehicle",
                            "rank" : 7
                    },
                    {
                        "value": "humvee",
                        "rank" : 8
                    },
                    {
                        "value": "munitions",
                        "rank" : 9
                    }
                ]
            }
            ]
        }
    }
    */
        JsonObject *pPolicies = json.getObject ("customPolicies");
        if (pPolicies == nullptr) {
            return;
        }

        CustomPolicies policies;
        if (policies.fromJson (pPolicies) < 0) {
            return;
        }

        CustomPolicyImpl *pPolicy = policies.getFirst();
        for (; pPolicy != nullptr; pPolicy = policies.getNext()) {
            pDSPro->addCustomPolicy (pPolicy);
        }
    }

    void dictate (DSProImpl *pDSPro, JsonObject &json)
    {
        const char *pszMethodName = "AMTDictator::dictate";
        String msgType;
        if (json.getString ("messageType", msgType) != 0) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "can't read the MIME type.\n");
            return;
        }
        if (msgType ^= MIST_MISSION_ASSIGNMENT) {
            missionAssignmentArrived (pDSPro, json);
        }
        else if (msgType ^= MIST_USEFUL_DISTANCE_ASSIGNMENT) {
            usefulDistanceAssignmentArrived (pDSPro, json);
        }
        else if (msgType ^= MIST_RANGE_OF_INFLUENCE_ASSIGNMENT) {
            rangeOfInfluenceAssignmentArrived (pDSPro, json);
        }
        else if (msgType ^= MIST_AREA_OF_INTEREST_ASSIGNMENT) {
            areaOfInterestAssignmentArrived (pDSPro, json);
        }
        else if (msgType ^= MIST_CUSTUM_POLICY_ASSIGNMENT) {
            customPolicyAssignmentArrived (pDSPro, json);
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "can't parse message of type %s.\n", msgType.c_str());
        }
    }

    class AsyncDictator : public ManageableThread
    {
        public:
            AsyncDictator (DSProImpl *pDSPro);
            ~AsyncDictator (void);

            void enqueue (JsonObject *pJson);

            void run (void);

        private:
            Mutex _mDeliveryQueue;
            ConditionVariable _cvDeliveryQueue;
            OSThread _ostDeliveryThread;
            FIFOQueue _deliveryQueue;
            DSProImpl *_pDSPro;
    };
}

using namespace AMT_DICTATOR;

AMTDictator::AMTDictator (DSProImpl *pDSPro)
    : _pDictator (new AsyncDictator (pDSPro)),
      _nodeId (pDSPro == nullptr ? "" : pDSPro->getNodeId())
{
}

AMTDictator::~AMTDictator (void)
{
    delete _pDictator;
    _pDictator = nullptr;
}

void AMTDictator::messageArrived (const void *pBuf, uint32 ui32Len, const char *pszMIMEType)
{
    messageArrived (parseAndValidate (pBuf, ui32Len, pszMIMEType, _nodeId));
}

void AMTDictator::messageArrived (NOMADSUtil::JsonObject *pJson)
{
    if ((_pDictator != nullptr) && (pJson != nullptr)) {
        if (!_pDictator->isRunning()) {
            _pDictator->start();
        }
        _pDictator->enqueue (pJson);  // _pDictator does _not_ make a copy of pJson! Do not deallocate it!
    }
}

NOMADSUtil::JsonObject * AMTDictator::parseAndValidate (const void *pBuf, uint32 ui32Len, const char *pszMIMEType, const char *pszNodeId)
{
    const char *pszMethodName = "AMTDictator::messageArrived";
    if ((pBuf == nullptr) || (ui32Len == 0) || (pszMIMEType == nullptr)) {
        return nullptr;
    }
    String sJson ((const char *)pBuf, (unsigned short)ui32Len);
    if ((PHOENIX_MIST_JSON_BASE64_MIME_TYPE ^= pszMIMEType) || (PHOENIX_JSON_BASE64_MIME_TYPE ^= pszMIMEType) || (SOIGEN2_JSON_BASE64_MIME_TYPE ^= pszMIMEType)) {
        // Decode base64-encoded json
        void *pDecodedJson = Base64Transcoders::decode (sJson, &ui32Len);
        if ((pDecodedJson == nullptr) || (ui32Len == 0U)) {
            return nullptr;
        }
        String sDecodedJson ((const char *)pDecodedJson, ui32Len);
        free (pDecodedJson);
        sJson = sDecodedJson;
    }
    else {
        // Unsupported format
        return nullptr;
    }
    JsonObject *pJson = new JsonObject (sJson);
    if (pJson == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not parse JSON.\n");
        return nullptr;
    }
    String callsign;
    if (pJson->getString ("callsign", callsign) != 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not read callsign.\n");
        delete pJson;
        return nullptr;
    }
    if ((pszNodeId != nullptr) && (callsign != pszNodeId)) {
        delete pJson;
        return nullptr;
    }
    return pJson;
}

AsyncDictator::AsyncDictator (DSProImpl *pDSPro)
    : _cvDeliveryQueue (&_mDeliveryQueue),
      _pDSPro (pDSPro)
{
}

AsyncDictator::~AsyncDictator (void)
{
    requestTerminationAndWait();
    for (JsonObject *pJson; nullptr != (pJson = (JsonObject*)_deliveryQueue.dequeue());) {
        delete pJson;
    }
}

void AsyncDictator::enqueue (JsonObject *pJson)
{
    _mDeliveryQueue.lock();
    _deliveryQueue.enqueue (pJson);
    _cvDeliveryQueue.notifyAll();
    _mDeliveryQueue.unlock();
}

void AsyncDictator::run (void)
{
    JsonObject *pJson;
    started();
    while (!terminationRequested()) {
        _mDeliveryQueue.lock();
        while (nullptr == (pJson = (JsonObject*)_deliveryQueue.dequeue())) {
            if (terminationRequested()) {
                _mDeliveryQueue.unlock();
                break;
            }
            _cvDeliveryQueue.wait (1000);
        }
        _mDeliveryQueue.unlock();
        dictate (_pDSPro, *pJson);
        delete pJson;
    }
    terminating();
}

