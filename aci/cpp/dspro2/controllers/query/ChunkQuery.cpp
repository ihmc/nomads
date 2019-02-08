/*
 * ChunkQuery.cpp
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

#include "ChunkQuery.h"

#include  "Writer.h"

using namespace IHMC_ACI;
using namespace IHMC_MISC;
using namespace NOMADSUtil;

const String ChunkQuery::GROUP = "dspcq";
const String ChunkQuery::QUERY_TYPE = "dsprochunkquery";

ChunkQuery::ChunkQuery (void)
    : _ui8CompressionQuality (100)
{
}

ChunkQuery::ChunkQuery (const char *pszBaseObjMsgId, IHMC_MISC::Chunker::Type inputType,
                        IHMC_MISC::Chunker::Type outputType, uint8 ui8CompressionQuality)
    : _ui8CompressionQuality (ui8CompressionQuality),
      _chunkDescr (inputType, outputType),
      _objectMsgId (pszBaseObjMsgId)
{
}

ChunkQuery::~ChunkQuery(void)
{
}

int ChunkQuery::addInterval (IHMC_MISC::Chunker::Interval &interval)
{
    _chunkDescr._dimensions.add (interval);
    return 0;
}

int ChunkQuery::read (Reader *pReader)
{
    if (pReader == nullptr) {
        return -1;
    }
    if (pReader->readUI8 (&_ui8CompressionQuality)) {
        return -2;
    }
    char *psztmp = nullptr;
    if ((pReader->readString (&psztmp) < 0) || (psztmp == nullptr)) {
        return -3;
    }
    _objectMsgId = psztmp;
    free (psztmp);
    if (_objectMsgId.length() <= 0) {
        return -4;
    }
    if (_chunkDescr.read (pReader) < 0) {
        return -5;
    }
    return 0;
}

int ChunkQuery::write (Writer *pWriter)
{
    if (pWriter == nullptr) {
        return -1;
    }
    if (pWriter->writeUI8 (&_ui8CompressionQuality)) {
        return -2;
    }
    if (_objectMsgId.length() <= 0) {
        return -3;
    }
    if (pWriter->writeString (_objectMsgId) < 0) {
        return -4;
    }
    if (_chunkDescr.write (pWriter) < 0) {
        return - 5;
    }
    return 0;
}

