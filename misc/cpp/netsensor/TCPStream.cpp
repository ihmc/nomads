/*
* TCPStream.h
* Author: bordway@ihmc.us rfronteddu@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
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
*
*/

#include "TCPStream.h";

using namespace NOMADSUtil;
namespace IHMC_NETSENSOR
{
    TCPStream::TCPStream (TCPStreamData & streamData, uint32 ui32Resolution)
        : _rttTable(ui32Resolution)
    {
        _data = streamData;
        _bIsClosed = false;
    }

    void TCPStream::addToRTTTable (uint32 ui32SeqNum, uint16 ui16TCPDataLength,
        int64 i64Timestamp)
    {
        uint32 ui32NextAckNum = ui16TCPDataLength + ui32SeqNum;
        _rttTable.putNewSeqEntry(ui32SeqNum, ui32NextAckNum, i64Timestamp);
    }

    uint32 TCPStream::clean (uint32 ui32MaxCleaningNumber)
    {
        return _rttTable.cleanTables(ui32MaxCleaningNumber);
    }

    uint8 TCPStream::getCount()
    {
        return _rttTable.getCount();
    }

    TCPStreamData TCPStream::getData()
    {
        return _data;
    }

    void TCPStream::checkRTTTable (uint32 ui32AckNum, int64 i64Timestamp)
    {
        if (_rttTable.hasAckNum(ui32AckNum))
        {
            _rttTable.putNewAckEntry(ui32AckNum, i64Timestamp);
        }
    }

    void TCPStream::print(void)
    {
        printf("\n\nStream data:\n---------------------\n"
            "Source IP: %15s Dest IP: %15s, Source Port: %6s, Dest Port: %6s\n",
            _data.sLocalIP.c_str(), _data.sRemoteIP.c_str(),
            _data.sLocalPort.c_str(), _data.sRemotePort.c_str());

        _rttTable.print();
    }

    bool TCPStream::hasClosed(void)
    {
        return _bIsClosed;
    }


    void TCPStream::updateStreamRTTs (IPHeader & ipHeader, int64 rcvTime, bool isIncoming)
    {
        i64LastUpdateTime = getTimeInMilliseconds();

        uint16 ui16IPHeaderLen = (ipHeader.ui8VerAndHdrLen & 0x0F) * 4;
        ipHeader.ntoh();
        uint16 ui16TotalLength = ipHeader.ui16TLen;
        ipHeader.hton();

        // Get TCP Header data
        TCPHeader *pTCPHeader = (TCPHeader*)(((uint8*)&ipHeader) +
            ui16IPHeaderLen);
        uint16 ui16TCPHeaderLen = (pTCPHeader->ui8Offset >> 4) * 4;
        uint8 ui8FlagValue = pTCPHeader->ui8Flags;

        uint32 seqNum = pTCPHeader->ui32SeqNum;
        uint32 ackNum = pTCPHeader->ui32AckNum;

        uint16 ui16PayloadSize = ui16TotalLength - (ui16IPHeaderLen + ui16TCPHeaderLen);

        // If there's a reset connection flag, this stream is dead
        if (hasRSTFlag(ui8FlagValue)) {
            _bIsClosed = true;
            return;
        }

        // If the fin handshake has been started, then the next values 
        // should be for the fin handshake
        if (_finHandshake.isInitiated()) {
            _finHandshake.offerFlag(ui8FlagValue);
            if (_finHandshake.isComplete()) {
                _bIsClosed = true;
                return;
            }
        }
        else {
            if (hasFINFlag(ui8FlagValue)) {
                _finHandshake.offerFlag(ui8FlagValue);
            }
        }

        if (isIncoming) {
            if (hasACKFlag(ui8FlagValue))  {
                checkRTTTable(ackNum, rcvTime);
            }
        }

        else 
        {
            // If this is a packet with outgoing data, an entry should be added
            if (hasPSHFlag(ui8FlagValue))  {
                addToRTTTable(seqNum, ui16PayloadSize, rcvTime);
            }
        } 
    }

    NOMADSUtil::PtrLList<measure::Measure>* TCPStream::createMeasures(NOMADSUtil::String sSensorIP)
    {
        PtrLList<measure::Measure>* pMeasureList = new PtrLList<measure::Measure>(true);
        ProtobufWrapper protobufWrapper;

        if (_rttTable.getCount() == 0) {
            return pMeasureList;
        }

        float  avgRTT = _rttTable.getAvgRTT();
        uint32 minRTT = _rttTable.getMinRTT();
        uint32 maxRTT = _rttTable.getMaxRTT();
        uint32 mostRecentRTT = _rttTable.getMostRecentRTT();

        measure::Measure *pMeasure = protobufWrapper.getMeasureRTT(sSensorIP, _data.sLocalIP, _data.sRemoteIP,
            "TCP", _data.sLocalPort, _data.sRemotePort, minRTT, maxRTT, mostRecentRTT, 5000, avgRTT);
        pMeasureList->append(pMeasure);

        return pMeasureList;
    }
}