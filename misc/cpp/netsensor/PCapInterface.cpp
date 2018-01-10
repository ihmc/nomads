#include <iostream>
#include <sstream>
#if defined (LINUX)
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#elif defined (WIN32)
    #define snprintf _snprintf
#endif

#include "Logger.h"
#include "StringTokenizer.h"
#include "FileUtils.h"
#include "NLFLib.h"

#include "PCapInterface.h"
#include "NetSensorUtilities.h"

using namespace NOMADSUtil;
#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_NETSENSOR
{
    PCapInterface * const PCapInterface::getPCapInterface(const char * const pszDevice,
                                                          int msValidity)
    {
        int rc;
        pcap_if_t *pAllDevs;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs(&pAllDevs, errbuf) == -1) {
            checkAndLogMsg("PCapInterface::getPCapInterface",
                           Logger::L_MildError, "failed to enumerate interfaces\n");
            return nullptr;
        }

        String sDeviceName(
            NetworkInterface::getDeviceNameFromUserFriendlyName(pszDevice));
        if (sDeviceName.length() <= 0) {
            checkAndLogMsg("PCapInterface::getPCapInterface",
                           Logger::L_MildError,
                           "Device query for device name failed%s \n", pszDevice);
            return nullptr;
        }

        PCapInterface *pPCapInterface = new PCapInterface(sDeviceName);
        if (0 != (rc = pPCapInterface->initLive(msValidity)))
        {
            checkAndLogMsg("PCapInterface::getPCapInterface",
                           Logger::L_MildError,
                           "failed to initialize PCapInterface; rc = %d\n", rc);
            delete pPCapInterface;
            return nullptr;
        }

        return pPCapInterface;
    }

    PCapInterface * const PCapInterface::getPCapInterface(const NOMADSUtil::String & sAdapterName,
                                                          const NOMADSUtil::String & sPcapFile,
                                                          uint32 ui32IPAddr, uint32 ui32Netmask,
                                                          uint32 ui32GwIPAddr,
                                                          NOMADSUtil::EtherMACAddr emacInterfaceMAC)
    {
        int rc;
        if (!FileUtils::fileExists(sPcapFile)) {
            checkAndLogMsg("PCapInterface::getPCapInterface", Logger::L_MildError,
                           "Pcap file <%s> for REPLAY MODE not found\n",
                           sPcapFile.c_str());
            return nullptr;
        }

        PCapInterface *pPCapInterface = new PCapInterface(sAdapterName, sPcapFile, ui32IPAddr,
                                                          ui32Netmask, ui32GwIPAddr, emacInterfaceMAC);
        if (0 != (rc = pPCapInterface->initReplay()))
        {
            checkAndLogMsg("PCapInterface::getPCapInterface", Logger::L_MildError,
                           "failed to initialize PCapInterface in REPLAY MODE; rc = %d\n", rc);
            delete pPCapInterface;
            return nullptr;
        }

        return pPCapInterface;
    }

    int PCapInterface::initLive(int msPacketValid)
    {
        char szErrorBuf[PCAP_ERRBUF_SIZE];
        if (nullptr == (_pPCapHandle = pcap_create(_pszNetworkAdapterNamePrefix +
			_sAdapterName, szErrorBuf))) {
            checkAndLogMsg("PCapInterface::init", Logger::L_MildError,
				"pcap_open_live() failed for device %s with error %s\n",
				_sAdapterName.c_str(), szErrorBuf);
            return -1;
        }
        pcap_set_snaplen(_pPCapHandle, 65535);
        pcap_set_promisc(_pPCapHandle, 1);
        pcap_set_timeout(_pPCapHandle, 1);

#if defined (LINUX)
        pcap_set_immediate_mode(_pPCapHandle, 1);
#endif
        pcap_activate(_pPCapHandle);

        const uint8 * const pszMACAddr = NetworkInterface::getMACAddrForDevice(
			_sAdapterName);

        if (pszMACAddr) {
            memcpy(static_cast<void*> (NetworkInterface::_aui8MACAddr),
                   pszMACAddr, 6);
            NetworkInterface::_bMACAddrFound = true;
            delete[] pszMACAddr;
        }
        else {
            checkAndLogMsg("PCapInterface::init", Logger::L_Warning,
				"Device query for MAC address failed\n");
        }

        retrieveAndSetIPv4Addr();
        IPv4Addr ipv4DefGW =
			NetworkInterface::getDefaultGatewayForInterface(_sAdapterName);
        if (ipv4DefGW.ui32Addr) {
            _bDefaultGatewayFound = true;
            _ipv4DefaultGateway = ipv4DefGW;
        }
        return 0;
    }

    int PCapInterface::initReplay(void)
    {
        static char szErrorBuf[PCAP_ERRBUF_SIZE];
        if (nullptr == (_pPCapHandle = pcap_open_offline(_sPcapFile, szErrorBuf))) {
            checkAndLogMsg("PCapInterface::init", Logger::L_MildError,
                           "pcap_open_offline() failed for device %s and file %s with error %s\n",
                           _sAdapterName.c_str(), _sPcapFile.c_str(), szErrorBuf);
            return -1;
        }

        return 0;
    }

    void PCapInterface::retrieveAndSetIPv4Addr(void)
    {
        if (_m == Mode::M_OFFLINE) {
            // Interface configuration is statically set up in OFFLINE mode
            return;
        }

        pcap_if_t *pAllDevs;
        pcap_if_t *pDevice;
        char errbuf[PCAP_ERRBUF_SIZE];
        if (pcap_findalldevs(&pAllDevs, errbuf) == -1) {
            _bIPAddrFound = false;
            _ipv4Addr.ui32Addr = 0;
            return;
        }

        String adapterName = _pszNetworkAdapterNamePrefix + _sAdapterName;
        for (pDevice = pAllDevs; pDevice != nullptr; pDevice = pDevice->next) {
            if (adapterName ^= pDevice->name) {
                for (const pcap_addr_t *a = pDevice->addresses; a; a = a->next) {
                    switch (a->addr->sa_family) {
                    case AF_INET:
                        // printf ("Interface %s has address family name:
						// AF_INET\n", pDevice->name);
                        if (a->addr) {
                            _ipv4Addr.ui32Addr =
								((struct sockaddr_in *)a->addr)->sin_addr.
								s_addr;
                            _bIPAddrFound = true;
                            //printf ("\t\tIPv4 address: %s\n",
							// InetAddr (_ipv4Addr.ui32Addr).getIPAsString());
                        }
                        if (a->netmask) {
                            _ipv4Netmask.ui32Addr =
								((struct sockaddr_in *)a->netmask)->sin_addr.
								s_addr;
                            _bNetmaskFound = true;
                            //printf ("\t\tNetmask: %s\n",
							// InetAddr (_ipv4Netmask.ui32Addr).
							// getIPAsString());
                        }
                        return;

                    case AF_INET6:
                        //printf (
						// "Interface %s has address family name: AF_INET6\n",
						// pDevice->name);
                        break;

                    default:
                        printf("Interface %s has address family name unknown\n",
							pDevice->name);
                    }
                }
            }
        }
    }

    int PCapInterface::readPacket(uint8 *pui8Buf, uint16 ui16BufSize,
		int64 *tus)
    {
        struct pcap_pkthdr *pPacketHeader;
        const u_char *pPacketData;
        while (true) {
            int rc = pcap_next_ex(_pPCapHandle, &pPacketHeader, &pPacketData);
            if (rc > 0) {
                *tus = pPacketHeader->ts.tv_usec +
                    (static_cast<int64> (pPacketHeader->ts.tv_sec) * 1000000);
                if (_m == Mode::M_OFFLINE) {
                    static const int64 i64FirstPacketReplayTime = getTimeInNanos();
                    static const int64 i64FirstPacketTime = *tus;

                    int64 i64ReplayTimeDiff = getTimeInNanos() - i64FirstPacketReplayTime;
                    int64 i64PacketTimeDiff = *tus - i64FirstPacketTime;
                    while (i64ReplayTimeDiff < i64PacketTimeDiff) {
                        // Spin to synch with pcap file
                        i64ReplayTimeDiff = getTimeInNanos() - i64FirstPacketReplayTime;
                    }
                }
                uint32 ui32PacketSize = ui16BufSize < pPacketHeader->caplen ?
					ui16BufSize : pPacketHeader->caplen;
                memcpy(pui8Buf, pPacketData, ui32PacketSize);
                return ui32PacketSize;
            }
            else if (rc == 0) {
                if (terminationRequested()) {
                    return 0;
                }
                // Timeout expired --> nothing to do
            }
            else if (rc < 0)
            {
                if ((_m == Mode::M_OFFLINE) && (rc == -2)) {
                    checkAndLogMsg("PCapInterface::readPacket", Logger::L_Info,
                                   "reached the end of the pcap file while "
                                   "running in REPLAY MODE\n");
                    requestTermination();
                    return 0;
                }
                checkAndLogMsg("PCapInterface::readPacket", Logger::L_MildError,
                               "pcap_next_ex() returned %d: %s\n", rc,
                               pcap_geterr(_pPCapHandle));
                return -1;
            }
        }
    }
}