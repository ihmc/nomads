#ifndef INCL_TAP_INTERFACE_H
#define INCL_TAP_INTERFACE_H

/*
 * TapInterface.h
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
 * The TAPInterface class manages the access to the TUN/TAP Driver.
 */

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "Mutex.h"

#include "NetworkInterface.h"
#include "ConfigurationParameters.h"
#include "PacketBufferManager.h"

#if defined (WIN32)
    #include <winsock2.h>    // To avoid conflicts occurring if some other file includes winsock2.h after windows.h is included below
    #include <windows.h>
#elif defined (LINUX)
    #include <linux/if.h>
#endif

#define TAP_FILENAME_LENGTH 255


namespace ACMNetProxy
{
    class TapInterface : public NetworkInterface
    {
        public:
            virtual ~TapInterface (void);
            static TapInterface * const getTAPInterface (void);

            int init (void);
            void requestTermination (void);
            bool checkMACAddress (void);

            int readPacket (uint8 *pui8Buf, uint16 ui16BufSize);
            int writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen);


        private:
            TapInterface (void);
            explicit TapInterface (const TapInterface& rCopy);

            #if defined (WIN32)
                HANDLE _hInterface;
                OVERLAPPED _oRead, _oWrite;
            #elif defined (LINUX)
                int _fdTAP;
                fd_set fdSet;
                struct timeval tvTimeout;
            #endif
            PacketBufferManager * const pPBM;

            NOMADSUtil::Mutex _mWrite;
    };


    inline void TapInterface::requestTermination (void)
    {
        NetworkInterface::requestTermination();
    }

    inline TapInterface * const TapInterface::getTAPInterface (void)
    {
        static TapInterface tapInterface;
        tapInterface.init();

        return &tapInterface;
    }

}

#endif   // #ifndef INCL_TAP_INTERFACE_H
