#ifndef INCL_PCAP_INTERFACE_H
#define INCL_PCAP_INTERFACE_H

/*
 * PCapInterface.h
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
 *
 * Class that manages the access to a hardware
 * network interface using the pcap library.
 */

#include <string>
#include <mutex>

#if defined (WIN32)
    #include <pcap.h>
#elif defined (LINUX)
    #include <pcap/pcap.h>
#endif

#include "FTypes.h"
#include "net/NetworkHeaders.h"

#include "NetworkInterface.h"


namespace ACMNetProxy
{
    class PCapInterface : public NetworkInterface
    {
    public:
        virtual ~PCapInterface (void);
        static PCapInterface * const getPCapInterface (const char * const pszDevice);

        int init (void);
        void requestTermination (void);

        int readPacket (const uint8 ** pui8Buf, uint16 & ui16PacketLen);
        int writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen);


    private:
        PCapInterface (const std::string & sInterfaceName, const std::string & sUserFriendlyInterfaceName);
        explicit PCapInterface (const PCapInterface & rCopy);

        void retrieveAndSetIPv4Addr (void);

        static pcap_t * const createAndActivateReadHandler (const std::string & sInterfaceName);
        static pcap_t * const createAndActivateWriteHandler (const std::string & sInterfaceName);


        pcap_t * _pPCapReadHandler;
        pcap_t * _pPCapWriteHandler;

        std::mutex _mtxWrite;
    };


    inline void PCapInterface::requestTermination (void)
    {
        NetworkInterface::requestTermination();
    }

}

#endif   // #ifndef INCL_RAW_SOCKET_INTERFACE_H
