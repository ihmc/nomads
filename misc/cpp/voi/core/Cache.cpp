/*
 * Cache.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on Febraury 15, 2017
 */

#include "Cache.h"

#include "MetadataInterface.h"

#include "StrClass.h"

#include "Database.h"
#include "PreparedStatement.h"
#include "MetadataImpl.h"
#include "Json.h"

#define synchronized MutexUnlocker unlocker (&_m)

using namespace IHMC_VOI;
using namespace NOMADSUtil;

namespace VOI_CACHE
{
    MetadataImpl * deserialize (String &json)
    {
        JsonObject obj (json);
        MetadataImpl *pMetadata = new MetadataImpl();
        if (pMetadata == NULL) {
            return NULL;
        }
        if (pMetadata->fromJson (&obj) < 0) {
            delete pMetadata;
            return NULL;
        }
        return pMetadata;
    }

    int serialize (MetadataInterface &metadata, String &json)
    {
        JsonObject *pJson = metadata.toJson();
        if (pJson == NULL) {
            return -1;
        }
        json = pJson->toString (true);
        return 0;
    }

    InformationObject * toMutableInformationObject (IHMC_MISC::Row *pRow)
    {
        if (pRow == NULL) {
            return NULL;
        }
        MetadataInterface *pMetadata = NULL;
        char *pszBuf = NULL;
        if (pRow->getValue (2, &pszBuf) == 0) {
            String json (pszBuf);
            if ((pszBuf != NULL) && (json.length() > 0)) {
                pMetadata = VOI_CACHE::deserialize (json);
                if (pMetadata == NULL) {
                    return NULL;
                }
            }
        }
        else {
            return NULL;
        }

        int iLen = 0U;
        void *pData = NULL;
        if (pRow->getValue (3, &pData, iLen) < 0) {
            delete pMetadata;
            return NULL;
        }

        return new MutableInformationObject (pMetadata, pData, iLen);
    }
}

using namespace IHMC_MISC;

Cache::Cache (const char *pszSessionId)
    : _sessionId (pszSessionId),
      _pInsertObjStmt (NULL),
      _pInsertInstStmt (NULL),
      _pInsertPeerStmt (NULL),
      _pGetHistoryStmt (NULL)
{
}

Cache::~Cache (void)
{
    delete _pInsertObjStmt;
    delete _pInsertInstStmt;
    delete _pInsertPeerStmt;
    delete _pGetHistoryStmt;
}

int Cache::init (void)
{
    String dbFilename ("voi");
    if (_sessionId.length() > 0) {
        dbFilename += "-";
        dbFilename += _sessionId;
    }
    dbFilename += ".sqlite";

    synchronized;

    if (_db.open (dbFilename) < 0) {
        return -1;
    }

    if (_db.execute (
        "CREATE TABLE IF NOT EXISTS objectIds ("
        "obj_id_seq INTEGER PRIMARY KEY AUTOINCREMENT,"
        "objectId TEXT UNIQUE NOT NULL)") < 0) {
        return -2;
    }

    if (_db.execute (
        "CREATE TABLE IF NOT EXISTS instances ("
        "inst_id_seq INTEGER PRIMARY KEY AUTOINCREMENT,"
        "obj_id_seq_fk INTEGER,"
        "instanceId TEXT,"
        "sourceTimestamp TEXT,"
        "metadata TEXT,"
        "data BLOB,"
        "FOREIGN KEY (obj_id_seq_fk) REFERENCES objectIds (obj_id_seq),"
        "UNIQUE (obj_id_seq_fk, instanceId))") < 0) {
        return -3;
    }

    if (_db.execute (
        "CREATE TABLE IF NOT EXISTS peers ("
        "nodeId STRING,"
        "inst_id_seq_fk INTEGER,"
        "FOREIGN KEY (inst_id_seq_fk) REFERENCES instances (inst_id_seq),"
        "PRIMARY KEY (nodeId, inst_id_seq_fk))") < 0) {
        return -4;
    }

    if ((_pInsertObjStmt = _db.prepare (
        "INSERT OR IGNORE INTO objectIds (objectId) "
        "VALUES (?1)")) == NULL) {
        return -5;
    }

    if ((_pInsertInstStmt = _db.prepare (
        "INSERT OR IGNORE INTO instances "
        "(obj_id_seq_fk, instanceId, sourceTimestamp, metadata, data) "
        "SELECT obj_id_seq, ?2, ?3, ?4, ?5 "
        "FROM objectIds WHERE objectId = ?1")) == NULL) {
        return -6;
    }

    if ((_pInsertPeerStmt = _db.prepare (
        "INSERT OR IGNORE INTO peers (nodeId, inst_id_seq_fk) "
        "SELECT ?1, inst_id_seq "
        "FROM instances JOIN objectIds ON obj_id_seq = obj_id_seq_fk "
        "WHERE objectId = ?2")) == NULL) {
        return -7;
    }

    if ((_pGetHistoryStmt = _db.prepare (
        "SELECT instanceId, sourceTimestamp, metadata, data "
        "FROM instances "
        "JOIN objectIds ON obj_id_seq = obj_id_seq_fk "
        "JOIN peers ON inst_id_seq = inst_id_seq_fk "
        "AND objectId = ?1 "
        "AND nodeId = ?2 "
        "ORDER BY sourceTimestamp, instanceId DESC "
        "LIMIT ?3")) == NULL) {
        return -8;
    }
    return 0;
}

int Cache::add (const char *pszNodeId, EntityInfo &ei, MetadataInterface *pMetadata, const void *pData, int64 i64DataLen)
{
    if ((pszNodeId == NULL) || (!ei.valid()) || (_pInsertObjStmt == NULL) || (_pInsertInstStmt == NULL)) {
        return -1;
    }

    String json;
    if ((pMetadata != NULL) && (VOI_CACHE::serialize (*pMetadata, json) < 0)) {
        return -2;
    }

    synchronized;
    Transaction tr (&_db);

    _pInsertObjStmt->reset();
    if ((_pInsertObjStmt->bind (1, ei._objectId) < 0) || (_pInsertObjStmt->update() < 0)) {
        return -3;
    }

    // "(seq_fk, instanceId, sourceTimestamp, metadata, data) "
    _pInsertInstStmt->reset();
    if (_pInsertInstStmt->bind (1, ei._objectId) < 0) {
        return -7;
    }
    if (_pInsertInstStmt->bind (2, ei._instanceId) < 0) {
        return -8;
    }
    if (_pInsertInstStmt->bind (3, ei._i64SourceTimeStamp) < 0) {
        return -9;
    }

    if (pMetadata == NULL) {
        if (_pInsertInstStmt->bindNull (4) < 0) {
            return -10;
        }
    }
    else if (_pInsertInstStmt->bind (4, json) < 0) {
        return -11;
    }

    if ((pData == NULL) || (i64DataLen <= 0)) {
        if (_pInsertInstStmt->bindNull (5)) {
            return -12;
        }
    }
    else if (_pInsertInstStmt->bind (5, pData, i64DataLen) < 0) {
        return -13;
    }

    if (_pInsertInstStmt->update() < 0) {
        return -14;
    }

    _pInsertPeerStmt->reset();
    if (_pInsertPeerStmt->bind (1, pszNodeId) < 0) {
        return -4;
    }
    if (_pInsertPeerStmt->bind (2, ei._objectId) < 0) {
        return -5;
    }
    if (_pInsertPeerStmt->update() < 0) {
        return -6;
    }

    tr.setSuccess();
    return 0;
}

int Cache::getPreviousVersions (const char *pszNodeId, const char *pszObjectId, unsigned int uiHistoryLen, InformationObjects &history) const
{
    if ((pszObjectId == NULL) || (_pGetHistoryStmt == NULL)) {
        return -1;
    }

    synchronized;

    _pGetHistoryStmt->reset();
    if (_pGetHistoryStmt->bind (1, pszObjectId) < 0) {
        return -2;
    }
    if (_pGetHistoryStmt->bind (2, pszNodeId) < 0) {
        return -2;
    }
    if (_pGetHistoryStmt->bind (3, uiHistoryLen) < 0) {
        return -3;
    }
    Row *pRow = _pGetHistoryStmt->getRow();
    if (pRow == NULL) {
        return -4;
    }

    while (_pGetHistoryStmt->next (pRow)) {
        InformationObject *pIO = VOI_CACHE::toMutableInformationObject (pRow);
        if (pIO != NULL) {
            history.prepend (pIO);
        }
    }
    return 0;
}

EntityInfo::EntityInfo (void)
    : _i64SourceTimeStamp (0)
{
}

EntityInfo::~EntityInfo (void)
{
}

bool EntityInfo::valid (void) const
{
    if (_objectId.length() <= 0) {
        return false;
    }
    if (_instanceId.length() <= 0) {
        return false;
    }
    return true;
}

