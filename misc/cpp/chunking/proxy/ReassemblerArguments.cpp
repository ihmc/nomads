/*
 * ReassemblerArguments.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) 2010-2016 IHMC.
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

#include "ReassemblerArguments.h"

#include "Writer.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

ReassembleArguments::ReassembleArguments (void)
    : _ui8NoOfChunks (0),
      _ui8CompressionQuality (0),
      _pFragments (NULL),
      _pAnnotations (NULL)
{
}

ReassembleArguments::ReassembleArguments (Chunks *pFragments,
                                          Annotations *pAnnotations, const char *pszChunkMimeType,
                                          uint8 ui8NoOfChunks, uint8 ui8CompressionQuality)
    : _ui8NoOfChunks (ui8NoOfChunks),
      _ui8CompressionQuality (ui8CompressionQuality),
      _chunkType (pszChunkMimeType),
      _pFragments (pFragments),
      _pAnnotations (pAnnotations)
{
}

ReassembleArguments::~ReassembleArguments (void)
{
}

int ReassembleArguments::read (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->read8 (&_ui8NoOfChunks) < 0) {
        return -2;
    }
    if (pReader->read8 (&_ui8CompressionQuality) < 0) {
        return -3;
    }
    char *pszChunkMimeType = NULL;
    if (pReader->readString (&pszChunkMimeType) < 0) {
        return -4;
    }
    _chunkType = pszChunkMimeType;

    // Read chunks
    if (_pFragments->read (pReader) < 0) {
        return -5;
    }

    // Read annotations
    uint8 ui8 = 0;
    if (pReader->read8 (&ui8) < 0) {
        return -6;
    }
    if (ui8 == 0) {
        return 0;
    }
    _pAnnotations = new Annotations();
    for (uint8 i = 0; i < ui8; i++) {
        Annotation *pAnnotation = new Annotation (true);
        if (pAnnotation == NULL) {
            return -7;
        }
        if (pAnnotation->read (pReader) < 0) {
            return -8;
        }
    }

    return 0;
}

int ReassembleArguments::write (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->write8 (&_ui8NoOfChunks) < 0) {
        return -2;
    }
    if (pWriter->write8 (&_ui8CompressionQuality) < 0) {
        return -3;
    }
    if (pWriter->writeString (_chunkType) < 0) {
        return -4;
    }

    // Write chunks
    if (_pFragments->write (pWriter) < 0) {
        return -5;
    }

    // Write annotations
    int iCount = _pAnnotations->getCount();
    if ((iCount < 0) || (iCount > 0xFF)) {
        return -6;
    }
    uint8 ui8 = (uint8) iCount;
    if (pWriter->write8 (&ui8) < 0) {
        return -7;
    }
    if (ui8 > 0) {
        Annotation *pAnnotation = _pAnnotations->getFirst();
        for (; pAnnotation != NULL; pAnnotation = _pAnnotations->getNext()) {
            if (pAnnotation->write (pWriter) < 0) {
                return -8;
            }
        }
    }

    return 0;
}

