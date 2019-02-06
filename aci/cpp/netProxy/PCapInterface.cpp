/*
 * PCapInterface.cpp
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
 */

#include <sstream>

#if defined (WIN32)
    #include <winsock2.h>
    #include <iphlpapi.h>

    #define snprintf _snprintf
#elif defined (LINUX)
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <net/if.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include "Logger.h"

#include "PCapInterface.h"
#include "Utilities.h"
#include "ConfigurationParameters.h"


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    PCapInterface * const PCapInterface::getPCapInterface (const char * const pcFriendlyInterfaceName)
    {
        int rc;
        pcap_if_t * pAllDevs;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs (&pAllDevs, errbuf) == -1) {
            return nullptr;
        }

        std::string sDeviceName (NetworkInterface::getDeviceNameFromUserFriendlyName (pcFriendlyInterfaceName));
        if (sDeviceName.length() <= 0) {
            checkAndLogMsg ("PCapInterface::getPCapInterface", NOMADSUtil::Logger::L_MildError,
                            "impossible to retrieve the network device name from the user-friendly name %s\n",
                            pcFriendlyInterfaceName);
            return nullptr;
        }
        PCapInterface *pPCapInterface = new PCapInterface (sDeviceName, pcFriendlyInterfaceName);
        if (0 != (rc = pPCapInterface->init())) {
            checkAndLogMsg ("PCapInterface::getPCapInterface", NOMADSUtil::Logger::L_MildError,
                            "failed to initialize PCapInterface; rc = %d\n", rc);
            delete pPCapInterface;
            return nullptr;
        }

        return pPCapInterface;
    }

    int PCapInterface::init (void)
    {
        if (nullptr == (_pPCapReadHandler =
                        createAndActivateReadHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sInterfaceName))) {
            return -1;
        }
        if (nullptr == (_pPCapWriteHandler =
                        createAndActivateWriteHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sInterfaceName))) {
            return -2;
        }

        const uint8 * const pszMACAddr = NetworkInterface::getMACAddrForDevice (_sInterfaceName.c_str());
        if (pszMACAddr) {
            memcpy (static_cast<void*> (_aui8MACAddr), pszMACAddr, 6);
            _bMACAddrFound = true;
            delete[] pszMACAddr;
        }

        retrieveAndSetIPv4Addr();

        #if defined (WIN32)
        _ui16MTU = retrieveMTUForInterface (_sInterfaceName.c_str());
        #elif defined (LINUX)
        _ui16MTU = retrieveMTUForInterface (_sInterfaceName.c_str(), pcap_get_selectable_fd (_pPCapReadHandler));
        #endif
        if (_ui16MTU > 0) {
            _bMTUFound = true;
        }

        NOMADSUtil::IPv4Addr ipv4DefGW {NetworkInterface::getDefaultGatewayForInterface (_sInterfaceName.c_str())};
        if (ipv4DefGW.ui32Addr) {
            _bDefaultGatewayFound = true;
            _ipv4DefaultGateway = ipv4DefGW;
        }

        return 0;
    }

    int PCapInterface::readPacket (const uint8 ** pui8Buf, uint16 & ui16PacketLen)
    {
        struct pcap_pkthdr * pPacketHeader;

        while (!_bIsTerminationRequested) {
            int rc = 0;
            if (_pPCapReadHandler == nullptr) {
                std::lock_guard<std::mutex> lg{_mtxWrite};
                checkAndLogMsg ("PCapInterface::readPacket", NOMADSUtil::Logger::L_Warning,
                                "the pcap read handler is null: restarting it\n");
                NOMADSUtil::sleepForMilliseconds (500);
                if (nullptr == (_pPCapReadHandler =
                                createAndActivateReadHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sInterfaceName))) {
                    checkAndLogMsg ("PCapInterface::ReadPacket", NOMADSUtil::Logger::L_SevereError,
                                    "failed to restart the pcap read handler; NetProxy will retry in the next cycle\n");
                }
                else {
                    checkAndLogMsg ("PCapInterface::ReadPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                                    "successfully restarted the pcap read handler\n");
                }
            }
            else {
                rc = pcap_next_ex (_pPCapReadHandler, &pPacketHeader, pui8Buf);
                if (rc > 0) {
                    if (pPacketHeader->caplen < pPacketHeader->len) {
                        checkAndLogMsg ("PCapInterface::readPacket", NOMADSUtil::Logger::L_Warning,
                                        "the caplen field (%u bytes) of the pcap_pkthdr struct is smaller than the len "
                                        "field (%u bytes); something went wrong (libpcap buffer too small or full?); "
                                        "discarding packet\n", pPacketHeader->caplen, pPacketHeader->len);
                        *pui8Buf = nullptr;
                        ui16PacketLen = 0;
                        continue;
                    }
                    ui16PacketLen = pPacketHeader->caplen;
                    return 0;
                }
                else if (rc < 0) {
                    checkAndLogMsg ("PCapInterface::readPacket", NOMADSUtil::Logger::L_MildError,
                                    "pcap_next_ex() returned %d; closing the current handler, "
                                    "it will be restarted in the next cycle\n", rc);
                    *pui8Buf = nullptr;
                    ui16PacketLen = 0;

                    std::lock_guard<std::mutex> lg{_mtxWrite};
                    if (_pPCapReadHandler != nullptr) {
                        pcap_close (_pPCapReadHandler);
                        _pPCapReadHandler = nullptr;
                    }
                }
            }
        }

        return 0;
    }

    int PCapInterface::writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen)
    {
        int rc;

        std::lock_guard<std::mutex> lg{_mtxWrite};
        if (_pPCapWriteHandler == nullptr) {
            checkAndLogMsg ("PCapInterface::writePacket", NOMADSUtil::Logger::L_Warning,
                            "the pcap write handler is null: restarting it");
            NOMADSUtil::sleepForMilliseconds (500);
            if (nullptr == (_pPCapWriteHandler =
                            createAndActivateWriteHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sInterfaceName))) {
                checkAndLogMsg ("PCapInterface::writePacket", NOMADSUtil::Logger::L_SevereError,
                                "failed to restart the pcap write handler\n");
                return 0;
            }
            else {
                checkAndLogMsg ("PCapInterface::ReadPacket", NOMADSUtil::Logger::L_LowDetailDebug,
                                "successfully restarted the pcap write handler\n");
            }
        }

        for (int counter = 0; counter < NetworkConfigurationSettings::PCAP_SEND_PACKET_ATTEMPTS; counter++) {
            if (0 != (rc = pcap_sendpacket (_pPCapWriteHandler, pui8Buf, ui16PacketLen))) {
                checkAndLogMsg ("PCapInterface::writePacket", NOMADSUtil::Logger::L_MildError,
                                "pcap_sendpacket() failed writing attempt #%d with a %hu bytes long packet: "
                                "%s;%s\n", counter + 1, ui16PacketLen, pcap_geterr (_pPCapWriteHandler),
                                (counter < (NetworkConfigurationSettings::PCAP_SEND_PACKET_ATTEMPTS - 1)) ?
                                " retrying..." : " send has failed");
                NOMADSUtil::sleepForMilliseconds (NetworkConfigurationSettings::PCAP_SEND_ATTEMPT_INTERVAL_IN_MS);
            }
            else {
                return ui16PacketLen;
            }
        }

        return rc;
    }

    PCapInterface::PCapInterface (const std::string & sInterfaceName, const std::string & sUserFriendlyInterfaceName) :
        NetworkInterface{Type::T_PCap, sInterfaceName, sUserFriendlyInterfaceName}
    {
        _pPCapReadHandler = nullptr;
        _pPCapWriteHandler = nullptr;
    }

    PCapInterface::~PCapInterface (void)
    {
        requestTermination();

        if (_pPCapReadHandler) {
            pcap_close (_pPCapReadHandler);
            _pPCapReadHandler = nullptr;
        }
        if (_pPCapWriteHandler) {
            pcap_close (_pPCapWriteHandler);
            _pPCapWriteHandler = nullptr;
        }
    }

    void PCapInterface::retrieveAndSetIPv4Addr (void)
    {
        pcap_if_t * pAllDevs;
        pcap_if_t * pDevice;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs (&pAllDevs, errbuf) == -1) {
            _bIPAddrFound = false;
            _ipv4Addr.ui32Addr = 0;
            return;
        }

        ci_string sAdapterName = NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + ci_string{_sInterfaceName.c_str()};
        for (pDevice = pAllDevs; pDevice != nullptr; pDevice = pDevice->next) {
            if (sAdapterName == pDevice->name) {
                for (const pcap_addr_t *a = pDevice->addresses; a; a = a->next) {
                    switch (a->addr->sa_family) {
                    case AF_INET:
                        printf ("Interface %s has address family name: AF_INET\n", pDevice->name);
                        if (a->addr) {
                            _ipv4Addr.ui32Addr = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;
                            _bIPAddrFound = true;
                            printf ("\t\tIPv4 address: %s\n", NOMADSUtil::InetAddr{_ipv4Addr.ui32Addr}.getIPAsString());
                        }
                        if (a->netmask) {
                            _ipv4Netmask.ui32Addr = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;
                            _bNetmaskFound = true;
                            printf ("\t\tNetmask: %s\n", NOMADSUtil::InetAddr{_ipv4Netmask.ui32Addr}.getIPAsString());
                        }
                        return;

                    case AF_INET6:
                        printf ("Interface %s has address family name: AF_INET6\n", pDevice->name);
                        break;

                    default:
                        printf ("Interface %s has address family name unknown\n", pDevice->name);
                    }
                }
            }
        }
    }

    pcap_t * const PCapInterface::createAndActivateReadHandler (const std::string & sInterfaceName)
    {
        char szErrorBuf[PCAP_ERRBUF_SIZE];
        auto * const pCapHandler = pcap_create (sInterfaceName.c_str(), szErrorBuf);
        if (!pCapHandler) {
            checkAndLogMsg ("PCapInterface::createAndActivateReadHandler", NOMADSUtil::Logger::L_MildError,
                            "pcap_create() failed for device %s with error %s\n",
                            sInterfaceName.c_str(), szErrorBuf);
            return nullptr;
        }

        pcap_set_snaplen (pCapHandler, 65535);
        pcap_set_promisc (pCapHandler, 1);
        pcap_set_timeout (pCapHandler, 1);
    #if defined (LINUX)
        pcap_set_immediate_mode (pCapHandler, 1);
    #endif
        pcap_activate (pCapHandler);

        return pCapHandler;
    }

    pcap_t * const PCapInterface::createAndActivateWriteHandler (const std::string & sInterfaceName)
    {
        char szErrorBuf[PCAP_ERRBUF_SIZE];
        auto * const pCapHandler = pcap_create (sInterfaceName.c_str(), szErrorBuf);
        if (!pCapHandler) {
            checkAndLogMsg ("PCapInterface::createAndActivateWriteHandler", NOMADSUtil::Logger::L_MildError,
                            "pcap_create() failed for device %s with error %s\n",
                            sInterfaceName.c_str(), szErrorBuf);
            return nullptr;
        }

        pcap_set_snaplen (pCapHandler, 100);
        pcap_set_promisc (pCapHandler, 1);
        pcap_set_timeout (pCapHandler, 1000);
        pcap_activate (pCapHandler);

        return pCapHandler;
    }

}
