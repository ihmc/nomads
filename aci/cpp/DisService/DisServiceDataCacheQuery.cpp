/*
 * DisServiceDataCacheQuery.cpp
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
 */

#include "DisServiceDataCacheQuery.h"

#include "DisServiceMsg.h"
//#include "StorageInterface.h"
#include "SQLMessageHeaderStorage.h"

#include "DArray.h"
#include "Logger.h"
#include "PtrLList.h"
#include "Reader.h"
#include "Writer.h"
#include "DSSFLib.h"

#include <stdarg.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_ACI;
using namespace NOMADSUtil;

DisServiceDataCacheQuery::DisServiceDataCacheQuery()
{
    reset();
}

DisServiceDataCacheQuery::~DisServiceDataCacheQuery()
{
    if (_customConstraint != NULL) {
        delete _customConstraint;
        _customConstraint = NULL;
    }
}

void DisServiceDataCacheQuery::reset()
{
    _bConstrGroupName = _bConstrSenderNodeId = _bSeqIdGreaterThan = _bSeqIdSmallerThan =
    _bSeqIdGreaterThanOrEqual = _bSeqIdSmallerThanOrEqual = _bSeqIdIs = _bConstrTag =
    _bConstrClientId = _bConstrClientType = _bConstrPriority = _bConstrFragment =
    _bFragRangeExactMatch = _bChunkId = _bChunkIdSmallerThanOrEqual = _bExpirationLowerThreshold = _bExpirationUpperThreshold =
    _bArrivalLowerThreshold = _bArrivalUpperThreshold = _bUtilityLowerThreshold =
    _bUtilityUpperThreshold = _bExcludeChunkIDs = _bExcludeRange = _bResultLimit = _bIncludeData =
    _isOrderSet = _bNoFragments = _bDescOrder = _bAllFields = _bMetadataFields =
    _bDeleteOperation = _bAddCustomConstraint = _bPersistent = _bSelectRowId = _bMsgIdNotIn = _bGrpPubSeqIdNotIn = _bMsgId = false;

    _bPrimaryKey = true;
    _customConstraint = NULL;
    _ui8TableType = _ui8NumOfClientIds = _ui8ChunkId = _ui8OperationType =
    _ui8NumOfClientType = 0;

    _sqlQuery = "";
}

void DisServiceDataCacheQuery::setPersistent()
{
    _bPersistent = true;
}

void DisServiceDataCacheQuery::selectOperation (uint8 ui8OperationType)
{
    _ui8OperationType = ui8OperationType;
    if (ui8OperationType == DELETE_OP) {
        _bDeleteOperation = true;
    }
}

void DisServiceDataCacheQuery::selectTable (uint8 ui8TableType)
{
    _ui8TableType = ui8TableType;
}

void DisServiceDataCacheQuery::selectAll (void)
{
    _bAllFields = true;
    _bPrimaryKey = false;
}

void DisServiceDataCacheQuery::selectMetadataFields (void)
{
    _bMetadataFields = true;
    _bPrimaryKey = false;
}

void DisServiceDataCacheQuery::selectPrimaryKey (void)
{
    _bPrimaryKey = true;
}

void DisServiceDataCacheQuery::selectRowId (void)
{
    _bSelectRowId = true;
    _bPrimaryKey = false;
}

void DisServiceDataCacheQuery::addCustomConstraint (NOMADSUtil::String *pszCondition)
{
    _bAddCustomConstraint = true;
    _customConstraint = pszCondition;
}

void DisServiceDataCacheQuery::addConstraintOnGroupName (const char *pszGroupName)
{
    _bConstrGroupName = true;
    _constrGroupName = pszGroupName;
}

void DisServiceDataCacheQuery::addConstraintOnSenderNodeId (const char *pszSenderNodeId)
{
    _bConstrSenderNodeId = true;
    _constrSenderNodeId = pszSenderNodeId;
}

void DisServiceDataCacheQuery::addConstraintOnMsgIdNotIn (const char **ppszMessageIdFilters)
{
    _bMsgIdNotIn = true;
    _constrMessageIdFilters = ppszMessageIdFilters;
}

void DisServiceDataCacheQuery::addConstraintOnGrpPubSeqIdNotIn (StringHashtable<ReceivedMessages::ReceivedMsgsByGrp> *pMessageIdFilters)
{
    _bGrpPubSeqIdNotIn = true;
    _pConstrGrpPubSeqIdFilters = pMessageIdFilters;
}

void DisServiceDataCacheQuery::addConstraintOnMsgId (const char *pszMessageId)
{
    _bMsgId = true;
    _constrMessageId = pszMessageId;
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqId (uint32 ui32SeqId)
{
    _bSeqIdIs = true;
    addConstraintOnMsgSeqId (ui32SeqId, ui32SeqId);
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqId (uint32 ui32SeqIdFrom, uint32 ui32SeqIdTo)
{
    if (ui32SeqIdFrom == ui32SeqIdTo) {
        _ui32SeqIdFrom = ui32SeqIdFrom;
    }
    else {
        addConstraintOnMsgSeqIdGreaterThan (ui32SeqIdFrom);
        addConstraintOnMsgSeqIdSmallerThan (ui32SeqIdTo);
    }
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqId (uint32 ui32SeqIdFrom, uint32 ui32SeqIdTo, bool bOpenInterval)
{
    if (ui32SeqIdFrom == ui32SeqIdTo) {
        _ui32SeqIdFrom = ui32SeqIdFrom;
    }
    else {
        if (bOpenInterval) {
            addConstraintOnMsgSeqIdGreaterThanOrEqual (ui32SeqIdFrom);
            addConstraintOnMsgSeqIdSmallerThanOrEqual (ui32SeqIdTo);
        }
        else {
            addConstraintOnMsgSeqIdGreaterThan (ui32SeqIdFrom);
            addConstraintOnMsgSeqIdSmallerThan (ui32SeqIdTo);
        }
    }
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqIdGreaterThan (uint32 ui32SeqId)
{
    _bSeqIdIs = false;
    _bSeqIdGreaterThan = true;
    _ui32SeqIdFrom = ui32SeqId;
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqIdSmallerThan (uint32 ui32SeqId)
{
    _bSeqIdIs = false;
    _bSeqIdSmallerThan = true;
    _ui32SeqIdTo = ui32SeqId;
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqIdGreaterThanOrEqual (uint32 ui32SeqId)
{
    _bSeqIdIs = false;
    _bSeqIdGreaterThanOrEqual = true;
    _ui32SeqIdFrom = ui32SeqId;
}

void DisServiceDataCacheQuery::addConstraintOnMsgSeqIdSmallerThanOrEqual (uint32 ui32SeqId)
{
    _bSeqIdIs = false;
    _bSeqIdSmallerThanOrEqual = true;
    _ui32SeqIdTo = ui32SeqId;
}

void DisServiceDataCacheQuery::addConstraintOnChunkId (uint8 ui8ChunkId)
{
    _bChunkId = true;
    _ui8ChunkId = ui8ChunkId;
}

void DisServiceDataCacheQuery::addConstraintOnChunkIdSmallerThanOrEqual (uint8 ui8ChunkId)
{
    _bChunkId = false;
    _bChunkIdSmallerThanOrEqual = true;
    _ui8ChunkIdTo = ui8ChunkId;
}

void DisServiceDataCacheQuery::addConstraintOnTag (uint16 ui16Tag)
{
    _bConstrTag = true;
    addConstraintOnTag (1, ui16Tag);
}

void DisServiceDataCacheQuery::addConstraintOnClientId (uint16 ui16ClientId)
{
    _bConstrClientId = true;
    addConstraintOnClientId (1, ui16ClientId);
}

void DisServiceDataCacheQuery::addConstraintOnClientType (uint16 ui8ClientType)
{
    _bConstrClientType = true;
    addConstraintOnClientType (1, ui8ClientType);
}

void DisServiceDataCacheQuery::addConstraintOnPriority (uint8 ui8Priority)
{
    _bConstrPriority = true;
    addConstraintOnPriority (ui8Priority, ui8Priority);
}

void DisServiceDataCacheQuery::addConstraintOnPriority (uint8 ui8PriorityFrom, uint8 ui8PriorityTo)
{
    _bConstrPriority = true;
    _ui8PriorityFrom = ui8PriorityFrom;
    _ui8PriorityTo = ui8PriorityTo;
}

void DisServiceDataCacheQuery::setNoFragments (bool bNoFrag)
{
    _bNoFragments = bNoFrag;
    _bConstrFragment = false;
}

void DisServiceDataCacheQuery::addConstraintOnFragment (uint32 ui32FragOffset, uint32 ui32FragLength, bool bFragRangeExactMatch)
{
    _bConstrFragment = true;
    _bNoFragments = false;
    _bFragRangeExactMatch = bFragRangeExactMatch;
    _ui32FragOffset = ui32FragOffset;
    _ui32FragLength = ui32FragLength;
}

void DisServiceDataCacheQuery::excludeChunks (uint8 ui8ChunkIdStart, uint8 ui8ChunkIdEnd)
{
    _bExcludeRange = true;
    _ui8ChunkIdStart = ui8ChunkIdStart;
    _ui8ChunkIdEnd = ui8ChunkIdEnd;
}

void DisServiceDataCacheQuery::excludeChunks (NOMADSUtil::DArray<uint8> *pChunkIDs)
{
    _pChunkIDs = pChunkIDs;
    _bExcludeChunkIDs = true;
}

void DisServiceDataCacheQuery::addConstraintOnExpirationTimeGreaterThan (int64 i64ExpirationLowerThreshold)
{
    _bExpirationLowerThreshold = true;
    _i64ExpirationLowerThreshold = i64ExpirationLowerThreshold;
}

void DisServiceDataCacheQuery::addConstraintOnExpirationTimeLowerThan (int64 i64ExpirationUpperThreshold)
{
    _bExpirationUpperThreshold = true;
    _i64ExpirationUpperThreshold = i64ExpirationUpperThreshold;
}

void DisServiceDataCacheQuery::addConstraintOnArrivalTimeGreaterThan (int64 i64ArrivalLowerThreshold)
{
    _bArrivalLowerThreshold = true;
    _i64ArrivalLowerThreshold = i64ArrivalLowerThreshold;
}

void DisServiceDataCacheQuery::addConstraintOnArrivalTimeLowerThan (int64 i64ArrivalUpperThreshold)
{
    _bArrivalUpperThreshold = true;
    _i64ArrivalUpperThreshold = i64ArrivalUpperThreshold;
}

/*
void DisServiceDataCacheQuery::setNoMetaData (bool bNoMeta)
{
    _bMetaData = true;
    _bNoMeta = bNoMeta;
}

void DisServiceDataCacheQuery::setOnlyMetadata (bool bOnlyMeta)
{
    _bMetaData = true;
    _bOnlyMeta = bOnlyMeta;
}
*/

void DisServiceDataCacheQuery::setIncludeData (bool bIncludeData)
{
    _bIncludeData = bIncludeData;
}

void DisServiceDataCacheQuery::addConstraintOnUtilityGreaterThan (int8 i8UtilityLowerThreshold)
{
    _bUtilityLowerThreshold = true;
    _i8UtilityLowerThreshold = i8UtilityLowerThreshold;
}

void DisServiceDataCacheQuery::addConstraintOnUtilityLowerThan (int8 i8UtilityUpperThreshold)
{
    _bUtilityUpperThreshold = true;
    _i8UtilityUpperThreshold = i8UtilityUpperThreshold;
}

void DisServiceDataCacheQuery::setLimit (uint16 ui16ResultLimit)
{
    _bResultLimit = true;
    _ui16ResultLimit = ui16ResultLimit;
}

bool DisServiceDataCacheQuery::isLimitSet()
{
    return _bResultLimit;
}

uint16 DisServiceDataCacheQuery::getLimit()
{
    return _ui16ResultLimit;
}

void DisServiceDataCacheQuery::addConstraintOnTag (uint8 ui8NumOfTags, ...)
{
    _bConstrTag = true;
    _ui8NumOfTags = ui8NumOfTags;
    if (ui8NumOfTags > 0) {
        va_list vl;
        va_start(vl, ui8NumOfTags);
        for (uint8 i = 0; i < ui8NumOfTags; i++) {
            int value = va_arg(vl, int);
            _tags[i] = (uint16) value;
        }
    }
}

void DisServiceDataCacheQuery::addConstraintOnClientId (uint8 ui8NumOfClientIds, ...)
{
    _bConstrClientId = true;
    _ui8NumOfClientIds = ui8NumOfClientIds;
    if (ui8NumOfClientIds > 0) {
        va_list vl;
        va_start(vl, ui8NumOfClientIds);
        for (uint8 i = 0; i < ui8NumOfClientIds; i++) {
            int value = va_arg(vl, int);
            _clientIds[i] = (uint16) value;
        }
    }
}

void DisServiceDataCacheQuery::addConstraintOnClientType (uint8 ui8NumOfClientTypes, ...)
{
    _bConstrClientType = true;
    _ui8NumOfClientType = ui8NumOfClientTypes;
    if (ui8NumOfClientTypes > 0) {
        va_list vl;
        va_start(vl, ui8NumOfClientTypes);
        for (uint8 i = 0; i < ui8NumOfClientTypes; i++) {
            int value = va_arg(vl, int);
            _clientTypes[i] = (uint8) value;
        }
    }
}

int DisServiceDataCacheQuery::setOrderBy (const char *pszField, bool bDesc)
{
    if ((SQLMessageHeaderStorage::FIELD_GROUP_NAME == pszField) ||
        (SQLMessageHeaderStorage::FIELD_SENDER_ID == pszField) ||
        (SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID == pszField) ||
        (SQLMessageHeaderStorage::FIELD_TAG == pszField) ||
        (SQLMessageHeaderStorage::FIELD_CHUNK_ID == pszField) ||
        (SQLMessageHeaderStorage::FIELD_CLIENT_ID == pszField) ||
        (SQLMessageHeaderStorage::FIELD_CLIENT_TYPE == pszField) ||
        (SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH == pszField) ||
        (SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH == pszField) ||
        (SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET == pszField) ||
        (SQLMessageHeaderStorage::FIELD_METADATA_LENGTH == pszField) ||
        (SQLMessageHeaderStorage::FIELD_HISTORY_WINDOW == pszField) ||
        (SQLMessageHeaderStorage::FIELD_PRIORITY == pszField) ||
        (SQLMessageHeaderStorage::FIELD_EXPIRATION == pszField) ||
        (SQLMessageHeaderStorage::FIELD_ACKNOLEDGMENT == pszField) ||
        (SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP == pszField) ||
        (SQLMessageHeaderStorage::FIELD_METADATA == pszField)) {
        _isOrderSet = true;
        _orderBy = pszField;
        _bDescOrder = bDesc;
        return 0;
    }
    return -1;
}

int DisServiceDataCacheQuery::setOrderBy (uint8 ui8Field, bool bDesc)
{

    if (ui8Field == SQLMessageHeaderStorage::FIELD_GROUP_NAME_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_GROUP_NAME;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_SENDER_ID_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_SENDER_ID;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_TAG_COLUMN_NUMBER) {
        _orderBy =SQLMessageHeaderStorage::FIELD_TAG;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_CLIENT_ID_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_CLIENT_ID;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_CLIENT_TYPE_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_CLIENT_TYPE;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_METADATA_LENGTH_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_METADATA_LENGTH;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_HISTORY_WINDOW_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_HISTORY_WINDOW;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_PRIORITY_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_PRIORITY;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_EXPIRATION_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_EXPIRATION;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_ACKNOLEDGMENT_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_ACKNOLEDGMENT;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP;
    }
    else if (ui8Field == SQLMessageHeaderStorage::FIELD_METADATA_COLUMN_NUMBER) {
        _orderBy = SQLMessageHeaderStorage::FIELD_METADATA;
    }

    _isOrderSet = true;
    _bDescOrder = bDesc;
    return 0;
}

#if defined (USE_SQLITE)
const char * DisServiceDataCacheQuery::getSqlQuery()
{
    String sql = "";
    String tableName;
    String whichFields = "";
    String comma = ", ";
    bool bIsFirstField = true;

    switch (_ui8OperationType)
    {
        case SELECT_OP :
            sql = (String) sql + "SELECT ";
            break;
        case DELETE_OP :
            sql = (String) sql + "DELETE ";
            break;
    }

    tableName = SQLMessageHeaderStorage::TABLE_NAME;

    if (_bSelectRowId && !_bDeleteOperation) {
        whichFields = (String) whichFields + " rowid";
        bIsFirstField = false;
    }

    // Select Primary Key
    if (_bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::PRIMARY_KEY;
        bIsFirstField = false;
    }
    // Select all fields in the table
    if (_bAllFields && !_bPrimaryKey && ! _bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + (_bPersistent ? SQLMessageHeaderStorage::ALL_PERSISTENT : SQLMessageHeaderStorage::ALL);
        bIsFirstField = false;
    }
    // Select the Metadata fields
    if (_bMetadataFields && !_bPrimaryKey && !_bAllFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::METAINFO_FIELDS;
        bIsFirstField = false;
    }

    /*if (_bConstrGroupName && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_GROUP_NAME;
        bIsFirstField = false;
    }

    if (_bConstrSenderNodeId && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_SENDER_ID;
        bIsFirstField = false;
    }

    if (_bConstrTag && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_TAG;
        bIsFirstField = false;
    }

    if (_bChunkId && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields= (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_CHUNK_ID;
        bIsFirstField = false;
    }

    if (_bConstrClientId && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_CLIENT_ID;
        bIsFirstField = false;
    }

    if (_bConstrClientType && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_CLIENT_TYPE;
        bIsFirstField = false;
    }

    if ((_bSeqIdGreaterThan || _bSeqIdSmallerThan || _bSeqIdGreaterThanOrEqual || _bSeqIdSmallerThanOrEqual || _bSeqIdIs) && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields= (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID;
        bIsFirstField = false;
    }

    if (_bConstrPriority && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_PRIORITY;
        bIsFirstField = false;
    }

    if (_bNoFragments && !_bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        switch (_ui8TableType)
        {
            case StorageInterface::CHUNK_TABLE:
                whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_CHUNK_LENGTH;
                bIsFirstField = false;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_CHUNK_FRAGMENT_OFFSET;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_CHUNK_FRAGMENT_LENGTH;
                break;
            case StorageInterface::DEFAULT_TABLE:
                whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH;
                bIsFirstField = false;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH;
            break;
        }
    }

    if (_bConstrFragment && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        switch (_ui8TableType)
        {
            case StorageInterface::CHUNK_TABLE:
                whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_CHUNK_FRAGMENT_OFFSET;
                bIsFirstField = false;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_CHUNK_FRAGMENT_LENGTH;
                break;
            case StorageInterface::DEFAULT_TABLE:
                whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET;
                bIsFirstField = false;
                whichFields = (String) whichFields + comma + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH;
                break;
        }
    }

    if ((_bExpirationUpperThreshold || _bExpirationLowerThreshold) && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_GROUP_NAME;
        bIsFirstField = false;
    }

    if ((_bArrivalLowerThreshold || _bArrivalUpperThreshold) && ! _bPrimaryKey && !_bAllFields && !_bMetadataFields && !_bDeleteOperation) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_GROUP_NAME;
        bIsFirstField = false;
    }*/

    if (_bIncludeData) {
        whichFields = (String) whichFields + (bIsFirstField ? "" : comma) + SQLMessageHeaderStorage::FIELD_DATA;
    }

    sql = (String) sql + whichFields + " FROM ";
    sql = (String) sql + tableName;

    const char * pszWhere = " WHERE ";
    const char * pszAnd =" AND ";
    bool isFirstCond = true;

    if (_bConstrGroupName) {
        sql = (String) sql + pszWhere + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = '" + _constrGroupName + "'";
        isFirstCond = false;
    }

    if (_bConstrSenderNodeId) {
        sql = (String) sql  + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = '" + _constrSenderNodeId + "'";
        isFirstCond = false;
    }
    char buf32[12];

    if (_bChunkIdSmallerThanOrEqual) {
        sql = (String) sql  + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " <= '" + itoa (buf32, _ui8ChunkIdTo) + "'";
        isFirstCond = false;
    }
    else if (_bChunkId) {
        sql = (String) sql  + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " = '" + itoa (buf32, _ui8ChunkId) + "'";
        isFirstCond = false;
    }

    if (_bConstrTag) {
        for (int i = 0; i < _ui8NumOfTags; i++) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_TAG + " = '" + itoa(buf32, _tags[i]) + "'";
            isFirstCond = false;
        }
    }

    if (_bConstrClientId) {
        for (int i = 0; i < _ui8NumOfClientIds; i++) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CLIENT_ID + " = '" + itoa(buf32, _clientIds[i]) + "'";
            isFirstCond = false;
        }
    }

    if (_bConstrClientType) {
        for (int i = 0; i < _ui8NumOfClientType; i++) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CLIENT_TYPE + " = '" + itoa(buf32, _clientTypes[i]) + "'";
            isFirstCond = false;
        }
    }

    if (_bMsgIdNotIn) {
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd);
        for (int i = 0; _constrMessageIdFilters[i] != NULL; i++) {
            DArray2<String> msgFields;
            convertKeyToField (_constrMessageIdFilters[i], msgFields, 6, MSG_ID_GROUP,
                               MSG_ID_SENDER, MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID, MSG_ID_OFFSET,
                               MSG_ID_LENGTH);
            if (i > 0) {
                sql = (String) sql + " AND ";
            }
            sql = (String) sql + "(";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " <> '" + msgFields[0] + "' OR ";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_SENDER_ID + " <> '" + msgFields[1] + "' OR ";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " <> '" + msgFields[2] + "' OR ";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " <> '" + msgFields[3] + "' OR ";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " <> '" + msgFields[4] + "' OR ";
            sql = (String) sql + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " <> '" + msgFields[5] + "')";
        }
    }

    if (_bGrpPubSeqIdNotIn) {
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd);

        sql = (String) sql + " rowid NOT IN (";
        sql = (String) sql + "SELECT rowid ";
        sql = (String) sql + "FROM " + tableName;

        bool bIsFirstInnerCond1 = true;
        StringHashtable<ReceivedMessages::ReceivedMsgsByGrp>::Iterator byGrpIter = _pConstrGrpPubSeqIdFilters->getAllElements();
        for (; !byGrpIter.end(); byGrpIter.nextElement()) {
            sql = (String) sql + (bIsFirstInnerCond1 ? " WHERE " : " OR ");
            bool bIsFirstInnerCond2 = true;
            StringHashtable<ReceivedMessages::ReceivedMsgsByPub>::Iterator byPubIter = byGrpIter.getValue()->msgsByPub.getAllElements();
            for (; !byPubIter.end(); byPubIter.nextElement()) {
                sql = (String) sql + (bIsFirstInnerCond2 ? "(" : " OR (");
                sql = (String) sql + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = '" + byGrpIter.getKey() + "' AND ";
                sql = (String) sql + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = '" + byPubIter.getKey() + "'";

                uint32 begin, end;
                if (byPubIter.getValue()->ranges.getFirst (begin, end) == 0) {
                    if (begin == end) {
                        sql = (String) sql + " AND (" + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = '" + itoa (buf32, begin) + "'";
                    }
                    else {
                        sql = (String) sql + " AND (" + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " >= '" + itoa (buf32, begin) + "'";
                        sql = (String) sql + " AND " + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " <= '" + itoa (buf32, end)+ "'";
                    }
                    while (byPubIter.getValue()->ranges.getNext (begin, end) == 0) {
                        if (begin == end) {
                            sql = (String) sql + " OR " + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = '" + itoa (buf32, begin) + "'";
                        }
                        else {
                            sql = (String) sql + " OR " + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " >= '" + itoa (buf32, begin) + "'";
                            sql = (String) sql + " AND " + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " <= '" + itoa (buf32, end)+ "'";
                        }
                    }
                    sql = (String) sql + ")";
                }
                sql = (String) sql + ") ";
                bIsFirstInnerCond2 = false;
            }
            bIsFirstInnerCond1 = false;
        }

        sql = (String) sql + ") ";
    }

    if (_bMsgId) {
        NOMADSUtil::DArray2<NOMADSUtil::String> msgFields;
        convertKeyToField(_constrMessageId, msgFields, 6, MSG_ID_GROUP, MSG_ID_SENDER, MSG_ID_SEQ_NUM, MSG_ID_CHUNK_ID, MSG_ID_OFFSET, MSG_ID_LENGTH);
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd)
            + SQLMessageHeaderStorage::FIELD_GROUP_NAME + " = '" + msgFields[0] + "' AND "
            + SQLMessageHeaderStorage::FIELD_SENDER_ID + " = '" + msgFields[1] + "' AND "
            + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = '" + msgFields[2] + "' AND "
            + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " = '" + msgFields[3] + "' AND "
            + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " = '" + msgFields[4] + "' AND "
            + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = '" + msgFields[5] + "'";
    }

    if (_bSeqIdGreaterThan || _bSeqIdSmallerThan || _bSeqIdGreaterThanOrEqual || _bSeqIdSmallerThanOrEqual) {
        if (_bSeqIdGreaterThan) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " > " + itoa(buf32, _ui32SeqIdFrom);
            isFirstCond = false;
        }
        else if (_bSeqIdGreaterThanOrEqual) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " >= " + itoa(buf32, _ui32SeqIdFrom);
            isFirstCond = false;
        }

        if (_bSeqIdSmallerThan) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " < " + itoa(buf32, _ui32SeqIdTo);
            isFirstCond = false;
        }
        else if (_bSeqIdSmallerThanOrEqual) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " <= " + itoa(buf32, _ui32SeqIdTo);
            isFirstCond = false;
        }
    }
    else if (_bSeqIdIs) {
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID + " = '" + itoa(buf32, _ui32SeqIdFrom) + "'";
        isFirstCond = false;
    }

    if (_bConstrPriority) {
        if (_ui8PriorityFrom < _ui8PriorityTo) {
            String rangeStatement;
            rangeStatement = (String) SQLMessageHeaderStorage::FIELD_PRIORITY + " BETWEEN " + itoa(buf32, _ui8PriorityFrom);
            rangeStatement = (String) rangeStatement + " AND " + itoa(buf32, _ui8PriorityTo);
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + "(" + rangeStatement + ")";
        }
        else if (_ui8PriorityFrom == _ui8PriorityTo) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_PRIORITY + " = '" + itoa(buf32, _ui8PriorityFrom) + "'";
        }
        isFirstCond = false;
    }

    if (_bNoFragments) {
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " = '0'";
        sql = (String) sql + " AND " + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = " + SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH;
        isFirstCond = false;
    }

    if (_bConstrFragment) {
        if (_bFragRangeExactMatch) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " = '" + itoa(buf32, _ui32FragOffset) + "'";
            sql = (String) sql + " AND " + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + " = '" + itoa(buf32, _ui32FragLength) + "'";
        }
        else {
            uint32 ui32OffsetEnd = _ui32FragOffset + _ui32FragLength;
            String rangeStatement;
            rangeStatement = (String) SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " BETWEEN " + itoa(buf32, _ui32FragOffset);
            rangeStatement = (String) rangeStatement + " AND " + itoa(buf32, ui32OffsetEnd);
            rangeStatement = (String) rangeStatement + " OR ";
            rangeStatement = (String) rangeStatement + "(" + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " + " + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + ") BETWEEN " + itoa(buf32, _ui32FragOffset);
            rangeStatement = (String) rangeStatement + " AND " + itoa(buf32, ui32OffsetEnd);
            rangeStatement = (String) rangeStatement + " OR (" + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " <= " + itoa(buf32, _ui32FragOffset);
            rangeStatement = (String) rangeStatement + " AND " + "(" + SQLMessageHeaderStorage::FIELD_FRAGMENT_OFFSET + " + " + SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH + ") >= " + itoa(buf32, ui32OffsetEnd);
            rangeStatement = (String) rangeStatement + ")";
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + "(" + rangeStatement + ")";
        }
        isFirstCond = false;
    }

    if (_bExcludeRange) {
        char buf [3];
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " > " + itoa (buf, _ui8ChunkIdStart);
        isFirstCond = false;
        sql = (String) sql + pszAnd + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " < " + itoa (buf, _ui8ChunkIdEnd);
    }

    if (_bExcludeChunkIDs) {
        char buf [3];
        for (int i = 0; i <= _pChunkIDs->getHighestIndex (); i++) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_CHUNK_ID + " <> " + itoa (buf, (*_pChunkIDs)[i]);
            isFirstCond = false;
        }
    }

    char buf64[22];

    if(_bExpirationUpperThreshold || _bExpirationLowerThreshold) {
        if(_bExpirationLowerThreshold) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_EXPIRATION + " > '" + i64toa(buf64, _i64ExpirationLowerThreshold) + "'";
            isFirstCond = false;
        }
        if(_bExpirationUpperThreshold) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_EXPIRATION + " < '" + i64toa(buf64, _i64ExpirationUpperThreshold) + "'";
            isFirstCond = false;
        }
    }

    if(_bArrivalLowerThreshold || _bArrivalUpperThreshold) {
        if(_bArrivalLowerThreshold) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP + " > '" + i64toa(buf64, _i64ArrivalLowerThreshold) + "'";
            isFirstCond = false;
        }
        if(_bArrivalUpperThreshold) {
            sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP + " < '" + i64toa(buf64, _i64ArrivalUpperThreshold) + "'";
            isFirstCond = false;
        }
    }

    if (_bAddCustomConstraint) {
        sql = (String) sql + (isFirstCond ? pszWhere : pszAnd) + _customConstraint->c_str();
        isFirstCond = false;
    }

    if (_isOrderSet) {
        sql = (String) sql + " ORDER BY " + _orderBy + (_bDescOrder ? " DESC " : " ASC ");
    }

    if (_bResultLimit) {
        sql = (String) sql + " LIMIT " + itoa(buf32, _ui16ResultLimit);
    }
    //Debug print
    //printf ("\nDisServiceDataCacheQuery::getSqlQuery ===> %s\n", (const char *)sql);
    _sqlQuery = sql;
    return _sqlQuery;
}

const char * DisServiceDataCacheQuery::getComputedSqlQuery ()
{
    return _sqlQuery;
}

#endif

int DisServiceDataCacheQuery::read (Reader *pReader, uint32 ui32MaxSize)
{
    uint8 ui8FirstFlag;
    uint8 ui8SecondFlag;
    uint8 ui8ThirdFlag;
    uint8 ui8ForthFlag;
    pReader->read8(&ui8FirstFlag);
    pReader->read8(&ui8SecondFlag);
    pReader->read8(&ui8ThirdFlag);
    pReader->read8(&ui8ForthFlag);
    _bDeleteOperation = ((ui8FirstFlag | READ_DELETE_OP) == 0xFF ? true : false);
    _bPersistent = ((ui8FirstFlag | READ_PERSISTENT) == 0xFF ? true : false);
    _bPrimaryKey = ((ui8FirstFlag | READ_PRIMARY_KEY) == 0xFF ? true : false);
    _bMetadataFields = ((ui8FirstFlag | READ_METADATA_FIELDS) == 0xFF ? true : false);
    _bAllFields = ((ui8FirstFlag | READ_ALL_FIELDS) == 0xFF ? true : false);
    _bSelectRowId = ((ui8FirstFlag | READ_SELECT_ROW_ID) == 0xFF ? true : false);
    _bAddCustomConstraint = ((ui8FirstFlag | READ_ADD_CONDITION) == 0xFF ? true : false);
    _bConstrGroupName = ((ui8FirstFlag | READ_SELECT_GROUP_NAME) == 0xFF ? true : false);
    _bConstrSenderNodeId = ((ui8SecondFlag | READ_SELECT_SENDER_NODE_ID) == 0xFF ? true : false);
    _bSeqIdGreaterThan = ((ui8SecondFlag | READ_SEQ_ID_GREATER_THAN) == 0xFF ? true : false);
    _bSeqIdSmallerThan = ((ui8SecondFlag | READ_SEQ_ID_SMALLER_THAN) == 0xFF ? true : false);
    _bSeqIdIs = ((ui8SecondFlag | READ_SEQ_ID) == 0xFF ? true : false);
    _bChunkId = ((ui8SecondFlag | READ_CHUNK_ID) == 0xFF ? true : false);
    _bConstrTag = ((ui8SecondFlag | READ_SELECT_TAG) == 0xFF ? true : false);
    _bConstrClientId = ((ui8SecondFlag | READ_SELECT_CLIENT_ID) == 0xFF ? true : false);
    _bConstrClientType = ((ui8SecondFlag | READ_SELECT_CLIENT_TYPE) == 0xFF ? true : false);
    _bConstrPriority = ((ui8ThirdFlag | READ_PRIORITY) == 0xFF ? true : false);
    _bExcludeChunkIDs = ((ui8ThirdFlag | READ_EXCLUDE_CHUNK_IDS) == 0xFF ? true : false);
    _bExcludeRange = ((ui8ThirdFlag | READ_EXCLUDE_RANGE) == 0xFF ? true : false);
    _bConstrFragment = ((ui8ThirdFlag | READ_SELECT_FRAGMENT) == 0xFF ? true : false);
    _bFragRangeExactMatch = ((ui8ThirdFlag | READ_FRAG_RANGE_EXACT_MATCH) == 0xFF ? true : false);
    _bExpirationLowerThreshold = ((ui8ThirdFlag | READ_EXPIRATION_LOWER_THRESHOLD) == 0xFF ? true : false);
    _bExpirationUpperThreshold = ((ui8ThirdFlag | READ_EXPIRATION_UPPER_THRESHOLD) == 0xFF ? true : false);
    _bArrivalLowerThreshold = ((ui8ThirdFlag | READ_ARRIVAL_LOWER_THRESHOLD) == 0xFF ? true : false);
    _bArrivalUpperThreshold = ((ui8ForthFlag | READ_ARRIVAL_UPPER_THRESHOLD) == 0xFF ? true : false);
    _bUtilityLowerThreshold = ((ui8ForthFlag | READ_UTILITY_LOWER_THRESHOLD) == 0xFF ? true : false);
    _bUtilityUpperThreshold = ((ui8ForthFlag | READ_UTILITY_UPPER_THRESHOLD) == 0xFF ? true : false);
    _bResultLimit = ((ui8ForthFlag | READ_RESULT_LIMIT) == 0xFF ? true : false);
    _bIncludeData = ((ui8ForthFlag | READ_INCLUDE_DATA) == 0xFF ? true : false);
    _bNoFragments = ((ui8ForthFlag | READ_NO_FRAGMENTS) == 0xFF ? true : false);
    _isOrderSet = ((ui8ForthFlag | READ_ORDER_SET) == 0xFF ? true : false);
    _bDescOrder = ((ui8ForthFlag | READ_DESC_ORDER) == 0xFF ? true : false);

    if(_bDeleteOperation) {pReader->read8(&_ui8OperationType);}
    pReader->read8(&_ui8TableType);
    if(_bAddCustomConstraint) {
        uint16 ui16;
        pReader->read16 (&ui16);
        char *pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        //_customConstraint = new NOMADSUtil::String (pszTemp);
        (*_customConstraint) = pszTemp;
        delete[] pszTemp;
    }
    if (_bConstrGroupName) {
        uint16 ui16;
        pReader->read16 (&ui16);
        char *pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _constrGroupName = pszTemp;
        delete[] pszTemp;
    }
    if (_bConstrSenderNodeId) {
        uint16 ui16;
        pReader->read16 (&ui16);
        char *pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _constrSenderNodeId = pszTemp;
        delete[] pszTemp;
    }
    if (_bSeqIdGreaterThan && _bSeqIdSmallerThan) {
        pReader->read32(&_ui32SeqIdFrom);
        pReader->read32(&_ui32SeqIdTo);
    }
    if (_bSeqIdGreaterThan || _bSeqIdIs) {
        pReader->read32(&_ui32SeqIdFrom);
    }
    if (_bSeqIdSmallerThan) {
        pReader->read32(&_ui32SeqIdTo);
    }
    if (_bChunkId) {
        pReader->read8(&_ui8ChunkId);
    }
    if (_bConstrTag) {
        uint32 ui32Size;
        pReader->read32(&ui32Size);
        DArray<uint16> tempArray;
        uint16 ui16Value;
        for (uint32 ui32Index = 0; ui32Index < ui32Size; ui32Index++) {
            pReader->read16(&ui16Value);
            tempArray[ui32Index] = ui16Value;
        }
        _tags = tempArray;
    }
    if (_bConstrClientId) {
        pReader->read8(&_ui8NumOfClientIds);
        DArray<uint16> tempArray;
        uint16 ui16Value;
        for (uint8 ui8Index = 0; ui8Index < _ui8NumOfClientIds; ui8Index++) {
            pReader->read16(&ui16Value);
            tempArray[ui8Index] = ui16Value;
        }
        _clientIds = tempArray;
    }
    if (_bConstrClientType) {
        pReader->read8(&_ui8NumOfClientType);
        DArray<uint8> tempArray;
        uint8 ui8Value;
        for (uint8 ui8Index = 0; ui8Index < _ui8NumOfClientType; ui8Index++) {
            pReader->read16(&ui8Value);
            tempArray[ui8Index] = ui8Value;
        }
        _clientTypes = tempArray;
    }
    if (_bConstrPriority) {
        pReader->read8(&_ui8PriorityFrom);
        pReader->read8(&_ui8PriorityTo);
    }
    if (_bExcludeRange) {
        pReader->read8(&_ui8ChunkIdStart);
        pReader->read8(&_ui8ChunkIdEnd);
    }
    if (_bExcludeChunkIDs) {
        uint8 ui8Size;
        pReader->read8(&ui8Size);
        DArray<uint8> *pTempArray = new DArray<uint8> ((uint32)ui8Size);
        uint8 ui8Value;
        for (uint8 ui8Index = 0; ui8Index < ui8Size; ui8Index++) {
            pReader->read8(&ui8Value);
            (*pTempArray)[ui8Index] = ui8Value;
        }
        _pChunkIDs = pTempArray;
    }
    if (_bConstrFragment) {
        pReader->read32(&_ui32FragOffset);
        pReader->read32(&_ui32FragLength);
    }
    if (_bExpirationLowerThreshold) {pReader->read64(&_i64ExpirationLowerThreshold);}
    if (_bExpirationUpperThreshold) {pReader->read64(&_i64ExpirationUpperThreshold);}
    if (_bArrivalLowerThreshold) {pReader->read64(&_i64ArrivalLowerThreshold);}
    if (_bArrivalUpperThreshold) {pReader->read64(&_i64ArrivalUpperThreshold);}
    if (_bUtilityLowerThreshold) {pReader->read8(&_i8UtilityLowerThreshold);}
    if (_bUtilityUpperThreshold) {pReader->read8(&_i8UtilityUpperThreshold);}
    if (_bResultLimit) {pReader->read16(&_ui16ResultLimit);}
    if (_isOrderSet) {
        uint16 ui16;
        pReader->read16 (&ui16);
        char *pszTemp = new char[ui16+1];
        pReader->readBytes (pszTemp, ui16);
        pszTemp[ui16] = '\0';
        _orderBy = pszTemp;
        delete[] pszTemp;
    }
    return 0;
}

int DisServiceDataCacheQuery::write (Writer *pWriter, uint32 ui32MaxSize)
{
    uint8 ui8FirstFlag = 0x00;
    uint8 ui8SecondFlag = 0x00;
    uint8 ui8ThirdFlag = 0x00;
    uint8 ui8ForthFlag = 0x00;
    if(_bDeleteOperation) {ui8FirstFlag |= WRITE_DELETE_OP;}
    if(_bPersistent) {ui8FirstFlag |= WRITE_PERSISTENT;}
    if(_bPrimaryKey) {ui8FirstFlag |= WRITE_PRIMARY_KEY;}
    if(_bMetadataFields) {ui8FirstFlag |= WRITE_METADATA_FIELDS;}
    if(_bAllFields) {ui8FirstFlag |= WRITE_ALL_FIELDS;}
    if(_bSelectRowId) {ui8FirstFlag |= WRITE_SELECT_ROW_ID;}
    if(_bAddCustomConstraint) {ui8FirstFlag |= WRITE_ADD_CONDITION;}
    if(_bConstrGroupName) {ui8FirstFlag |= WRITE_SELECT_GROUP_NAME;}
    if(_bConstrSenderNodeId) {ui8SecondFlag |= WRITE_SELECT_SENDER_NODE_ID;}
    if(_bSeqIdGreaterThan) {ui8SecondFlag |= WRITE_SEQ_ID_GREATER_THAN;}
    if(_bSeqIdSmallerThan) {ui8SecondFlag |= WRITE_SEQ_ID_SMALLER_THAN;}
    if(_bSeqIdIs) {ui8SecondFlag |= WRITE_SEQ_ID;}
    if(_bChunkId) {ui8SecondFlag |= WRITE_CHUNK_ID;}
    if(_bConstrTag) {ui8SecondFlag |= WRITE_SELECT_TAG;}
    if(_bConstrClientId) {ui8SecondFlag |= WRITE_SELECT_CLIENT_ID;}
    if(_bConstrClientType) {ui8SecondFlag |= WRITE_SELECT_CLIENT_TYPE;}
    if(_bConstrPriority) {ui8ThirdFlag |= WRITE_PRIORITY;}
    if(_bExcludeChunkIDs) {ui8ThirdFlag |= WRITE_EXCLUDE_CHUNK_IDS;}
    if(_bExcludeRange) {ui8ThirdFlag |= WRITE_EXCLUDE_RANGE;}
    if(_bConstrFragment) {ui8ThirdFlag |= WRITE_SELECT_FRAGMENT;}
    if(_bFragRangeExactMatch) {ui8ThirdFlag |= WRITE_FRAG_RANGE_EXACT_MATCH;}
    if(_bExpirationLowerThreshold) {ui8ThirdFlag |= WRITE_EXPIRATION_LOWER_THRESHOLD;}
    if(_bExpirationUpperThreshold) {ui8ThirdFlag |= WRITE_EXPIRATION_UPPER_THRESHOLD;}
    if(_bArrivalLowerThreshold) {ui8ThirdFlag |= WRITE_ARRIVAL_LOWER_THRESHOLD;}
    if(_bArrivalUpperThreshold) {ui8ForthFlag |= WRITE_ARRIVAL_UPPER_THRESHOLD;}
    if(_bUtilityLowerThreshold) {ui8ForthFlag |= WRITE_UTILITY_LOWER_THRESHOLD;}
    if(_bUtilityUpperThreshold) {ui8ForthFlag |= WRITE_UTILITY_UPPER_THRESHOLD;}
    if(_bResultLimit) {ui8ForthFlag |= WRITE_RESULT_LIMIT;}
    if(_bIncludeData) {ui8ForthFlag |= WRITE_INCLUDE_DATA;}
    if(_bNoFragments) {ui8ForthFlag |= WRITE_NO_FRAGMENTS;}
    if(_isOrderSet) {ui8ForthFlag |= WRITE_ORDER_SET;}
    if(_bDescOrder) {ui8ForthFlag |= WRITE_DESC_ORDER;}

    pWriter->write8(&ui8FirstFlag);
    pWriter->write8(&ui8SecondFlag);
    pWriter->write8(&ui8ThirdFlag);
    pWriter->write8(&ui8ForthFlag);

    if(_bDeleteOperation) {pWriter->write8(&_ui8OperationType);}
    pWriter->write8(&_ui8TableType);
    if(_bAddCustomConstraint) {
        uint16 ui16 = _customConstraint->length();
        pWriter->write16(&ui16);
        pWriter->writeBytes(_customConstraint->c_str(), ui16);
    }
    if (_bConstrGroupName) {
        uint16 ui16 = _constrGroupName.length();
        pWriter->write16(&ui16);
        pWriter->writeBytes((const char*)_constrGroupName, ui16);
    }
    if (_bConstrSenderNodeId) {
        uint16 ui16 = _constrSenderNodeId.length();
        pWriter->write16(&ui16);
        pWriter->writeBytes((const char*)_constrSenderNodeId, ui16);
    }
    if (_bSeqIdGreaterThan && _bSeqIdSmallerThan) {
        pWriter->write32(&_ui32SeqIdFrom);
        pWriter->write32(&_ui32SeqIdTo);
    }
    if (_bSeqIdGreaterThan || _bSeqIdIs) {pWriter->write32(&_ui32SeqIdFrom);}
    if (_bSeqIdSmallerThan) {pWriter->write32(&_ui32SeqIdTo);}
    if (_bChunkId) {pWriter->write8(&_ui8ChunkId);}
    if (_bConstrTag) {
        uint32 ui32Size = _tags.getSize();
        pWriter->write32(&ui32Size);
        uint16 ui16Value;
        for (uint32 ui32Index = 0; ui32Index < ui32Size; ui32Index++) {
            ui16Value = _tags[ui32Index];
            pWriter->write16(&ui16Value);
        }
    }
    if (_bConstrClientId) {
        pWriter->write8(&_ui8NumOfClientIds);
        uint16 ui16Value;
        for (uint8 ui8Index = 0; ui8Index < _ui8NumOfClientIds; ui8Index++) {
            ui16Value = _clientIds[ui8Index];
            pWriter->write16(&ui16Value);
        }
    }
    if (_bConstrClientType) {
        pWriter->write8(&_ui8NumOfClientType);
        uint16 ui16Value;
        for (uint8 ui8Index = 0; ui8Index < _ui8NumOfClientType; ui8Index++) {
            ui16Value = _clientTypes [ui8Index];
            pWriter->write16(&ui16Value);
        }
    }
    if (_bConstrPriority) {
        pWriter->write8(&_ui8PriorityFrom);
        pWriter->write8(&_ui8PriorityTo);
    }
    if (_bExcludeRange) {
        pWriter->write8(&_ui8ChunkIdStart);
        pWriter->write8(&_ui8ChunkIdEnd);
    }
    if (_bExcludeChunkIDs) {
        uint8 ui8Size = (uint8) _pChunkIDs->size ();
        pWriter->write8(&ui8Size);
        uint8 ui8Value;
        for (uint8 ui8Index = 0; ui8Index < ui8Size; ui8Index++) {
            ui8Value = (*_pChunkIDs)[ui8Index];
            pWriter->write8(&ui8Value);
        }
    }
    if (_bConstrFragment) {
        pWriter->write32(&_ui32FragOffset);
        pWriter->write32(&_ui32FragLength);
    }
    if (_bExpirationLowerThreshold) {pWriter->write64(&_i64ExpirationLowerThreshold);}
    if (_bExpirationUpperThreshold) {pWriter->write64(&_i64ExpirationUpperThreshold);}
    if (_bArrivalLowerThreshold) {pWriter->write64(&_i64ArrivalLowerThreshold);}
    if (_bArrivalUpperThreshold) {pWriter->write64(&_i64ArrivalUpperThreshold);}
    if (_bUtilityLowerThreshold) {pWriter->write8(&_i8UtilityLowerThreshold);}
    if (_bUtilityUpperThreshold) {pWriter->write8(&_i8UtilityUpperThreshold);}
    if (_bResultLimit) {pWriter->write16(&_ui16ResultLimit);}
    if (_isOrderSet) {
        uint16 ui16 = _orderBy.length();
        pWriter->write16 (&ui16);
        pWriter->writeBytes ((const char *)_orderBy, ui16);
    }
    return 0;
}
