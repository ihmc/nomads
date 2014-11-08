#ifndef INCL_PCAP_INTERFACE_H
#define INCL_PCAP_INTERFACE_H

/*
 * PCapInterface.h
 * 
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2014 IHMC.
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

#if defined (WIN32)
    #include <pcap.h>
#elif defined (LINUX)
    #include <pcap/pcap.h>
#endif

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "StrClass.h"
#include "Mutex.h"

#include "NetworkInterface.h"


namespace ACMNetProxy
{
    class PCapInterface : public NetworkInterface
    {
        public:
            virtual ~PCapInterface (void);
            static PCapInterface * const getPCapInterface (const char *pszDevice);

            int init (void);
            void requestTermination (void);
            bool checkMACAddress (void);

            int readPacket (uint8 *pui8Buf, uint16 ui16BufSize);
            int writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen);

        private:
            PCapInterface (const NOMADSUtil::String &sDeviceName);
            explicit PCapInterface (const PCapInterface &rCopy);

            void retrieveAndSetIPv4Addr (void);

            pcap_t *_pPCapHandle;

            NOMADSUtil::Mutex _mWrite;
    };


    inline void PCapInterface::requestTermination (void)
    {
        NetworkInterface::requestTermination();
    }

}

#endif   // #ifndef INCL_RAW_SOCKET_INTERFACE_H
