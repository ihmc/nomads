/*
 * CircularOrderedBuffer.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2018 IHMC.
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

#include <cstring>
#include <algorithm>

#include "net/NetworkHeaders.h"
#include "Logger.h"

#include "CircularOrderedBuffer.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    CircularOrderedBuffer::CircularOrderedBuffer (unsigned int & uiStartingSequenceNumber, unsigned int uiMaxSize, unsigned int uiMinSize) :
        _pui8Buf{new unsigned char[uiMinSize]}, _uiDataReaderPointer{0}, _uiBufMinSize{uiMinSize}, _uiBufMaxSize{uiMaxSize},
        _uiBufCurrentSize{uiMinSize}, _uiReadingSequenceNumber{0}, _uiNextExpectedSequenceNumber{uiStartingSequenceNumber},
        _iReadyBytesInBuffer{0}, _iTotalBytesInBuffer{0}, _plSeparateNodesList{false}
    { }

    CircularOrderedBuffer::~CircularOrderedBuffer (void)
    {
        if (_pui8Buf) {
            delete[] _pui8Buf;
        }
        _pui8Buf = nullptr;

        TCPSegment *pOBW = nullptr;
        while ((pOBW = _plSeparateNodesList.getFirst())) {
            delete _plSeparateNodesList.remove (pOBW);
        }
    }

    int CircularOrderedBuffer::insertData (TCPSegment * const pOrderableItem, bool bOverwriteData)
    {
        if (!pOrderableItem) {
            delete pOrderableItem;
            return -1;
        }
        if (!fitsInBuffer (pOrderableItem)) {
            // Item Sequence Number is out of the range covered by the CircularBuffer
            delete pOrderableItem;
            return -2;
        }
        if (!isDataUseful (pOrderableItem, bOverwriteData)) {
            // Check if received item contains useful data
            delete pOrderableItem;
            return 0;
        }

        if (growBufferIfNecessary (pOrderableItem) < 0) {
            delete pOrderableItem;
            return -3;
        }

        int insertedBytes = 0;
        auto ui8Flags = pOrderableItem->getTCPFlags();
        if (NOMADSUtil::SequentialArithmetic::greaterThan (_uiNextExpectedSequenceNumber, pOrderableItem->getSequenceNumber())) {
            unsigned int uiUselessData = NOMADSUtil::SequentialArithmetic::delta (_uiNextExpectedSequenceNumber, pOrderableItem->getSequenceNumber());
            if (uiUselessData > pOrderableItem->getItemLength()) {
                delete pOrderableItem;
                return -4;
            }
            pOrderableItem->incrementValues (uiUselessData);
        }

        auto * pOBW = _plSeparateNodesList.getFirst();
        if (!pOBW) {
            // No items in the buffer
            if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                delete pOrderableItem;
                return -5;
            }
            /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                            "buffer action: packet with SEQ number %u, byte size %u (address in buffer %p), and "
                            "following packet SEQ number %u has been added and it is the only one in the buffer\n",
                            pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                            getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                            pOrderableItem->getFollowingSequenceNumber());*/
            pOrderableItem->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
            _plSeparateNodesList.append (pOrderableItem);
            insertedBytes = pOrderableItem->getItemLength();
        }
        else if (pOrderableItem->getSequenceNumber() == _uiNextExpectedSequenceNumber) {
            // The newly received item is the expected one
            if (pOrderableItem->follows (*pOBW)) {
                // An item is already in the buffer and the one that immediately follows it has been received
                TCPSegment *pOBWNext = _plSeparateNodesList.getNext();
                if (!pOBWNext || (!pOrderableItem->overlaps (*pOBWNext) && !pOrderableItem->isFollowedBy (*pOBWNext))) {
                    // No out-of-order items were previously received or they cannot be joined with the new one --> enqueuing new item and joining it with the previously buffered one
                    if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                        delete pOrderableItem;
                        return -6;
                    }

                    /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                    "buffer action: packet with the expected SEQ number %u, byte size %u (address in buffer %p), and following "
                                    "packet SEQ number %u has been merged with the preceeding packet with SEQ number %u and byte size %u "
                                    "(address in buffer %p, following packet with SEQ number %u)\n", pOrderableItem->getSequenceNumber(),
                                    pOrderableItem->getItemLength(), getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                    pOrderableItem->getFollowingSequenceNumber(), pOBW->getSequenceNumber(), pOBW->getItemLength(),
                                    pOBW->getData(), pOBW->getFollowingSequenceNumber());*/
                    pOBW->setItemLength (pOBW->getItemLength() + pOrderableItem->getItemLength());
                    pOBW->addTCPFlags (ui8Flags);
                    insertedBytes = pOrderableItem->getItemLength();
                    delete pOrderableItem;
                }
                else if (bOverwriteData) {
                    // At least one out-of-order item is already in the buffer and any overlapping data can be overwritten
                    if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                        delete pOrderableItem;
                        return -7;
                    }
                    while (pOBWNext && (pOrderableItem->overlaps (*pOBWNext) || pOrderableItem->isFollowedBy (*pOBWNext))) {
                        ui8Flags |= pOBW->getTCPFlags();
                        delete _plSeparateNodesList.remove (pOBW);
                        pOBW = pOBWNext;
                        pOBWNext = _plSeparateNodesList.getNext();
                    }
                    ui8Flags |= pOBW->getTCPFlags();
                    unsigned int uiHighestSequenceNumber = NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (pOrderableItem->getFollowingSequenceNumber(), pOBW->getFollowingSequenceNumber()) ?
                        pOrderableItem->getFollowingSequenceNumber() : pOBW->getFollowingSequenceNumber();
                    delete _plSeparateNodesList.replace (pOBW, new TCPSegment (_uiReadingSequenceNumber, NOMADSUtil::SequentialArithmetic::delta (uiHighestSequenceNumber, _uiReadingSequenceNumber),
                                                                               getPositionInBuffer (_uiReadingSequenceNumber), ui8Flags));
                    insertedBytes = pOrderableItem->getItemLength();
                    delete pOrderableItem;
                }
                else {
                    // Inserting new data between pOBW (_uiNextExpectedSequenceNumber) and pOBWNext
                    unsigned int uiBytesToWrite = NOMADSUtil::SequentialArithmetic::delta (pOBWNext->getSequenceNumber(), _uiNextExpectedSequenceNumber);
                    if (uiBytesToWrite != copyBytesToCircularBuffer (pOrderableItem, _uiNextExpectedSequenceNumber, pOBWNext->getSequenceNumber())) {
                        delete pOrderableItem;
                        return -8;
                    }

                    /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                    "buffer action: packet with the expected SEQ number %u, byte size %u (address in buffer %p), "
                                    "and following packet SEQ number %u fills the gap between the packet with SEQ number %u and "
                                    "byte size %u (address in buffer %p, following packet with SEQ number %u) and the packet with "
                                    "SEQ number %u and byte size %u (address in buffer %p, following packet with SEQ number %u)\n",
                                    pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                    getPositionInBuffer (pOrderableItem->getSequenceNumber()), pOrderableItem->getFollowingSequenceNumber(),
                                    pOBW->getSequenceNumber(), pOBW->getItemLength(), pOBW->getData(), pOBW->getFollowingSequenceNumber(),
                                    pOBWNext->getSequenceNumber(), pOBWNext->getItemLength(), pOBWNext->getData(),
                                    pOBWNext->getFollowingSequenceNumber());*/
                    pOBW->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOBWNext->getFollowingSequenceNumber(), _uiReadingSequenceNumber));
                    ui8Flags |= pOBWNext->getTCPFlags();
                    pOBW->addTCPFlags (ui8Flags);
                    insertedBytes = uiBytesToWrite;
                    delete _plSeparateNodesList.remove (pOBWNext);
                    delete pOrderableItem;
                }
            }
            else if (pOrderableItem->isFollowedBy (*pOBW) || pOrderableItem->overlaps (*pOBW)) {
                // An item is already in the buffer and it overlaps/comes immediately after the one with the expected SEQ number just received
                if (bOverwriteData) {
                    // New item contains useful data - overwriting data and joining old item with the new one; also, check if this will reunite two or more disjointed items
                    if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                        delete pOrderableItem;
                        return -9;
                    }

                    TCPSegment *pOBWNext = nullptr;
                    while ((pOBWNext = _plSeparateNodesList.getNext()) && (pOrderableItem->overlaps (*pOBWNext) || pOrderableItem->isFollowedBy (*pOBWNext))) {
                        ui8Flags |= pOBW->getTCPFlags();
                        delete _plSeparateNodesList.remove (pOBW);
                        pOBW = pOBWNext;
                    }
                    ui8Flags |= pOBW->getTCPFlags();
                    unsigned int uiHighestSequenceNumber = NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (pOrderableItem->getFollowingSequenceNumber(), pOBW->getFollowingSequenceNumber()) ?
                        pOrderableItem->getFollowingSequenceNumber() : pOBW->getFollowingSequenceNumber();
                    pOBW->setSequenceNumber (_uiNextExpectedSequenceNumber);
                    pOBW->setItemLength (NOMADSUtil::SequentialArithmetic::delta (uiHighestSequenceNumber, _uiNextExpectedSequenceNumber));
                    pOBW->setData (getPositionInBuffer (_uiNextExpectedSequenceNumber));
                    pOBW->addTCPFlags (ui8Flags);
                    insertedBytes = pOrderableItem->getItemLength();
                    delete pOrderableItem;
                }
                else {
                    // Overlapping data won't be overwritten - inserting new item before the old one pOBW
                    unsigned int uiBytesToWrite = NOMADSUtil::SequentialArithmetic::delta (pOBW->getSequenceNumber(), _uiNextExpectedSequenceNumber);
                    if (uiBytesToWrite != copyBytesToCircularBuffer (pOrderableItem, _uiNextExpectedSequenceNumber, pOBW->getSequenceNumber())) {
                        delete pOrderableItem;
                        return -10;
                    }

                    /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                    "buffer action: packet with the expected SEQ number %u, byte size %u (address in buffer %p), and following "
                                    "packet SEQ number %u can be merged with the following packet with SEQ number %u and byte size %u (address "
                                    "in buffer %p, following packet with SEQ number %u)\n", pOrderableItem->getSequenceNumber(),
                                    pOrderableItem->getItemLength(), getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                    pOrderableItem->getFollowingSequenceNumber(), pOBW->getSequenceNumber(), pOBW->getItemLength(),
                                    pOBW->getData(), pOBW->getFollowingSequenceNumber());*/
                    ui8Flags |= pOBW->getTCPFlags();
                    const unsigned int uiNewItemLength = NOMADSUtil::SequentialArithmetic::delta (pOBW->getFollowingSequenceNumber(), _uiNextExpectedSequenceNumber);       // Necessary because we are changing the SEQ number of the segment pointed by pOBW
                    pOBW->setSequenceNumber (_uiNextExpectedSequenceNumber);
                    pOBW->setItemLength (uiNewItemLength);
                    pOBW->setData (getPositionInBuffer (_uiNextExpectedSequenceNumber));
                    pOBW->addTCPFlags (ui8Flags);
                    insertedBytes = uiBytesToWrite;
                    delete pOrderableItem;
                }
            }
            else {
                // New item is the expected one and only items with higher SEQ numbers are already in the list - inserting data.
                if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                    delete pOrderableItem;
                    return -11;
                }

                /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                "buffer action: packet with the expected SEQ number %u, byte size %u (address in buffer %p), "
                                "and following packet SEQ number %u cannot be merged with any other packet in the buffer\n",
                                pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                getPositionInBuffer (pOrderableItem->getSequenceNumber()), pOrderableItem->getFollowingSequenceNumber(),
                                pOBW->getSequenceNumber(), pOBW->getItemLength(), pOBW->getData(), pOBW->getFollowingSequenceNumber());*/
                pOrderableItem->setData (getPositionInBuffer (_uiNextExpectedSequenceNumber));
                _plSeparateNodesList.insert (pOrderableItem);
                insertedBytes = pOrderableItem->getItemLength();
            }
        }
        else {
            // There are already items in the list AND an item has been received out of order
            TCPSegment *pPreviousItem = nullptr;
            while (pOBW && (*pOBW <= *pOrderableItem)) {
                pPreviousItem = pOBW;
                pOBW = _plSeparateNodesList.getNext();
            }
            // pPreviousItem might still be null, in case pOBW had a sequence number which was higher than pOrderableItem

            if (bOverwriteData) {
                // Copy data, regardless of any item already in the list and any data already in the buffer
                if (pOrderableItem->getItemLength() != copyBytesToCircularBuffer (pOrderableItem)) {
                    delete pOrderableItem;
                    return -12;
                }

                insertedBytes = pOrderableItem->getItemLength();
                if (pOBW && (pOrderableItem->overlaps (*pOBW) || pOrderableItem->isFollowedBy (*pOBW))) {
                    // Remove all the items included within the new one and join it with partial overlapping items
                    TCPSegment *pOBWNext = nullptr;
                    while ((pOBWNext = _plSeparateNodesList.getNext()) && (pOrderableItem->overlaps (*pOBWNext) || pOrderableItem->isFollowedBy (*pOBWNext))) {
                        ui8Flags |= pOBW->getTCPFlags();
                        delete _plSeparateNodesList.remove (pOBW);
                        pOBW = pOBWNext;
                    }
                    ui8Flags |= pOBW->getTCPFlags();
                    if (pPreviousItem && (pOrderableItem->overlaps (*pPreviousItem) || pOrderableItem->follows (*pPreviousItem))) {
                        // pPreviousItem <= pOrderable and they overlap --> join
                        pPreviousItem->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOBW->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber()));
                        pPreviousItem->addTCPFlags (ui8Flags);
                        delete _plSeparateNodesList.remove (pOBW);
                        delete pOrderableItem;
                    }
                    else {
                        // pPreviousItem < pOrderableItem or pOrderableItem is the new first item of the queue --> only join with following item
                        const unsigned int uiNewItemLength = NOMADSUtil::SequentialArithmetic::delta (pOBW->getFollowingSequenceNumber(), pOrderableItem->getSequenceNumber());       // Necessary because we are changing the SEQ number of the segment pointed by pOBW
                        pOBW->setSequenceNumber (pOrderableItem->getSequenceNumber());
                        pOBW->setItemLength (uiNewItemLength);
                        pOBW->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
                        pOBW->addTCPFlags (ui8Flags);
                        delete pOrderableItem;
                    }
                }
                else {
                    // Newly received item is the one with the highest Sequence number, or it cannot be joined with subsequent items
                    if (pPreviousItem && (pOrderableItem->overlaps (*pPreviousItem) || pOrderableItem->follows (*pPreviousItem))) {
                        // pPreviousItem <= pOrderable and they overlap --> join
                        pPreviousItem->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber()));
                        pPreviousItem->addTCPFlags (ui8Flags);
                        delete pOrderableItem;
                    }
                    else {
                        // pPreviousItem < pOrderableItem or pOrderableItem is the new first item of the queue --> impossible to join with any received item
                        pOrderableItem->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
                        _plSeparateNodesList.insert (pOrderableItem);
                    }
                }
            }
            else if (pOBW) {
                // Filling the gap in between pPreviousItem and pOBW - do not overwrite data
                unsigned int uiHighEnd = NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pOrderableItem->getFollowingSequenceNumber(), pOBW->getSequenceNumber()) ?
                                            pOrderableItem->getFollowingSequenceNumber() : pOBW->getSequenceNumber();
                unsigned int uiLowEnd;
                if (pPreviousItem) {
                    uiLowEnd = NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (pPreviousItem->getFollowingSequenceNumber(), pOrderableItem->getSequenceNumber()) ?
                                pPreviousItem->getFollowingSequenceNumber() : pOrderableItem->getSequenceNumber();
                }
                else {
                    uiLowEnd = pOrderableItem->getSequenceNumber();
                }
                unsigned int uiBytesToWrite = NOMADSUtil::SequentialArithmetic::delta (uiHighEnd, uiLowEnd);
                if (uiBytesToWrite != copyBytesToCircularBuffer (pOrderableItem, uiLowEnd, uiHighEnd)) {
                    delete pOrderableItem;
                    return -13;
                }
                // Inserting new item in the list
                if (pPreviousItem && (pPreviousItem->overlaps (*pOrderableItem) || pPreviousItem->isFollowedBy (*pOrderableItem))) {
                    // Received item can be joined with pPreviousItem
                    pPreviousItem->addTCPFlags (ui8Flags);
                    if (pOrderableItem->overlaps (*pOBW) || pOrderableItem->isFollowedBy (*pOBW)) {
                        // Received item perfectly fills the gap in between pPreviousItem and pOBW
                        /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                        "buffer action: packet with SEQ number %u, byte size %u (address in buffer %p), and following "
                                        "packet SEQ number %u fills the gap between packet with SEQ number %u, byte size %u (address "
                                        "in buffer %p, following packet with SEQ number %u) and packet with SEQ number %u, byte size %u "
                                        "(address in buffer %p, following packet has SEQ number %u)\n", pOrderableItem->getSequenceNumber(),
                                        pOrderableItem->getItemLength(), getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                        pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber(),
                                        pPreviousItem->getItemLength(), pPreviousItem->getData(), pPreviousItem->getFollowingSequenceNumber(),
                                        pOBW->getSequenceNumber(), pOBW->getItemLength(), pOBW->getData(), pOBW->getFollowingSequenceNumber());*/
                        pPreviousItem->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOBW->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber()));
                        pPreviousItem->addTCPFlags (pOBW->getTCPFlags());       // Flags from the current item (*pOrderableItem) added above
                        delete _plSeparateNodesList.remove (pOBW);
                        delete pOrderableItem;
                    }
                    else {
                        // There is still a gap between the received item and pOBW, which follows it
                        /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                        "buffer action: packet with SEQ number %u, byte size %u (address in buffer %p), and "
                                        "following packet SEQ number %u merges with the preceeding packet with SEQ number "
                                        "%u, byte size %u (address in buffer %p, following packet with SEQ number %u)\n",
                                        pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                        getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                        pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber(),
                                        pPreviousItem->getItemLength(), pPreviousItem->getData(),
                                        pPreviousItem->getFollowingSequenceNumber());*/
                        pPreviousItem->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber()));
                        delete pOrderableItem;
                    }
                }
                else {
                    // There is still a gap between received item and pPreviousItem, or the received item is the new first item in the queue
                    if (pOrderableItem->overlaps (*pOBW) || pOrderableItem->isFollowedBy (*pOBW)) {
                        // Newly received item con be joined with pOBW
                        /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                        "buffer action: packet with SEQ number %u, byte size %u (address in buffer %p), and "
                                        "following packet SEQ number %u merges with the following packet with SEQ number "
                                        "%u, byte size %u (address in buffer %p, following packet with SEQ number %u)\n",
                                        pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                        getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                        pOrderableItem->getFollowingSequenceNumber(), pOBW->getSequenceNumber(),
                                        pOBW->getItemLength(), pOBW->getData(), pOBW->getFollowingSequenceNumber());*/
                        const unsigned int uiNewItemLength = NOMADSUtil::SequentialArithmetic::delta (pOBW->getFollowingSequenceNumber(), pOrderableItem->getSequenceNumber());       // Necessary because we are changing the SEQ number of the segment pointed by pOBW
                        pOBW->setSequenceNumber (pOrderableItem->getSequenceNumber());
                        pOBW->setItemLength (uiNewItemLength);
                        pOBW->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
                        pOBW->addTCPFlags (ui8Flags);
                        delete pOrderableItem;
                    }
                    else {
                        // New item has to be inserted as a new item in the gap between pPreviousItem (if not null) and pOBW - no joinings
                        /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                        "buffer action: packet with SEQ number %u, byte size %u (address in buffer %p), and "
                                        "following packet SEQ number %u cannot be merged with any other packet in the buffer\n",
                                        pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                        getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                        pOrderableItem->getFollowingSequenceNumber());*/
                        pOrderableItem->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
                        _plSeparateNodesList.insert (pOrderableItem);
                    }
                }
                insertedBytes = uiBytesToWrite;
            }
            else {
                // Newly received item fits at the end of the list - do not overwrite data
                unsigned int uiLowEnd = NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (pPreviousItem->getFollowingSequenceNumber(), pOrderableItem->getSequenceNumber()) ?
                                            pPreviousItem->getFollowingSequenceNumber() : pOrderableItem->getSequenceNumber();
                unsigned int uiBytesToWrite = NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), uiLowEnd);
                if (uiBytesToWrite != copyBytesToCircularBuffer (pOrderableItem, uiLowEnd)) {
                    delete pOrderableItem;
                    return -14;
                }
                if (pPreviousItem->overlaps (*pOrderableItem) || pPreviousItem->isFollowedBy (*pOrderableItem)) {
                    // Received item can be joined with pPreviousItem
                    /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                    "buffer action: packet with highest SEQ number %u, byte size %u (address in buffer %p), "
                                    "and following packet SEQ number %u merges with the preceeding packet with SEQ number "
                                    "%u, byte size %u (address in buffer %p, following packet with SEQ number %u)\n",
                                    pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                    getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                    pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber(),
                                    pPreviousItem->getItemLength(), pPreviousItem->getData(),
                                    pPreviousItem->getFollowingSequenceNumber());*/
                    pPreviousItem->setItemLength (NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), pPreviousItem->getSequenceNumber()));
                    pPreviousItem->addTCPFlags (ui8Flags);
                    delete pOrderableItem;
                }
                else {
                    // New item has to be appended at the end of the list, and there is a gap between it and the previous item
                    /*checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                                    "buffer action: packet with highest SEQ number %u, byte size %u (address in buffer %p), "
                                    "and following packet SEQ number %u cannot be merged with any other packet in the buffer\n",
                                    pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength(),
                                    getPositionInBuffer (pOrderableItem->getSequenceNumber()),
                                    pOrderableItem->getFollowingSequenceNumber());*/
                    pOrderableItem->setData (getPositionInBuffer (pOrderableItem->getSequenceNumber()));
                    _plSeparateNodesList.append (pOrderableItem);
                }
                insertedBytes = uiBytesToWrite;
            }
        }

        unsigned int iPacketCount = _plSeparateNodesList.getCount();
        auto * pOrderableItemInList = _plSeparateNodesList.getFirst();
        _uiNextExpectedSequenceNumber = (pOrderableItemInList->getSequenceNumber() == _uiReadingSequenceNumber) ?
            pOrderableItemInList->getFollowingSequenceNumber() : _uiReadingSequenceNumber;
        _iReadyBytesInBuffer = NOMADSUtil::SequentialArithmetic::delta (_uiNextExpectedSequenceNumber, _uiReadingSequenceNumber);
        _iTotalBytesInBuffer = NOMADSUtil::SequentialArithmetic::delta (_plSeparateNodesList.getTail()->getFollowingSequenceNumber(), _uiReadingSequenceNumber);
        checkAndLogMsg ("CircularOrderedBuffer::insertData", NOMADSUtil::Logger::L_HighDetailDebug,
                        "Inserted %d bytes into the circular buffer; current buffer size is %u and reader pointer offset is %u; "
                        "there are %u total bytes (of which %u are ready to be processed, beginning with SEQ number %u); "
                        "there are %d distinct packets in the buffer and the next expected one has SEQ number %u\n",
                        insertedBytes, _uiBufCurrentSize, _uiDataReaderPointer, _iTotalBytesInBuffer, _iReadyBytesInBuffer,
                        _uiReadingSequenceNumber, iPacketCount, _uiNextExpectedSequenceNumber);
        /*
        unsigned int index = 0;
        while (pOrderableItemInList) {
            checkAndLogMsg ("CircularOrderedBuffer::insertData", Logger::L_HighDetailDebug,
                            "buffer summary: packet #%u - SEQ number %u, byte size %u (address in buffer %p), "
                            "following packet has SEQ number %u\n", index++, pOrderableItem->getSequenceNumber(),
                            pOrderableItem->getItemLength(), pOrderableItem->getData(),
                            pOrderableItem->getFollowingSequenceNumber());
            pOrderableItemInList = _plSeparateNodesList.getNext();
        }
        */

        return insertedBytes;
    }

    int CircularOrderedBuffer::peekData (TCPSegment * const pTCPSegment) const
    {
        unsigned int uiSequenceNumber = 0;
        unsigned char *pBuf = const_cast<unsigned char *> (pTCPSegment->getData());
        uint8 ui8Flag = 0;
        unsigned int uiPeekedBytes = peekData (&uiSequenceNumber, pBuf, pTCPSegment->getItemLength(), &ui8Flag);
        pTCPSegment->setSequenceNumber (uiSequenceNumber);
        pTCPSegment->setItemLength (uiPeekedBytes);
        pTCPSegment->setData (pBuf);
        pTCPSegment->setTCPFlags (ui8Flag);

        return uiPeekedBytes;
    }

    int CircularOrderedBuffer::peekData (unsigned int * uiSequenceNumber, unsigned char * pBuf,
                                         unsigned int uiBytesToRead, uint8 * ui8Flag) const
    {
        unsigned int peekedBytes;
        TCPSegment *pOBW = _plSeparateNodesList.getFirst();
        if ((pOBW != nullptr) && (_iReadyBytesInBuffer > 0)) {
            if (_uiReadingSequenceNumber == pOBW->getSequenceNumber()) {
                peekedBytes = std::min (uiBytesToRead, pOBW->getItemLength());

                if (copyBytesFromCircularBuffer (pBuf, peekedBytes) == 0) {
                    *uiSequenceNumber = _uiReadingSequenceNumber;
                    *ui8Flag = (peekedBytes == pOBW->getItemLength()) ?
                        pOBW->getTCPFlags() : NOMADSUtil::TCPHeader::TCPF_ACK;
                    return peekedBytes;
                }
                else {
                    return -1;
                }
            }
            else {
                return -2;
            }
        }

        // Data is not ready to be read from buffer (an item with the next expected SEQ number has never been received)
        return 0;
    }

    int CircularOrderedBuffer::extractData (TCPSegment * const pTCPSegment)
    {
        unsigned int uiSequenceNumber = 0;
        unsigned char *pBuf = const_cast<unsigned char *> (pTCPSegment->getData());
        uint8 ui8Flag = 0;
        unsigned int uiExtractedBytes = extractData (&uiSequenceNumber, pBuf, pTCPSegment->getItemLength(), &ui8Flag);
        pTCPSegment->setSequenceNumber (uiSequenceNumber);
        pTCPSegment->setItemLength (uiExtractedBytes);
        pTCPSegment->setData (pBuf);
        pTCPSegment->setTCPFlags (ui8Flag);

        return uiExtractedBytes;
    }

    int CircularOrderedBuffer::extractData (unsigned int * const uiSequenceNumber, unsigned char * const pBuf,
                                            unsigned int uiBytesToRead, uint8 * const ui8Flag)
    {
        TCPSegment *pOBW;
        if (_iReadyBytesInBuffer <= 0) {
            if ((pOBW = _plSeparateNodesList.getFirst()) != nullptr) {
                // An empty packet was received when buffer was empty --> retrieving flags
                *ui8Flag = pOBW->getTCPFlags();
                delete _plSeparateNodesList.remove (pOBW);
            }
            return 0;
        }

       pOBW = _plSeparateNodesList.getFirst();
        if (!pOBW) {
            return -1;
        }

        int iExtractedBytes = peekData (uiSequenceNumber, pBuf, uiBytesToRead, ui8Flag);
        if (iExtractedBytes <= 0) {
            checkAndLogMsg ("CircularOrderedBuffer::extractData", NOMADSUtil::Logger::L_MildError,
                            "peekData() called with uiSequenceNumber = %u and uiBytesToRead = %u returned with error code %d\n",
                            uiSequenceNumber, uiBytesToRead, iExtractedBytes);
            return -2;
        }

        if (pOBW->getItemLength() < iExtractedBytes) {
            // We cannot extract more bytes than there are available
            return -3;
        }
        else if (pOBW->getItemLength() == iExtractedBytes) {
            // All the available bytes have been extracted
            delete _plSeparateNodesList.remove (pOBW);
        }
        else if (iExtractedBytes != pOBW->incrementValues (iExtractedBytes)) {
            // Error incrementing values, since it must be true that extractedBytes < pOBW->getItemLength()
            return -4;
        }

        int rc;
        if (0 != (rc = removeData (iExtractedBytes))) {
            checkAndLogMsg ("CircularOrderedBuffer::extractData", NOMADSUtil::Logger::L_MildError,
                            "removeData() failed with error code %d\n", rc);
        }
        checkAndLogMsg ("CircularOrderedBuffer::extractData", NOMADSUtil::Logger::L_HighDetailDebug,
                        "extracted %d bytes from the circular buffer; current buffer size is %u and reader pointer offset is %u; "
                        "there are %u total bytes (of which %u are ready to be processed, beginning with SEQ num %u); "
                        "next expected packet has SEQ number %u\n", iExtractedBytes, _uiBufCurrentSize, _uiDataReaderPointer,
                        _iTotalBytesInBuffer, _iReadyBytesInBuffer, _uiReadingSequenceNumber, _uiNextExpectedSequenceNumber);

        return iExtractedBytes;
    }

    int CircularOrderedBuffer::removeData (unsigned int uiBytesToRemove)
    {
        if (_iReadyBytesInBuffer < uiBytesToRemove) {
            return -1;
        }

        _uiReadingSequenceNumber += uiBytesToRemove;
        _uiDataReaderPointer = (_uiDataReaderPointer + uiBytesToRemove) % _uiBufCurrentSize;
        _iReadyBytesInBuffer -= uiBytesToRemove;
        if (_iReadyBytesInBuffer < 0) {
            _iReadyBytesInBuffer = 0;
        }
        _iTotalBytesInBuffer -= uiBytesToRemove;
        if (_iTotalBytesInBuffer < 0) {
            _iTotalBytesInBuffer = 0;
        }

        return 0;
    }

    void CircularOrderedBuffer::resetBuffer (void)
    {
        if (_pui8Buf && (_uiBufCurrentSize != _uiBufMinSize)) {
            delete[] _pui8Buf;
            _pui8Buf = new unsigned char[_uiBufMinSize];
        }
        else if (!_pui8Buf) {
            _pui8Buf = new unsigned char[_uiBufMinSize];
        }
        _uiBufCurrentSize = _uiBufMinSize;

        _uiDataReaderPointer = 0;
        _uiReadingSequenceNumber = 0;
        _uiNextExpectedSequenceNumber = 0;
        _iReadyBytesInBuffer = 0;
        _iTotalBytesInBuffer = 0;

        TCPSegment *pOBW = _plSeparateNodesList.getFirst();
        while (pOBW) {
            delete _plSeparateNodesList.remove (pOBW);
            pOBW = _plSeparateNodesList.getFirst();
        }
    }

    bool CircularOrderedBuffer::isDataUseful (const OrderableBufferWrapper<unsigned char> * const pOrderableItem, bool bOverwriteData)
    {
        if (!pOrderableItem) {
            return false;
        }

        // Even if new item overlaps existing data, it will refresh old data
        if (bOverwriteData && (pOrderableItem->getItemLength() > 0)) {
            return true;
        }

        // Look for any group of data which may overlaps new packet of data completely
        OrderableBufferWrapper<unsigned char> *pOBW = _plSeparateNodesList.getFirst();
        while (pOBW && (*pOBW <= *pOrderableItem)) {
            if (pOBW->overlaps (*pOrderableItem)) {
                // Data is useless only in the moment that existing items include the new one
                if (pOBW->includes (*pOrderableItem)) {
                    return false;
                }
                // New item is not included into any existing one --> new data is useful
                return true;
            }
            pOBW = _plSeparateNodesList.getNext();
        }

        return true;
    }

    bool CircularOrderedBuffer::fitsInBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem) const
    {
        if (!pOrderableItem) {
            return false;
        }
        if (NOMADSUtil::SequentialArithmetic::lessThan (pOrderableItem->getFollowingSequenceNumber(), _uiNextExpectedSequenceNumber)) {
            return false;
        }
        if (NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), _uiReadingSequenceNumber) > _uiBufMaxSize) {
            return false;
        }

        return true;
    }

    bool CircularOrderedBuffer::isDataWrappingAround (unsigned int uiStartingSequenceNumber, unsigned int uiBytesToRead) const
    {
        if (uiBytesToRead == 0) {
            return false;
        }

        unsigned int uiFirstEnd = _uiDataReaderPointer + NOMADSUtil::SequentialArithmetic::delta (uiStartingSequenceNumber, _uiReadingSequenceNumber);
        unsigned int uiFinalEnd = _uiDataReaderPointer + NOMADSUtil::SequentialArithmetic::delta ((uiStartingSequenceNumber + uiBytesToRead), _uiReadingSequenceNumber);

        if ((uiFirstEnd < _uiBufCurrentSize) && (uiFinalEnd > _uiBufCurrentSize)) {
            return true;
        }
        return false;
    }

    int CircularOrderedBuffer::growBufferIfNecessary (const OrderableBufferWrapper<unsigned char> * const pOrderableItem)
    {
        if (!pOrderableItem) {
            return -1;
        }

        unsigned int uiMinNewSize = NOMADSUtil::SequentialArithmetic::delta (pOrderableItem->getFollowingSequenceNumber(), _uiReadingSequenceNumber);
        if ((uiMinNewSize <= _uiBufCurrentSize) || (_uiBufCurrentSize == _uiBufMaxSize)) {
            return _uiBufCurrentSize;
        }
        if (uiMinNewSize > _uiBufMaxSize) {
            return -2;
        }

        // Doubling current buffer size
        unsigned int uiNewBufSize = 2 * _uiBufCurrentSize;
        while ((uiNewBufSize < _uiBufMaxSize) && (uiNewBufSize < uiMinNewSize)) {
            uiNewBufSize *= 2;
        }
        if (uiNewBufSize > _uiBufMaxSize) {
            uiNewBufSize = _uiBufMaxSize;
        }

        unsigned char *pBufNew = new unsigned char[uiNewBufSize];
        if (!pBufNew) {
            return -3;
        }
        if (0 != rawCopyFromCircularBuffer (pBufNew, _uiBufCurrentSize)) {
            delete[] pBufNew;
            return -4;
        }

        checkAndLogMsg ("CircularOrderedBuffer::growBufferIfNecessary", NOMADSUtil::Logger::L_MediumDetailDebug,
                        "CircularBuffer grown from %u to %u bytes; old buffer at position %p and new one at position %p; "
                        "old reader pointer offset was %u, pointing to data with SEQ num %u; there are %u bytes in buffer "
                        "(of which %u are ready to be read) and a packet with SEQ num %u and %hu bytes of data needs to be enqueued\n",
                        _uiBufCurrentSize, uiNewBufSize, _pui8Buf, pBufNew, _uiDataReaderPointer, _uiReadingSequenceNumber,
                        _iTotalBytesInBuffer, _iReadyBytesInBuffer, pOrderableItem->getSequenceNumber(), pOrderableItem->getItemLength());
        delete[] _pui8Buf;
        _pui8Buf = pBufNew;
        _uiBufCurrentSize = uiNewBufSize;
        _uiDataReaderPointer = 0;

        // Updating pointers to data of the items in the list
        OrderableBufferWrapper<unsigned char> *pOBW = _plSeparateNodesList.getFirst();
        while (pOBW) {
            pOBW->setData (getPositionInBuffer (pOBW->getSequenceNumber()));
            pOBW = _plSeparateNodesList.getNext();
        }

        return _uiBufCurrentSize;
    }

    unsigned char * CircularOrderedBuffer::getPositionInBuffer (unsigned int uiSequenceNumber) const
    {
        if (NOMADSUtil::SequentialArithmetic::lessThan (uiSequenceNumber, _uiReadingSequenceNumber)) {
            return nullptr;
        }

        unsigned int uiDisplacement = NOMADSUtil::SequentialArithmetic::delta (uiSequenceNumber, _uiReadingSequenceNumber);
        if (uiDisplacement >= _uiBufCurrentSize) {
            return nullptr;
        }

        unsigned int uiPositionInBuffer = (_uiDataReaderPointer + uiDisplacement) % _uiBufCurrentSize;
        return &(_pui8Buf[uiPositionInBuffer]);
    }

    int CircularOrderedBuffer::availableBytesBeforeWrappingAround (unsigned int uiSequenceNumber) const
    {
        if (NOMADSUtil::SequentialArithmetic::lessThan (uiSequenceNumber, _uiReadingSequenceNumber)) {
            return -1;
        }

        unsigned int uiDisplacement = NOMADSUtil::SequentialArithmetic::delta (uiSequenceNumber, _uiReadingSequenceNumber);
        if (uiDisplacement >= _uiBufCurrentSize) {
            return -2;
        }

        unsigned int uiPositionInBuffer = _uiDataReaderPointer + uiDisplacement;
        if (uiPositionInBuffer >= _uiBufCurrentSize) {
            return 0;
        }

        return (_uiBufCurrentSize - uiPositionInBuffer);
    }

    int CircularOrderedBuffer::copyBytesToCircularBuffer (const OrderableBufferWrapper<unsigned char> * const pOrderableItem,
                                                          unsigned int uiStartingSequenceNumber, unsigned int uiFinalSequenceNumber)
    {
        if (!pOrderableItem) {
            return -1;
        }
        if (NOMADSUtil::SequentialArithmetic::greaterThan (uiStartingSequenceNumber, pOrderableItem->getFollowingSequenceNumber())) {
            return -2;
        }
        if (NOMADSUtil::SequentialArithmetic::lessThan (uiFinalSequenceNumber, pOrderableItem->getSequenceNumber())) {
            return -3;
        }
        if (NOMADSUtil::SequentialArithmetic::greaterThan (uiStartingSequenceNumber, uiFinalSequenceNumber)) {
            return -4;
        }

        if ((pOrderableItem->getItemLength() == 0) || !pOrderableItem->getData()) {
            return 0;
        }

        if (NOMADSUtil::SequentialArithmetic::lessThan (uiStartingSequenceNumber, pOrderableItem->getSequenceNumber())) {
            uiStartingSequenceNumber = pOrderableItem->getSequenceNumber();
        }
        if (NOMADSUtil::SequentialArithmetic::greaterThan (uiFinalSequenceNumber, pOrderableItem->getFollowingSequenceNumber())) {
            uiFinalSequenceNumber = pOrderableItem->getFollowingSequenceNumber();
        }

        unsigned int uiBytesToCopy = NOMADSUtil::SequentialArithmetic::delta (uiFinalSequenceNumber, uiStartingSequenceNumber);
        unsigned char *pCopyPosition = getPositionInBuffer (uiStartingSequenceNumber);
        if (!pCopyPosition) {
            return -5;
        }

        if (isDataWrappingAround (uiStartingSequenceNumber, uiBytesToCopy)) {
            // Buffer is wrapping around with this item
            int iAvailableBytesBeforeWrapping = availableBytesBeforeWrappingAround (uiStartingSequenceNumber);
            if ((iAvailableBytesBeforeWrapping < 0) || ((unsigned int) iAvailableBytesBeforeWrapping >= uiBytesToCopy)) {
                return -6;
            }
            checkAndLogMsg ("CircularOrderedBuffer::copyBytesToCircularBuffer", NOMADSUtil::Logger::L_HighDetailDebug,
                            "Copying %u bytes starting with SEQ num %u to Circularbuffer; %d bytes copied from address %p "
                            "to address %p in the circular buffer, and remaining %u bytes are copied from address %p to "
                            "address %p in the circular buffer\n", uiBytesToCopy, uiStartingSequenceNumber, iAvailableBytesBeforeWrapping,
                            pOrderableItem->getData(), pCopyPosition, (uiBytesToCopy - iAvailableBytesBeforeWrapping),
                            &(pOrderableItem->getData()[iAvailableBytesBeforeWrapping]), _pui8Buf);
            memcpy (pCopyPosition, pOrderableItem->getData(), iAvailableBytesBeforeWrapping);
            memcpy (_pui8Buf, &(pOrderableItem->getData()[iAvailableBytesBeforeWrapping]), (uiBytesToCopy - iAvailableBytesBeforeWrapping));
        }
        else {
            // No need to wrap around the buffer or it wrapped around with another item
            memcpy (pCopyPosition, pOrderableItem->getData(), uiBytesToCopy);
        }

        return uiBytesToCopy;
    }

    int CircularOrderedBuffer::copyBytesFromCircularBuffer (unsigned char * const pDest, unsigned int uiBytesToRead) const
    {
        if (!pDest) {
            return -1;
        }

        OrderableBufferWrapper<unsigned char> *pOBW = _plSeparateNodesList.getFirst();
        if (pOBW == nullptr) {
            return -2;
        }
        if (uiBytesToRead > pOBW->getItemLength()) {
            return -3;
        }

        if (0 != rawCopyFromCircularBuffer (pDest, uiBytesToRead)) {
            return -4;
        }

        return 0;
    }

    int CircularOrderedBuffer::rawCopyFromCircularBuffer (unsigned char * const pDest, unsigned int uiBytesToRead) const
    {
        if (!pDest) {
            return -1;
        }

        if (!isDataWrappingAround (_uiReadingSequenceNumber, uiBytesToRead)) {
            memcpy (pDest, &_pui8Buf[_uiDataReaderPointer], uiBytesToRead);
            return 0;
        }
        else {
            int iFirstBlockOfBytes = availableBytesBeforeWrappingAround (_uiReadingSequenceNumber);
            if ((iFirstBlockOfBytes < 0) || (uiBytesToRead <= (unsigned int) iFirstBlockOfBytes)) {
                return -4;
            }
            checkAndLogMsg ("CircularOrderedBuffer::rawCopyFromCircularBuffer", NOMADSUtil::Logger::L_HighDetailDebug,
                            "Copying %u bytes to buffer with address %p; copying %d bytes from address %p in the source buffer to address "
                            "%p in the destination buffer, and the remaining %hu bytes from address %p in the source buffer to address "
                            "%p in the destination buffer\n", uiBytesToRead, pDest, iFirstBlockOfBytes, (&_pui8Buf[_uiDataReaderPointer]),
                            pDest, (uiBytesToRead - iFirstBlockOfBytes), _pui8Buf, &pDest[iFirstBlockOfBytes]);

            memcpy (pDest, &_pui8Buf[_uiDataReaderPointer], iFirstBlockOfBytes);
            memcpy (&pDest[iFirstBlockOfBytes], _pui8Buf, (uiBytesToRead - iFirstBlockOfBytes));
            return 0;
        }
    }

}
