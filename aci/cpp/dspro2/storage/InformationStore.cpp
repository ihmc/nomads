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
#include "SQLAVList.h"
#include "MatchmakingQualifier.h"
#include "MetaData.h"
#include "MetadataConfiguration.h"

#include "DataStore.h"
#include "SQLiteFactory.h"
#include "DSLib.h"

#include "Logger.h"
#include "MD5.h"
#include "NLFLib.h"
#include "StrClass.h"

#include "sqlite3.h"

#include "tinyxml.h"

#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_C45;
using namespace IHMC_MISC;
using namespace IHMC_ACI;

const char * InformationStore::DEFAULT_DATABASE_NAME = ":memory:";
const char * InformationStore::DEFAULT_METADATA_TABLE_NAME = "MetaData_Table";

InformationStore::InformationStore (DataStore *pDataCache, const char *pszDSProGroupName)
    : _pszMetadataDBName (NULL),
      _pszMetadataTableName (NULL),
      _pszMetadataPrimaryKey (NULL),
      _pszMetadataAll (NULL),
      _uiMetadataAll (0),
      _pSQL3DB (NULL),
      _psqlInserted (NULL),
      _psqlGetMetadata (NULL),
      _psqlGetReferringMetadata (NULL),
      _errorCode (0),
      _pDataStore (pDataCache),
      _pszDSProGroupName (pszDSProGroupName),
      _pszStartsWithDSProGroupNameTemplate (NULL),
      _pMetadataConf (NULL),
      _m (MutexId::InformationStore_m, LOG_MUTEX)
{
    String tmp (_pszDSProGroupName);
    tmp += "*";
    _pszStartsWithDSProGroupNameTemplate = tmp.r_str();
}

InformationStore::~InformationStore (void)
{
    // TODO : it doesn't compile on visual studio 2008. Debug it
    //while((_psqlInserted = sqlite3_next_stmt(_pSQL3DB, 0)) != 0) sqlite3_finalize(_psqlInserted);
    sqlite3_close(_pSQL3DB); _pSQL3DB = NULL;
    free (_pszMetadataDBName);
    _pszMetadataDBName = NULL;
    free (_pszMetadataTableName);
    _pszMetadataTableName = NULL;
    free (_pszMetadataAll);
    _pszMetadataAll = NULL;

    sqlite3_finalize (_psqlGetMetadata);
    _psqlGetMetadata = NULL;

    sqlite3_finalize (_psqlGetReferringMetadata);
    _psqlGetReferringMetadata = NULL;
}

int InformationStore::insertIntoDB (MetadataInterface *pMetadata)
{
    const char *pszMethodName = "InformationStore::insertIntoDB";
    if (_psqlInserted == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "ByteCode NULL\n");
        return -1;
    }

    _m.lock (1018);
    uint16 metadataFieldsNumber;
    const MetadataFieldInfo **ppMetadataFieldInfos = _pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);

    for (int i = 0; i < metadataFieldsNumber; i++) {
        if (1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::TEXT)) {
            char *pValue = NULL;
            int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[i]->_sFieldName, &pValue);
            if (rc == 0) {
                if ((i == _pMetadataConf->getMessageIdIndex()) || (i == _pMetadataConf->getRefersToIndex())) {
                    if (pValue == NULL || strcmp (pValue, MetadataValue::UNKNOWN) == 0) {
                        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                        "trying to add message with unknown message "
                                        "ID or refersTo field\n");
                        return -4;
                    }
                }
                if (sqlite3_bind_text (_psqlInserted, i + 1, pValue, sizeof(char) * strlen (pValue), SQLITE_TRANSIENT) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s with value = <%s>\n",
                                    ppMetadataFieldInfos[i]->_sFieldName.c_str(), pValue);
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
            else {
                assert ((ppMetadataFieldInfos[i]->_sFieldName == MetadataInterface::MESSAGE_ID) == 1 ? false : true);
                assert ((ppMetadataFieldInfos[i]->_sFieldName == MetadataInterface::REFERS_TO) == 1 ? false : true);
                if (sqlite3_bind_null (_psqlInserted, i + 1) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                    ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
            free (pValue);
            pValue = NULL;
        }
        else if ((1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER8)) ||
                 (1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER16)) ||
                 (1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER32))) {
            int value = -1;
            int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[i]->_sFieldName, &value);
            if (rc == 0) {
                if(sqlite3_bind_int (_psqlInserted, i + 1, value) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                    ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
            else {
                if (sqlite3_bind_null (_psqlInserted, i + 1) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "Could not bind field %s\n", ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
        }
        else if (1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::INTEGER64)) {
            int64 value = -1;
            int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[i]->_sFieldName, &value);
            if (rc == 0) {
                if (sqlite3_bind_int64 (_psqlInserted, i + 1, value) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Could not bind field %s\n",
                                    ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
            else {
                if(sqlite3_bind_null (_psqlInserted, i + 1) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "Could not bind field %s\n", ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
        }
        else if ((1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::FLOAT)) ||
                 (1 == (ppMetadataFieldInfos[i]->_sFieldType == MetadataType::DOUBLE))) {
            double value = -1;
            int rc = pMetadata->getFieldValue (ppMetadataFieldInfos[i]->_sFieldName, &value);
            if (rc == 0) {
                if (sqlite3_bind_double (_psqlInserted, i + 1, value) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "Could not bind field %s\n", ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
            else {
                if (sqlite3_bind_null (_psqlInserted, i + 1) != SQLITE_OK) {
                    checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                                    "Could not bind field %s\n", ppMetadataFieldInfos[i]->_sFieldName.c_str());
                    sqlite3_reset (_psqlInserted);
                    _m.unlock (1018);
                    return -4;
                }
            }
        }
    }

    int rc = sqlite3_step (_psqlInserted);
    switch (rc) {
        case SQLITE_OK:
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Insert successful\n");
            rc = 0;
            break;

        case SQLITE_DONE:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                            "Insert successful. the last row inserted is: %d\n",
                            sqlite3_last_insert_rowid (_pSQL3DB));
            rc = 0;
            break;

        default:
            checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Insert failed: %s\n",
                            SQLiteFactory::getErrorAsString (rc));
            rc = -5;
    }

    sqlite3_reset (_psqlInserted);
    _m.unlock (1018);
    return rc;
}

char ** InformationStore::getDSProIds (const char *pszObjectId, const char *pszInstanceId)
{
    if (pszObjectId == NULL) {
        return NULL;
    }

    String sql = "SELECT ";
    sql += MetaData::MESSAGE_ID;
    sql += " FROM ";
    sql += _pszMetadataTableName;
    sql += " WHERE ";
    sql += MetaData::REFERRED_DATA_OBJECT_ID;
    sql += " = '";
    sql += pszObjectId;
    sql += "'";
    if (pszInstanceId != NULL) {
        sql += " AND ";
        sql += MetaData::REFERRED_DATA_INSTANCE_ID;
        sql += " = '";
        sql += pszInstanceId;
        sql += "'";
    }
    sql += ";";

    return getDSProIds (sql.c_str());
}

char ** InformationStore::getDSProIds (const char *pszQuery)
{
    if (pszQuery == NULL) {
        return NULL;
    }
    unsigned int uiQueryLen = strlen (pszQuery);
    if (uiQueryLen == 0) {
        return NULL;
    }

    _m.lock (1019);

    sqlite3_stmt *psqlGetIDs = NULL;
    int rc = sqlite3_prepare_v2 (_pSQL3DB, pszQuery, uiQueryLen, &psqlGetIDs, NULL);
    if (rc != 0 || psqlGetIDs == NULL) {
        _m.unlock (1019);
        return NULL;
    }

    unsigned int uiRow = 0;
    DArray2<String> ids;
    while (((sqlite3_step (psqlGetIDs)) == SQLITE_ROW)) {
        if (sqlite3_column_type (psqlGetIDs, 0) == SQLITE_TEXT) {
            const char *pszValue = (const char *) sqlite3_column_text (psqlGetIDs, 0);
            if (pszValue != NULL) {
                ids[uiRow] = pszValue;
                uiRow++;
            }
        }
    }

    checkAndLogMsg ("InformationStore::getDSProIds", Logger::L_Info,
                    "%u dspro ids were found for query <%s>\n", ids.size(), pszQuery);

    char **ppszIds = NULL;
    if (ids.size() > 0) {
        ppszIds = (char **) calloc (ids.size() + 1, sizeof (char*));
        if (ppszIds == NULL) {
            checkAndLogMsg ("InformationStore::getDSProIds", memoryExhausted);
        }
        else {
            for (unsigned int i = 0; i < ids.size(); i++) {
                ppszIds[i] = ids[i].r_str();
            }
        }
    }

    sqlite3_finalize (psqlGetIDs);
  //  if (psqlGetIDs != NULL) {
//        free (psqlGetIDs);
    //}

    _m.unlock (1019);
    return ppszIds;
}

MetadataInterface * InformationStore::getMetadata (const char *pszKey)
{
    if (pszKey == NULL) {
        return NULL;
    }

    _m.lock (1019);
    if (_psqlGetMetadata == NULL) {
        // Create the prepared statement if it does not already exist
        String sql = "SELECT ";
        sql += _pszMetadataAll;
        sql += " FROM ";
        sql += _pszMetadataTableName;
        sql += " WHERE ";
        sql += _pszMetadataPrimaryKey;
        sql += " = ?;";
        int rc = sqlite3_prepare_v2 (_pSQL3DB, sql.c_str(), sql.length(),
                                     &_psqlGetMetadata, NULL);

        if ((rc == SQLITE_OK) && (_psqlGetMetadata != NULL)) {
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
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (sqlite3_bind_text (_psqlGetMetadata, 1, pszKey, strlen (pszKey), SQLITE_STATIC) != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::getMetadata", Logger::L_SevereError, "Error when binding values.\n");
        sqlite3_reset (_psqlGetReferringMetadata);
        _m.unlock (1019);
        return NULL;
    }

    MetadataList *pMetadataList = getMetadataInternal (_psqlGetMetadata);
    if (pMetadataList == NULL) {
        _m.unlock (1019);
        return NULL;
    }
    MetadataInterface *pMetadata = pMetadataList->getFirst();
    assert (pMetadataList->getNext() == NULL);
    delete pMetadataList;    
    _m.unlock (1019);
    return pMetadata;
}

MetadataList * InformationStore::getMetadataForData (const char *pszReferring)
{
    if (pszReferring == NULL) {
        return NULL;
    }

    _m.lock (1020);
    if (_psqlGetReferringMetadata == NULL) {
        // Create the prepared statement if it does not already exist
        String extract = "SELECT ";
        extract += _pszMetadataAll;
        extract += " FROM ";
        extract += _pszMetadataTableName;
        extract += " WHERE ";
        extract += MetaData::REFERS_TO;
        extract += " = ? ";
        extract += "LIMIT 1;";
        int rc = sqlite3_prepare_v2 (_pSQL3DB, extract.c_str(), extract.length(),
                                     &_psqlGetReferringMetadata, NULL);

        if ((rc == SQLITE_OK) && (_psqlGetReferringMetadata != NULL)) {
            checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_Info,
                            "Statement %s prepared successfully.\n", (const char *)extract);
        }
        else {
            checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_SevereError,
                            "Could not prepare statement: %s. Error code: %s\n",
                            (const char *)extract, SQLiteFactory::getErrorAsString (rc));
            sqlite3_reset (_psqlGetReferringMetadata);
            _m.unlock (1020);
            return NULL;
        }
    }

    // Bind the values to the prepared statement
    if (sqlite3_bind_text (_psqlGetReferringMetadata, 1, pszReferring, strlen (pszReferring), SQLITE_STATIC) != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::getMetadataForData", Logger::L_SevereError,
                        "Error when binding values.\n");
        sqlite3_reset (_psqlGetReferringMetadata);
        _m.unlock (1020);
        return NULL;
    }

    MetadataList *pMetadataList = getMetadataInternal (_psqlGetReferringMetadata);    
    _m.unlock (1020);
    return pMetadataList;
}

MetadataList * InformationStore::getMetadataInternal (sqlite3_stmt *pStmt, bool bAllowMultipleMatches)
{   
    unsigned short metadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = _pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    if (pMetadataFieldInfos == NULL || metadataFieldsNumber == 0) {
        return NULL;
    }

    // Execute the statement
    int rc;
    int rows = 0;
    MetadataList *pMetadataList = NULL;
    for (; ((rc = sqlite3_step (pStmt)) == SQLITE_ROW) && (bAllowMultipleMatches || (rows < 1)); rows++) {
        SQLAVList *pValues = new SQLAVList (metadataFieldsNumber);
        if (pValues != NULL) {
            for (int i = 0; i < metadataFieldsNumber; i ++) {
                if (sqlite3_column_type (pStmt, i) == SQLITE_NULL) {
                    pValues->addPair (pMetadataFieldInfos[i]->_sFieldName, SQLAVList::UNKNOWN);
                }
                else {
                    const char *pszValue = (const char *) sqlite3_column_text (pStmt, i);
                    if (pszValue != NULL) {
                        pValues->addPair (pMetadataFieldInfos[i]->_sFieldName, pszValue);
                    }
                    else {
                        pValues->addPair (pMetadataFieldInfos[i]->_sFieldName, SQLAVList::UNKNOWN);
                    }
                }
            }
            if (pMetadataList == NULL) {
                pMetadataList = new MetadataList();
            }
            if (pMetadataList != NULL) {
                pMetadataList->prepend (pValues);
            }
        }
    }

    if ((rc != SQLITE_ROW) && (rc != SQLITE_DONE)) {
        checkAndLogMsg ("SQLMessageHeaderStorage::getMetadataInternal", Logger::L_SevereError,
                        "sqlite3_step returned %s.\n", SQLiteFactory::getErrorAsString(rc));
    }
    else if (rows == 0) {
        checkAndLogMsg ("InformationStore::getMetadataInternal", Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query\n");
    }
    else if (rows > 1 && !bAllowMultipleMatches) {
        checkAndLogMsg ("InformationStore::getMetadataInternal", Logger::L_MildError,
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
    if (pMetadataFieldInfos == NULL || pMetadataFieldInfos[uiReceiverTimeStampIndex] == NULL) {
        return NULL;
    }
    const char *pszReceiverTimeStampFieldName = pMetadataFieldInfos[uiReceiverTimeStampIndex]->_sFieldName;

    String sql = "SELECT ";
    sql += _pszMetadataAll;
    sql += " FROM ";
    sql += _pszMetadataTableName;

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

    if (pAVQueryList != NULL && !pAVQueryList->isEmpty()) {
        unsigned int uiLen = (unsigned int) pAVQueryList->getLength();
        for (unsigned int i = 0; i < uiLen; i++) {
            const char *pszAttribute = pAVQueryList->getAttribute (i);
            const char *pszValue = pAVQueryList->getValueByIndex (i);
            const char *pszFieldType = _pMetadataConf->getFieldType (pszAttribute);
            if (pszAttribute != NULL && pszValue != NULL && pszFieldType != NULL) {

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
    MetadataList *pMetadataList = getAllMetadata ((const char *) sql, _uiMetadataAll);
    _m.unlock (1021);

    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadataInArea (MatchmakingQualifiers *pMatchmakingQualifiers,
                                                       const char **ppszMessageIdFilters,
                                                       float fMaxLat, float fMinLat, float fMaxLong, float fMinLong)
{
    if (fMinLat > fMaxLat) {
        float tmp = fMaxLat;
        fMaxLat = fMinLat;
        fMinLat = tmp;
    }
    if (fMinLong > fMaxLong) {
        float tmp = fMaxLong;
        fMaxLong = fMinLong;
        fMinLong = tmp;
    }

    String extract = "SELECT ";
    extract += _pszMetadataAll;
    extract += " FROM ";

    if (pMatchmakingQualifiers != NULL &&
        pMatchmakingQualifiers->_qualifiers.getFirst() != NULL) {
        extract += "(";
        extract += toSqlStatement (pMatchmakingQualifiers);
        extract +=  ")";
    }
    else {
        extract += _pszMetadataTableName;
    }

    if ((ppszMessageIdFilters != NULL) && (ppszMessageIdFilters[0] != NULL)) {
        extract += (String) " WHERE " + MetadataInterface::MESSAGE_ID + " NOT IN (";
        for (int i = 0; ppszMessageIdFilters[i] != NULL; i++) {
            if (i > 0) {
                extract += ", ";
            }
            extract += (String) "'" + ppszMessageIdFilters[i] + "'";
        }
        extract += ")";
    }
    extract += (((ppszMessageIdFilters != NULL) && (ppszMessageIdFilters[0] != NULL)) ? " AND " : " WHERE ");

    // Metadata's bounding-box
    const char *pszMetaMaxLat = MetadataInterface::LEFT_UPPER_LATITUDE;
    const char *pszMetaMinLat = MetadataInterface::RIGHT_LOWER_LATITUDE;
    const char *pszMetaMaxLong = MetadataInterface::RIGHT_LOWER_LONGITUDE;
    const char *pszMetaMinLong = MetadataInterface::LEFT_UPPER_LONGITUDE;

    // Area's bounding-box
    uint8 ui8BufLen = 20;
    char *pszMaxLat = (char *) calloc (ui8BufLen, sizeof (char));
    DSLib::floatToString (pszMaxLat, ui8BufLen, fMaxLat);
    char *pszMinLat = (char *) calloc (ui8BufLen, sizeof (char));
    DSLib::floatToString (pszMinLat, ui8BufLen, fMinLat);
    char *pszMaxLong = (char *) calloc (ui8BufLen, sizeof (char));
    DSLib::floatToString (pszMaxLong, ui8BufLen, fMaxLong);
    char *pszMinLong = (char *) calloc (ui8BufLen, sizeof (char));
    DSLib::floatToString (pszMinLong, ui8BufLen, fMinLong);

    extract += (String) " ("
                            // At least one corner of the metadata's bounding-box
                            // is included in the path's bounding-box
            +              "(((" + pszMetaMinLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ") OR "
            +               "(" + pszMetaMaxLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + "))"
            +                                                         " AND "
            +             "((" + pszMetaMinLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ") OR "
            +              "(" + pszMetaMaxLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ")))"
            +                                                            " OR "
                            // The metadata's bounding-box includes, or partially
                            // includes, the path's bounding-box (1)
            +              "((" + pszMetaMinLat + " <= " + pszMinLat + " AND " +  pszMetaMaxLat + " >= " + pszMaxLat + ") AND "
            +              "    ("
            +                     // Partial overlapping
            +                     "("
            +                         "(" + pszMetaMinLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ") OR "
            +                         "(" + pszMetaMaxLong + " BETWEEN " + pszMinLong + " AND " + pszMaxLong + ")"
            +                     ")" +                            " OR "
            +                     // Total overlapping
            +                     "(" + pszMetaMinLong + " <= " + pszMinLong + " AND " +  pszMetaMaxLong + " >= " + pszMaxLong + ")"
            +                  ") "
            +               ")"
            +                                                            " OR "
                           // The metadata's bounding-box includes, or partially
                           // includes, the path's bounding-box (2)
            +              "((" + pszMetaMinLong + " <= " + pszMinLong + " AND " +  pszMetaMaxLong + " >= " + pszMaxLong + ") AND "
            +              "    ("
            +                     // Partial overlapping
            +                     "("
            +                         "(" + pszMetaMinLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ") OR "
            +                         "(" + pszMetaMaxLat + " BETWEEN " + pszMinLat + " AND " + pszMaxLat + ")"
            +                     ")" +                            " OR "
            +                     // Total overlapping
            +                     "(" + pszMetaMinLat + " <= " + pszMinLat + " AND " +  pszMetaMaxLat + " >= " + pszMaxLat + ")"
            +                  ") "
            +               ")"
            +                                                            " OR "
                         "(" + MetadataInterface::LEFT_UPPER_LATITUDE + " IS NULL AND " + MetadataInterface::RIGHT_LOWER_LATITUDE + " IS NULL AND "
            +                  MetadataInterface::RIGHT_LOWER_LONGITUDE + " IS NULL AND " + MetadataInterface::LEFT_UPPER_LONGITUDE + " IS NULL)" +

            +            ")";

    extract += ";";

    _m.lock (1021);
    MetadataList *pMetadataList = getAllMetadata ((const char *) extract, _uiMetadataAll);

    free (pszMaxLat);       free (pszMinLat);
    free (pszMaxLong);      free (pszMinLong);
    pszMaxLat = pszMinLat = pszMaxLong = pszMinLong = NULL;

    _m.unlock (1021);
    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadata (const char **ppszMessageIdFilters, bool bExclusiveFilter)
{
    String extract = "SELECT ";
    extract += _pszMetadataAll;
    extract += " FROM ";
    extract += _pszMetadataTableName;
    if ((ppszMessageIdFilters != NULL) && (ppszMessageIdFilters[0] != NULL)) {
        extract += (String) " WHERE " + MetaData::MESSAGE_ID;
        extract += (bExclusiveFilter ? " NOT IN (" : " IN (");
        for (int i = 0; ppszMessageIdFilters[i] != NULL; i++) {
            if (i > 0) {
                extract += ", ";
            }
            extract += (String) "'" + ppszMessageIdFilters[i] + "'";
        }
        extract += ")";
    }
    extract += ";";

    _m.lock (1022);
    MetadataList *pMetadataList = getAllMetadata ((const char *) extract, _uiMetadataAll);
    _m.unlock (1022);

    return pMetadataList;
}

MetadataList * InformationStore::getAllMetadata (const char *pszSQL, unsigned int uiExpectedNumberOfColumns)
{
    char *pErrMessage = NULL;
    char **ppQueryResults;
    int rc, noRows, noColumns;
    rc = sqlite3_get_table (_pSQL3DB, pszSQL, &ppQueryResults, &noRows, &noColumns, &pErrMessage);
    if (rc != SQLITE_OK) {
        checkAndLogMsg ("InformationStore::getAllMetadata", Logger::L_SevereError,
                        "sqlite3_get_table() failed with return value = %d; query = %s; error msg = <%s>\n",
                        rc, pszSQL, pErrMessage);
        sqlite3_free (pErrMessage);
        sqlite3_free_table (ppQueryResults);
        return NULL;
    }
    if (noRows == 0) {
        checkAndLogMsg ("InformationStore::getAllMetadata", Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query %s\n", pszSQL);
        sqlite3_free_table(ppQueryResults);
        return NULL;
    }
    if ((noColumns < 0) || (((unsigned int)noColumns) != uiExpectedNumberOfColumns)) {
        checkAndLogMsg ("InformationStore::getAllMetadata", Logger::L_MildError,
                        "sqlite3_get_table() returned %d columns for query %s\n but there should have been %d\n",
                        noColumns, (const char *) pszSQL, uiExpectedNumberOfColumns);
        sqlite3_free_table (ppQueryResults);
        return NULL;
    }

    unsigned short metadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = _pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    MetadataList *pMetadataList = new MetadataList();
    for (int i = 1; i <= noRows; i ++) {
        SQLAVList *pSQLAVList = new SQLAVList (noColumns);
        if (pSQLAVList != NULL) {
            unsigned int uiColIdx = 0;
            for (int j = 0; j < noColumns && uiColIdx < _uiMetadataAll; j++) {
                if (ppQueryResults[noColumns * i + j] == NULL) {
                    pSQLAVList->addPair (pMetadataFieldInfos[uiColIdx]->_sFieldName, SQLAVList::UNKNOWN);
                }
                else {
                    if (strcmp (pMetadataFieldInfos[uiColIdx]->_sFieldName, MetaData::IMPORTANCE) == 0) {
                        checkAndLogMsg ("IMPORTANCE", Logger::L_Info, "the value of importance is %s\n",
                                        ppQueryResults[noColumns*i + j] == NULL ? "NULL" : ppQueryResults[noColumns*i + j]);
                        pSQLAVList->addPair (pMetadataFieldInfos[uiColIdx]->_sFieldName, SQLAVList::UNKNOWN);
                    }
                    else {
                        pSQLAVList->addPair (pMetadataFieldInfos[uiColIdx]->_sFieldName, ppQueryResults[noColumns*i + j]);
                    }
                }
                uiColIdx++;
            }
            pMetadataList->prepend (pSQLAVList);
        }
    }

    sqlite3_free_table (ppQueryResults);
    return pMetadataList;
}

PtrLList<const char> * InformationStore::getMessageIDs (const char *pszGroupName, const char *pszSqlConstraints, char **ppszFilters)
{
    if (pszSqlConstraints == NULL) {
        return NULL;
    }

    String sql = "SELECT ";
    sql = sql + MetaData::MESSAGE_ID;
    sql = sql + " FROM ";
    sql = sql + _pszMetadataTableName;
    sql = sql + " WHERE ";
    sql = sql + pszSqlConstraints;

    if ((ppszFilters != NULL) && (ppszFilters[0] != NULL)) {
        sql += (String) " AND " + MetaData::MESSAGE_ID + " NOT IN (";
        for (int i = 0; ppszFilters[i] != NULL; i++) {
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
        return NULL;
    }
    if (noRows == 0) {
        checkAndLogMsg ("InformationStore::extractMessageIDsFromDBBase", Logger::L_LowDetailDebug,
                        "sqlite3_get_table() did not find any matches for query <%s>\n", (const char *) pszSQL);
        sqlite3_free_table(ppzsQueryResults);
        _errorCode = 7;
        return NULL;
    }
    if (noColumns != 1) {
        checkAndLogMsg ("InformationStore::extractMessageIDsFromDBBase", Logger::L_MildError,
                        "sqlite3_get_table() returned %d columns for query <%s> but there should have been 1\n",
                        noColumns, (const char *) pszSQL);
        sqlite3_free_table(ppzsQueryResults);
        _errorCode = 6;
        return NULL;
    }

    // Verify that the selected messages exist and that it belongs to the
    // specified group
    _pDataStore->lock();
    PtrLList<const char> *pResultsList = NULL;
    for (int i = 0; i < noRows; i++) {
        if (wildcardStringCompare (ppzsQueryResults[i + 1],
                                   _pszStartsWithDSProGroupNameTemplate)) {
            if (_pDataStore->isMetadataMessageStored (ppzsQueryResults[i + 1])) {
                if (pResultsList == NULL) {
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
    if (pszKey == NULL) {
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
    update += _pszMetadataTableName;
    update += " WHERE ";
    update += _pszMetadataPrimaryKey;
    update += " = '";
    update += pszKey;
    update += "';";
    char *pszErrMsg = NULL;
    char **ppszQueryResults = NULL;
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
    if (ppszQueryResults[noColumns] != NULL) {
        sqlite3_free_table (ppszQueryResults);
        _m.unlock (1024);
        return 0;
    }
	if (ppszQueryResults != NULL) {
        sqlite3_free_table (ppszQueryResults);
    }

    update = "UPDATE ";
    update += _pszMetadataTableName;
    update += " SET ";
    update += MetaData::USAGE;
    update += " = ";
    update += buffer;
    update += " WHERE ";
    update += _pszMetadataPrimaryKey;
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
    if (pszFieldName == NULL) {
        _errorCode = 8;
        return _errorCode;
    }
    String count = "SELECT COUNT(*) FROM ";
    count += _pszMetadataTableName;
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
    if (pszKey == NULL) {
        _errorCode = 8;
        _m.unlock (1026);
        return _errorCode;
    }
    String deletesql = "DELETE FROM ";
    deletesql += _pszMetadataTableName;
    deletesql += " WHERE ";
    deletesql += _pszMetadataPrimaryKey;
    deletesql += " = '";
    deletesql += pszKey;
    deletesql += "';";
    checkAndLogMsg ("InformationStore::deleteMetaDataFromDB", Logger::L_Info,
                    "The query is %s\n", (const char *) deletesql);
    char *pszErrMsg;
    if (sqlite3_exec (_pSQL3DB, deletesql, NULL, NULL, &pszErrMsg) != SQLITE_OK) {
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

int InformationStore::init (MetadataConfiguration *pMetadataConf,
                            const char *pszMetadataTableName,
                            const char *pszMetadataDBName)
{
    if (pMetadataConf == NULL) {
        return -1;
    }

    _pMetadataConf = pMetadataConf;

    _psqlGetMetadata = NULL;
    _psqlGetReferringMetadata = NULL;

    // initialize DB and metadata table
    if (pszMetadataDBName == NULL) {
        _pszMetadataDBName = strDup (DEFAULT_DATABASE_NAME);
    }
    else {
        _pszMetadataDBName = strDup (pszMetadataDBName);
    }
    if (pszMetadataTableName == NULL) {
        _pszMetadataTableName = strDup (DEFAULT_METADATA_TABLE_NAME);
    }
    else {
        _pszMetadataTableName = strDup (pszMetadataTableName);
    }
    
    // Set ALL
    uint16 metadataFieldsNumber = 0;
    const MetadataFieldInfo **ppMetadataFieldInfos = pMetadataConf->getMetadataFieldInfos (metadataFieldsNumber);
    if (ppMetadataFieldInfos == NULL) {
        return -2;
    }
    String metadataAll;
    _uiMetadataAll = 0;
    for (int i = 0; i < metadataFieldsNumber; i++) {
        if (i != 0) {
            metadataAll += ",";
        }
        metadataAll += ppMetadataFieldInfos[i]->_sFieldName;
        _uiMetadataAll++;
    }
    _pszMetadataAll = metadataAll.r_str();
    if (_pszMetadataAll == NULL) {
        checkAndLogMsg ("InformationStore::init", memoryExhausted);
        return -3;
    }

    // Set PRIMARY KEY
    _pszMetadataPrimaryKey = strDup (MetaData::MESSAGE_ID);
    if (_pszMetadataAll == NULL) {
        checkAndLogMsg ("InformationStore::init", memoryExhausted);
        return -4;
    }

    return openDataBase (pMetadataConf);
}

int InformationStore::openDataBase (MetadataConfiguration *pMetadataConf)
{
    const char *pszMethodName = "InformationStore::openDataBase";

    // open database
    _errorCode = 0;
    _pSQL3DB = SQLiteFactory::getInstance(); // It assumes that the database
                                             // has been already opened/created
                                             // by DisService.
    if (_pSQL3DB == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError, "Can't open database.\n");
        sqlite3_close (_pSQL3DB);
        _pSQL3DB = NULL;
        _errorCode = 1;
        return _errorCode;
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "Database %s successfully open\n",
                    _pszMetadataDBName);

    uint16 ui16MetadataFieldsNumber = 0;
    const MetadataFieldInfo **pMetadataFieldInfos = pMetadataConf->getMetadataFieldInfos (ui16MetadataFieldsNumber);
    unsigned short usMessageIDIndex = pMetadataConf->getMessageIdIndex();

    // create metadata table
    String table ("CREATE TABLE IF NOT EXISTS ");
    table += _pszMetadataTableName;
    table += " (";
    table += _pszMetadataPrimaryKey;
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
    table += ");";
    char *errMsg;
    int rc = sqlite3_exec (_pSQL3DB, table, NULL, NULL, &errMsg);
    switch (rc) {
        case SQLITE_OK:
        {
            checkAndLogMsg (pszMethodName, Logger::L_Info, "Creating table by query:\n<%s>\nrun successfully\n", table.c_str());
            char *pszChecksum = MD5Utils::getMD5Checksum (table.c_str(), table.length());
            if (pszChecksum != NULL) {
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
    tryInsert += _pszMetadataTableName;
    tryInsert += " ( ";
    tryInsert += _pszMetadataAll;
    tryInsert += " ) VALUES ( ";
    for (uint16 i = 0; i < ui16MetadataFieldsNumber - 1; i++) {
        tryInsert += " ?,";
    }
    tryInsert += " ?);";
    rc = sqlite3_prepare_v2 (_pSQL3DB, tryInsert, strlen (tryInsert), &_psqlInserted, NULL);
    if (rc == SQLITE_OK && _psqlInserted != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "Preparing statement for insert query:"
                        "\n<%s>\nrun successfully\n", (const char *) tryInsert);
    }
    else {
        checkAndLogMsg ("InformationStore::openDataBase", Logger::L_SevereError,
                        "Preparing statement for insert query\n<%s>\n"
                        "failed with error code %d\n", tryInsert.c_str(), rc);
        sqlite3_reset (_psqlInserted);
        _psqlInserted = NULL;
        _errorCode = 3;
    }
    return _errorCode;
}

char * InformationStore::toSqlConstraint (MatchmakingQualifier *pMatchmakingQualifier)
{
    if (pMatchmakingQualifier == NULL) {
        return NULL;
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
        innerSql       += _pszMetadataTableName;
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
        stmt +=   _pszMetadataTableName;
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
    stmt += _pszMetadataAll;
    stmt += " FROM ";
    stmt += _pszMetadataTableName;

    MatchmakingQualifier *pMatchmakingQualifier = pCMatchmakingQualifier->_qualifiers.getFirst();
    for (unsigned short i = 0; pMatchmakingQualifier != NULL; i++) {
        stmt += (i == 0) ? " WHERE " : " AND ";
        char *pszCondition = toSqlConstraint (pMatchmakingQualifier);
        if (pszCondition != NULL) {
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
    for (unsigned short i = 0; pCMatchmakingQualifier != NULL; i++) {
        if (i > 0) {
            stmt += " UNION (";
        }
        stmt += toSqlConstraints (pCMatchmakingQualifier);
        if (i > 0) {
            stmt += ")";
        }
        const char *pszDataFormat = pCMatchmakingQualifier->getDataFormat();
        if (pszDataFormat == NULL) {
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

void InformationStore::createSpatialIndexes()
{
    const char *pszMethodName = "InformationStore::createSpatialIndexes";
    String sql;
    char *pszErrMsg;
    int rc;

    sql = (String) "CREATE INDEX IF NOT EXISTS LeftUpperLat_Idx ON " + _pszMetadataTableName
        +          "(" + MetaData::LEFT_UPPER_LATITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, NULL, NULL, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS LeftUpperLong_Idx ON "
        +          _pszMetadataTableName + "(" + MetaData::LEFT_UPPER_LONGITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, NULL, NULL, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS RightLowerLat_Idx ON "
        +          _pszMetadataTableName + "(" + MetaData::RIGHT_LOWER_LATITUDE + ");";
    if ((rc = sqlite3_exec (_pSQL3DB, sql, NULL, NULL, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }

    sql = (String) "CREATE INDEX IF NOT EXISTS RightLowerLong_Idx ON "
        +          _pszMetadataTableName + "(" + MetaData::RIGHT_LOWER_LONGITUDE + ");";

    if ((rc = sqlite3_exec (_pSQL3DB, sql, NULL, NULL, &pszErrMsg)) != SQLITE_OK) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning,
                        "Could not create index = <%s>\n", (const char *) sql);
        return;
    }
}

