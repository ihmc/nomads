/*
* ICMPTypesContainer.cpp
* Author: bordway@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
* This class stores an instance of an ICMP packet and additional information
* that can be used by NetSensor recipients.
*/
#include"ICMPTypesContainer.h"
#include"InetAddr.h"

using namespace IHMC_NETSENSOR_NET_UTILS;
namespace IHMC_NETSENSOR
{

    // Constructor, initialize member variables and the TIA
    ICMPTypesContainer::ICMPTypesContainer(NS_ICMPPacket *pICMPPacket, const uint32 ui32msResolution)
        : tiaICMP(ui32msResolution)
    {
        _ui32Count = 0; // Start at 0; incrementCount gets called by caller
        pContainerIcmpPacket = new NS_ICMPPacket();

        pContainerIcmpPacket->pMacHeader = new NS_EtherFrameHeader();
        pContainerIcmpPacket->pIpHeader  = new NS_IPHeader();
        pContainerIcmpPacket->pIcmpData  = new NS_ICMP();

        // Create a copy of the packet
        *pContainerIcmpPacket->pMacHeader = *pICMPPacket->pMacHeader;
        *pContainerIcmpPacket->pIpHeader  = *pICMPPacket->pIpHeader;
        *pContainerIcmpPacket->pIcmpData  = *pICMPPacket->pIcmpData;
        pContainerIcmpPacket->i64RcvTimeStamp = pICMPPacket->i64RcvTimeStamp;

        if (hasExtraAddresses())
            extraIPAddresses.configure(true, true, true, true);
    }

    ICMPTypesContainer::~ICMPTypesContainer()
    {
        delete pContainerIcmpPacket->pMacHeader;
        pContainerIcmpPacket->pMacHeader = nullptr;
        delete pContainerIcmpPacket->pIpHeader;
        pContainerIcmpPacket->pIpHeader = nullptr;
        delete pContainerIcmpPacket->pIcmpData;
        pContainerIcmpPacket->pIcmpData = nullptr;
        delete pContainerIcmpPacket;
        pContainerIcmpPacket = nullptr;


        if (extraIPAddresses.getCount() != 0)
        {
            NOMADSUtil::String cleaningValues[C_MAX_CLEANING_NUMBER];
            int cleaningCounter = 0;
            for (NOMADSUtil::StringHashtable<uint32>::Iterator keyVal =
                extraIPAddresses.getAllElements();
                !keyVal.end(); keyVal.nextElement())
            {
                cleaningValues[cleaningCounter] = keyVal.getKey();
                cleaningCounter++;
            }

            for (uint32 counter = 0; counter < cleaningCounter; counter++)
            {
                delete extraIPAddresses.remove(cleaningValues[counter]);
            }
        }
    }

    bool ICMPTypesContainer::isExpired()
    {
        return tiaICMP.getAverage() == 0;
    }

    uint32 ICMPTypesContainer::getCount()
    {
        return _ui32Count;
    }

    void ICMPTypesContainer::incrementCount()
    {
        _ui32Count += (_ui32Count == _maxUINT32) ? 0 : 1; // Increment count unless it's at the max
    }

    // Return icmp packet's type
    uint8 ICMPTypesContainer::getUi8Type()
    {
        return pContainerIcmpPacket->pIcmpData->ui8Type;
    }

    // Return icmp packet's code
    uint8 ICMPTypesContainer::getUi8Code()
    {
        return pContainerIcmpPacket->pIcmpData->ui8Code;
    }

    // This function stores and updates # of unique redirect addresses or address masks
    void ICMPTypesContainer::updateExtraAddresses(const char *pIPA)
    {
        uint32 *count = extraIPAddresses.get(pIPA);

        if (count == nullptr)
        {
            count = new uint32;
            *count = 1;
            extraIPAddresses.put(pIPA, count);
        }
        else
        {
            *count += 1;
        }
    }

    // If the packet is a specific type, it will have an additional address
    bool ICMPTypesContainer::hasExtraAddresses()
    {
        return (getUi8Type() == NS_ICMP::T_Redirect_Message || getUi8Type() == NS_ICMP::T_Address_Mask_Reply || getUi8Type() == NS_ICMP::T_Address_Mask_Request);
    }
}
