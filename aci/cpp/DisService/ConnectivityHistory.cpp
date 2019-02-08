/*
 * ConnectivityHistory.cpp
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
 * Author: Mirko Gilioli    mgilioli@ihmc.us
 * Created on May 21, 2009, 2:57 PM
 */

#include "ConnectivityHistory.h"

#include "DisServiceDefs.h"
#include "Logger.h"
#include "NLFLib.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

ConnectivityHistory::ConnectivityHistory (int64 i64WinSize)
    : _ui8CountNodes (0),
      _i64WinSize (i64WinSize),
      _m (4)
{
}

ConnectivityHistory::~ConnectivityHistory()
{
    Entry *pEntry;
    Entry *pEntryTmp = _sortedList.getFirst();
    while ((pEntry = pEntryTmp) != NULL) {
        pEntryTmp = _sortedList.getNext();
        _sortedList.remove (pEntry);
        delete pEntry;
    }
    _ui8CountNodes = 0;
}

int ConnectivityHistory::add (const char *pszNodeId)
{
    if (pszNodeId == NULL) {
        checkAndLogMsg ("ConnectivityHistory::add", Logger::L_MildError, "pszNodeId is NULL\n");
        return -1;
    }

    _m.lock (10);
    if (_i64WinSize == 0) {
        // the ConnectivityHistory is disabled
        _m.unlock (10);
        return 0;
    }

    Entry *pNewEntry = new Entry (pszNodeId, getTimeInMilliseconds());
    if (pNewEntry == NULL) {
        checkAndLogMsg ("ConnectivityHistory::add", memoryExhausted);
        _m.unlock (10);
        return -2;
    }

    Entry *pOldEntry = _sortedList.remove (pNewEntry);
    //If the New Entry is already present in the SortedList, delete it
    if (pOldEntry != NULL) {
        delete pOldEntry;
        pOldEntry = NULL;
        _ui8CountNodes--;
    }
    //Append the new Entry and increment the nodes counter
    if (_ui8CountNodes < 255) {
        _ui8CountNodes++;
    }
    _sortedList.append (pNewEntry);

    _m.unlock (10);
    return 0;
}

uint8 ConnectivityHistory::getCount()
{
    uint8 ui8TempCounter;
    Entry *pEntry;
    _m.lock (11);
    if (_i64WinSize == 0) {
        // the ConnectivityHistory is disabled
        _m.unlock (11);
        return (uint8) 0;
    }
    Entry *pEntryTmp = _sortedList.getFirst();
    while (((pEntry = pEntryTmp) != NULL) &&
           (pEntry->_i64TimeStamp < (getTimeInMilliseconds() - _i64WinSize))) {
        pEntryTmp = _sortedList.getNext();
        _sortedList.remove (pEntry);
        _ui8CountNodes--;
        delete pEntry;
    }
    ui8TempCounter = _ui8CountNodes;
    _m.unlock (11);
    return ui8TempCounter;
}

bool ConnectivityHistory::Entry::operator == (const Entry& rhsEntry)
{
    return (this->_nodeId == rhsEntry._nodeId);
}

