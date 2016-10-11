/*
 * ACKManager.cpp
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

#include "ACKManager.h"

#include "Packet.h"
#include "PacketMutators.h"

#include "Logger.h"
#include "NLFLib.h"


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

ACKManager::ACKManager (void)
{
    _i64LastUpdateTime = 0;
    _bNewUpdateAtSameTimeStamp = false;
}

int ACKManager::init (uint32 ui32InitialControlTSN, uint32 ui32InitialReliableSequencedTSN, uint32 ui32InitialReliableUnsequencedID)
{
    _controlTSNHandler.setCumulativeTSN (ui32InitialControlTSN - 1);
    _reliableSequencedTSNHandler.setCumulativeTSN (ui32InitialReliableSequencedTSN - 1);
    _reliableUnsequencedTSNHandler.setCumulativeTSN (ui32InitialReliableUnsequencedID - 1);
    return 0;
}

void ACKManager::receivedControlPacket (uint32 ui32TSN)
{
    _m.lock();
    _controlTSNHandler.addTSN (ui32TSN);
    int64 i64NewUpdateTime = getTimeInMilliseconds();
    if (_i64LastUpdateTime == i64NewUpdateTime) {
        _bNewUpdateAtSameTimeStamp = true;
    }
    else {
        _i64LastUpdateTime = i64NewUpdateTime;
        _bNewUpdateAtSameTimeStamp = false;
    }
    _m.unlock();
}

void ACKManager::receivedReliableSequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableSequencedTSNHandler.addTSN (ui32TSN);
    int64 i64NewUpdateTime = getTimeInMilliseconds();
    if (_i64LastUpdateTime == i64NewUpdateTime) {
        _bNewUpdateAtSameTimeStamp = true;
    }
    else {
        _i64LastUpdateTime = i64NewUpdateTime;
        _bNewUpdateAtSameTimeStamp = false;
    }
    _m.unlock();
}

void ACKManager::receivedReliableUnsequencedPacket (uint32 ui32TSN)
{
    _m.lock();
    _reliableUnsequencedTSNHandler.addTSN (ui32TSN);
    int64 i64NewUpdateTime = getTimeInMilliseconds();
    if (_i64LastUpdateTime == i64NewUpdateTime) {
        _bNewUpdateAtSameTimeStamp = true;
    }
    else {
        _i64LastUpdateTime = i64NewUpdateTime;
        _bNewUpdateAtSameTimeStamp = false;
    }
    _m.unlock();
}

int ACKManager::appendACKInformation (Packet *pPacket)
{
    // This method will return a negative value if ALL of the SAck information cannot be added to the packet
    // But partial SAck info will still be in the piggyback chunk
    int rc;
    _m.lock();
    SAckChunkMutator scm = pPacket->addSAckChunk (_controlTSNHandler.getCumulativeTSN(), _reliableSequencedTSNHandler.getCumulativeTSN(), _reliableUnsequencedTSNHandler.getCumulativeTSN());
    if (!scm.initializedWithoutErrors()) {
        checkAndLogMsg ("ACKManager::appendSAckInformation", Logger::L_MediumDetailDebug,
                        "failed to add SAckChunk to packet\n");
        _m.unlock();
        return -1;
    }
    scm.selectControlFlow();
    if (0 != (rc = _controlTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the control flow; rc = %d\n", rc);
        _m.unlock();
        return -2;
    }
    scm.selectReliableSequencedFlow();
    if (0 != (rc = _reliableSequencedTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the reliable sequenced flow; rc = %d\n", rc);
        _m.unlock();
        return -3;
    }
    scm.selectReliableUnsequencedFlow();
    if (0 != (rc = _reliableUnsequencedTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the reliable unsequenced flow; rc = %d\n", rc);
        _m.unlock();
        return -4;
    }
    _m.unlock();
    return 0;
}

// This SAck is also used to pass the info to estimate the bandwidth receiver side
int ACKManager::appendACKInformation (Packet *pPacket, int64 i64Timestamp, uint32 ui32BytesReceived)
{
    // This method will return a negative value if ALL of the SAck information cannot be added to the packet
    // But partial SAck info will still be in the piggyback chunk
    int rc;
    _m.lock();
    SAckChunkMutator scm = pPacket->addSAckChunk (_controlTSNHandler.getCumulativeTSN(), _reliableSequencedTSNHandler.getCumulativeTSN(), _reliableUnsequencedTSNHandler.getCumulativeTSN(), i64Timestamp, ui32BytesReceived);
    if (!scm.initializedWithoutErrors()) {
        checkAndLogMsg ("ACKManager::appendSAckInformation", Logger::L_MediumDetailDebug,
                        "failed to add SAckChunk to packet\n");
        _m.unlock();
        return -1;
    }
    scm.selectControlFlow();
    if (0 != (rc = _controlTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the control flow; rc = %d\n", rc);
        _m.unlock();
        return -2;
    }
    scm.selectReliableSequencedFlow();
    if (0 != (rc = _reliableSequencedTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the reliable sequenced flow; rc = %d\n", rc);
        _m.unlock();
        return -3;
    }
    scm.selectReliableUnsequencedFlow();
    if (0 != (rc = _reliableUnsequencedTSNHandler.appendTSNInformation (&scm))) {
        checkAndLogMsg ("ACKManager::appendACKInformation", Logger::L_MildError,
                        "failed to append ACK information for the reliable unsequenced flow; rc = %d\n", rc);
        _m.unlock();
        return -4;
    }
    _m.unlock();
    return 0;
}

int ACKManager::freeze (ObjectFreezer &objectFreezer)
{
    if (0 != _controlTSNHandler.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    if (0 != _reliableSequencedTSNHandler.freeze (objectFreezer)) {
        return -3;
    }
    if (0 != _reliableUnsequencedTSNHandler.freeze (objectFreezer)) {
        return -4;
    }
    return 0;
}

int ACKManager::defrost (ObjectDefroster &objectDefroster)
{
    if (0 != _controlTSNHandler.defrost (objectDefroster)) {
        return -2;
    }
    if (0 != _reliableSequencedTSNHandler.defrost (objectDefroster)) {
        return -3;
    }
    if (0 != _reliableUnsequencedTSNHandler.defrost (objectDefroster)) {
        return -4;
    }
    return 0;
}
