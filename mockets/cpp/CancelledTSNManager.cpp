/*
 * CancelledTSNManager.cpp
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2016 IHMC.
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

#include "CancelledTSNManager.h"

#include "Packet.h"
#include "PacketMutators.h"

#include "Logger.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

CancelledTSNManager::CancelledTSNManager (void)
{
    _sf = SF_None;
}

bool CancelledTSNManager::haveInformation (void)
{
    return ((_reliableSequencedTSNHandler.haveInformation()) || (_reliableUnsequencedTSNHandler.haveInformation()) || (_unreliableSequencedTSNHandler.haveInformation()));
}

int CancelledTSNManager::startCancellingReliableSequencedPackets (void)
{
    _m.lock();
    _sf = SF_ReliableSequenced;
    return 0;
}

int CancelledTSNManager::startCancellingReliableUnsequencedPackets (void)
{
    _m.lock();
    _sf = SF_ReliableUnsequenced;
    return 0;
}

int CancelledTSNManager::startCancellingUnreliableSequencedPacket (void)
{
    _m.lock();
    _sf = SF_UnreliableSequenced;
    return 0;
}

int CancelledTSNManager::addCancelledPacketTSN (uint32 ui32TSN)
{
    switch (_sf) {
        case SF_ReliableSequenced:
            _reliableSequencedTSNHandler.addTSN (ui32TSN);
            break;
        case SF_ReliableUnsequenced:
            _reliableUnsequencedTSNHandler.addTSN (ui32TSN);
            break;
        case SF_UnreliableSequenced:
            _unreliableSequencedTSNHandler.addTSN (ui32TSN);
            break;
        default:
            return -1;
    }
    return 0;
}

int CancelledTSNManager::endCancellingPackets (void)
{
    _sf = SF_None;
    _m.unlock();
    return 0;
}

void CancelledTSNManager::cancelReliableSequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableSequencedTSNHandler.addTSN (ui32TSN);
    _m.unlock();
}

void CancelledTSNManager::cancelReliableUnsequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableUnsequencedTSNHandler.addTSN (ui32TSN);
    _m.unlock();
}

void CancelledTSNManager::cancelUnreliableSequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _unreliableSequencedTSNHandler.addTSN (ui32TSN);
    _m.unlock();
}

int CancelledTSNManager::processSAckChunk (SAckChunkAccessor sackChunkAccessor)
{
    _m.lock();
    _reliableSequencedTSNHandler.deleteTSNsUpTo (sackChunkAccessor.getReliableSequencedCumulateAck());
    _reliableUnsequencedTSNHandler.deleteTSNsUpTo (sackChunkAccessor.getReliableUnsequencedCumulativeAck());
    //_unreliableSequencedTSNHandler.deleteTSNsUpTo (...);     /*!!*/ // Need to implement this in some fashion
                                                                      // Consider either not keeping this information once it has been sent
                                                                      // (since it is unreliable), or adding another cumulative ack for the
                                                                      // unreliable sequenced case also
    /*!!*/ // Need to also handle the ranges and singles in the SAckChunk
    checkAndLogMsg ("CancelledTSNManager::processSAckChunk", Logger::L_MediumDetailDebug,
                    "processed SAck chunk - haveInformation now returns %s\n",
                    haveInformation() ? "true" : "false");
    _m.unlock();
    return 0;
}

void CancelledTSNManager::deliveredReliableSequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableSequencedTSNHandler.deleteTSNsUpTo (ui32TSN);
    _m.unlock();
}

void CancelledTSNManager::deliveredReliableUnsequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableUnsequencedTSNHandler.deleteTSNsUpTo (ui32TSN);
    _m.unlock();
}

void CancelledTSNManager::deliveredUnreliableSequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _unreliableSequencedTSNHandler.deleteTSNsUpTo (ui32TSN);
    _m.unlock();
}

int CancelledTSNManager::appendCancelledTSNInformation (Packet *pPacket)
{
    // This method will return a negative value if ALL of the cancelled TSN information cannot be added to the packet
    // But partial info will still be in the piggyback chunk
    _m.lock();
    CancelledChunkMutator ccm = pPacket->addCancelledChunk();
    if (!ccm.initializedWithoutErrors()) {
        checkAndLogMsg ("CancelledTSNManager::appendCancelledTSNInformation", Logger::L_MediumDetailDebug,
                        "failed to add CancelledChunk to packet\n");
        _m.unlock();
        return -1;
    }
    ccm.selectReliableSequencedFlow();
    if (_reliableSequencedTSNHandler.appendTSNInformation (&ccm)) {
        checkAndLogMsg ("CancelledTSNManager::appendCancelledTSNInformation", Logger::L_MildError,
                        "failed to append cancelled information for the reliable sequenced flow\n");
        _m.unlock();
        return -2;
    }
    ccm.selectReliableUnsequencedFlow();
    if (_reliableUnsequencedTSNHandler.appendTSNInformation (&ccm)) {
        checkAndLogMsg ("CancelledTSNManager::appendCancelledTSNInformation", Logger::L_MildError,
                        "failed to append cancelled information for the reliable unsequenced flow\n");
        _m.unlock();
        return -3;
    }
    ccm.selectUnreliableSequencedFlow();
    if (_unreliableSequencedTSNHandler.appendTSNInformation (&ccm)) {
        checkAndLogMsg ("CancelledTSNManager::appendCancelledPacketInformation", Logger::L_MildError,
                        "failed to append cancelled information for the unreliable sequenced flow\n");
        _m.unlock();
        return -4;
    }
    _m.unlock();
    return 0;
}

int CancelledTSNManager::freeze (ObjectFreezer &objectFreezer)
{
    if (0 != _reliableSequencedTSNHandler.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    if (0 != _reliableUnsequencedTSNHandler.freeze (objectFreezer)) {
        return -3;
    }
    if (0 != _unreliableSequencedTSNHandler.freeze (objectFreezer)) {
        return -4;
    }
    return 0;
}

int CancelledTSNManager::defrost (ObjectDefroster &objectDefroster)
{
    if (0 != _reliableSequencedTSNHandler.defrost (objectDefroster)) {
        return -2;
    }
    if (0 != _reliableUnsequencedTSNHandler.defrost (objectDefroster)) {
        return -3;
    }
    if (0 != _unreliableSequencedTSNHandler.defrost (objectDefroster)) {
        return -4;
    }
    return 0;
}

