/*
 * RangeDLList.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Created on March 21, 2011, 4:54 PM
 */

#include "RangeDLList.h"

#include "BufferWriter.h"
#include "Logger.h"
#include "Reader.h"

#include <string.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

int write16 (char * pBuf, void *pInt);

//------------------------------------------------------------------------------
//    UInt8RangeDLList
//------------------------------------------------------------------------------

UInt8RangeDLList::UInt8RangeDLList (bool bUseSequentialArithmetic)
    : RangeDLList<uint8> (bUseSequentialArithmetic, 0U, 0xFF)
{
}

UInt8RangeDLList::~UInt8RangeDLList (void)
{
}

int UInt8RangeDLList::read (Reader *pReader, uint32 ui32MaxSize)
{
    const char * const pszMethod = "UInt8RangeDLList::read";
    uint16 ui16Count;
    uint8 ui8BeginTSN;

    // Read TSN ranges
    pReader->read16 (&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
            uint8 ui8EndTSN;
            pReader->read8 (&ui8BeginTSN);
            pReader->read8 (&ui8EndTSN);
            for (uint8 ui8 = ui8BeginTSN;  SequentialArithmetic::lessThanOrEqual (ui8, ui8EndTSN); ui8++) {
                addTSN (ui8);
            }
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Reading TSNs:\t%u - %u\n",
                            ui8BeginTSN, ui8EndTSN);
    }

    // Read individual TSNs
    pReader->read16 (&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
        pReader->read8 (&ui8BeginTSN);
        addTSN (ui8BeginTSN);
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                        "Reading TSN:\t%u\n", ui8BeginTSN);
    }

    return 0;
}

int UInt8RangeDLList::write (BufferWriter *pBWriter, uint32 ui32MaxSize)
{
    const char * const pszMethod = "UInt8RangeDLList::write";
    uint16 ui16Index = (uint16)pBWriter->getBufferLength();
    uint16 ui16Len;

    // Keep 2 bytes for the number of ranges - for now write a "0", later it'll be updated with the actual number of ranges
    uint16 ui16Count = 0;
    pBWriter->write16 (&ui16Len);

    // Append the ranges
    // NOTE: 2 more bytes are needed to write the number of individual TSNs
    // if this condition is reached, the number of individual TSNs will be 0).
    for (Range *pCurrNode = _pFirstNode; pCurrNode && ((pBWriter->getBufferLength() + 2) < ui32MaxSize); pCurrNode = pCurrNode->pNext) {
        if (pCurrNode->begin == pCurrNode->end) {
            // Skip for now
            continue;
        }
        else {
            uint8 ui8 = pCurrNode->begin;
            pBWriter->write8 (&ui8);
            ui8 = pCurrNode->end;
            pBWriter->write8 (&ui8);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Writing TSNs:\t%u - %u\n",
                            pCurrNode->begin, pCurrNode->end);
        }
    }

    // Write the number of ranges
    char *pBuf = (char *) pBWriter->getBuffer();
    write16 (&pBuf[ui16Index], &ui16Count);
    ui16Index = (uint16) pBWriter->getBufferLength();

    // Keep 2 bytes for the number of individual TSNs
    ui16Count = 0;
    pBWriter->write16 (&ui16Count);

    // Now append the individual TSNs
    for (Range *pCurrNode = _pFirstNode; pCurrNode; pCurrNode = pCurrNode->pNext) {
        if (pCurrNode->begin == pCurrNode->end) {
            uint8 ui8 = pCurrNode->begin;
            pBWriter->write8 (&ui8);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Writing TSN:\t%u\n", ui8);
        }
    }

    // Write the number of individual TSNs
    pBuf = (char *) pBWriter->getBuffer();
    write16 (&pBuf[ui16Index], &ui16Count);

    return 0;
}

//------------------------------------------------------------------------------
//    UInt16RangeDLList
//------------------------------------------------------------------------------

UInt16RangeDLList::UInt16RangeDLList (bool bUseSequentialArithmetic)
    : RangeDLList<uint16> (bUseSequentialArithmetic, 0U, 0xFFFF)
{
}

UInt16RangeDLList::~UInt16RangeDLList (void)
{
}

int UInt16RangeDLList::read (Reader *pReader, uint32 ui16MaxSize)
{
    const char * const pszMethod = "UInt16RangeDLList::read";
    uint16 ui16Count; uint16 ui16BeginTSN;

    // Read TSN ranges
    pReader->read16(&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
            uint16 ui16EndTSN;
            pReader->read16(&ui16BeginTSN);
            pReader->read16(&ui16EndTSN);
            for (uint16 ui16 = ui16BeginTSN;  SequentialArithmetic::lessThanOrEqual (ui16, ui16EndTSN); ui16++) {
                addTSN (ui16);
            }
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Reading TSNs:\t%u - %u\n",
                            ui16BeginTSN, ui16EndTSN);
    }

    // Read individual TSNs
    pReader->read16(&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
        pReader->read16(&ui16BeginTSN);
        addTSN (ui16BeginTSN);
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                        "Reading TSN:\t%u\n", ui16BeginTSN);
    }

    return 0;
}

int UInt16RangeDLList::write (BufferWriter *pBWriter, uint32 ui32MaxSize)
{
    const char * const pszMethod = "UInt16RangeDLList::write";
    uint16 ui16Index = (uint16)pBWriter->getBufferLength();
    uint16 ui16Len;

    // Keep 2 bytes for the number of ranges
    uint16 ui16Count = 0;
    pBWriter->write16(&ui16Len);

    // Append the ranges
    // NOTE: 2 more bytes are needed to write the number of individual TSNs
    // if this condition is reached, the number of individual TSNs will be 0).
    for (Range *pCurrNode = _pFirstNode; pCurrNode && ((pBWriter->getBufferLength() + 2) < ui32MaxSize); pCurrNode = pCurrNode->pNext) {
        if (pCurrNode->begin == pCurrNode->end) {
            // Skip for now
            continue;
        }
        else {
            ui16Len = pCurrNode->begin;
            pBWriter->write16(&ui16Len);
            ui16Len = pCurrNode->end;
            pBWriter->write16(&ui16Len);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Writing TSNs:\t%u - %u\n",
                            pCurrNode->begin, pCurrNode->end);
        }
    }

    // Write the number of ranges
    char * pBuf = (char *) pBWriter->getBuffer();
    write16 (&pBuf[ui16Index], &ui16Count);
    ui16Index = (uint16)pBWriter->getBufferLength();

    // Keep 2 bytes for the number of individual TSNs
    ui16Count = 0;
    pBWriter->write16(&ui16Count);

    // Now append the individual TSNs
    for (Range *pCurrNode = _pFirstNode; pCurrNode; pCurrNode = pCurrNode->pNext) {
        if (pCurrNode->begin == pCurrNode->end) {
            ui16Len = pCurrNode->begin;
            pBWriter->write16(&ui16Len);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                              "Writing TSN:\t%u\n", ui16Len);
        }
    }

    // Write the number of individual TSNs
    pBuf = (char *) pBWriter->getBuffer();
    write16 (&pBuf[ui16Index], &ui16Count);

    return 0;
}

//------------------------------------------------------------------------------
//    UInt32RangeDLList
//------------------------------------------------------------------------------

UInt32RangeDLList::UInt32RangeDLList (bool bUseSequentialArithmetic)
    : RangeDLList<uint32> (bUseSequentialArithmetic, 0U, 0xFFFFFFFF)
{
}

UInt32RangeDLList::~UInt32RangeDLList()
{
}

void UInt32RangeDLList::display (FILE *pFileOut)
{
    for (Range *pCurrNode = _pFirstNode; pCurrNode; pCurrNode = pCurrNode->pNext) {
        fprintf (pFileOut, "%u-%u\t", pCurrNode->begin, pCurrNode->end);
    }
    fprintf (pFileOut, "\n");
}

int UInt32RangeDLList::read (Reader *pReader, uint32 ui32MaxSize)
{
    const char * const pszMethod = "UInt32RangeDLList::read";
    // Read ranges
    uint16 ui16Count;
    uint32 ui32Begin;
    uint32 ui32End;

    // Read TSN ranges
    pReader->read16 (&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
        pReader->read32 (&ui32Begin);
        pReader->read32 (&ui32End);
        for (uint32 ui32 = ui32Begin;  SequentialArithmetic::lessThanOrEqual (ui32, ui32End); ui32++) {
            addTSN (ui32);
        }
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Reading TSNs:\t%u - %u\n", ui32Begin, ui32End);
    }

    // Read individual TSNs
    pReader->read16(&ui16Count);
    for (uint16 i = 0; i < ui16Count; i++) {
        pReader->read32 (&ui32Begin);
        addTSN (ui32Begin);
        checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug, "Reading TSN:\t%u\n", ui32Begin);
    }

    return 0;
}

int UInt32RangeDLList::write (BufferWriter *pBWriter, uint32 ui32MaxSize)
{
    const char * const pszMethod = "UInt32RangeDLList::write";
    // Write ranges. Format: [uint16]#-of-ranges|ranges|[uint16]#-of-individual-TSNs|individual-TSNs

    uint32 ui32Index = pBWriter->getBufferLength();

    // Keep 2 bytes for the number of ranges - for now write a "0", later it'll be updated with the actual number of ranges
    uint16 ui16Count = 0;
    pBWriter->write16(&ui16Count);

    // Append the ranges: [uint32]range1.begin|[uint32]range1.end|[uint32]range2.begin|[uint32]range2.end|...
    for (Range *pCurrNode = _pFirstNode; pCurrNode && ((pBWriter->getBufferLength() + 2) < ui32MaxSize); pCurrNode = pCurrNode->pNext) {
        uint32 ui32Begin = pCurrNode->begin;
        uint32 ui32End = pCurrNode->end;
        if (ui32Begin == ui32End) {
            // Skip for now - individual TSNs are written later, after ranges
            continue;
        }
        else {
            pBWriter->write32 (&ui32Begin);
            pBWriter->write32 (&ui32End);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                            "Writing TSNs:\t%u - %u\n", ui32Begin, ui32End);
        }
    }

    // Update the number of ranges
    char *pBuf = (char *) pBWriter->getBuffer();
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (&ui16Count);
    #endif
    memcpy (&pBuf[ui32Index], &ui16Count, 2);

    ui32Index = (uint32)pBWriter->getBufferLength();

    // Keep 2 bytes for the number of individual TSNs - for now write a "0", later it'll be updated with the actual number ind. TSNs
    ui16Count = 0;
    pBWriter->write16(&ui16Count);

    // Append the individual TSNs: [uint32]TSN1|[uint32]TSN2|...
    for (Range *pCurrNode = _pFirstNode; pCurrNode && ((pBWriter->getBufferLength() + 2) < ui32MaxSize); pCurrNode = pCurrNode->pNext) {
        uint32 ui32Begin = pCurrNode->begin;
        uint32 ui32End = pCurrNode->end;
        if (ui32Begin == ui32End) {
            pBWriter->write32 (&ui32Begin);
            ui16Count++;
            checkAndLogMsg (pszMethod, Logger::L_HighDetailDebug,
                                "Writing TSN:\t%u\n", ui32Begin);
        }
    }

    // Update the number of individual TSNs
    pBuf = (char *) pBWriter->getBuffer();
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (&ui16Count);
    #endif
    memcpy (&pBuf[ui32Index], &ui16Count, 2);

    return 0;
}

//------------------------------------------------------------------------------

int write16 (char *pBuf, void *pInt)
{
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (pInt);
    #endif
    memcpy (pBuf, pInt, 2);
    #if defined (LITTLE_ENDIAN_SYSTEM)
        Reader::byteSwap16 (pInt);
    #endif
    return 0;
}
