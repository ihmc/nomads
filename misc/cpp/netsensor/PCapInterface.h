#ifndef NETSENSOR_PCapInterface__INCLUDED
#define NETSENSOR_PCapInterface__INCLUDED
/*
* PCapInterface.h
* Author: rfronteddu@ihmc.us
* This file is part of the IHMC NetProxy Library/Component
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
* Class that manages the access to a hardware
* network interface using the pcap library.
*
* PCap interface wrapper
*/

#if defined (WIN32)
    #include "pcap.h"
#elif defined (LINUX)
    #include <pcap/pcap.h>
#endif

#include "FTypes.h"
#include "net/NetworkHeaders.h"
#include "StrClass.h"
#include "Mutex.h"
#include "NetworkInterface.h"
#include "InetAddr.h"

#include "InterfaceCaptureMode.h"

namespace IHMC_NETSENSOR
{
class PCapInterface : public NetworkInterface
{
    enum class Mode {M_LIVE, M_OFFLINE};

public:
    static PCapInterface * const getPCapInterface(const char * const pszDevice,
                                                  int msValidity);
    static PCapInterface * const getPCapInterface(const NOMADSUtil::String & sAdapterName,
                                                  const NOMADSUtil::String & sPcapFile,
                                                  uint32 ui32IPAddr, uint32 ui32Netmask,
                                                  uint32 ui32GwIPAddr,
                                                  NOMADSUtil::EtherMACAddr emacInterfaceMAC);

    virtual ~PCapInterface(void);

    int readPacket(uint8 *pui8Buf, uint16 ui16BufSize, int64 *tus);

private:
    PCapInterface(const NOMADSUtil::String & sAdapterName);
    PCapInterface(const NOMADSUtil::String & sAdapterName,
                  const NOMADSUtil::String & sPcapFile,
                  uint32 ui32IPAddr, uint32 ui32Netmask,
                  uint32 ui32GwIPAddr,
                  NOMADSUtil::EtherMACAddr emacInterfaceMAC);

    int initLive(int msPacketValid);
    int initReplay(void);

    void retrieveAndSetIPv4Addr(void);

//<--------------------------------------------------------------------------->
private:
#if defined (WIN32)
    const char * const _pszNetworkAdapterNamePrefix = "\\Device\\NPF_";
#else
    const char * const _pszNetworkAdapterNamePrefix = "";
#endif

    NOMADSUtil::String  _sPcapFile;
    pcap_t             *_pPCapHandle;
    const Mode          _m;
};


inline PCapInterface::PCapInterface(const NOMADSUtil::String & sAdapterName) :
    NetworkInterface(), _sPcapFile(""), _pPCapHandle(nullptr), _m{Mode::M_LIVE}
{
    _sAdapterName = sAdapterName;
}

inline PCapInterface::PCapInterface(const NOMADSUtil::String & sAdapterName,
                                    const NOMADSUtil::String & sPcapFile,
                                    uint32 ui32IPAddr, uint32 ui32Netmask,
                                    uint32 ui32GwIPAddr,
                                    NOMADSUtil::EtherMACAddr emacInterfaceMAC) :
    NetworkInterface(ui32IPAddr, ui32Netmask, ui32GwIPAddr, emacInterfaceMAC),
    _sPcapFile(sPcapFile), _pPCapHandle(nullptr), _m{Mode::M_OFFLINE}
{
    _sAdapterName = sAdapterName;
}

inline PCapInterface::~PCapInterface(void)
{
    requestTermination();
    if (_pPCapHandle) {
        pcap_close(_pPCapHandle);
        _pPCapHandle = nullptr;
    }
}

}
#endif
