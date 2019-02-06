/*
 * TapInterface.cpp
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
#include <string>
#if defined (LINUX)
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include "win32/RegUtils.h"
#include "InetAddr.h"
#include "Logger.h"

#include "TapInterface.h"

#if defined (WIN32)
    #define snprintf _snprintf
#endif


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    TapInterface::TapInterface (void) :
        NetworkInterface{Type::T_Tap, "TUN/TAP Interface", "TUN/TAP Interface"},
        _bInitDone{false}
    {
        #if defined (WIN32)
            _hInterface = nullptr;
            _oRead.hEvent = nullptr;
            _oWrite.hEvent = nullptr;
        #elif defined (LINUX)
            _sInterfaceName = "tap0";
            tvTimeout.tv_sec = 0;
            tvTimeout.tv_usec = 0;
        #endif
    }

    TapInterface::~TapInterface (void)
    {
        requestTermination();

        #if defined (WIN32)
        CloseHandle (_hInterface);
        _hInterface = nullptr;
        CloseHandle (_oRead.hEvent);
        _oRead.hEvent = nullptr;
        CloseHandle (_oWrite.hEvent);
        _oWrite.hEvent = nullptr;
        #elif defined (LINUX)
        close (_fdTAP);
        _fdTAP = -1;
        #endif
    }

    int TapInterface::init (void)
    {
        std::lock_guard<std::mutex> lg (_mtxWrite);

        if (_bInitDone) {
            return 0;
        }
        _bInitDone = true;

        #if defined (WIN32)
            const char * WIN32_NETWORK_ADAPTERS_REG_KEY = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
            const char * WIN32_NETWORK_ADAPTERS_NAMES_REG_KEY = "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
            const char * WIN32_TCPIP_INTERFACE_CONFIGURATION_REG_KEY = "SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters\\Interfaces";
            const char * TAP_INTERFACE_DESCRIPTION1 = "TAP-Win32";
            const char * TAP_INTERFACE_DESCRIPTION2 = "TAP-Windows Adapter V9";
            const char * USER_MODE_DEVICE_DIR_PREFIX = "\\\\.\\Global\\";       // From OpenVPN - tap-win32/common.h
            const char * SYS_DEVICE_DIR = "\\Device\\";                         // From OpenVPN - tap-win32/common.h
            const char * USER_DEVICE_DIR = "\\DosDevices\\Global\\";            // From OpenVPN - tap-win32/common.h
            const char * TAP_DEVICE_SUFFIX = ".tap";                            // From OpenVPN - tap-win32/common.h

            int iCount = 0;
            std::string sInstanceID, sMACAddr, sIPAddr, sSubnetMask, sDefaultGateway, sMTU;
            NOMADSUtil::RegKeys *pRK = NOMADSUtil::getRegistrySubKeys (WIN32_NETWORK_ADAPTERS_REG_KEY);
            for (int i = 0; i <= pRK->getHighestIndex(); i++) {
                const char *pszSubKey = (*pRK)[i];
                char szKey [MAX_PATH];
                szKey[MAX_PATH-1] = '\0';
                snprintf (szKey, MAX_PATH, "%s\\%s", WIN32_NETWORK_ADAPTERS_REG_KEY, pszSubKey);
                NOMADSUtil::RegEntries *pREs = NOMADSUtil::getRegistryEntries (szKey);
                if (pREs != nullptr) {
                    NOMADSUtil::RegEntry *pEntry = pREs->get ("DriverDesc");
                    if ((pEntry != nullptr) && (pEntry->type == NOMADSUtil::RegEntry::RET_String)) {
                        checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                        "found network adaptor <%s>\n", pEntry->value);
                        if ((strstr (pEntry->value, TAP_INTERFACE_DESCRIPTION1)) || (strstr (pEntry->value, TAP_INTERFACE_DESCRIPTION2))) {
                            auto * const pREInstanceId = pREs->get ("NetCfgInstanceId");
                            if ((pREInstanceId != nullptr) && (pREInstanceId->type == NOMADSUtil::RegEntry::RET_String)) {
                                iCount++;
                                sInstanceID = pREInstanceId->value;
                                auto * const pREMACAddr = pREs->get ("MAC");
                                if ((pREMACAddr != nullptr) && (pREMACAddr->type == NOMADSUtil::RegEntry::RET_String)) {
                                    sMACAddr = pREMACAddr->value;
                                }
                                auto * const pREMTU = pREs->get ("MTU");
                                if ((pREMTU != nullptr) && (pREMTU->type == NOMADSUtil::RegEntry::RET_String)) {
                                    sMTU = pREMTU->value;
                                }

                                snprintf (szKey, MAX_PATH, "%s\\%s\\Connection", WIN32_NETWORK_ADAPTERS_NAMES_REG_KEY, sInstanceID.c_str());
                                auto * const pNetworkConnEntries = NOMADSUtil::getRegistryEntries (szKey);
                                if (pNetworkConnEntries != nullptr) {
                                    auto * const pREName = pNetworkConnEntries->get ("Name");
                                    if (pREName != nullptr) {
                                        _sUserFriendlyInterfaceName = _sInterfaceName = pREName->value;
                                        checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                                        "TUN/TAP Network Adaptor name is %s\n", _sInterfaceName.c_str());
                                    }
                                    else {
                                        checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                        "unable to find the TUN/TAP interface name\n");
                                    }
                                }

                                snprintf (szKey, MAX_PATH, "%s\\%s", WIN32_TCPIP_INTERFACE_CONFIGURATION_REG_KEY, sInstanceID.c_str());
                                auto * const pTCPIPEntries = NOMADSUtil::getRegistryEntries (szKey);
                                if (pTCPIPEntries != nullptr) {
                                    // IP address
                                    auto * const pREIPAddr = pTCPIPEntries->get ("IPAddress");
                                    if ((pREIPAddr != nullptr) && (pREIPAddr->type == NOMADSUtil::RegEntry::RET_MultiString)) {
                                        if (pREIPAddr->values.getHighestIndex() < 0) {
                                            checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                            "TUN/TAP Network Adaptor does not have an IP Address\n");
                                        }
                                        else {
                                            sIPAddr = pREIPAddr->values[0];
                                            if (pREIPAddr->values.getHighestIndex() > 0) {
                                                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                                "TUN/TAP Network Adaptor has more than one IP address - using <%s>\n",
                                                                sIPAddr.c_str());
                                            }
                                        }
                                    }

                                    // Netmask
                                    auto * const pRENetmask = pTCPIPEntries->get ("SubnetMask");
                                    if ((pRENetmask != nullptr) && (pRENetmask->type == NOMADSUtil::RegEntry::RET_MultiString)) {
                                        if (pRENetmask->values.getHighestIndex() < 0) {
                                            checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                            "TUN/TAP Network Adaptor does not have a specified SubnetMask\n");
                                        }
                                        else {
                                            sSubnetMask = pRENetmask->values[0];
                                            if (pRENetmask->values.getHighestIndex() > 0) {
                                                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                                "TUN/TAP Network Adaptor has more than one SubnetMask - using <%s>\n",
                                                                sSubnetMask.c_str());
                                            }
                                        }
                                    }

                                    // Netmask
                                    auto * const pREGateway = pTCPIPEntries->get ("DefaultGateway");
                                    if ((pREGateway != nullptr) && (pREGateway->type == NOMADSUtil::RegEntry::RET_MultiString)) {
                                        if (pREGateway->values.getHighestIndex() < 0) {
                                            checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                            "TUN/TAP Network Adaptor does not have a specified Default Gateway\n");
                                        }
                                        else {
                                            sDefaultGateway = pRENetmask->values[0];
                                            if (pRENetmask->values.getHighestIndex() > 0) {
                                                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                                                "TUN/TAP Network Adaptor has more than one Default Gateway - using <%s>\n",
                                                                sDefaultGateway.c_str());
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            delete pRK;

            if (iCount == 0) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "did not find the TAP-Win32 interface\n");
                return -1;
            }
            else if (iCount > 1) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "found %d TAP-Win32 interfaces - do not know which one to use\n", iCount);
                return -2;
            }

            char szDeviceFilePath[MAX_PATH];
            szDeviceFilePath[MAX_PATH-1] = '\0';
            snprintf (szDeviceFilePath, MAX_PATH, "%s%s%s", USER_MODE_DEVICE_DIR_PREFIX, sInstanceID.c_str(), TAP_DEVICE_SUFFIX);

            checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                            "opening device %s\n", szDeviceFilePath);

            _hInterface = CreateFile (szDeviceFilePath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            if (_hInterface == INVALID_HANDLE_VALUE) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "CreateFile failed with error %d while attempting to open <%s>\n",
                                GetLastError(), szDeviceFilePath);
                return -3;
            }

            memset (&_oRead, 0, sizeof(_oRead));
            memset (&_oWrite, 0, sizeof(_oWrite));
            _oRead.hEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);
            if (_oRead.hEvent == INVALID_HANDLE_VALUE) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "CreateEvent 1 failed with error %d\n", GetLastError());
                return -4;
            }
            _oWrite.hEvent = CreateEvent (nullptr, TRUE, FALSE, nullptr);
            if (_oWrite.hEvent == INVALID_HANDLE_VALUE) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "CreateEvent 2 failed with error %d\n", GetLastError());
                return -5;
            }

            // Process MAC address
            if (sMACAddr.length() > 0) {
                // MAC Address was configured - parse it
                _bMACAddrFound = true;
                auto svMACAddressBytes = splitStringToVector (sMACAddr, ':');
                if (svMACAddressBytes.size() != 6) {
                    _bMACAddrFound = false;
                    checkAndLogMsg("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                    "Error parsing the retrieved MAC address <%s>\n", sMACAddr.c_str());
                }

                for (int i = 0; (i < 6) && _bMACAddrFound; i++) {
                    char pszByte[5];
                    strcpy (pszByte, "0x");
                    strcat (pszByte, svMACAddressBytes[i].c_str());
                    strcat (pszByte, "\0");

                    unsigned int tmp;
                    std::stringstream ss;
                    ss << std::hex << pszByte;
                    ss >> tmp;
                    _aui8MACAddr[i] = static_cast<uint8> (tmp);
                }
            }
            if (_bMACAddrFound) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                "TUN/TAP interface MAC address is: %s\n", sMACAddr.c_str());
            }
            else {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "could not retrieve TUN/TAP interface MAC address\n");
                return -6;
            }

            // Process IP address
            if (sIPAddr.length() > 0) {
                _bIPAddrFound = true;
                _ipv4Addr.ui32Addr = NOMADSUtil::InetAddr{sIPAddr.c_str()}.getIPAddress();
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                "TUN/TAP interface IP address is: %s\n", sIPAddr.c_str());
            }
            else {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                "impossible to retrieve the IP address of the TUN/TAP interface\n");
            }

            // Process SubnetMask
            if (sSubnetMask.length() > 0) {
                _bNetmaskFound = true;
                _ipv4Netmask.ui32Addr = NOMADSUtil::InetAddr{sSubnetMask.c_str()}.getIPAddress();
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                "TUN/TAP interface netmask is: %s\n", sSubnetMask.c_str());
            }
            else {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                "impossible to retrieve the Netmask of the TUN/TAP interface\n");
            }

            // Process Default Gateway
            if (sDefaultGateway.length() > 0) {
                _bDefaultGatewayFound = true;
                _ipv4DefaultGateway.ui32Addr = NOMADSUtil::InetAddr{sDefaultGateway.c_str()}.getIPAddress();
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                "TUN/TAP interface Default Gateway is: %s\n",
                                sDefaultGateway.c_str());
            }
            else {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                "impossible to retrieve the Default Gateway of the TUN/TAP interface\n");
            }

            // Process MTU value
            if (sMTU.length() > 0) {
                // MTU value found
                int32 i32MTU = std::atoi (sMTU.c_str());
                if ((i32MTU <= 0) || (i32MTU > NetProxyApplicationParameters::ETHERNET_MAX_MTU)) {
                    checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_MildError,
                                    "detected an invalid value of %d bytes as MTU of the TUN/TAP interface\n", i32MTU);
                }
                else {
                    _bMTUFound = true;
                    _ui16MTU = i32MTU;
                    checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                                    "value of the MTU of the TUN/TAP interface has been detected to be %hu bytes\n", _ui16MTU);
                }
            }
            else {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                "impossible to retrieve the MTU of the TUN/TAP interface\n");
            }

        #elif defined (LINUX)
            #if defined (ANDROID)
                const char * const TAP_DEVICE_PATH = "/dev/tun";
            #else
                const char * const TAP_DEVICE_PATH = "/dev/net/tun";
            #endif
            struct ifreq ifr;
            int err = 0, sock_fd;

            if ((_fdTAP = open (TAP_DEVICE_PATH, O_RDWR)) < 0 ) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "Attempt to open TUN/TAP device failed with error %d\n", _fdTAP);
                return -1;
            }
            //FD_SET (_fdTAP, &fdSet);
            memset (&ifr, 0, sizeof(ifr));
            strncpy (ifr.ifr_name, _sInterfaceName.c_str(), _sInterfaceName.length());
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            if ((err = ioctl (_fdTAP, TUNSETIFF, (void *) &ifr)) < 0 ) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "Attempt to apply settings to TUN/TAP device failed with error %d\n", err);
                close (_fdTAP);
                return -2;
            }

            _sInterfaceName = ifr.ifr_name;
            printf ("TUN/TAP interface name: %s\n", _sInterfaceName.c_str());

            if ((err = ioctl (_fdTAP, TUNSETPERSIST, 1)) < 0) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_MildError,
                                "Attempt to turn persitence on on TUN/TAP device failed with error %d\n", err);
                return -3;
            }
            else {
                printf ("\t\tPersistence: ON\n");
            }

            // MAC Address
            memset (&ifr, 0, sizeof(ifr));
            strncpy (ifr.ifr_name, _sInterfaceName.c_str(), _sInterfaceName.length());
            if ((err = ioctl (_fdTAP, SIOCGIFHWADDR, (void *) &ifr)) < 0) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_SevereError,
                                "could not retrieve TUN/TAP interface MAC address; rc = %d\n", errno);
                perror("ioctl");
                return -4;
            }
            else {
                _bMACAddrFound = true;
                memcpy (_aui8MACAddr, &ifr.ifr_hwaddr.sa_data, 6);
                printf ("\t\tMAC address: %2x:%2x:%2x:%2x:%2x:%2x\n", _aui8MACAddr[0], _aui8MACAddr[1],
                        _aui8MACAddr[2], _aui8MACAddr[3], _aui8MACAddr[4], _aui8MACAddr[5]);
            }

            // Try and open a UDP/IP socket on the TUN/TAP interface to retrieve IP address, netmask, and MTU
            if ((sock_fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
                checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_MildError,
                                "failed to open a temporary UDP/IPv4 socket on the TUN/TAP interface "
                                "to retrieve useful network configuration information; rc = %d\n", errno);
            }
            else {
                // IP Address
                memset (&ifr, 0, sizeof(ifr));
                strncpy (ifr.ifr_name, _sInterfaceName.c_str(), sizeof(ifr.ifr_name) - 1);
                ifr.ifr_addr.sa_family = AF_INET;
                if ((err = ioctl (sock_fd, SIOCGIFADDR, (void *) &ifr)) < 0) {
                    checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                    "impossible to retrieve the IP address of the TUN/TAP interface; rc = %d\n", errno);
                }
                else {
                    _bIPAddrFound = true;
                    _ipv4Addr.ui32Addr = NOMADSUtil::InetAddr{((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr}.getIPAddress();
                    printf ("\t\tIP address: %s\n", NOMADSUtil::InetAddr{_ipv4Addr.ui32Addr}.getIPAsString());
                }

                // Netmask
                memset (&ifr, 0, sizeof(ifr));
                strncpy (ifr.ifr_name, _sInterfaceName.c_str(), sizeof(ifr.ifr_name) - 1);
                ifr.ifr_addr.sa_family = AF_INET;
                if ((err = ioctl (sock_fd, SIOCGIFNETMASK, (void *) &ifr)) < 0) {
                    checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Warning,
                                    "impossible to retrieve the Netmask of the TUN/TAP interface; rc = %d\n", errno);
                }
                else {
                    _bNetmaskFound = true;
                    _ipv4Netmask.ui32Addr = NOMADSUtil::InetAddr{((struct sockaddr_in *) &ifr.ifr_netmask)->sin_addr.s_addr}.getIPAddress();
                    printf ("\t\tNetmask: %s\n", NOMADSUtil::InetAddr{_ipv4Netmask.ui32Addr}.getIPAsString());
                }

                // Netmask
                memset (&ifr, 0, sizeof(ifr));
                strncpy (ifr.ifr_name, _sInterfaceName.c_str(), sizeof(ifr.ifr_name) - 1);
                ifr.ifr_addr.sa_family = AF_INET;
                if ((err = ioctl (sock_fd, SIOCGIFMTU, (void *) &ifr)) < 0 ) {
                    checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_MildError,
                                    "Attempt to get the value of the MTU of the TUN/TAP interface failed; rc = %d\n", errno);
                }
                else {
                    _bMTUFound = true;
                    _ui16MTU = (uint16) ifr.ifr_mtu;
                    printf ("\t\tMTU: %hu bytes\n", _ui16MTU);
                }
                close (sock_fd);
            }

            tvTimeout.tv_sec = NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT / 1000;
            tvTimeout.tv_usec = (NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT % 1000) * 1000;
            checkAndLogMsg ("TapInterface::init", NOMADSUtil::Logger::L_Info,
                            "select() on the TUN/TAP interface will use a timeout of %ld.%04ld sec\n",
                            tvTimeout.tv_sec, tvTimeout.tv_usec);
        #endif

        return 0;
    }

    int TapInterface::readPacket (const uint8 ** pui8Buf, uint16 & ui16PacketLen)
    {
        static const uint16 UI16BUFFER_SIZE = NetProxyApplicationParameters::ETHERNET_MAX_MFS;

        #if defined (WIN32)
        DWORD dwBytesRead = 0, dwWaitForSingleObjectRet = 0;
        if (!ReadFile (_hInterface, ui8Buf, UI16BUFFER_SIZE, &dwBytesRead, &_oRead)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                checkAndLogMsg ("TapInterface::readPacket", NOMADSUtil::Logger::L_MildError,
                                "ReadFile failed with error %d\n", GetLastError());
                *pui8Buf = nullptr;
                ui16PacketLen = 0;
                return -1;
            }
            else {
                while (WAIT_TIMEOUT == (dwWaitForSingleObjectRet = WaitForSingleObject (
                                        _oRead.hEvent, NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT))) {
                    if (_bIsTerminationRequested) {
                        CancelIo (_hInterface);
                        *pui8Buf = nullptr;
                        ui16PacketLen = 0;
                        return 0;
                    }
                }

                // If the handle has been signaled, retrieve result
                if ((dwWaitForSingleObjectRet == WAIT_OBJECT_0) && !GetOverlappedResult (_hInterface, &_oRead, &dwBytesRead, TRUE)) {
                    checkAndLogMsg ("TapInterface::readPacket", NOMADSUtil::Logger::L_MildError,
                                    "GetOverlappedResult failed with error %d\n", GetLastError());
                    *pui8Buf = nullptr;
                    ui16PacketLen = 0;
                    return -2;
                }
                else if (dwWaitForSingleObjectRet != WAIT_OBJECT_0) {
                    checkAndLogMsg ("TapInterface::readPacket", NOMADSUtil::Logger::L_MildError,
                                    "WaitForSingleObject failed returning %d, last error is %d\n",
                                    dwWaitForSingleObjectRet, GetLastError());
                    *pui8Buf = nullptr;
                    ui16PacketLen = 0;
                    return -3;
                }
            }
        }

        #elif defined (LINUX)
        int rc;
        FD_SET (_fdTAP, &fdSet);
        tvTimeout.tv_sec = NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT / 1000;
        tvTimeout.tv_usec = (NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT % 1000) * 1000;
        while (0 == (rc = select (_fdTAP + 1, &fdSet, nullptr, nullptr, &tvTimeout))) {
            if (_bIsTerminationRequested) {
                *pui8Buf = nullptr;
                ui16PacketLen = 0;
                return 0;
            }
            FD_SET (_fdTAP, &fdSet);
            tvTimeout.tv_sec = NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT / 1000;
            tvTimeout.tv_usec = (NetProxyApplicationParameters::TAP_INTERFACE_READ_TIMEOUT % 1000) * 1000;
        }
        if (rc < 0) {
            *pui8Buf = nullptr;
            ui16PacketLen = 0;
            return rc;
        }

        int64 dwBytesRead = read (_fdTAP, ui8Buf, UI16BUFFER_SIZE);
        if ((dwBytesRead < 0) && !_bIsTerminationRequested) {
            checkAndLogMsg ("TapInterface::readPacket", NOMADSUtil::Logger::L_MildError,
                            "read() failed with error %d\n", errno);
            *pui8Buf = nullptr;
            ui16PacketLen = 0;
            return -1;
        }
        #endif

        *pui8Buf = ui8Buf;
        ui16PacketLen = dwBytesRead;
        return 0;
    }

    int TapInterface::writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen)
    {
        std::lock_guard<std::mutex> lg (_mtxWrite);

        #if defined (WIN32)
        DWORD dwBytesWritten = 0;
            if (!WriteFile (_hInterface, pui8Buf, ui16PacketLen, &dwBytesWritten, &_oWrite)) {
                if (GetLastError() !=  ERROR_IO_PENDING) {
                    checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_MildError,
                                    "WriteFile failed with error %d\n", GetLastError());
                    return -1;
                }
                else {
                    if (!GetOverlappedResult (_hInterface, &_oWrite, &dwBytesWritten, TRUE)) {
                        checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_MildError,
                                        "GetOverlappedResult failed with error %d\n", GetLastError());
                        return -2;
                    }
                }
            }
            checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_HighDetailDebug,
                            "correctly written %d bytes to the TUN/TAP interface\n", dwBytesWritten);

        #elif defined (LINUX)
        int64 dwBytesWritten = write (_fdTAP, pui8Buf, ui16PacketLen);
            if (dwBytesWritten < 0) {
                checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_MildError,
                                "write() failed with error %d\n", errno);
                return -1;
            }
            else if (dwBytesWritten < ui16PacketLen) {
                checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_MildError,
                                "write() wrote only %d bytes to the TUN/TAP interface\n", dwBytesWritten);
                return -2;
            }
            else {
                checkAndLogMsg ("TapInterface::writePacket", NOMADSUtil::Logger::L_HighDetailDebug,
                                "correctly written %d bytes to the TUN/TAP interface\n", dwBytesWritten);
            }
        #endif

        return (int) dwBytesWritten;
    }

}
