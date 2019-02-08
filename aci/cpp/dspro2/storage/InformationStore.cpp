/*
 * InformationStore.cpp
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
 */

#include "InformationStore.h"

#include "Defs.h"
#include "MatchmakingQualifier.h"
#include "MetaData.h"
#include "MetadataConfigurationImpl.h"

#include "DataStore.h"
#include "SQLiteFactory.h"
#include "DSLib.h"

#include "AVList.h"
#include "Logger.h"
#include "MD5.h"
#include "NLFLib.h"
#include "SessionId.h"
#include "StrClass.h"

#include "sqlite3.h"

#include "tinyxml.h"

#include <string.h>
#include "BufferWriter.h"
#include "Json.h"

using namespace NOMADSUtil;
using namespace IHMC_C45;
using namespace IHMC_MISC;
using namespace IHMC_ACI;
using namespace IHMC_VOI;

const char * InformationStore::DEFAULT_DATABASE_NAME = ":memory:";
const char * InformationStore::DEFAULT_METADATA_TABLE_NAME = "MetaData_Table";

namespace INFORMATION_STORE
{
    String JSON_BLOB = "json_blob";

    int insertInternal (MetadataInterface *pMetadata, MetadataConfigurationImpl *pMetadataConf,
                        sqlite3 *pSQL3DB, sqlite3_stmt *psqlInserted)
    {
        const char *pszMethodName = "InformationStore::insertInternal";

        JsonObject *pJson = pMetadata->toJson();
        if (pJson == nullptr) {
            return -3;
        }
        String json (pJson->toString (true));
        delete pJson;

        BufferWriter bw (1024, 1024);
        if (pMetadata->write (&bw, 0) < 0) {
            return -3;
        }

        uint16 metadataFieldsNumber;
        const MetadataFieldInfo **ppMetadataFieldInfos = pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
        const BoundingBox bbox = pMetadata->getLocation();

        int iIndex = 0;
        for (; iIndex < metadataFieldsNumber; iIndex++) {
            if ( (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::FLOAT)) ||
                 (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::DOUBLE))) {
                int rc;
                double value = -1;
                if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::LEFT_UPPER_LATITUDE)) {
                    value = bbox._leftUpperLatitude;
                    rc = bbox.isValid() ? 0 : -1;
                }
                else if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::LEFT_UPPER_LONGITUDE)) {
                    value = bbox._leftUpperLongitude;
                    rc = bbox.isValid() ? 0 : -1;
                }
                else if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::RIGHT_LOWER_LATITUDE)) {
                    value = bbox._rightLowerLatitude;
                    rc = bbox.isValid() ? 0 : -1;
                }
                else if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::RIGHT_LOWER_LONGITUDE)) {
                    value = bbox._rightLowerLongitude;
                    rc = bbox.isValid() ? 0 : -1;
                }
                else {
                    rc = pMetadata->getFieldValue (ppMetadataFieldInfos[iIndex]->_sFieldName, &value);
                }

                if (rc == 0) {
                    if (sqlite3_bind_double (psqlInserted, iIndex + 1, value) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Could not bind field %s\n", ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
                else {
                    if (sqlite3_bind_null (psqlInserted, iIndex + 1) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Could not bind field %s\n", ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
            }
            if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::TEXT)) {
                char *pValue = nullptr;
                int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[iIndex]->_sFieldName, &pValue);
                if (rc == 0) {
                    if ((iIndex == pMetadataConf->getMessageIdIndex()) || (iIndex == pMetadataConf->getRefersToIndex())) {
                        if (pValue == nullptr || strcmp (pValue, MetadataValue::UNKNOWN) == 0) {
                            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "trying to add message with "
                                            "unknown message ID or refersTo field\n");
                            return -4;
                        }
                    }
                    if (sqlite3_bind_text (psqlInserted, iIndex + 1, pValue, sizeof(char) * strlen (pValue), SQLITE_TRANSIENT) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s with value = <%s>\n",
                                        ppMetadataFieldInfos[iIndex]->_sFieldName.c_str(), pValue);
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
                else {
                    assert ((ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::MESSAGE_ID) == 1 ? false : true);
                    assert ((ppMetadataFieldInfos[iIndex]->_sFieldName == MetadataInterface::REFERS_TO) == 1 ? false : true);
                    if (sqlite3_bind_null (psqlInserted, iIndex + 1) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                        ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);

                        return -4;
                    }
                }
                free (pValue);
                pValue = nullptr;
            }
            else if ((1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::INTEGER8)) ||
                     (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::INTEGER16)) ||
                     (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::INTEGER32))) {
                int value = -1;
                int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[iIndex]->_sFieldName, &value);
                if (rc == 0) {
                    if(sqlite3_bind_int (psqlInserted, iIndex + 1, value) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                        ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
                else {
                    if (sqlite3_bind_null (psqlInserted, iIndex + 1) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                        "Could not bind field %s\n", ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
            }
            else if (1 == (ppMetadataFieldInfos[iIndex]->_sFieldType == MetadataType::INTEGER64)) {
                int64 value = -1;
                int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[iIndex]->_sFieldName, &value);
                if (rc == 0) {
                    if (sqlite3_bind_int64 (psqlInserted, iIndex + 1, value) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                        ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
                else {
                    if(sqlite3_bind_null (psqlInserted, iIndex + 1) != SQLITE_OK) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                        "Could not bind field %s\n", ppMetadataFieldInfos[iIndex]->_sFieldName.c_str());
                        sqlite3_reset (psqlInserted);
                        return -4;
                    }
                }
            }
        }

        if (sqlite3_bind_text (psqlInserted, iIndex + 1, json, json.length(), SQLITE_TRANSIENT) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n", JSON_BLOB.c_str());
            sqlite3_reset (psqlInserted);
            return -5;
        }

        int rc = sqlite3_step (psqlInserted);
        switch (rc) {
            case SQLITE_OK:
                checkAndLogMsg (pszMethodName, Logger::L_Info, "Insert successful\n");
                rc = 0;
                break;

            case SQLITE_DONE:
                checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                "Insert successful. the last row inserted is: %d\n",
                                sqlite3_last_insert_rowid (pSQL3DB));
                rc = 0;
                break;

            default:
                String err (sqlite3_errmsg (pSQL3DB));
                checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Insert failed: %s: %s.\n",
                                SQLiteFactory::getErrorAsString (rc), err.length() > 0 ? err.c_str() : "") ;
                rc = -5;
        }

        sqlite3_reset (psqlInserted);
        return rc;
    }

    class ClearTable : public SessionIdListener
    {
        public:
            ClearTable (InformationStore *pInfoStore)
                : _pInfoStore (pInfoStore)
            {
            }

            ~ClearTable (void)
            {
            }

            void sessionIdChanged (void)
            {
                _pInfoStore->clear();
            }

        private:
            InformationStore *_pInfoStore;
    };
}

InformationStore::InformationStore (DataStore *pDataCache, const char *pszDSProGroupName)
    : _dbName (DEFAULT_DATABASE_NAME),
      _tableName (DEFAULT_METADATA_TABLE_NAME),
      _primaryKey (MetadataInterface::MESSAGE_ID),
      _uiAllColumnsCount (0),
      _pSQL3DB (nullptr),
      _psqlInserted (nullptr),
      _psqlDeleteObsolete (nullptr),
      _psqlDeleteByObjectId ( nullptr),
      _psqlDeleteByObjectInstanceId (nullptr),
      _psqlGetMetadata (nullptr),
      _psqlGetReferringMetadata (nullptr),
      _errorCode (0),
      _pDataStore (pDataCache),
      _pszDSProGroupName (pszDSProGroupName),
      _pszStartsWithDSProGroupNameTemplate (nullptr),
      _pMetadataConf (nullptr),
      _m (MutexId::InformationStore_m, LOG_MUTEX),
      _pSessionIdListener (nullptr)
{
    String tmp (_pszDSProGroupName);
    tmp += "*";
    _pszStartsWithDSProGroupNameTemplate = tmp.r_str();
}

InformationStore::~InformationStore (void)
{
    // TODO : it doesn't compile on visual studio 2008. Debug it
    //while((_psqlInserted = sqlite3_next_stmt(_pSQL3DB, 0)) != 0) sqlite3_finalize(_psqlInserted);
    sqlite3_close(_pSQL3DB); _pSQL3DB = nullptr;
    sqlite3_finalize (_psqlGetMetadata);
    _psqlGetMetadata = nullptr;
    sqlite3_finalize (_psqlGetReferringMetadata);
    _psqlGetReferringMetadata = nullptr;
}

int InformationStore::insert (MetadataInterface *pMetadata)
{
    const char *pszMethodName = "InformationStore::insert";

    if ((_psqlInserted == nullptr) || (_psqlDeleteObsolete == nullptr)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "_psqlInserted or _psqlDeleteObsolete are nullptr\n");
        return -1;
    }
    if (pMetadata == nullptr) {
        return -2;
    }
    int64 i64SourceTimestamp = 0U;
    String objectId, instanceId;
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_OBJECT_ID, objectId);
    pMetadata->getFieldValue (MetadataInterface::REFERRED_DATA_INSTANCE_ID, instanceId);
    pMetadata->getFieldValue (MetadataInterface::SOURCE_TIME_STAMP, &i64SourceTimestamp);

    _m.lock (1018);

    int rc = 0;
    sqlite3_reset (_psqlDeleteObsolete);
    if ((objectId.length() >= 0) && (instanceId.length() >= 0) && (i64SourceTimestamp > 0)) {
        // Delete obsolete instances
        if (sqlite3_bind_text (_psqlDeleteObsolete, 1, objectId, objectId.length(), SQLITE_TRANSIENT) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind referredObjectId field\n");
            _m.unlock (1018);
            return -4;
        }
        if (sqlite3_bind_int64 (_psqlDeleteObsolete, 2, i64SourceTimestamp) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind source time stamp field\n");
            _m.unlock (1018);
            return -5;
        }
        rc = sqlite3_step (_psqlDeleteObsolete);
        switch (rc) {
            case SQLITE_OK:
            case SQLITE_DONE:
                checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Delete successful.");
                break;
            default:
                checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Delete failed: %s\n",
                                SQLiteFactory::getErrorAsString (rc));
                rc = -6;
        }
    }

    // Add most recent instance
    if (INFORMATION_STORE::insertInternal (pMetadata, _pMetadataConf, _pSQL3DB, _psqlInserted) < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "Could not insert message\n");
        rc = -7;
    }

    _m.unlock (1018);
    return rc;
}

char ** InformationStore::getDSProIds (const char *pszObjectId, const char *pszInstanceId)
{
    if (pszObjectId == nullptr) {
        return nullptr;
    }

    String sql = "SELECT ";
    sql += MetaData::MESSAGE_ID;
    sql += " FROM ";
    sql += _tableName;
    sql += " WHERE ";
    sql += MetaData::REFERRED_DATA_OBJECT_ID;
    sql += " = '";
    sql += pszObjectId;
    sql += "'";
    if (pszInstanceId != nullptr) {
        sql += " AND ";
        sql += MetaData::REFERRED_DATA_INSTANCE_ID;
        sql += " = '";
        sql += pszInstanceId;
        sql += "'";
    }
    sql += ";";

    return getDSProIds (sql);
}

char ** InformationStore::getDSProIds (const char *pszQuery)
{
    if (pszQuery == nullptr) {
        return nullptr;
    }
    unsigned int uiQueryLen = strlen (pszQuery);
    if (uiQueryLen == 0) {
        return nullptr;
    }

    _m.lock (1019);

    sqlite3_stmt *psqlGetIDs = nullptr;
    int rc = sqlite3_prepare_v2 (_pSQL3DB, pszQuery, uiQueryLen, &psqlGetIDs, nullptr);
    if (rc != 0 || psqlGetIDs == nullptr) {
        _m.unlock (1019);
        return nullptr;
    }

    unsigned int uiRow = 0;
    DArray2<String> ids;
    while (((sqlite3_step (psqlGetIDs)) == SQLITE_ROW)) {
        if (sqlite3_column_type (psqlGetIDs, 0) == SQLITE_TEXT) {
            const char *pszValue = (const char *) sqlite3_column_text (psqlGetIDs, 0);
            if (pszValue != nullptr) {
                ids[uiRow] = pszValue;
                uiRow++;
            }
        }
    }

    checkAndLogMsg ("InformationStore::getDSProIds", Logger::L_Info,
                    "%u dspro ids were found for query <%s>\n", ids.size(), pszQuery);

    char **ppszIds = nullptr;
    if (ids.size() > 0) {
        ppszIds = (char **) calloc (ids.size() + 1, sizeof (char*));
        if (ppszIds == nullptr) {
            checkAndLogMsg ("InformationStore::getDSProIds", memoryExhausted);
        }
        else {
            for (unsigned int i = 0; i < ids.size(); i++) {
                ppszIds[i] = ids[i].r_str();
            }
        }
    }

    sqlite3_finalize (psqlGetIDs);
  //  if (psqlGetIDs != nullptr) {
//        free (psqlGetIDs);
    //}

    _m.unlock (1019);
    return ppszIds;
}

MetadataInterface * InformationStore::getMetadata (const char *pszKey)
{
    if (pszKey == nullptr) {
        return nullptr;
    }

    _m.lock (1019);
    if (_psqlGetMetadata == nullptr) {
        // Create the prepared statement if it does not already exist
        String sql = "SELECT ";
        sql += INFORMATION_STORE::JSON_BLOB;
        sql += " FROM ";
        sql += _tableName;
        sql += " WHERE ";
        sql += _primaryKey;
        sql += " = ?;";
        int rc = sqlite3_prepare_v2 (_pSQL3DB, sql.c_str(), sql.length(),
                                     &_psqlGetMetadata, nullptr);

        if ((rc == SQLITE_OK) && (_psqlGetMetadata != nullptr)) {
            checkAndLogMsg ("InformationStore::getMetadata", Logger::L_Info,
                            "Statement prepared successfully. Sql statement': %s.\n",
                            sql.c_str());
        }
        else {
            checkAndLogMsg ("InformationStore::getMetadata", Logger::L_SevereError,
                            "Could not prepare statement: %s. Error code: %s\n",
                            sql.c_str(), SQLiteFactory::getErrorAsString (rc));
            sqlite3_reset (_psqlGetMetadata);
            _m.lock (1019);
            return nullptr;
        }
    }

    // Bind the values to the prepared statement
    if (sqlite3_bind_text (_psqlGetMetadata, 1, pszKey, strlen (pszKey), SQLITE_STATIC) != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::getMetadata", Logger::L_SevereError, "Error when binding values.\n");
        sqlite3_reset (_psqlGetReferringMetadata);
        _m.unlock (1019);
        return nullptr;
    }

    MetadataList *pMetadataList = getMetadataInternal (_psqlGetMetadata);
    if (pMetadataList == nullptr) {
        _m.unlock (1019);
        return nullptr;
    }
    MetadataInterface *pMetadata = pMetadataList->getFirst();
    assert (pMetadataList->getNext() == nullptr);
    delete pMetadataList;
    _m.unlock (1019);
    return pMetadata;
}

MetadataList * InformationStore::getMetadataForData (const char *pszReferring)
{
    if (pszReferring == nullptr) {
        return nullptr;
    }

    _m.lock (1020);
    if (_psqlGetReferringMetadata == nullptr) {
        // Create the prepared statement if it does not already exist
        String extract = "SELECT ";
        extract += INFORMATION_STORE::JSON_BLOB;
        extract += " FROM ";
        extract += _tableName;
        extract += " WHERE ";
        extract += MetaData::REFERS_TO;
        extract += " = ? ";
        extract += "LIMIT 1;";
        int rc = sqlite3_prepare_v2 (_pSQL3DB, extract.c_str(), extract.length(),
                                     &_psqlGetReferringMetadata, nullptr);

        if ((rc == SQLITE_OK) && (_psqlGetReferringMetadata != nullptr)) {
            checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)extract);
        }
        else {
            checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_SevereError,
                            "Could not prepare statement: %s. Error code: %s\n",
                            extract.c_str(), SQLiteFactory::getErrorAsString (rc));
            sqlite3_reset (_psqlGetReferringMetadata);
            _m.unlock (1020);
            return nullptr;
        }
    }

    // Bind the values to the prepared statement
    if (sqlite3_bind_text (_psqlGetReferringMetadata, 1, pszReferring, strlen (pszReferring), SQLITE_STATIC) != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_SevereError,
                        "Error when binding values.\n");
        sqlite3_reset (_psqlGetReferringMetadata);
        _m.unlock (1020);
        return nullptr;
    }

    MetadataList *pMetadataList = getMetadataInternal (_psqlGetReferringMetadata);
    _m.unlock (1020);
    return pMetadataList;
}

MetadataList * InformationStore::getMetadataInternal (sqlite3_stmt *pStmt, bool bAllowMultipleMatches) const
{
    const char *pszMethodName = "InformationStore::getMetadataInternal";
    unsigned short metadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = _pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    if (pMetadataFieldInfos == nullptr || metadataFieldsNumber == 0) {
        return nullptr;
    }

    // Execute the statement
    int rc;
    int rows = 0;
    MetadataList *pMetadataList = nullptr;
    for (; ((rc = sqlite3_step (pStmt)) == SQLITE_ROW) && (bAllowMultipleMatches || (rows < 1)); rows++) {
        MetaData *pMetadata = new MetaData();
        if (pMetadata != nullptr) {
            for (int i = 0; i < 1; i ++) {
                if (sqlite3_column_type (pStmt, i) != SQLITE_NULL) {
                    int iLen = sqlite3_column_bytes (pStmt, 0);
                    if (iLen >= 0) {
                        String json ((const char *) sqlite3_column_text (pStmt, 0));
                        JsonObject obj (json);
                        pMetadata->fromJson (&obj);
                    }
                    else {
                        delete pMetadata;
                    }
                }
            }
            if (pMetadataList == nullptr) {
                pMetadataList = new MetadataList();
            }
            if (pMetadataList != nullptr) {
                pMetadataList->prepend (pMetadata);
            }
        }
    }

    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE)) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "sqlite3_step returned %s.\n", SQLiteFactory::getErrorAsString (rc));
    }
    else if (rows == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query\n");
    }
    else if (rows > 1 && !bAllowMultipleMatches) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError,
                        "sqlite3_get_table() found more that 1 match for query but there "
                        "should have only been one match\n");
    }

    sqlite3_reset (pStmt);
    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadata (AVList *pAVQueryList, int64 i64BeginArrivalTimestamp,
                                                 int64 i64EndArrivalTimestamp)
{
    unsigned int uiReceiverTimeStampIndex = _pMetadataConf->getReceiverTimeStampIndex();
    unsigned short metadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = _pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    if (pMetadataFieldInfos == nullptr || pMetadataFieldInfos[uiReceiverTimeStampIndex] == nullptr) {
        return nullptr;
    }
    const char *pszReceiverTimeStampFieldName = pMetadataFieldInfos[uiReceiverTimeStampIndex]->_sFieldName;

    String sql = "SELECT ";
    sql += INFORMATION_STORE::JSON_BLOB;
    sql += " FROM ";
    sql += _tableName;

    bool bIsFirstConstraint = true;
    if (i64BeginArrivalTimestamp > 0) {
        sql += bIsFirstConstraint ? " WHERE " : " AND ";
        bIsFirstConstraint = false;
        sql += pszReceiverTimeStampFieldName;
        sql += " > ";
        char timestamp[23];
        i64toa (timestamp, i64BeginArrivalTimestamp);
        sql += timestamp;
    }
    if (i64EndArrivalTimestamp > 0) {
        sql += bIsFirstConstraint ? " WHERE " : " AND ";
        bIsFirstConstraint = false;
        sql += pszReceiverTimeStampFieldName;
        sql += " <= ";
        char timestamp[23];
        i64toa (timestamp, i64EndArrivalTimestamp);
        sql += timestamp;
    }

    if (pAVQueryList != nullptr && !pAVQueryList->isEmpty()) {
        unsigned int uiLen = (unsigned int) pAVQueryList->getLength();
        for (unsigned int i = 0; i < uiLen; i++) {
            const char *pszAttribute = pAVQueryList->getAttribute (i);
            const char *pszValue = pAVQueryList->getValueByIndex (i);
            const char *pszFieldType = _pMetadataConf->getFieldType (pszAttribute);
            if (pszAttribute != nullptr && pszValue != nullptr && pszFieldType != nullptr) {

                if (bIsFirstConstraint) {
                    sql += " WHERE ";
                    bIsFirstConstraint = false;
                }
                else {
                    sql += " AND ";
                }
                sql += pszAttribute;
                sql += " = ";
                // TODO: fix this! strcmp (pszFieldType, MetadataValue::UNKNOWN)
                // should be strcmp (pszValue, MetadataValue::UNKNOWN), but SQLite
                // writes something else instead...
                bool bIsString = strcmp (pszFieldType, MetadataType::TEXT) || strcmp (pszFieldType, MetadataValue::UNKNOWN);
                if (bIsString) {
                    sql += "'";
                }
                sql += pszValue;
                if (bIsString) {
                    sql += "'";
                }
            }
        }
    }

    // TODO: FIX LOCK ID!
    _m.lock (1021);
    MetadataList *pMetadataList = getAllMetadata (sql, 1);
    _m.unlock (1021);

    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadataInArea (MatchmakingQualifiers *pMatchmakingQualifiers,
                                                       const char **ppszMessageIdFilters,
                                                       const BoundingBox &area, bool bEmptyPedigree)
{
    const float fMaxLat = area._leftUpperLatitude;
    const float fMinLat = area._rightLowerLatitude;
    const float fMaxLong = area._rightLowerLongitude;
    const float fMinLong = area._leftUpperLongitude;

    String sql = "SELECT ";
    sql += INFORMATION_STORE::JSON_BLOB;
    sql += " FROM ";

    if (pMatchmakingQualifiers != nullptr &&
        pMatchmakingQualifiers->_qualifiers.getFirst() != nullptr) {
        sql += "(";
        sql += toSqlStatement (pMatchmakingQualifiers);
        sql +=  ")";
    }
    else {
        sql += _tableName;
    }

    if ((ppszMessageIdFilters != nullptr) && (ppszMessageIdFilters[0] != nullptr)) {
        sql += (String) " WHERE " + MetadataInterface::MESSAGE_ID + " NOT IN (";
        for (int i = 0; ppszMessageIdFilters[i] != nullptr; i++) {
            if (i > 0) {
                sql += ", ";
            }
            sql += (String) "'" + ppszMessageIdFilters[i] + "'";
        }
        sql += ")";
    }
    sql += (((ppszMessageIdFilters != nullptr) && (ppszMessageIdFilters[0] != nullptr)) ? " AND " : " WHERE ");

    // Metadata's bounding-box
    const char *pszMetaMaxLat = MetadataInterface::LEFT_UPPER_LATITUDE;
    const char *pszMetaMinLat = MetadataInterface::RIGHT_LOWER_LATITUDE;
    const char *pszMetaMaxLong = MetadataInterface::RIGHT_LOWER_LONGITUDE;
    const char *pszMetaMinLong = MetadataInterface::LEFT_UPPER_LONGITUDE;

    // Area's bounding-box
    static const uint8 ui8BufLen = 20;
    char pszMaxLat[ui8BufLen];
    DSLib::floatToString (pszMaxLat, ui8BufLen, fMaxLat);
    char pszMinLat[ui8BufLen];
    DSLib::floatToString (pszMinLat, ui8BufLen, fMinLat);
    char pszMaxLong[ui8BufLen];
    DSLib::floatToString (pszMaxLong, ui8BufLen, fMaxLong);
    char pszMinLong[ui8BufLen];
    DSLib::floatToString (pszMinLong, ui8BufLen, fMinLong);

    sql += (String) " ("
                // At least one corner of the metadata's bounding-box
                // is included in the path's bounding-box
            +   "(((" + pszMetaMinLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ") OR "
            +       "(" + pszMetaMaxLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + "))"
            +                                                         " AND "
            +       "((" + pszMetaMinLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ") OR "
            +       "(" + pszMetaMaxLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ")))"
            +                                                            " OR "
                    // The metadata's bounding-box includes, or partially
                    // includes, the path's bounding-box (1)
            +       "((" + pszMetaMinLat + " <= " + pszMinLat + " AND " +  pszMetaMaxLat + " >= " + pszMaxLat + ") AND "
            +       "    ("
            +                 // Partial overlapping
            +                 "("
            +                     "(" + pszMetaMinLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ") OR "
            +                     "(" + pszMetaMaxLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ")"
            +                 ")" +                            " OR "
            +                 // Total overlapping
            +                 "(" + pszMetaMinLong + " <= " + pszMinLong + " AND " +  pszMetaMaxLong + " >= " + pszMaxLong + ")"
            +              ") "
            +           ")"
            +                                                        " OR "
                       // The metadata's bounding-box includes, or partially
                       // includes, the path's bounding-box (2)
            +          "((" + pszMetaMinLong + " <= " + pszMinLong + " AND " +  pszMetaMaxLong + " >= " + pszMaxLong + ") AND "
            +          "    ("
            +                 // Partial overlapping
            +                 "("
            +                     "(" + pszMetaMinLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ") OR "
            +                     "(" + pszMetaMaxLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ")"
            +                 ")" +                            " OR "
            +                 // Total overlapping
            +                 "(" + pszMetaMinLat + " <= " + pszMinLat + " AND " +  pszMetaMaxLat + " >= " + pszMaxLat + ")"
            +              ") "
            +          ")"
            +                                                            " OR "
                     "(" + MetadataInterface::LEFT_UPPER_LATITUDE + " IS NULL AND " + MetadataInterface::RIGHT_LOWER_LATITUDE + " IS NULL AND "
            +              MetadataInterface::RIGHT_LOWER_LONGITUDE + " IS NULL AND " + MetadataInterface::LEFT_UPPER_LONGITUDE + " IS NULL)" +

            +        ")";

    if (bEmptyPedigree) {
        sql += " AND (";
        sql += MetadataInterface::PEDIGREE;
        sql += " + IS NULL OR ";
        sql += MetadataInterface::PEDIGREE;
        sql += " = '')";
    }

    sql += ";";

    _m.lock (1021);
    MetadataList *pMetadataList = getAllMetadata (sql, 1);
    _m.unlock (1021);
    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadata (const char **ppszMessageIdFilters, bool bExclusiveFilter)
{
    String extract = "SELECT ";
    extract += INFORMATION_STORE::JSON_BLOB;
    extract += " FROM ";
    extract += _tableName;
    if ((ppszMessageIdFilters != nullptr) && (ppszMessageIdFilters[0] != nullptr)) {
        extract += (String) " WHERE " + MetaData::MESSAGE_ID;
        extract += (bExclusiveFilter ? " NOT IN (" : " IN (");
        for (int i = 0; ppszMessageIdFilters[i] != nullptr; i++) {
            if (i > 0) {
                extract += ", ";
            }
            extract += (String) "'" + ppszMessageIdFilters[i] + "'";
        }
        extract += ")";
    }
    extract += ";";

    _m.lock (1022);
    MetadataList *pMetadataList = getAllMetadata (extract, 1);
    _m.unlock (1022);

    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadata (const char *pszSQL, unsigned int uiExpectedNumberOfColumns) const
{
    const char *pszMethodName = "InformationStore::getAllMetadata";
    char *pErrMessage = nullptr;
    char **ppQueryResults;
    int rc, noRows, noColumns;
    rc = sqlite3_get_table (_pSQL3DB, pszSQL, &ppQueryResults, &noRows, &noColumns, &pErrMessage);
    if (rc != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "sqlite3_get_table() failed with return "
                        "value = %d; query = %s; error msg = <%s>\n", rc, pszSQL, pErrMessage);
        sqlite3_free (pErrMessage);
        sqlite3_free_table (ppQueryResults);
        return nullptr;
    }
    if (noRows == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "sqlite3_get_table() "
                        "did not find any matches for query %s\n", pszSQL);
        sqlite3_free_table (ppQueryResults);
        return nullptr;
    }
    if ((noColumns < 0) || (((unsigned int)noColumns) != uiExpectedNumberOfColumns)) {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "sqlite3_get_table() returned %d columns for query %s\n"
                        "but there should have been %d\n", noColumns, pszSQL, uiExpectedNumberOfColumns);
        sqlite3_free_table (ppQueryResults);
        return nullptr;
    }

    MetadataList *pMetadataList = new MetadataList();
    for (int i = 1; i <= noRows; i ++) {
        MetaData *pMetadata = new MetaData();
        if (pMetadata != nullptr) {
            unsigned int uiColIdx = 0;
            for (int j = 0; j < noColumns && uiColIdx < _uiAllColumnsCount; j++) {
                const char *pszJson = ppQueryResults[noColumns * i + j];
                if (pszJson != nullptr) {
                    JsonObject obj (pszJson);
                    pMetadata->fromJson (&obj);
                }
                uiColIdx++;
            }
            pMetadataList->prepend (pMetadata);
        }
    }

    sqlite3_free_table (ppQueryResults);
    return pMetadataList;
}

PtrLList<const char> * InformationStore::getMessageIDs (const char *pszGroupName, const char *pszSqlConstraints, char **ppszFilters)
{
    if (pszSqlConstraints == nullptr) {
        return nullptr;
    }

    String sql = "SELECT ";
    sql = sql + MetaData::MESSAGE_ID;
    sql = sql + " FROM ";
    sql = sql + _tableName;
    sql = sql + " WHERE ";
    sql = sql + pszSqlConstraints;

    if ((ppszFilters != nullptr) && (ppszFilters[0] != nullptr)) {
        sql += (String) " AND " + MetaData::MESSAGE_ID + " NOT IN (";
        for (int i = 0; ppszFilters[i] != nullptr; i++) {
            if (i > 0) {
                sql += ", ";
            }
            sql += (String) "'" + ppszFilters[i] + "'";
        }
        sql += ")";
    }

    sql = sql + ";";

    _m.lock (1023);
    PtrLList<const char> *pRet = extractMessageIDsFromDBBase (pszGroupName, (const char *) sql);
    _m.unlock (1023);
    return pRet;
}

PtrLList<const char> * InformationStore::extractMessageIDsFromDBBase (const char *pszGroupName, const char *pszSQL)
{
    // Get the IDs of the selected messages
    char *pszErrMsg;
    char **ppzsQueryResults;
    int ret, noRows, noColumns;
    ret = sqlite3_get_table (_pSQL3DB, pszSQL, &ppzsQueryResults, &noRows, &noColumns, &pszErrMsg);
    if (ret != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::extractMessageIDsFromDBBase", Logger::L_MildError,
                        "sqlite3_get_table() failed with return value = %d; search query = <%s>; error msg = <%s>\n",
                        ret, pszSQL, pszErrMsg);
        sqlite3_free (pszErrMsg);
        sqlite3_free_table (ppzsQueryResults);
        _errorCode = 6;
        return nullptr;
    }
    if (noRows == 0) {
        checkAndLogMsg ("InformationStore::extractMessageIDsFromDBBase", Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query <%s>\n", (const char *) pszSQL);
        sqlite3_free_table(ppzsQueryResults);
        _errorCode = 7;
        return nullptr;
    }
    if (noColumns != 1) {
        checkAndLogMsg ("InformationStore::extractMessageIDsFromDBBase", Logger::L_MildError,
                        "sqlite3_get_table() returned %d columns for query <%s> but there should have been 1\n",
                        noColumns, (const char *) pszSQL);
        sqlite3_free_table(ppzsQueryResults);
        _errorCode = 6;
        return nullptr;
    }

    // Verify that the selected messages exist and that it belongs to the
    // specified group
    _pDataStore->lock();
    PtrLList<const char> *pResultsList = nullptr;
    for (int i = 0; i < noRows; i++) {
        if (wildcardStringCompare (ppzsQueryResults[i + 1],
                                   _pszStartsWithDSProGroupNameTemplate)) {
            if (_pDataStore->isMetadataMessageStored (ppzsQueryResults[i + 1])) {
                if (pResultsList == nullptr) {
                    pResultsList = new PtrLList<const char>();
                }
                pResultsList->prepend (strDup (ppzsQueryResults[i + 1]));
            }
        }
    }
    _pDataStore->unlock();

    sqlite3_free_table (ppzsQueryResults);
    _errorCode = 0;
    return pResultsList;
}

int InformationStore::updateUsage (const char *pszKey, int usage)
{
    if (pszKey == nullptr) {
        _errorCode = 8;
        return _errorCode;
    }
    if ((usage < -1) || (usage > 1)) {
        _errorCode = 8;
        return _errorCode;
    }
    if (usage == -1) {
        return 0;
    }
    char buffer[10];
    _m.lock (1024);
    int rc = sprintf (buffer, "%d", usage);
    if (rc != 1) {
        _errorCode = 9;
        _m.unlock (1024);
        return _errorCode;
    }
    String update = "SELECT ";
    update += MetaData::USAGE;
    update += " FROM ";
    update += _tableName;
    update += " WHERE ";
    update += _primaryKey;
    update += " = '";
    update += pszKey;
    update += "';";
    char *pszErrMsg = nullptr;
    char **ppszQueryResults = nullptr;
    int noRows, noColumns;
    rc = sqlite3_get_table (_pSQL3DB, update.c_str(), &ppszQueryResults, &noRows, &noColumns, &pszErrMsg);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::updateUsage", Logger::L_MildError,
                        "sqlite3_get_table() failed with return value = %d; query = %s; error msg = <%s>\n",
                         rc, (const char *) update, pszErrMsg);
        sqlite3_free (pszErrMsg);
        sqlite3_free_table (ppszQueryResults);
        _errorCode = 6;
        _m.unlock (1024);
        return _errorCode;
    }
    if (noRows == 0) {
        checkAndLogMsg ("InformationStore::updateUsage", Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query %s\n", (const char *) update);
        sqlite3_free_table (ppszQueryResults);
        _errorCode = 7;
        _m.unlock (1024);
        return _errorCode;
    }
    if (noColumns != 1) {
        checkAndLogMsg ("InformationStore::updateUsage", Logger::L_MildError,
                        "sqlite3_get_table() returned %d columns for query %s\n but there should have been 1\n",
                        noColumns, (const char *) update);
        sqlite3_free_table (ppszQueryResults);
        _errorCode = 6;
        _m.unlock (1024);
        return _errorCode;
    }
    if (ppszQueryResults[noColumns] != nullptr) {
        sqlite3_free_table (ppszQueryResults);
        _m.unlock (1024);
        return 0;
    }
    if (ppszQueryResults != nullptr) {
        sqlite3_free_table (ppszQueryResults);
    }

    update = "UPDATE ";
    update += _tableName;
    update += " SET ";
    update += MetaData::USAGE;
    update += " = ";
    update += buffer;
    update += " WHERE ";
    update += _primaryKey;
    update += " = '";
    update += pszKey;
    update += "';";
    rc = sqlite3_get_table (_pSQL3DB, update, &ppszQueryResults, &noRows, &noColumns, &pszErrMsg);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::updateUsage", Logger::L_MildError,
                        "sqlite3_get_table() failed with return value = %d; search key = <%s>; error msg = <%s>\n",
                        rc, pszKey, pszErrMsg);
        sqlite3_free (pszErrMsg);
        sqlite3_free_table (ppszQueryResults);
        _errorCode = 9;
        _m.unlock (1024);
        return _errorCode;
    }
    sqlite3_free_table (ppszQueryResults);
    _errorCode = 0;
    _m.unlock (1024);
    return _errorCode;
}

int InformationStore::groupAndCount (const char *pszFieldName, float *perc)
{
    if (pszFieldName == nullptr) {
        _errorCode = 8;
        return _errorCode;
    }
    String count = "SELECT COUNT(*) FROM ";
    count += _tableName;
    count += " GROUP BY ";
    count += pszFieldName;
    count += ";";
    char *pszErrMsg;
    char **ppszQueryResults;
    int noRows, noColumns;
    _m.lock (1025);
    int ret = sqlite3_get_table (_pSQL3DB, count, &ppszQueryResults, &noRows, &noColumns, &pszErrMsg);
    if (ret != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::groupAndCount", Logger::L_MildError,
                        "sqlite3_get_table() failed with return value = %d; query = <%s>; error msg = <%s>\n",
                        ret, (const char *) count, pszErrMsg);
        sqlite3_free (pszErrMsg);
        sqlite3_free_table (ppszQueryResults);
        _errorCode = 9;
        _m.unlock (1025);
        return _errorCode;
    }
    int c = 0;
    for (int i = 1; i <= noRows; i++) {
        if (0 == strcmp (ppszQueryResults[i], "1")) {
            c ++;
        }
    }
    (*perc) = ((float) c) / ((float) noRows);
    sqlite3_free_table (ppszQueryResults);
    _errorCode = 0;
    _m.unlock (1025);
    return _errorCode;
}

int InformationStore::deleteMetadataFromDB (const char *pszKey)
{
    _m.lock (1026);
    if (pszKey == nullptr) {
        _errorCode = 8;
        _m.unlock (1026);
        return _errorCode;
    }
    String deletesql = "DELETE FROM ";
    deletesql += _tableName;
    deletesql += " WHERE ";
    deletesql += _primaryKey;
    deletesql += " = '";
    deletesql += pszKey;
    deletesql += "';";
    checkAndLogMsg ("InformationStore::deleteMetaDataFromDB", Logger::L_Info,
                    "The query is %s\n", deletesql.c_str());
    char *pszErrMsg;
    if (sqlite3_exec (_pSQL3DB, deletesql, nullptr, nullptr, &pszErrMsg) != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::deleteMetaDataFromDB", Logger::L_SevereError,
                        "%s\n", pszErrMsg);
        sqlite3_free (pszErrMsg);
        _errorCode = 10;
        _m.unlock (1026);
        return _errorCode;
    }
    sqlite3_free (pszErrMsg);
    _errorCode = 0;
    _m.unlock (1026);
    return _errorCode;
}

int InformationStore::deleteMetadata (const char *pszObjectId, const char *pszInstanceId)
{
    const char *pszMethodName = "InformationStore::deleteMetadata";
    if (pszObjectId == nullptr) {
        return -1;
    }
    const int objectIdLen = strlen (pszObjectId);
    if (objectIdLen <= 0) {
        return -2;
    }
    const int instanceIdLen = (pszInstanceId == nullptr ? 0 : strlen (pszInstanceId));
    sqlite3_stmt *pStmt = nullptr;
    _m.lock (1026);
    if (instanceIdLen > 0) {
        sqlite3_reset (_psqlDeleteByObjectInstanceId);
        if (sqlite3_bind_text (_psqlDeleteByObjectInstanceId, 1, pszObjectId, objectIdLen, SQLITE_TRANSIENT) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind object id field\n");
            _m.unlock (1026);
            return -3;
        }
        if (sqlite3_bind_text (_psqlDeleteByObjectInstanceId, 2, pszInstanceId, instanceIdLen, SQLITE_TRANSIENT) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind instance id field\n");
            _m.unlock (1026);
            return -3;
        }
        pStmt = _psqlDeleteByObjectInstanceId;
    }
    else {
        sqlite3_reset (_psqlDeleteByObjectId);
        if (sqlite3_bind_text (_psqlDeleteByObjectId, 1, pszObjectId, objectIdLen, SQLITE_TRANSIENT) != SQLITE_OK) {
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind object id field\n");
            _m.unlock (1026);
            return -3;
        }
        pStmt = _psqlDeleteByObjectId;
    }

    int rc = sqlite3_step (pStmt);
    switch (rc) {
        case SQLITE_OK:
        case SQLITE_DONE:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Delete of %s %s successful\n",
                            pszObjectId, (pszInstanceId == nullptr ? "" : pszInstanceId));
            break;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Delete of %s: %s failed: %s.\n",
                            pszObjectId, (pszInstanceId == nullptr ? "" : pszInstanceId),
                            SQLiteFactory::getErrorAsString (rc));
        rc = -4;
    }

    _m.unlock (1026);
    return 0;
}

int InformationStore::init (MetadataConfigurationImpl *pMetadataConf,
                            const char *pszMetadataTableName,
                            const char *pszMetadataDBName)
{
    if (pMetadataConf == nullptr) {
        return -1;
    }

    MutexUnlocker synchronized (&_m);

    _pMetadataConf = pMetadataConf;

    _psqlGetMetadata = nullptr;
    _psqlGetReferringMetadata = nullptr;

    // initialize DB and metadata table
    if (pszMetadataDBName != nullptr) {
        _dbName = pszMetadataDBName;
    }
    if (pszMetadataTableName != nullptr) {
        _tableName = pszMetadataTableName;
    }

    // Set ALL
    uint16 metadataFieldsNumber = 0;
    const MetadataFieldInfo **ppMetadataFieldInfos = pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    if (ppMetadataFieldInfos == nullptr) {
        return -2;
    }
    _uiAllColumnsCount = 0;
    for (int i = 0; i < metadataFieldsNumber; i++) {
        if (i != 0) {
            _allColumns += ",";
        }
        _allColumns += ppMetadataFieldInfos[i]->_sFieldName;
        _uiAllColumnsCount++;
    }
    if (_allColumns.length() <= 0) {
        checkAndLogMsg ("InformationStore::init", memoryExhausted);
        return -3;
    }

    return openDataBase (pMetadataConf);
}

int InformationStore::openDataBase (MetadataConfigurationImpl *pMetadataConf)
{
    const char *pszMethodName = "InformationStore::openDataBase";

    // open database
    _errorCode = 0;
    _pSQL3DB = SQLiteFactory::getInstance(); // It assumes that the database
                                             // has been already opened/created
                                             // by DisService.
    if (_pSQL3DB == nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Can't open database.\n");
        sqlite3_close (_pSQL3DB);
        _pSQL3DB = nullptr;
        _errorCode = 1;
        return _errorCode;
    }

    sqlite3_busy_timeout (_pSQL3DB, 60 * 1000);  // 1 minute

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Database %s successfully open\n",
                    _dbName.c_str());

    uint16 ui16MetadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = pMetadataConf->getMetadataFieldInfos (ui16MetadataFieldsNumber);
    unsigned short usMessageIDIndex = pMetadataConf->getMessageIdIndex();

    // create metadata table
    String table ("CREATE TABLE IF NOT EXISTS ");
    table += _tableName;
    table += " (";
    table += _primaryKey;
    table += " ";
    table += pMetadataFieldInfos[usMessageIDIndex]->_sFieldType;
    table += " PRIMARY KEY,";
    for (uint16 i = 1; i < ui16MetadataFieldsNumber; i++) {
        table += " ";
        table += pMetadataFieldInfos[i]->_sFieldName;
        table += " ";
        if ((1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER8)) ||
            (1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER16)) ||
            (1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER32)) ||
            (1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER64))) {
            table += "INTEGER";
        }
        else if ((1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::FLOAT)) ||
                 (1 == (pMetadataFieldInfos[i]->_sFieldType == MetadataType::DOUBLE))) {
            table += "REAL";
        }
        else {
            table += pMetadataFieldInfos[i]->_sFieldType;
        }
        if (pMetadataFieldInfos[i]->_bNotNull) {
            table += " NOT NULL";
        }
        if (i != ui16MetadataFieldsNumber - 1) {
            table += ",";
        }
    }

    table += ", " + INFORMATION_STORE::JSON_BLOB + " TEXT,";
    table += "UNIQUE (";
    table += MetadataInterface::REFERRED_DATA_OBJECT_ID;
    table += ", ";
    table += MetadataInterface::REFERRED_DATA_INSTANCE_ID;
    table += ")";
    table += ");";
    char *errMsg;
    int rc = sqlite3_exec (_pSQL3DB, table, nullptr, nullptr, &errMsg);
    switch (rc) {
        case SQLITE_OK:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Creating table by query:\n<%s>\nrun successfully\n", table.c_str());
            char *pszChecksum = MD5Utils::getMD5Checksum (table.c_str(), table.length());
            if (pszChecksum != nullptr) {
                checkAndLogMsg (pszMethodName, Logger::L_Info, "Creating table SQL statement checksum: <%s>\n", pszChecksum);
                free (pszChecksum);
            }
            break;
        }

        case SQLITE_ERROR:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Creating table by query:"
                            "\n<%s>\ngenerated error %s\n", table.c_str(), errMsg);
            sqlite3_free (errMsg);
            _errorCode = 2;
            return _errorCode;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Creating table by query\n<%s>\n"
                            "generated  error %d\n", table.c_str(), rc);
            _errorCode = 2;
            return _errorCode;
    }

    String tryInsert = "INSERT INTO ";
    tryInsert += _tableName;
    tryInsert += " ( ";
    tryInsert += _allColumns;
    tryInsert += "," + INFORMATION_STORE::JSON_BLOB;
    tryInsert += " ) VALUES ( ";
    for (uint16 i = 0; i < ui16MetadataFieldsNumber; i++) {
        tryInsert += " ?,";
    }
    tryInsert += "?);";
    rc = sqlite3_prepare_v2 (_pSQL3DB, tryInsert, tryInsert.length(), &_psqlInserted, nullptr);
    if (rc == SQLITE_OK && _psqlInserted != nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Preparing statement for insert query:"
                        "\n<%s>\nrun successfully\n", tryInsert.c_str());
    }
    else {
        const String msg (sqlite3_errmsg (_pSQL3DB));
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for insert query\n<%s>\n"
                        "failed with error code %d: %s.\n",
                        tryInsert.c_str(), rc, msg.c_str());
        sqlite3_reset (_psqlInserted);
        _psqlInserted = nullptr;
        _errorCode = 3;
    }

    String deleteOldInstances = "DELETE FROM " + _tableName;
    deleteOldInstances += " WHERE ";
    deleteOldInstances += MetadataInterface::REFERRED_DATA_OBJECT_ID;
    deleteOldInstances += " = $1";
    deleteOldInstances += " AND ((";
    deleteOldInstances += MetadataInterface::SOURCE_TIME_STAMP;
    deleteOldInstances += " > 0) AND (";
    deleteOldInstances += MetadataInterface::SOURCE_TIME_STAMP;
    deleteOldInstances += " < ?2))";
    rc = sqlite3_prepare_v2 (_pSQL3DB, deleteOldInstances, deleteOldInstances.length(), &_psqlDeleteObsolete, nullptr);
    if (rc == SQLITE_OK && _psqlDeleteObsolete != nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Preparing statement for delete query:"
                        "\n<%s>\nrun successfully\n", deleteOldInstances.c_str());
    }
    else {
        const String msg (sqlite3_errmsg (_pSQL3DB));
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for deleteOldInstances query\n<%s>\n"
                        "failed with error code %d: %s.\n", deleteOldInstances.c_str(), rc, msg.c_str());
        sqlite3_reset (_psqlDeleteObsolete);
        _psqlDeleteObsolete = nullptr;
        _errorCode = 4;
    }

    deleteOldInstances = "DELETE FROM " + _tableName;
    deleteOldInstances += " WHERE ";
    deleteOldInstances += MetadataInterface::REFERRED_DATA_OBJECT_ID;
    deleteOldInstances += " = $1";
    rc = sqlite3_prepare_v2 (_pSQL3DB, deleteOldInstances, deleteOldInstances.length(), &_psqlDeleteByObjectId, nullptr);
    if (rc == SQLITE_OK && _psqlDeleteByObjectId != nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Preparing statement for delete query:"
                        "\n<%s>\nrun successfully\n", deleteOldInstances.c_str());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for deleteOldInstances query\n<%s>\n"
                        "failed with error code %d\n", deleteOldInstances.c_str(), rc);
        sqlite3_reset (_psqlDeleteByObjectId);
        _psqlDeleteByObjectId = nullptr;
        _errorCode = 5;
    }

    deleteOldInstances = "DELETE FROM " + _tableName;
    deleteOldInstances += " WHERE ";
    deleteOldInstances += MetadataInterface::REFERRED_DATA_OBJECT_ID;
    deleteOldInstances += " = $1 AND ";
    deleteOldInstances += MetadataInterface::REFERRED_DATA_INSTANCE_ID;
    deleteOldInstances += " = $2";
    rc = sqlite3_prepare_v2 (_pSQL3DB, deleteOldInstances, deleteOldInstances.length(), &_psqlDeleteByObjectInstanceId, nullptr);
    if (rc == SQLITE_OK && _psqlDeleteByObjectInstanceId != nullptr) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Preparing statement for delete query:"
                        "\n<%s>\nrun successfully\n", deleteOldInstances.c_str());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for deleteOldInstances query\n<%s>\n"
                        "failed with error code %d\n", deleteOldInstances.c_str(), rc);
        sqlite3_reset (_psqlDeleteByObjectInstanceId);
        _psqlDeleteByObjectInstanceId = nullptr;
        _errorCode = 6;
    }

    _pSessionIdListener = new INFORMATION_STORE::ClearTable (this);
    SessionId::getInstance()->registerSessionIdListener (_pSessionIdListener);

    return _errorCode;
}

char * InformationStore::toSqlConstraint (MatchmakingQualifier *pMatchmakingQualifier)
{
    if (pMatchmakingQualifier == nullptr) {
        return nullptr;
    }

    String stmt;

    if (pMatchmakingQualifier->_operation == MatchmakingQualifier::EQUALS ||
        pMatchmakingQualifier->_operation == MatchmakingQualifier::GREATER ||
        pMatchmakingQualifier->_operation == MatchmakingQualifier::LESS) {

        stmt += pMatchmakingQualifier->_attribute;
        stmt += " ";
        stmt += pMatchmakingQualifier->_operation;
        stmt += " ";
        bool bIsString = _pMetadataConf->isStringFieldType (pMatchmakingQualifier->_attribute);
        if (bIsString) {
            stmt += "'";
        }
        stmt += pMatchmakingQualifier->_value;
        if (bIsString) {
            stmt += "'";
        }
    }
    else if (pMatchmakingQualifier->_operation == MatchmakingQualifier::MATCH) {
        stmt += pMatchmakingQualifier->_attribute;
        stmt += " LIKE ";
        bool bIsString = _pMetadataConf->isStringFieldType (pMatchmakingQualifier->_attribute);
        if (bIsString) {
            stmt += "'";
        }
        stmt += pMatchmakingQualifier->_value;
        if (bIsString) {
            stmt += "'";
        }
    }
    else if (pMatchmakingQualifier->_operation == MatchmakingQualifier::TOP) {
        String innerSql = "SELECT COUNT (*) FROM ";
        innerSql       += _tableName;
        innerSql       += " AS m WHERE m.";
        innerSql       += pMatchmakingQualifier->_attribute;
        innerSql       += " = t.";
        innerSql       += pMatchmakingQualifier->_attribute;
        innerSql       += " AND m. ";
        innerSql       += pMatchmakingQualifier->_value;
        innerSql       += " >= t.";
        innerSql       += pMatchmakingQualifier->_value;

        stmt += MetaData::MESSAGE_ID;
        stmt += " IN (";
        stmt +=   "SELECT ";
        stmt +=   MetaData::MESSAGE_ID;
        stmt +=   " FROM ";
        stmt +=   _tableName;
        stmt +=   " AS t WHERE (";
        stmt +=     innerSql;
        stmt +=   ")  <= ";
        stmt += "1)";
    }

    return stmt.r_str();
}

char * InformationStore::toSqlConstraints (ComplexMatchmakingQualifier *pCMatchmakingQualifier)
{
    String stmt ("SELECT ");
    stmt += _allColumns;
    stmt += " FROM ";
    stmt += _tableName;

    MatchmakingQualifier *pMatchmakingQualifier = pCMatchmakingQualifier->_qualifiers.getFirst();
    for (unsigned short i = 0; pMatchmakingQualifier != nullptr; i++) {
        stmt += (i == 0) ? " WHERE " : " AND ";
        char *pszCondition = toSqlConstraint (pMatchmakingQualifier);
        if (pszCondition != nullptr) {
            stmt += pszCondition;
            free (pszCondition);
        }
        pMatchmakingQualifier = pCMatchmakingQualifier->_qualifiers.getNext();
    }

    return stmt.r_str();
}

char * InformationStore::toSqlStatement (MatchmakingQualifiers *pMatchmakingQualifiers)
{
    String stmt;
    ComplexMatchmakingQualifier *pCMatchmakingQualifier = pMatchmakingQualifiers->_qualifiers.getFirst();
    bool bAnyDataFormat = false;
    DArray2<String> dataFormats;
    for (unsigned short i = 0; pCMatchmakingQualifier != nullptr; i++) {
        if (i > 0) {
            stmt += " UNION (";
        }
        stmt += toSqlConstraints (pCMatchmakingQualifier);
        if (i > 0) {
            stmt += ")";
        }
        const char *pszDataFormat = pCMatchmakingQualifier->getDataFormat();
        if (pszDataFormat == nullptr) {
            bAnyDataFormat = true;
        }
        else if (!bAnyDataFormat) {
            static unsigned int i = 0;
            dataFormats[i] = pszDataFormat;
            i++;
        }
        pCMatchmakingQualifier = pMatchmakingQualifiers->_qualifiers.getNext();
    }

    if (!bAnyDataFormat) {
        stmt += " UNION ( SELECT FROM WHERE ";
        stmt += MetaData::DATA_FORMAT;
        stmt += " NOT IN (";
        stmt += "))";
    }

    return stmt.r_str();
}

void InformationStore::clear (void)
{
    const char *pszMethodName = "InformationStore::clear";
    const String query ("DELETE FROM MetaData_Table WHERE Data_Format != '" + SessionId::MIME_TYPE + "';");
    char *pszErrMsg;

    _m.lock (1027);
    int rc = sqlite3_exec (_pSQL3DB, query, nullptr, nullptr, &pszErrMsg);
    if (rc == SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "MetaData_Table cleared\n");
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "could not clear table: %s\n",
                        pszErrMsg == nullptr ? "" : pszErrMsg);
        if (pszErrMsg) {
            sqlite3_free (pszErrMsg);
        }
    }
    _m.unlock (1027);
}

void InformationStore::createSpatialIndexes()
{
    const char *pszMethodName = "InformationStore::createSpatialIndexes";
    String sql;
    char *pszErrMsg;
    int rc;

    sql = (String) "CREATE INDEX IF NOT EXISTS LeftUpperLat_Idx ON " + _tableName
        +          "(" + MetaData::LEFT_UPPER_LATITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, nullptr, nullptr, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS LeftUpperLong_Idx ON "
        +          _tableName + "(" + MetaData::LEFT_UPPER_LONGITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, nullptr, nullptr, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS RightLowerLat_Idx ON "
        +          _tableName + "(" + MetaData::RIGHT_LOWER_LATITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, nullptr, nullptr, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS RightLowerLong_Idx ON "
        +          _tableName + "(" + MetaData::RIGHT_LOWER_LONGITUDE + ");";

    if ((rc = sqlite3_exec (_pSQL3DB, sql, nullptr, nullptr, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }
}
