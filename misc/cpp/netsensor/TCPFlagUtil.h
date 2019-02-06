#ifndef NETSENSOR_TCPFlagUtil__INCLUDED
#define NETSENSOR_TCPFlagUtil__INCLUDED
/*
* FINHandshake.h
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

#include "NetworkHeaders.h"
namespace IHMC_NETSENSOR
{
    inline bool hasCWRFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_CWR ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_CWR);
    }

    inline bool hasECEFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_ECE ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_ECE);
    }

    inline bool hasURGFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_URG ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_URG);
    }

    inline bool hasACKFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_ACK ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_ACK);
    }

    inline bool hasPSHFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_PSH ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_PSH);
    }

    inline bool hasRSTFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_RST ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_RST);
    }

    inline bool hasSYNFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_SYN ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_SYN);
    }

    inline bool hasFINFlag(uint8 flagValue)
    {
        return NOMADSUtil::TCPHeader::Flags::TCPF_FIN ==
            (flagValue & NOMADSUtil::TCPHeader::Flags::TCPF_FIN);
    }
}

#endif