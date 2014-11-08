#ifndef INCL_DATA_BUFFER_H
#define INCL_DATA_BUFFER_H

/*
 * DataBuffer.h
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

#include "LList.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"

#include "Packet.h"
#include "PacketAccessors.h"

#include <stddef.h>


class PacketProcessor;

class DataBuffer
{
    public:
        // Constructor used when defrosting the state
        DataBuffer (PacketProcessor *pPacketProcessor);
        
        // Constructor used when a normal message has been received
        DataBuffer (PacketProcessor *pPacketProcessor, Packet *pPacket, uint32 ui32MessageSize);

        // Constructor used when a fragmented message has been received
        DataBuffer (PacketProcessor *pPacketProcessor, NOMADSUtil::LList<Packet*> *pFragments, uint32 ui32MessageSize);

        ~DataBuffer (void);

        bool fragmentedMessage (void);
        uint32 getMessageSize (void);
        Packet * getPacket (void);
        NOMADSUtil::LList<Packet*> * getFragments (void);

    private:
        friend class PacketQueue;
        
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

        PacketProcessor *_pPacketProcessor;
        Packet *_pPacket;
        NOMADSUtil::LList<Packet*> *_pFragments;
        uint32 _ui32MessageSize;
};

inline DataBuffer::DataBuffer (PacketProcessor *pPacketProcessor)
{
    _pPacketProcessor = pPacketProcessor;
    _pFragments = NULL;
    _pPacket = NULL;
    _ui32MessageSize = 0;
}

inline DataBuffer::DataBuffer (PacketProcessor *pPacketProcessor, Packet *pPacket, uint32 ui32MessageSize)
{
    _pPacketProcessor = pPacketProcessor;
    _pFragments = NULL;
    _pPacket = pPacket;
    _ui32MessageSize = ui32MessageSize;
}

inline DataBuffer::DataBuffer (PacketProcessor *pPacketProcessor, NOMADSUtil::LList<Packet*> *pFragments, uint32 ui32MessageSize)
{
    _pPacketProcessor = pPacketProcessor;
    _pFragments = pFragments;
    _ui32MessageSize = ui32MessageSize;
    _pPacket = NULL;
}

inline bool DataBuffer::fragmentedMessage (void)
{
    return (_pFragments != NULL);
}

inline uint32 DataBuffer::getMessageSize (void)
{
    return _ui32MessageSize;
}

inline Packet * DataBuffer::getPacket (void)
{
    return _pPacket;
}

inline NOMADSUtil::LList<Packet*> * DataBuffer::getFragments (void)
{
    return _pFragments;
}

inline int DataBuffer::freeze (NOMADSUtil::ObjectFreezer &objectFreezer)
{
    // Chek if this is a normal message or a fragmented message
    if (_pPacket != NULL) {
        // Normal message it contains a packet. Mark it with a control char 1
        objectFreezer << (unsigned char) 1;
        //Data from _pPacket
        _pPacket->freeze (objectFreezer);
    }
    else {
        // Fragmented message, it contains LList<Packet*>. Mark it with a control char 0
        objectFreezer << (unsigned char) 0;
        Packet *pCurrPacket = NULL;
        _pFragments->getFirst (pCurrPacket);
        //Data from the current Packet
        pCurrPacket->freeze (objectFreezer);
        while (1 ==  _pFragments->getNext (pCurrPacket)) {
            // Insert a control char to signal that another fragment Packet follows
            objectFreezer << (unsigned char) 1;
            //Data from the current Packet
            pCurrPacket->freeze (objectFreezer);            
        }
        // Insert a control char to signal that there are no more data
        objectFreezer << (unsigned char) 0;
    }
    
    objectFreezer.putUInt32 (_ui32MessageSize);
    return 0;
}

inline int DataBuffer::defrost (NOMADSUtil::ObjectDefroster &objectDefroster)
{
    unsigned char controlChar;
    objectDefroster >> controlChar;
    if (controlChar) {
        // Normal message
        // Recontract the object Packet
        _pPacket = new Packet (objectDefroster);
    }
    else {
        // Fragmented message
        _pFragments = new NOMADSUtil::LList<Packet*>;
        // There is at list one fragment
        unsigned char moreData = (unsigned char) 1;
        while (moreData) {
            Packet * pPacket = new Packet (objectDefroster);
            _pFragments->add (pPacket);
            
            objectDefroster >> moreData;
        }
    }
    
    objectDefroster >> _ui32MessageSize;
    return 0;
}

#endif   // #ifndef INCL_DATA_BUFFER_H
