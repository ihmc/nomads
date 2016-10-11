/*
 * Analytics.cpp
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

#include "Analytics.h"

#include "DisServiceDefs.h"

#include "SQLiteFactory.h"

#include "Database.h"
#include "PreparedStatement.h"
#include "Result.h"

#include "Logger.h"
#include "StrClass.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

Analytics::Analytics()
{
    _pGetSubscriptionStats = NULL;
    _pPercRcvdBytes = NULL;
    _pPercRcvdBytesByGrpSdr = NULL;
    _pPercRcvdBytesByGrp = NULL;
    _pPercMissMsgs = NULL;
}

Analytics::~Analytics()
{
    delete _pGetSubscriptionStats;
    delete _pPercRcvdBytes;
    delete _pPercRcvdBytesByGrpSdr;
    delete _pPercRcvdBytesByGrp;
    delete _pPercMissMsgs;
    _pGetSubscriptionStats = NULL;
    _pPercRcvdBytes = NULL;
    _pPercRcvdBytesByGrpSdr = NULL;
    _pPercRcvdBytesByGrp = NULL;
    _pPercMissMsgs = NULL;
}

int Analytics::init()
{
    const char * pszMethodName = "Analytics::init";

    const char * pszGrp = SQLMessageHeaderStorage::FIELD_GROUP_NAME;
    const char * pszSender = SQLMessageHeaderStorage::FIELD_SENDER_ID;
    const char * pszMsgSeqID = SQLMessageHeaderStorage::FIELD_MSG_SEQ_ID;
    const char * pszTArrival = SQLMessageHeaderStorage::FIELD_ARRIVAL_TIMESTAMP;
    const char * pszTotBytes = SQLMessageHeaderStorage::FIELD_TOT_MSG_LENGTH;
    const char * pszRcvdBytes = SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH;
    const char * pszTable = SQLMessageHeaderStorage::FIELD_FRAGMENT_LENGTH;

    String tWin = (String) "(SELECT max (" + pszTArrival + ") - ((max(" + pszTArrival + ") - min(" + pszTArrival + "))/100) "
                           "FROM " + pszTable + ")";

    tWin = "?";

    String sql;

    sql = (String) "SELECT " + pszGrp + ", count (*), (max(" + pszTArrival + ") - min(" + pszTArrival + ")), sum(" + pszTotBytes + "), sum(" + pszRcvdBytes + ") "
        +          "FROM (SELECT * FROM " + pszTable + " "
    //           +          "WHERE " + pszTArrival + " > " + tWin + " ) "
        +          "GROUP BY " + pszGrp + ";";

    _pGetSubscriptionStats = _pDB->prepare ((const char *)sql);
    if (_pGetSubscriptionStats != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Preparing statement for insert query into the main table\n<%s>\nrun successfully\n",
                        (const char *)sql);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for insert query into the main table\n<%s>\nfailed\n",
                        (const char *)sql); 
        _pGetSubscriptionStats = NULL;
        return -1;
    }

    // ---

    String messages, query;

    messages = (String) "SELECT " + pszGrp + ", " + pszSender + ", " + pszMsgSeqID + ", "
             +                   "max (" + pszTotBytes + ") AS TOT, sum(" + pszRcvdBytes + ") AS RCVD, "
             +                   "max (" + pszTArrival + ") AS LATEST, min(" + pszTArrival + ") AS FIRST "
             +           "FROM " + pszTable + " "
             +           "GROUP BY " + pszGrp + ", " + pszSender + ", " + pszMsgSeqID + " "
             +           "HAVING " + pszGrp + " = ?";

    sql = (String) "SELECT max(LATEST), min(FIRST), sum(TOT), avg(TOT), sum(RCVD), avg(RCVD)"
        +          "FROM (" + messages + ");";

    query = (String) sql + "(" + messages + ");";

    _pPercRcvdBytes = _pDB->prepare ((const char *) query);
    if (_pPercRcvdBytes != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Preparing statement for insert query into the main table\n<%s>\nrun successfully\n",
                        (const char *)sql);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for insert query into the main table\n<%s>\nfailed\n",
                        (const char *)sql);
        _pPercRcvdBytes = NULL;
        return -1;
    }

    // ---

    messages += (String) "AND " + pszSender + " = ? "; // Add condition
    query = (String) sql + "(" + messages + ");";

    _pPercRcvdBytesByGrpSdr = _pDB->prepare ((const char *)query);
    if (_pPercRcvdBytesByGrpSdr != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Preparing statement for insert query into the main table\n<%s>\nrun successfully\n",
                        (const char *)sql);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for insert query into the main table\n<%s>\n",
                        (const char *)sql);
        _pPercRcvdBytesByGrpSdr = NULL;
        return -1;
    }

    // ---

    messages += (String) "AND " + pszMsgSeqID + " = ? "; // Add condition
    query = (String) sql + "(" + messages + ");";

    _pPercRcvdBytes = _pDB->prepare ((const char *) query);
    if (_pPercRcvdBytes != NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Info,
                        "Preparing statement for insert query into the main table\n<%s>\nrun successfully\n",
                        (const char *)sql);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "Preparing statement for insert query into the main table\n<%s>\n",
                        (const char *)query);
        _pPercRcvdBytes = NULL;
        return -1;
    }

    return 0;
}

float Analytics::getPercentRcvdBytes (const char *pszGroupName,
                                      int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes)
{
    _m.lock (227);

    // Bind the values to the prepared statement
    if (_pPercRcvdBytesByGrp->bind (1, pszGroupName) < 0) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes", bindingError);
        _pPercRcvdBytesByGrp->reset();
        _m.unlock (227);
        return 0.0f;
    }

    Row *pRow = _pPercRcvdBytesByGrp->getRow();
    if (pRow == NULL) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes", Logger::L_SevereError,
                        "could not instantiate row\n");
        _pPercRcvdBytesByGrp->reset();
        _m.unlock (227);
        return 0.0f;
    }

    // Execute the statement
    int rc = 0;
    int64 rcvdBytes;
    i64TFirst = i64TLatest = rcvdBytes = i64TotBytes = 0;
    if (_pPercRcvdBytesByGrp->next (pRow)) {
        if (pRow->getValue (0, i64TFirst) < 0 ||
            pRow->getValue (1, i64TLatest) < 0 ||
            pRow->getValue (2, rcvdBytes) < 0 ||
            pRow->getValue (3, i64TotBytes) < 0) {
           rc = -2;
        }
    }
    else if (_pPercRcvdBytesByGrp->next (pRow)) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes", Logger::L_SevereError,
                        "_pPercRcvdBytesByGrp returned multiple rows when 1 was expected for group %s\n",
                        pszGroupName);
    }
    _pPercRcvdBytesByGrp->reset();
    delete pRow;
    float fRet = (rc == 0 && i64TotBytes > 0 ? rcvdBytes/i64TotBytes : 0.0f);
    _m.unlock (227);
    return fRet;
}

float Analytics::getPercentRcvdBytes (const char *pszGroupName, const char *pszSenderNodeId,
                                      int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes)
{
    /*_m.lock();
    // Bind the values to the prepared statement
    if ((sqlite3_bind_text (_pPercRcvdBytesByGrpSdr, 1, pszGroupName, strlen(pszGroupName), SQLITE_STATIC) != SQLITE_OK) ||
        (sqlite3_bind_text (_pPercRcvdBytesByGrpSdr, 2, pszSenderNodeId, strlen(pszSenderNodeId), SQLITE_STATIC) != SQLITE_OK)) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes (2)", Logger::L_SevereError, "Error when binding values.\n");
        sqlite3_reset(_pPercRcvdBytesByGrpSdr);
        _m.unlock();
        return 0.0f;
    }

    // Execute the statement
    int rc;
    int64 rcvdBytes;
    i64TFirst = i64TLatest = rcvdBytes = i64TotBytes = 0;
    while ((rc = sqlite3_step(_pPercRcvdBytesByGrpSdr)) == SQLITE_ROW) {
        i64TFirst = sqlite3_column_int64 (_pPercRcvdBytesByGrpSdr, 0);
        i64TLatest = sqlite3_column_int64 (_pPercRcvdBytesByGrpSdr, 1);
        rcvdBytes = sqlite3_column_int64 (_pPercRcvdBytesByGrpSdr, 2);
        i64TotBytes = sqlite3_column_int64 (_pPercRcvdBytesByGrpSdr, 3);
    }
    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes (2)", Logger::L_SevereError,
                        "sqlite3_step returned %s. The parameter was: %s,%s.\n",
                        SQLiteFactory::getErrorAsString(rc), pszGroupName, pszSenderNodeId);
    }
    sqlite3_reset(_pPercRcvdBytesByGrpSdr);

    float fRet = (i64TotBytes > 0 ? rcvdBytes/i64TotBytes : 0.0f);
    _m.unlock();
    return fRet;*/
    return 0.0f;
}

float Analytics::getPercentRcvdBytes (const char *pszGroupName, const char *pszSenderNodeId, uint32 ui32MsgSeqId,
                                      int64 &i64TFirst, int64 &i64TLatest, int64 &i64TotBytes)
{
    /*
    _m.lock();
    if (pPercRcvdBytes->bind (1, pszGroupName) < 0 ||
        (sqlite3_bind_text (_pPercRcvdBytes, 2, pszSenderNodeId, strlen(pszSenderNodeId), SQLITE_STATIC) != SQLITE_OK) ||
        (sqlite3_bind_int (_pPercRcvdBytes, 3, ui32MsgSeqId) != SQLITE_OK)) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes (3)", Logger::L_SevereError, "Error when binding values.\n");
        sqlite3_reset(_pPercRcvdBytes);
        _m.unlock();
        return 0.0f;
    }

    // Execute the statement
    int rc;
    int64 rcvdBytes;
    i64TFirst = i64TLatest = rcvdBytes = i64TotBytes = 0;
    while ((rc = sqlite3_step(_pPercRcvdBytes)) == SQLITE_ROW) {
        i64TFirst = sqlite3_column_int64 (_pPercRcvdBytes, 0);
        i64TLatest = sqlite3_column_int64 (_pPercRcvdBytes, 1);
        rcvdBytes = sqlite3_column_int64 (_pPercRcvdBytes, 2);
        i64TotBytes = sqlite3_column_int64 (_pPercRcvdBytes, 3);
    }
    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        checkAndLogMsg ("Analytics::getPercentRcvdBytes (3)", Logger::L_SevereError,
                        "sqlite3_step returned %s. The parameter was: %s,%s.\n",
                        SQLiteFactory::getErrorAsString(rc), pszGroupName, pszSenderNodeId);
    }

    float fRet = (i64TotBytes > 0 ? rcvdBytes/i64TotBytes : 0.0f);
    _m.unlock();
    return fRet;*/
    return 0.0f;
}

int Analytics::getPercentMissingMessages (const char *pszGroupName, uint32 &ui32RcvdMsg,
                                          uint32 &ui32MissingMsg, int64 &i64Time)
{
    // TODO: implement this
    return 0;
}

