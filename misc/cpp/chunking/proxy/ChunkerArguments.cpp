/*
 * ChunkerArguments.cpp
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

#include "ChunkerArguments.h"

#include "Writer.h"

using namespace IHMC_MISC;
using namespace NOMADSUtil;

FragmentBufferArguments::FragmentBufferArguments (void)
    : _ui8NoOfChunks (0),
      _ui8ChunkCompressionQuality (0),
      _ui32Len (0U),
      _pBuf (NULL)
{
}

FragmentBufferArguments::FragmentBufferArguments (const void *pBuf, uint32 ui32Len, const char *pszInputMimeType,
                                                  uint8 ui8NoOfChunks, const char *pszOutputMimeTypee,
                                                  uint8 ui8ChunkCompressionQuality)
    : _ui8NoOfChunks (ui8NoOfChunks),
      _ui8ChunkCompressionQuality (ui8ChunkCompressionQuality),
      _inputObjectType (pszInputMimeType),
      _outputChunkType (pszOutputMimeTypee),
      _ui32Len (ui32Len),
      _pBuf (pBuf)
{
}

FragmentBufferArguments::~FragmentBufferArguments (void)
{
}

int FragmentBufferArguments::read (Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }
    if (pReader->read8 (&_ui8NoOfChunks) < 0) {
        return -2;
    }
    if (pReader->read8 (&_ui8ChunkCompressionQuality) < 0) {
        return -3;
    }
    char *pszString = NULL;
    if (pReader->readString (&pszString) < 0) {
        return -4;
    }
    _inputObjectType = pszString;
    free (pszString);
    if (pReader->readString (&pszString) < 0) {
        return -5;
    }
    _outputChunkType = pszString;
    free (pszString);
    if (pReader->read32 (&_ui32Len) < 0) {
        return -6;
    }
    if (_ui32Len > 0) {
        void *pBuf = malloc (_ui32Len);
        if (pBuf == NULL) {
            _ui32Len = 0U;
            return -7;
        }
        if (pReader->readBytes (pBuf, _ui32Len) < 0) {
            free (pBuf);
            return -8;
        }
        _pBuf = pBuf;
    }
    return 0;
}

int FragmentBufferArguments::write (Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }
    if (pWriter->write8 (&_ui8NoOfChunks) < 0) {
        return -2;
    }
    if (pWriter->write8 (&_ui8ChunkCompressionQuality) < 0) {
        return -3;
    }
    if (pWriter->writeString (_inputObjectType) < 0) {
        return -4;
    }
    if (pWriter->writeString (_outputChunkType) < 0) {
        return -5;
    }
    if (pWriter->write32 (&_ui32Len) < 0) {
        return -6;
    }
    if (pWriter->writeBytes (_pBuf, _ui32Len) < 0) {
        return -7;
    }
    return 0;
}

ExtractFromBufferArguments::ExtractFromBufferArguments (void)
    : FragmentBufferArguments(),
      _ppPortionIntervals (NULL)
{
}

ExtractFromBufferArguments::ExtractFromBufferArguments (const void *pBuf, uint32 ui32Len,
                                                        const char *pszInputMimeType, const char *pszOutputMimeTypee,
                                                        uint8 ui8ChunkCompressionQuality, Chunker::Interval **ppPortionIntervals)
    : FragmentBufferArguments (pBuf, ui32Len, pszInputMimeType, 0, pszOutputMimeTypee, ui8ChunkCompressionQuality),
      _ppPortionIntervals (ppPortionIntervals)
{
}

ExtractFromBufferArguments::~ExtractFromBufferArguments (void)
{
}

int ExtractFromBufferArguments::read (Reader *pReader)
{
    int rc = FragmentBufferArguments::read (pReader);
    if (rc < 0) {
        return rc;
    }
    uint8 ui8Count = 0;
    if (pReader->read8 (&ui8Count) < 0) {
        return -9;
    }
    if (ui8Count == 0) {
        return -10;
    }
    _ppPortionIntervals = static_cast<Chunker::Interval **>(calloc(ui8Count + 1, sizeof (Chunker::Interval *)));
    if (_ppPortionIntervals == NULL) {
        return -11;
    }
    for (uint8 i = 0; i < ui8Count; i++) {
        _ppPortionIntervals[i] = new Chunker::Interval();
        if (_ppPortionIntervals[i] == NULL) {
            return -12;
        }
        if (_ppPortionIntervals[i]->read (pReader) < 0) {
            return -13;
        }
    }
    return 0;
}

int ExtractFromBufferArguments::write (Writer *pWriter)
{
    int rc = FragmentBufferArguments::write (pWriter);
    if (rc < 0) {
        return rc;
    }
    uint8 ui8Count = 0;
    for (; _ppPortionIntervals[ui8Count] != NULL; ui8Count++);
    if (pWriter->write8 (&ui8Count) < 0) {
        return -8;
    }
    for (; _ppPortionIntervals[ui8Count] != NULL; ui8Count++) {
        if (_ppPortionIntervals[ui8Count]->write (pWriter) < 0) {
            return -9;
        }
    }
    return 0;
}

