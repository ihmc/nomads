/*
 * PCapInterface.cpp
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
#if defined (LINUX)
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <net/if.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include "StringTokenizer.h"
#include "Logger.h"

#include "PCapInterface.h"
#include "ConfigurationParameters.h"

#if defined (WIN32)
    #include <winsock2.h>
    #include <iphlpapi.h>

    #define snprintf _snprintf
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    PCapInterface::PCapInterface (const String &sAdapterName) :
        NetworkInterface (Type::T_PCap)
    {
        _sAdapterName = sAdapterName;
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

    PCapInterface * const PCapInterface::getPCapInterface (const char * const pszDevice)
    {
        int rc;
        pcap_if_t *pAllDevs;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs (&pAllDevs, errbuf) == -1) {
            return nullptr;
        }

        String sDeviceName (NetworkInterface::getDeviceNameFromUserFriendlyName (pszDevice));
        if (sDeviceName.length() <= 0) {
            checkAndLogMsg ("PCapInterface::getPCapInterface", Logger::L_MildError,
                            "impossible to retrieve the network device name from the user friendly name %s \n", pszDevice);
            return nullptr;
        }
        PCapInterface *pPCapInterface = new PCapInterface (sDeviceName);
        if (0 != (rc = pPCapInterface->init())) {
            checkAndLogMsg ("PCapInterface::getPCapInterface", Logger::L_MildError,
                            "failed to initialize PCapInterface; rc = %d\n", rc);
            delete pPCapInterface;
            return nullptr;
        }

        return pPCapInterface;
    }

    int PCapInterface::init (void)
    {
        if (nullptr == (_pPCapReadHandler =
                        createAndActivateReadHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sAdapterName))) {
            return -1;
        }
        if (nullptr == (_pPCapWriteHandler =
                        createAndActivateWriteHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sAdapterName))) {
            return -2;
        }

        const uint8 * const pszMACAddr = NetworkInterface::getMACAddrForDevice (_sAdapterName);
        if (pszMACAddr) {
            memcpy (static_cast<void*> (_aui8MACAddr), pszMACAddr, 6);
            _bMACAddrFound = true;
            delete[] pszMACAddr;
        }

        retrieveAndSetIPv4Addr();

        #if defined (WIN32)
        _ui16MTU = retrieveMTUForInterface (_sAdapterName);
        #elif defined (LINUX)
        _ui16MTU = retrieveMTUForInterface (_sAdapterName, pcap_get_selectable_fd (_pPCapReadHandler));
        #endif
        if (_ui16MTU > 0) {
            _bMTUFound = true;
        }

        IPv4Addr ipv4DefGW = NetworkInterface::getDefaultGatewayForInterface (_sAdapterName);
        if (ipv4DefGW.ui32Addr) {
            _bDefaultGatewayFound = true;
            _ipv4DefaultGateway = ipv4DefGW;
        }

        return 0;
    }

    bool PCapInterface::checkMACAddress (void)
    {
        if ((!_bMACAddrFound) || (!_bIPAddrFound)) {
            return false;
        }

        uint8 ui8IPAddrOctet3, ui8IPAddrOctet4;
        const char *nextToken = nullptr;
        InetAddr virtualIPAddr (NetProxyApplicationParameters::EXTERNAL_IP_ADDR);
        StringTokenizer st (virtualIPAddr.getIPAsString(), '.');
        st.getNextToken();
        st.getNextToken();

        nextToken = st.getNextToken();
        if (!nextToken) {
            return false;
        }
        ui8IPAddrOctet3 = (uint8) atoi (nextToken);
        nextToken = st.getNextToken();
        if (!nextToken) {
            return false;
        }
        ui8IPAddrOctet4 = (uint8) atoi (nextToken);

        // Checking matching between 3rd and 4th bytes of the IP with 5th and 6th bytes of the MAC
        if ((_aui8MACAddr[4] == ui8IPAddrOctet3) && (_aui8MACAddr[5] == ui8IPAddrOctet4)) {
            return true;
        }
        return false;
    }

    int PCapInterface::readPacket (const uint8 ** pui8Buf, uint16 & ui16PacketLen)
    {
        struct pcap_pkthdr *pPacketHeader;

		while (!_bIsTerminationRequested) {
			int rc = 0;
			if (_pPCapReadHandler == nullptr) {
                MutexUnlocker mu (&_mWrite);
				checkAndLogMsg ("PCapInterface::readPacket", Logger::L_Warning,
					            "the pcap read handler is null: restarting it\n");
				sleepForMilliseconds (500);
				if (nullptr == (_pPCapReadHandler =
                                createAndActivateReadHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sAdapterName))) {
					checkAndLogMsg ("PCapInterface::ReadPacket", Logger::L_SevereError,
						            "failed to restart the pcap read handler; NetProxy will retry in the next cycle\n");
				}
				else {
					checkAndLogMsg ("PCapInterface::ReadPacket", Logger::L_LowDetailDebug,
						            "successfully restarted the pcap read handler\n");
				}
			}
			else {
				rc = pcap_next_ex (_pPCapReadHandler, &pPacketHeader, pui8Buf);
				if (rc > 0) {
                    if (pPacketHeader->caplen < pPacketHeader->len) {
                        checkAndLogMsg ("PCapInterface::readPacket", Logger::L_Warning,
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
					checkAndLogMsg ("PCapInterface::readPacket", Logger::L_MildError,
						            "pcap_next_ex() returned %d; closing the current handler, "
                                    "it will be restarted in the next cycle\n", rc);
                    *pui8Buf = nullptr;
                    ui16PacketLen = 0;

                    MutexUnlocker mu (&_mWrite);
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

        MutexUnlocker mu (&_mWrite);
        if (_pPCapWriteHandler == nullptr) {
            checkAndLogMsg ("PCapInterface::writePacket", Logger::L_Warning,
                            "the pcap write handler is null: restarting it");
            sleepForMilliseconds (500);
            if (nullptr == (_pPCapWriteHandler =
                            createAndActivateWriteHandler (NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sAdapterName))) {
                checkAndLogMsg ("PCapInterface::writePacket", Logger::L_SevereError,
                                "failed to restart the pcap write handler\n");
                return 0;
            }
            else {
                checkAndLogMsg ("PCapInterface::ReadPacket", Logger::L_LowDetailDebug,
                                "successfully restarted the pcap write handler\n");
            }
        }

		for (int counter = 0; counter < NetworkConfigurationSettings::PCAP_SEND_PACKET_ATTEMPTS; counter++) {
			if (0 != (rc = pcap_sendpacket (_pPCapWriteHandler, pui8Buf, ui16PacketLen))) {
				checkAndLogMsg ("PCapInterface::writePacket", Logger::L_MildError,
                                "pcap_sendpacket() failed writing attempt #%d with a %hu bytes long packet: "
                                "%s;%s\n", counter + 1, ui16PacketLen, pcap_geterr (_pPCapWriteHandler),
                                (counter < (NetworkConfigurationSettings::PCAP_SEND_PACKET_ATTEMPTS - 1)) ?
                                " retrying..." : " send has failed");
				sleepForMilliseconds (NetworkConfigurationSettings::PCAP_SEND_ATTEMPT_INTERVAL_IN_MS);
			}
            else {
                return ui16PacketLen;
            }
		}

        return rc;
    }

    void PCapInterface::retrieveAndSetIPv4Addr (void)
    {
        pcap_if_t *pAllDevs;
        pcap_if_t *pDevice;
        char errbuf[PCAP_ERRBUF_SIZE];
        if (pcap_findalldevs (&pAllDevs, errbuf) == -1) {
            _bIPAddrFound = false;
            _ipv4Addr.ui32Addr = 0;
            return;
        }

        String adapterName = NetProxyApplicationParameters::pszNetworkAdapterNamePrefix + _sAdapterName;
        for (pDevice = pAllDevs; pDevice != nullptr; pDevice = pDevice->next) {
            if (adapterName ^= pDevice->name) {
                for (const pcap_addr_t *a = pDevice->addresses; a; a = a->next) {
                    switch (a->addr->sa_family) {
                    case AF_INET:
                        printf ("Interface %s has address family name: AF_INET\n", pDevice->name);
                        if (a->addr) {
                            _ipv4Addr.ui32Addr = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;
                            _bIPAddrFound = true;
                            printf ("\t\tIPv4 address: %s\n", InetAddr (_ipv4Addr.ui32Addr).getIPAsString());
                        }
                        if (a->netmask) {
                            _ipv4Netmask.ui32Addr = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;
                            _bNetmaskFound = true;
                            printf ("\t\tNetmask: %s\n", InetAddr (_ipv4Netmask.ui32Addr).getIPAsString());
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

    pcap_t * const PCapInterface::createAndActivateReadHandler (const NOMADSUtil::String &sAdapterName)
    {
        char szErrorBuf[PCAP_ERRBUF_SIZE];
        auto * const pCapHandler = pcap_create (sAdapterName, szErrorBuf);
        if (!pCapHandler) {
            checkAndLogMsg ("PCapInterface::createAndActivateReadHandler", Logger::L_MildError,
                            "pcap_create() failed for device %s with error %s\n",
                            sAdapterName.c_str(), szErrorBuf);
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

    pcap_t * const PCapInterface::createAndActivateWriteHandler (const NOMADSUtil::String &sAdapterName)
    {
        char szErrorBuf[PCAP_ERRBUF_SIZE];
        auto * const pCapHandler = pcap_create (sAdapterName, szErrorBuf);
        if (!pCapHandler) {
            checkAndLogMsg ("PCapInterface::createAndActivateWriteHandler", Logger::L_MildError,
                            "pcap_create() failed for device %s with error %s\n",
                            sAdapterName.c_str(), szErrorBuf);
            return nullptr;
        }
        pcap_set_snaplen (pCapHandler, 100);
        pcap_set_promisc (pCapHandler, 1);
        pcap_set_timeout (pCapHandler, 1000);
        pcap_activate (pCapHandler);

        return pCapHandler;
    }

}
