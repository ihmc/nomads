/*
 * TSNRangeHandler.cpp
 *
 * This file is part of the IHMC Util Library
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
 */

#include "TSNRangeHandler.h"

#include "BufferWriter.h"
#include "Reader.h"
#include "Logger.h"

#include <string.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

/*!!*/ // Need to fix bug where the cumulative TSN should not be here because it
       // will cause a problem when
       // this class is used for the cancelled chunk

using namespace NOMADSUtil;

int SAckTSNRangeHandler::addTSN (uint16 ui16TSN)
{
    if (SequentialArithmetic::greaterThanOrEqual (_ui16CumulativeTSN, ui16TSN)) {
        return 0;
    }

    int rc = UInt16RangeDLList::addTSN (ui16TSN);

    // Check to see if the last cumulative acknowledgement must be updated
    uint16 ui16Tmp = _ui16CumulativeTSN;
    ui16Tmp++;
    if (ui16Tmp == _pFirstNode->begin) {
        _ui16CumulativeTSN = _pFirstNode->end;
        Range *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
        if (_pFirstNode == NULL) {
            // The list is empty
            _pLastNode = NULL;
        }
    }

    return rc;
}

int SAckTSNRangeHandler::read (Reader *pReader, uint16 ui16MaxSize)
{
    pReader->read16(&_ui16CumulativeTSN);
    checkAndLogMsg ("SAckTSNRangeHandler::read", Logger::L_HighDetailDebug,
                    "Reading Cumulative SAck\t%u\n", _ui16CumulativeTSN);
    return UInt16RangeDLList::read (pReader, ui16MaxSize-2);
}

int SAckTSNRangeHandler::write (BufferWriter *pWriter, uint16 ui16MaxSize)
{
    pWriter->write16(&_ui16CumulativeTSN);
    checkAndLogMsg ("SAckTSNRangeHandler::write", Logger::L_HighDetailDebug,
                    "Writing Cumulative SAck\t%u\n", _ui16CumulativeTSN);
    return UInt16RangeDLList::write (pWriter, ui16MaxSize-sizeof(_ui16CumulativeTSN));
}

