/*
 * TSNRangeHandler.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
#include "PacketMutators.h"

#include "SequentialArithmetic.h"

//#include "stdio.h"

using namespace NOMADSUtil;

/*!!*/ // Need to fix bug where the cumulative TSN should not be here because it will cause a problem when
       // this class is used for the cancelled chunk

TSNRangeHandler::TSNRangeHandler (void)
{
    _pFirstNode = _pLastNode = NULL;
}

TSNRangeHandler::~TSNRangeHandler (void)
{
    while (_pFirstNode != NULL) {
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
    }
    _pFirstNode = _pLastNode = NULL;
}

int TSNRangeHandler::addTSN (uint32 ui32TSN)
{
    bool bAddedTSN = false;
    if (_pFirstNode == NULL) {
        // There are no other nodes
        _pFirstNode = _pLastNode = new Node();
        _pFirstNode->ui32Begin = _pFirstNode->ui32End = ui32TSN;
        bAddedTSN = true;
    }
    else if ((ui32TSN + 1) == _pFirstNode->ui32Begin) {
        // This sequence number immediately preceeds that of the first range node
        _pFirstNode->ui32Begin--;
        bAddedTSN = true;
    }
    else if (ui32TSN == (_pLastNode->ui32End + 1)) {
        // This sequence number immediately follows that of the last range node
        _pLastNode->ui32End++;
        bAddedTSN = true;
    }
    else if (NOMADSUtil::SequentialArithmetic::lessThan (ui32TSN, _pFirstNode->ui32Begin)) {
        // This sequence number belongs to a range that preceeds the first range node
        Node *pNewNode = new Node();
        pNewNode->ui32Begin = pNewNode->ui32End = ui32TSN;
        pNewNode->pNext = _pFirstNode;
        _pFirstNode->pPrev = pNewNode;
        _pFirstNode = pNewNode;
        bAddedTSN = true;
    }
    else if (NOMADSUtil::SequentialArithmetic::greaterThan (ui32TSN, _pLastNode->ui32End)) {
        // This sequence number belongs to a range that follows the last range node
        Node *pNewNode = new Node();
        pNewNode->ui32Begin = pNewNode->ui32End = ui32TSN;
        pNewNode->pPrev = _pLastNode;
        _pLastNode->pNext = pNewNode;
        _pLastNode = pNewNode;
        bAddedTSN = true;
    }
    else {
        // Find the right spot for this sequence number
        Node *pCurrNode = _pFirstNode;
        Node *pNextNode = _pFirstNode->pNext;
        while (pCurrNode != NULL) {
            bool bCheckForMerge = false;
            if (ui32TSN == (pCurrNode->ui32End + 1)) {
                // The sequence number immediately follows that of the current range node
                pCurrNode->ui32End++;
                bCheckForMerge = true;
                bAddedTSN = true;
            }
            else if (pNextNode != NULL) {
                if ((ui32TSN + 1) == pNextNode->ui32Begin) {
                    // The sequence number immediately preceeds the next range node
                    pNextNode->ui32Begin--;
                    bCheckForMerge = true;
                    bAddedTSN = true;
                }
                else if (NOMADSUtil::SequentialArithmetic::greaterThan (ui32TSN, pCurrNode->ui32End) &&
                         NOMADSUtil::SequentialArithmetic::lessThan (ui32TSN, pNextNode->ui32Begin)) {
                    // The sequence number belongs to a new range that is in between the current and next range nodes
                    Node *pNewNode = new Node();
                    pNewNode->ui32Begin = pNewNode->ui32End = ui32TSN;
                    pNewNode->pPrev = pCurrNode;
                    pNewNode->pNext = pNextNode;
                    pCurrNode->pNext = pNewNode;
                    pNextNode->pPrev = pNewNode;
                    bAddedTSN = true;
                    break;
                }
            }
            // Check to see if we need to do a merge
            if (bCheckForMerge && (pNextNode != NULL)) {
                if ((pCurrNode->ui32End + 1) == pNextNode->ui32Begin) {
                    // Need to merge current and next range nodes
                    pCurrNode->ui32End = pNextNode->ui32End;
                    pCurrNode->pNext = pNextNode->pNext;
                    if (_pLastNode == pNextNode) {
                        // pNextNode was the last node - update the pLastNode pointer
                        _pLastNode = pCurrNode;
                    }
                    else {
                        // pNextNode is going away, so make the back pointer of the node
                        // following pNextNode point to pCurrNode
                        // NOTE: Since pNextNode is not the last note (as determined by the
                        // previous if test), there must be a node following pNextNode
                        pNextNode->pNext->pPrev = pCurrNode;
                    }
                    delete pNextNode;
                    break;
                }
            }
            pCurrNode = pNextNode;
            if (pNextNode) {
                pNextNode = pNextNode->pNext;
            }
        }
    }

    if (bAddedTSN) {
        return 1;
    }
    else {
        return 0;
    }
}

int TSNRangeHandler::appendTSNInformation (TSNChunkMutator *pTCM)
{
    Node *pCurrNode = _pFirstNode;
    // Append all ranges first
    if (pTCM->startAddingRanges()) {
        return -1;
    }
    while (pCurrNode != NULL) {
        if (pCurrNode->ui32Begin == pCurrNode->ui32End) {
            // Skip for now
        }
        else {
            if (pTCM->addRange (pCurrNode->ui32Begin, pCurrNode->ui32End)) {
                return -2;
            }
        }
        pCurrNode = pCurrNode->pNext;
    }
    if (pTCM->doneAddingRanges()) {
        return -3;
    }

    // Now append the individual TSNs
    if (pTCM->startAddingTSNs()) {
        return -4;
    }
    pCurrNode = _pFirstNode;
    while (pCurrNode != NULL) {
        if (pCurrNode->ui32Begin == pCurrNode->ui32End) {
            if (pTCM->addTSN (pCurrNode->ui32Begin)) {
                return -5;
            }
        }
        pCurrNode = pCurrNode->pNext;
    }
    if (pTCM->doneAddingTSNs()) {
        return -6;
    }

    return 0;
}

int TSNRangeHandler::freeze (ObjectFreezer &objectFreezer)
{
    // Go through the whole list of nodes
    Node *pCurrNode = _pFirstNode;
    while (pCurrNode != NULL) {
        // Insert a control char to signal that another node follows
        objectFreezer << (unsigned char) 1;
        objectFreezer.putUInt32 (pCurrNode->ui32Begin);
        objectFreezer.putUInt32 (pCurrNode->ui32End);
        
/*        printf ("ui32Begin %lu\n", pCurrNode->ui32Begin);
        printf ("ui32End %lu\n", pCurrNode->ui32End);*/
        
        pCurrNode = pCurrNode->pNext;
    }
    // Insert a control char to signal that there are no more data
    objectFreezer << (unsigned char) 0;
    
    return 0;
}

int TSNRangeHandler::defrost (ObjectDefroster &objectDefroster)
{
    uint32 ui32Begin;
    uint32 ui32End;
    unsigned char moreData;
    objectDefroster >> moreData;
    // Insert all nodes.
    // Add every node as last node because nodes were ordered when freezed
    while (moreData) {
        objectDefroster >> ui32Begin;
        objectDefroster >> ui32End;
        if (_pLastNode == NULL) {
            _pFirstNode = _pLastNode = new Node();
            _pFirstNode->ui32Begin = _pLastNode->ui32Begin = ui32Begin;
            _pFirstNode->ui32End = _pLastNode->ui32End = ui32End;
        }
        else {
            Node *pNewNode = new Node();
            pNewNode->ui32Begin = ui32Begin;
            pNewNode->ui32End = ui32End;
            pNewNode->pPrev = _pLastNode;
            _pLastNode->pNext = pNewNode;
            _pLastNode = pNewNode;
        }
        objectDefroster >> moreData;
    }
    
    Node *pCurrNode = _pFirstNode;
    while (pCurrNode != NULL) {
/*        printf ("ui32Begin %lu\n", pCurrNode->ui32Begin);
        printf ("ui32End %lu\n", pCurrNode->ui32End);*/
        
        pCurrNode = pCurrNode->pNext;
    }
    
    return 0;
}

SAckTSNRangeHandler::SAckTSNRangeHandler (void)
{
    _ui32CumulativeTSN = 0;
}

int SAckTSNRangeHandler::addTSN (uint32 ui32TSN)
{
    if (NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (_ui32CumulativeTSN, ui32TSN)) {
        return 0;
    }

    int rc = TSNRangeHandler::addTSN (ui32TSN);

    // Check to see if the last cumulative acknowledgement must be updated
    if ((_ui32CumulativeTSN + 1) == _pFirstNode->ui32Begin) {
        _ui32CumulativeTSN = _pFirstNode->ui32End;
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
        if (_pFirstNode == NULL) {
            // The list is empty
            _pLastNode = NULL;
        }
    }

    return rc;
}

int SAckTSNRangeHandler::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32CumulativeTSN);
    
	/*  
	printf ("SAckTSNRangeHandler\n");
    printf ("_ui32CumulativeTSN %lu\n", _ui32CumulativeTSN);
	*/
    
    // call the superclass method freeze
    TSNRangeHandler::freeze (objectFreezer);
    return 0;
}

int SAckTSNRangeHandler::defrost (ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32CumulativeTSN;
    
/*    printf ("SAckTSNRangeHandler\n");
    printf ("_ui32CumulativeTSN %lu\n", _ui32CumulativeTSN);*/
    
    // call the superclass method defrost
    if (0 != TSNRangeHandler::defrost (objectDefroster)) {
        return -2;
    }
    return 0;
}

int CancelledTSNRangeHandler::deleteTSNsUpTo (uint32 ui32TSN)
{
    if (_pFirstNode == NULL) {
        return 0;
    }

    // As an optimization, first check if the TSN >= the TSN in the last node
    if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (_pLastNode->ui32End, ui32TSN)) {
        // The whole list can be deleted
        while (_pFirstNode != NULL) {
            Node *pTempNode = _pFirstNode;
            _pFirstNode = _pFirstNode->pNext;
            delete pTempNode;
        }
        _pFirstNode = _pLastNode = NULL;
        return 0;
    }

    // Starting at the beginning, remove nodes whose end TSN is less than or equal to the specified TSN
    while ((_pFirstNode) && (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (_pFirstNode->ui32End, ui32TSN))) {
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
        if (_pFirstNode == NULL) {
            // The list is empty
            _pLastNode = NULL;
        }
    }

    // The last possibility is that the first node's range includes the specified TSN
    // If so, the start TSN needs to be updated
    if ((_pFirstNode) && (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (_pFirstNode->ui32Begin, ui32TSN))) {
        _pFirstNode->ui32Begin = ui32TSN + 1;
    }

    return 0;
}

ReceivedTSNRangeHandler::ReceivedTSNRangeHandler (void)
{
    _bAdded0TSN = false;
    _ui32CumulativeTSN = 0;
}

int ReceivedTSNRangeHandler::addTSN (uint32 ui32TSN)
{
    if (ui32TSN == 0) {
        _bAdded0TSN = true;
    }
    if (NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (_ui32CumulativeTSN, ui32TSN)) {
        // The cumulative TSN already encompasses the new TSN, so nothing needs to be done
        return 0;
    }

    int rc = TSNRangeHandler::addTSN (ui32TSN);

    // Check to see if the last cumulative acknowledgement must be updated
    if ((_ui32CumulativeTSN + 1) == _pFirstNode->ui32Begin) {
        _ui32CumulativeTSN = _pFirstNode->ui32End;
        Node *pTempNode = _pFirstNode;
        _pFirstNode = _pFirstNode->pNext;
        delete pTempNode;
        if (_pFirstNode == NULL) {
            // The list is empty
            _pLastNode = NULL;
        }
    }

    return rc;
}

bool ReceivedTSNRangeHandler::alreadyReceived (uint32 ui32TSN)
{
    if (ui32TSN == 0) {
        return _bAdded0TSN;
    }
    if (NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (_ui32CumulativeTSN, ui32TSN)) {
        // The cumulative TSN is greater than the specified TSN, so it has already been received
        return true;
    }
    // Search the list
    Node *pCurrNode = _pFirstNode;
    while (pCurrNode != NULL) {
        if (NOMADSUtil::SequentialArithmetic::lessThanOrEqual (pCurrNode->ui32Begin, ui32TSN) &&
            NOMADSUtil::SequentialArithmetic::greaterThanOrEqual (pCurrNode->ui32End, ui32TSN)) {
            // The specified TSN falls within the range contained in this node in the list
            return true;
        }
        pCurrNode = pCurrNode->pNext;
    }
    // No entries in the list were found that contained the specified TSN
    return false;
}

int ReceivedTSNRangeHandler::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    objectFreezer.putUInt32 (_ui32CumulativeTSN);
    objectFreezer.putBool (_bAdded0TSN);
    
    // Call the superclass method freeze
    if (0 != TSNRangeHandler::freeze (objectFreezer)) {
        return -1;
    }
    return 0;
}

int ReceivedTSNRangeHandler::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    objectDefroster >> _ui32CumulativeTSN;
    objectDefroster >> _bAdded0TSN;

    // Call the superclass method defrost
    if (0 != TSNRangeHandler::defrost (objectDefroster)) {
        return -1;
    }
    return 0;
}
