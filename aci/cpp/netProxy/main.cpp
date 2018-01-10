/*
 * main.cpp
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
 *
 * NetProxy is a proxy for the components of the IHMC Agile Computing Middleware (ACM)
 *
 * NetProxy transparently interfaces with legacy applications by using packet
 * capture / injection mechanisms and then uses ACM components such as
 * DisService or Mockets to perform the actual data communications.
 */

#include <cstdio>
#include <csignal>
#if defined (WIN32)
    #include <iostream>
    #include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #define stricmp strcasecmp
    #define FOREVER 0xFFFFFFFFU
#endif

#include "NLFLib.h"
#include "InetAddr.h"
#include "StrClass.h"
#include "Logger.h"

#include "Utilities.h"
#include "TapInterface.h"
#include "PCapInterface.h"
#include "ConfigurationManager.h"
#include "PacketRouter.h"

#if defined (UNIX)
    #define stricmp strcasecmp
    #define FOREVER 0xFFFFFFFFU
#endif


using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace ACMNetProxy
{
    #if defined (WIN32)
        void abortExecution (int param)
        {
            (void) param;
            checkAndLogMsg ("abortExecution", Logger::L_MildError,
                            "execution aborted!\n");
            exit (-1);
        }

    #elif defined (UNIX)
        void abortExecution (int sig, siginfo_t *siginfo, void *context)
        {
            (void) sig;
            (void) siginfo;
            (void) context;
            checkAndLogMsg ("abortExecution", Logger::L_MildError,
                            "execution aborted!\n");
            exit (-1);
        }
    #endif

    void terminateExecution (int param)
    {
        (void) param;
        #if defined (WIN32)
            // installing abortExecution handler
            if (signal (SIGINT, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-2);
            }
            if (signal (SIGTERM, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-3);
            }
            if (signal (SIGBREAK, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-4);
            }
        #elif defined (UNIX)
            struct sigaction actInt, actTerm;
            memset (&actInt, '\0', sizeof(actInt));
            memset (&actTerm, '\0', sizeof(actTerm));

            /* Use the sa_sigaction field because the handles has two additional parameters */
            actInt.sa_sigaction = &abortExecution;
            actTerm.sa_sigaction = &abortExecution;

            /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
            actInt.sa_flags = SA_SIGINFO;
            actTerm.sa_flags = SA_SIGINFO;

            if (sigaction (SIGINT, &actInt, nullptr) < 0) {
                checkAndLogMsg ("terminateExecution", Logger::L_SevereError,
                                "impossible to install abort handler for SIGINT! Exiting now...\n");
                exit (-2);
            }

            if (sigaction (SIGTERM, &actTerm, nullptr) < 0) {
                checkAndLogMsg ("terminateExecution", Logger::L_SevereError,
                                "impossible to install abort handler for SIGTERM! Exiting now...\n");
                exit (-3);
            }
        #endif

        checkAndLogMsg ("terminateExecution", Logger::L_Info,
                        "exit requested by user! Terminating execution...\n");
        PacketRouter::requestTermination();

        #if defined (WIN32)
        sleepForMilliseconds (2000);
        #endif
    }
}

int main (int argc, char *argv[])
{
    int rc;
    pLogger = new Logger();
    pLogger->enableScreenOutput();      // any errors occurring before reading configuration file are logged to screen
    pLogger->setDebugLevel (Logger::L_Info);

    const char * const pszDefaultConfigFileName = "netproxy.cfg";
    // Check to see if a config file path was specified on the command-line
    String homeDir;
    String configFilePath;

    int i = 1;
    while (i < argc) {
        #if defined (WIN32)
            if (0 == stricmp (argv[i], "-testtap")) {
                ACMNetProxy::TapInterface *pTI = ACMNetProxy::TapInterface::getTAPInterface();

                // TODO: Fix uninitialized rc variable
                switch (rc) {
                case 0:
                    if (pTI->checkMACAddress()) {
                        checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_Info,
                                        "SUCCESSFULLY RUN\n");
                        return 0;
                    }
                    else {
                        checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                        "THE IP ADDRESS DOES NOT MATCH THE MAC ADDRESS\n");
                        return -4;
                    }
                case -1:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "NO TAP-WIN32 INTERFACE FOUND\n");
                    return -5;
                case -2:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "FOUND MORE THAN ONE TAP INTERFACE: DO NOT KNOW WHICH ONE TO USE\n");
                    return -6;
                case -3:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "FAILED OPENING THE TAP INTERFACE DEVICE\n");
                    return -7;
                case -4:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "FAILED CREATING THE READING EVENT\n");
                    return -8;
                case -5:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "FAILED CREATING THE WRITING EVENT\n");
                    return -9;
                default:
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError,
                                    "UNKNOWN ERROR\n");
                    return -10;
                }
            }
        #endif

        if (0 == stricmp (argv[i], "-conf")) {
            i++;
            if (i < argc) {
                checkAndLogMsg ("main", Logger::L_Info,
                                "setting config file path to %s\n", argv[i]);
                configFilePath = argv[i];
            }
        }
        i++;
    }

    if (configFilePath.length() <= 0) {
        // Nothing was specified on the command-line
        // Try to determine the path to the config file
        const char *pszProgDir = getProgHomeDir (argv[0]);
        if (pszProgDir == nullptr) {
            checkAndLogMsg ("main", Logger::L_MildError,
                            "unable to determine the home directory for the executable\n");
            // Simply try to open the default config file name in the current directory
            configFilePath = pszDefaultConfigFileName;
            checkAndLogMsg ("main", Logger::L_Info,
                            "will attempt to open the config file %s\n",
                            (const char*) configFilePath);
        }
        else {
            // Compute the Home Directory
            // Assume that the executable is in <home>\\bin (or <home/bin>)
            char szHomeDir[PATH_MAX];
            strcpy (szHomeDir, pszProgDir);
            // Strip off last directory level in path
            char *pszTemp = strrchr (szHomeDir, getPathSepChar());
            if (pszTemp == nullptr) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "executable not installed in expected directory structure - could not parse "
                                "directory <%s> to compute the config file directory\n", pszProgDir);
                return -2;
            }
            *pszTemp = '\0';
            homeDir = szHomeDir;
            configFilePath = szHomeDir;
            configFilePath += getPathSepCharAsString();
            configFilePath += "conf";
            configFilePath += getPathSepCharAsString();
            configFilePath += pszDefaultConfigFileName;
            checkAndLogMsg ("main", Logger::L_Info,
                            "using %s as path for the config files\n",
                            (const char*) configFilePath);
        }
    }

    ACMNetProxy::NetProxyConfigManager *pCfgManager = ACMNetProxy::NetProxyConfigManager::getNetProxyConfigManager();
    // Read the config file
    if (0 != (rc = pCfgManager->init (homeDir, configFilePath))) {
        checkAndLogMsg ("main", Logger::L_MildError,
                        "failed to call init() on ConfigurationManager with config file path <%s>; rc = %d\n",
                        (const char*) configFilePath, rc);
        return -3;
    }

    // Process the config file
    if (0 != (rc = pCfgManager->processConfigFiles())) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "config files processing failed with rc = %d\n", rc);
        return -4;
    }

    /* The internal network interface will be instantiated only if running in Gateway Mode,
     * while the external one needs to be accessed anyway (for statistics and for the NPUID)
     */
    ACMNetProxy::NetworkInterface *pInternalNetworkInterface = nullptr, *pExternalNetworkInterface = nullptr;
    const char *pszExternalInterfaceName = pCfgManager->getValue ("ExternalInterfaceName");
    if (pszExternalInterfaceName == nullptr) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "external interface name not specified in the config file\n");
        return -5;
    }
    if ((pExternalNetworkInterface = ACMNetProxy::PCapInterface::getPCapInterface (pszExternalInterfaceName)) == nullptr) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "could not open external network interface <%s>\n",
                        pszExternalInterfaceName);
        return -6;
    }
    ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_NAME = pszExternalInterfaceName;

    if (ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
        // Try to retrieve the IP address of the external network interface by querying the device itself
        if (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR == 0U) {
            ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR = pExternalNetworkInterface->getIPv4Addr() ?
                pExternalNetworkInterface->getIPv4Addr()->ui32Addr : 0U;
            if (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine IP address of external network interface <%s>\n",
                                pszExternalInterfaceName);
                delete pExternalNetworkInterface;
                return -7;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved IP address %s for external network interface <%s>\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR).getIPAsString(),
                                pszExternalInterfaceName);
            }
        }
        else if (pExternalNetworkInterface->getIPv4Addr() &&
            (pExternalNetworkInterface->getIPv4Addr()->ui32Addr != ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR)) {
            // Check if the IP address retrieved from the external network interface does not match the IP provided in the configuration
            checkAndLogMsg ("main", Logger::L_Warning,
                            "the IP address retrieved from the external network interface <%s> differs from the one specified in the configuration file: retrieved "
                            "IP address is %s - configured IP address is %s; NetProxy will use the IP address retrieved from the external network interface\n",
                            pszExternalInterfaceName, InetAddr(pExternalNetworkInterface->getIPv4Addr()->ui32Addr).getIPAsString(),
                            InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR).getIPAsString());
            ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR = pExternalNetworkInterface->getIPv4Addr()->ui32Addr;
        }

        // Retrieve IPv4 netmask of the external interface
        if (pExternalNetworkInterface->getNetmask()) {
            if ((ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK != 0U) &&
                (pExternalNetworkInterface->getNetmask()->ui32Addr != ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK)) {
                ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK = pExternalNetworkInterface->getNetmask()->ui32Addr;
                checkAndLogMsg ("main", Logger::L_Warning,
                                "configured value for the external network netmask differs from the one retrieved from the "
                                "external interface; NetProxy will use the one provided by the interface: %s\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK).getIPAsString());
            }
            else if (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK == 0U) {
                ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK = pExternalNetworkInterface->getNetmask()->ui32Addr;
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved external network netmask %s from the external interface\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK).getIPAsString());
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "using the netmask %s for the external network interface\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK).getIPAsString());
            }
        }
        else {
            if (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine Netmask of the external network interface\n");
                delete pExternalNetworkInterface;
                return -8;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Warning,
                                "could not retrieve the external network netmask from the external interface; "
                                "NetProxy will use the mask %s, found in the configuration file\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_NETWORK_NETMASK).getIPAsString());
            }
        }

        // Retrieve the MTU size for the external network interface
        if ((pExternalNetworkInterface->getMTUSize() > 0) &&
            (pExternalNetworkInterface->getMTUSize() <= ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MAX_MTU)) {
            checkAndLogMsg ("main", Logger::L_Info,
                            "retrieved an MTU size of %hu bytes for the external network interface <%s>\n",
                            pExternalNetworkInterface->getMTUSize(), pszExternalInterfaceName);
            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF = pExternalNetworkInterface->getMTUSize();
        }
        else if (pExternalNetworkInterface->getMTUSize() == 0) {
            checkAndLogMsg ("main", Logger::L_Warning,
                            "impossible to retrieve the MTU size from the external network interface <%s>; using default value of %hu bytes\n",
                            pszExternalInterfaceName, ACMNetProxy::NetProxyApplicationParameters::ETHERNET_DEFAULT_MTU);
            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF = ACMNetProxy::NetProxyApplicationParameters::ETHERNET_DEFAULT_MTU;
        }
        else {
            // Retrieved MTU value larger than ETHERNET_MAX_MTU --> using default value
            checkAndLogMsg ("main", Logger::L_Warning,
                            "retrieved a value of %hu bytes for the MTU size from the external network interface <%s>, "
                            "but the maximum MTU size allowed is %hu bytes; using default value of %hu bytes\n",
                            pExternalNetworkInterface->getMTUSize(), pszExternalInterfaceName,
                            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_EXTERNAL_IF);
        }

        // Running in Gateway Mode: use PCap library and retrieve MAC addresses of the internal and external interface
        const char * const pszInternalInterfaceName = pCfgManager->getValue ("InternalInterfaceName");
        if (pszInternalInterfaceName == nullptr) {
            checkAndLogMsg ("main", Logger::L_SevereError,
                            "internal interface to use for Gateway Mode has not been specified\n");
            delete pExternalNetworkInterface;
            return -9;
        }
        if ((pInternalNetworkInterface = ACMNetProxy::PCapInterface::getPCapInterface (pszInternalInterfaceName)) == nullptr) {
            checkAndLogMsg ("main", Logger::L_SevereError,
                            "could not open device <%s>\n", pInternalNetworkInterface);
            delete pExternalNetworkInterface;
            return -10;
        }

        ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_NAME = pszInternalInterfaceName;

        // Retrieve MAC address
        const uint8 *pszInternalMACAddr = nullptr, *pszExternalMACAddr = nullptr;
        if (nullptr == (pszExternalMACAddr = pExternalNetworkInterface->getMACAddr())) {
            checkAndLogMsg ("main", Logger::L_SevereError,
                            "could not obtain MAC address for external network interface <%s>\n",
                            pszExternalInterfaceName);
            delete pInternalNetworkInterface;
            delete pExternalNetworkInterface;
            return -11;
        }
        if ((pszInternalMACAddr = pInternalNetworkInterface->getMACAddr()) == nullptr) {
            checkAndLogMsg ("main", Logger::L_SevereError,
                            "could not obtain MAC address for internal network interface <%s>\n",
                            pszInternalInterfaceName);
            delete pInternalNetworkInterface;
            delete pExternalNetworkInterface;
            return -12;
        }

        ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR = ACMNetProxy::buildEthernetMACAddressFromString (pszInternalMACAddr);
        checkAndLogMsg ("main", Logger::L_Info,
                        "Internal Interface MAC address: %2x:%2x:%2x:%2x:%2x:%2x\n",
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte1),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte2),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte3),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte4),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte5),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_INTERFACE_MAC_ADDR.ui8Byte6));

        ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR = ACMNetProxy::buildEthernetMACAddressFromString (pszExternalMACAddr);
        checkAndLogMsg ("main", Logger::L_Info,
                        "External Interface MAC address: %2x:%2x:%2x:%2x:%2x:%2x\n",
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte1),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte2),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte3),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte4),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte5),
                        static_cast<int> (ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_INTERFACE_MAC_ADDR.ui8Byte6));

        // Try to retrieve the IP address of the internal network interface by querying the device itself
        if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR == 0U) {
            ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR = pInternalNetworkInterface->getIPv4Addr() ?
                pInternalNetworkInterface->getIPv4Addr()->ui32Addr : 0U;
            if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine the IP address of the internal network interface <%s>\n",
                                pszInternalInterfaceName);
                delete pInternalNetworkInterface;
                delete pExternalNetworkInterface;
                return -13;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved IP address %s for external network interface <%s>\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR).getIPAsString(),
                                pszInternalInterfaceName);
            }
        }

        // Retrieve IPv4 Netmask of the internal interface
        if (pInternalNetworkInterface->getNetmask()) {
            if ((ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK != 0U) &&
                (pInternalNetworkInterface->getNetmask()->ui32Addr != ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK)) {
                ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK = pInternalNetworkInterface->getNetmask()->ui32Addr;
                checkAndLogMsg ("main", Logger::L_Warning,
                                "configured value for the external network netmask differs from the one retrieved from the "
                                "external interface; NetProxy will use the one provided by the interface: %s\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK).getIPAsString());
            }
            else if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK == 0U) {
                ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK = pInternalNetworkInterface->getNetmask()->ui32Addr;
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved external network netmask %s from the external interface\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK).getIPAsString());
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "using the netmask %s for the external network interface\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK).getIPAsString());
            }
        }
        else {
            if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine Netmask of the external network interface\n");
                delete pInternalNetworkInterface;
                delete pExternalNetworkInterface;
                return -14;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Warning,
                                "could not retrieve the external network netmask from the external interface; "
                                "NetProxy will use the mask %s, found in the configuration file\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_NETWORK_NETMASK).getIPAsString());
            }
        }

        // Retrieve the MTU size for the internal network interface
        if ((pInternalNetworkInterface->getMTUSize() > 0) &&
            (pInternalNetworkInterface->getMTUSize() <= ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MAX_MTU)) {
            checkAndLogMsg ("main", Logger::L_Info,
                            "retrieved an MTU size of %hu bytes for the internal network interface <%s>\n",
							pInternalNetworkInterface->getMTUSize(), pszInternalInterfaceName);
            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF = pExternalNetworkInterface->getMTUSize();
        }
        else if (pInternalNetworkInterface->getMTUSize() == 0) {
            checkAndLogMsg ("main", Logger::L_Warning,
                            "impossible to retrieve the MTU size from the internal network interface <%s>; using default value of %hu bytes\n",
                            pInternalNetworkInterface, ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF);
        }
        else {
            // Retrieved MTU value larger than ETHERNET_MAX_MTU --> using default value
            checkAndLogMsg ("main", Logger::L_Warning,
                            "retrieved a value of %hu bytes for the MTU size from the internal network interface <%s>, "
                            "but the maximum MTU size allowed is %hu bytes; using default value of %hu bytes\n",
                            pInternalNetworkInterface->getMTUSize(), pInternalNetworkInterface,
                            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF);
        }

        // Retrieve IPv4 address of the Default Gateway
        if (ACMNetProxy::NetProxyApplicationParameters::NETWORK_GATEWAY_IP_ADDR == 0U) {
            // Try and retrieve Default Gateway from the device itself
            ACMNetProxy::NetProxyApplicationParameters::NETWORK_GATEWAY_IP_ADDR = pExternalNetworkInterface->getDefaultGateway() ?
                pExternalNetworkInterface->getDefaultGateway()->ui32Addr : 0U;
            if (ACMNetProxy::NetProxyApplicationParameters::NETWORK_GATEWAY_IP_ADDR == 0U) {
                checkAndLogMsg ("main", Logger::L_Warning,
                                "could not determine the IP address of the default gateway node by querying the external network "
                                "interface and no IP address for the default gateway was specified in the configuration file\n");
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved IP %s as the address of the default gateway node for external network interface <%s>\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::NETWORK_GATEWAY_IP_ADDR).getIPAsString(),
                                pszExternalInterfaceName);
            }
        }
    }
    else {
        // Running in Host Mode: use the TUN/TAP interface
        pInternalNetworkInterface = ACMNetProxy::TapInterface::getTAPInterface();
        // Retrieve IPv4 address of the TAP interface
        if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR == 0U) {
            ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR = pInternalNetworkInterface->getIPv4Addr() ?
                                                                           pInternalNetworkInterface->getIPv4Addr()->ui32Addr : 0U;
            if (ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine IP address of TUN/TAP interface and "
                                "no IP address was specified in the config file\n");
                delete pExternalNetworkInterface;
                return -15;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved IP address %s for the TUN/TAP interface\n",
                                InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR).getIPAsString());
            }
        }
        else if (pInternalNetworkInterface->getIPv4Addr() &&
                (pInternalNetworkInterface->getIPv4Addr()->ui32Addr != ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR)) {
            checkAndLogMsg ("main", Logger::L_Warning,
                            "the IP address retrieved from the TAP interface differs from the one specified in the configuration file: "
                            "retrieved IP address is %s - configured IP address is %s; NetProxy will use the IP address retrieved form "
                            "the external network interface\n", InetAddr(pInternalNetworkInterface->getIPv4Addr()->ui32Addr).getIPAsString(),
                            InetAddr(ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR).getIPAsString());
            ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR = pInternalNetworkInterface->getIPv4Addr()->ui32Addr;
        }

        // Retrieve MTU
        if (ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF == 0U) {
            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF = pInternalNetworkInterface->getMTUSize();
            if (ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF == 0U) {
                checkAndLogMsg ("main", Logger::L_SevereError,
                                "could not determine the MTU of the TUN/TAP interface and "
                                "no MTU size was specified in the config file\n");
                delete pExternalNetworkInterface;
                return -16;
            }
            else {
                checkAndLogMsg ("main", Logger::L_Info,
                                "retrieved MTU size of %hu bytes for the TUN/TAP interface\n",
                                ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF);
            }
        }
        else if ((pInternalNetworkInterface->getMTUSize() != 0U) &&
                 (pInternalNetworkInterface->getMTUSize() != ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF)) {
            checkAndLogMsg ("main", Logger::L_Warning,
                            "configured MTU size (%hu bytes) does not match the value retrieved from the "
                            "TUN/TAP interface (%hu bytes); NetProxy will use the latter value\n",
                            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF,
                            pInternalNetworkInterface->getMTUSize());
            ACMNetProxy::NetProxyApplicationParameters::ETHERNET_MTU_INTERNAL_IF = pInternalNetworkInterface->getMTUSize();
        }
    }

    // Check if the NPUID was set and, if not, set it to the 32-bits value of the IP address of the external/TAP interface
    if (ACMNetProxy::NetProxyApplicationParameters::NETPROXY_UNIQUE_ID == 0U) {
        ACMNetProxy::NetProxyApplicationParameters::NETPROXY_UNIQUE_ID = ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE ?
            ACMNetProxy::NetProxyApplicationParameters::EXTERNAL_IP_ADDR : ACMNetProxy::NetProxyApplicationParameters::INTERNAL_IP_ADDR;
        checkAndLogMsg ("main", Logger::L_Info,
                        "no UniqueID was specified in the config file; using the IP address of the %s interface as UniqueID (%u)\n",
                        ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE ? "external" : "TAP",
                        ACMNetProxy::NetProxyApplicationParameters::NETPROXY_UNIQUE_ID);
    }

    // If running in Host Mode, delete the reference to the external network interface
    if (!ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
        delete pExternalNetworkInterface;
        pExternalNetworkInterface = nullptr;
    }

    // PacketRouter destructor will take care of deleting the references to the internal and external interfaces
    ACMNetProxy::PacketRouter *pr = ACMNetProxy::PacketRouter::getPacketRouter();
    if (0 != (rc = pr->init (pInternalNetworkInterface, pExternalNetworkInterface))) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "init() on PacketRouter failed with rc = %d\n", rc);
        return -17;
    }

    signal (SIGINT, ACMNetProxy::terminateExecution);
    signal (SIGTERM, ACMNetProxy::terminateExecution);
    #if defined (WIN32)
        signal (SIGBREAK, ACMNetProxy::terminateExecution);      // WIN32 OS sends a SIGBREAK whenever an user closes a console application clicking on the X button
    #endif

    if (0 != (rc = pr->startThreads())) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "startThreads() on PacketRouter failed with rc = %d\n", rc);
        return -18;
    }

    // Send ARP request for the MAC address of the gateway and any node in the gateways set
    if (ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
        if (0 != (rc = ACMNetProxy::PacketRouter::sendARPRequestForGatewayMACAddress())) {
            checkAndLogMsg ("main", Logger::L_Warning,
                            "sendARPRequestForGatewayMACAddress() on "
                            "PacketRouter failed with rc = %d\n", rc);
        }
    }

    //#if defined (WIN32)
    if (0 != (rc = pr->joinThreads())) {
        checkAndLogMsg ("main", Logger::L_SevereError,
                        "joinThreads() on PacketRouter failed with rc = %d\n", rc);
        return -19;
    }
    else {
        checkAndLogMsg ("main", Logger::L_Info,
                        "Execution terminated successfully! Exiting program...\n");
        sleepForMilliseconds (1000);
    }

    return 0;
}
