/*
 * main.cpp
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
 * NetProxy is a proxy for the components of the IHMC Agile Computing Middleware (ACM)
 *
 * NetProxy transparently interfaces with legacy applications by using packet
 * capture / injection mechanisms and then uses ACM components such as
 * DisService or Mockets to perform the actual data communications.
 */

#include <cstdio>
#include <csignal>
#include <string>
#include <sstream>
#if defined (WIN32)
    #include <iostream>
    #include <stdio.h>
    #include <winsock2.h>
    #include <windows.h>
#elif defined (UNIX)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #define stricmp strcasecmp
    #define FOREVER 0xFFFFFFFFU
#endif

#include "Logger.h"

#include "version.h"
#include "PacketRouter.h"

#if defined (UNIX)
    #define stricmp strcasecmp
    #define FOREVER 0xFFFFFFFFU
#endif


#define checkAndLogMsg(_f_name_, _log_level_, ...) \
    if (NOMADSUtil::pLogger && (NOMADSUtil::pLogger->getDebugLevel() >= _log_level_)) \
        NOMADSUtil::pLogger->logMsg (_f_name_, _log_level_, __VA_ARGS__)

namespace ACMNetProxy
{
    PacketRouter pr;

    void terminateNetProxy (void)
    {
        checkAndLogMsg ("terminateNetProxy", NOMADSUtil::Logger::L_Info,
                        "exit requested by user! Terminating execution...\n");
        pr.requestTermination();
    }

    #if defined (WIN32)
        void abortExecution (int param)
        {
            (void) param;
            checkAndLogMsg ("abortExecution", NOMADSUtil::Logger::L_MildError,
                            "execution aborted!\n");
            exit (-1);
        }

        BOOL WINAPI control_handler (DWORD controlType)
        {
            // installing abortExecution handler
            if (signal (SIGINT, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-2);
            }
            if (signal (SIGTERM, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-3);
            }
            if (signal (SIGBREAK, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-4);
            }

            switch (controlType) {
            case CTRL_C_EVENT:
            case CTRL_CLOSE_EVENT:
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                terminateNetProxy();

                // Sleep for the maximum time allowed by the OS before returning, because that would force the program exit
                NOMADSUtil::sleepForMilliseconds (5000);
                return true;
            default:
                return false;
            }
        }

        void terminateExecution (int param)
        {
            (void) param;
            // installing abortExecution handler
            if (signal (SIGINT, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-2);
            }
            if (signal (SIGTERM, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-3);
            }
            if (signal (SIGBREAK, abortExecution) == SIG_ERR) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler! Exiting now...\n");
                exit (-4);
            }

            terminateNetProxy();
            // Sleep for the maximum time allowed by the OS before returning, because that would force the program exit
            NOMADSUtil::sleepForMilliseconds (5000);
        }

    #elif defined (UNIX) || defined (LINUX) || defined (ANDROID) || defined (OSX)
        void abortExecution (int sig, siginfo_t *siginfo, void *context)
        {
            (void) sig;
            (void) siginfo;
            (void) context;
            checkAndLogMsg ("abortExecution", NOMADSUtil::Logger::L_MildError,
                            "execution aborted!\n");
            exit (-1);
        }

        void terminateExecution (int param)
        {
            (void) param;
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
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler for SIGINT! Exiting now...\n");
                exit (-2);
            }

            if (sigaction (SIGTERM, &actTerm, nullptr) < 0) {
                checkAndLogMsg ("terminateExecution", NOMADSUtil::Logger::L_SevereError,
                                "impossible to install abort handler for SIGTERM! Exiting now...\n");
                exit (-3);
            }

            terminateNetProxy();
        }
    #endif
}

int main (int argc, char *argv[])
{
    int rc;
    std::string sConfigFilePath;
    NOMADSUtil::pLogger = new NOMADSUtil::Logger();
    NOMADSUtil::pLogger->enableScreenOutput();      // any errors occurring before reading configuration file are logged to screen
    NOMADSUtil::pLogger->setDebugLevel (NOMADSUtil::Logger::L_Info);

    int i = 1;
    while (i < argc) {
        // Check to see if a config file path was specified on the command-line
        if (0 == stricmp (argv[i], "-conf")) {
            i++;
            if (i < argc) {
                checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                                "setting config file path to %s\n", argv[i]);
                sConfigFilePath = argv[i];
            }
        }

        i++;
    }

    // Print build information to command line
    std::cout << "NetProxy version:\t" << _GIT_TAG << std::endl;
    std::cout << "Git branch name:\t" << _GIT_BRANCH << std::endl;
    std::cout << "Git commit sha:\t\t" << _GIT_SHA << std::endl;
    std::cout << "Git commit date:\t" << _GIT_DATE << std::endl;
    std::cout << "Build time:\t\t" << _BUILD_DATE << " at " << _BUILD_TIME << std::endl;

    // Try to determine the executable's directory
    std::string sProgDir{ACMNetProxy::nullprtToEmptyString (NOMADSUtil::getProgHomeDir (argv[0]))};
    if ((sProgDir.find_last_of (NOMADSUtil::getPathSepChar()) == std::string::npos) && (sConfigFilePath.length() == 0)) {
        checkAndLogMsg ("main", NOMADSUtil::Logger::L_SevereError,
                        "executable not installed in expected directory structure - could not parse "
                        "directory <%s> to compute the config file directory\n", sProgDir.c_str());
        return -1;
    }
    // Strip off last directory level in path
    auto sHomeDir = sProgDir.substr (0, sProgDir.find_last_of (NOMADSUtil::getPathSepChar()));

    if (sConfigFilePath.length() == 0) {
        // The config file was not specified on the command-line --> assume a default directory structure to determine it
        if (sHomeDir.length() == 0) {
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_MildError,
                            "unable to determine the home directory for the executable\n");
            // Simply try to open the default config file name in the current directory
            sConfigFilePath = ACMNetProxy::NetProxyApplicationParameters::S_DEFAULT_MAIN_CONFIGURATION_FILE_NAME;
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                            "will attempt to open the config file %s\n", sConfigFilePath.c_str());
        }
        else {
            // Assuming the main config file path is located at ..\\conf or ../conf; ../ is already in sHomeDir
            std::ostringstream osConfigFile;
            osConfigFile << sHomeDir << NOMADSUtil::getPathSepCharAsString() << "conf" << NOMADSUtil::getPathSepCharAsString() <<
                ACMNetProxy::NetProxyApplicationParameters::S_DEFAULT_MAIN_CONFIGURATION_FILE_NAME;
            sConfigFilePath = osConfigFile.str();
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                            "using %s as path for the config files\n", sConfigFilePath.c_str());
        }
    }

    // PacketRouter destructor will take care of deleting the references to the internal and external interfaces
    if (0 != (rc = ACMNetProxy::pr.init (sHomeDir, sConfigFilePath))) {
        checkAndLogMsg ("main", NOMADSUtil::Logger::L_SevereError,
                        "init() on PacketRouter failed with rc = %d\n", rc);
        return -4;
    }

    #if defined (WIN32)
        if (SetConsoleCtrlHandler (ACMNetProxy::control_handler, TRUE)) {
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_MediumDetailDebug,
                            "installed control handler function\n");
        }
        else {
            checkAndLogMsg ("main", NOMADSUtil::Logger::L_Warning,
                            "Could not install control handler function using the SetConsoleCtrlHandler() system "
                            "call; the NetProxy will try to set it up using the signal() system call\n");
            signal (SIGINT, ACMNetProxy::terminateExecution);
            signal (SIGTERM, ACMNetProxy::terminateExecution);
            signal (SIGBREAK, ACMNetProxy::terminateExecution);      // WIN32 OS sends a SIGBREAK whenever an user closes a console application clicking on the X button
        }
    #elif defined (UNIX) || defined (LINUX) || defined (OSX) || defined (ANDROID)
        signal (SIGINT, ACMNetProxy::terminateExecution);
        signal (SIGTERM, ACMNetProxy::terminateExecution);
    #endif

    if (0 != (rc = ACMNetProxy::pr.startThreads())) {
        checkAndLogMsg ("main", NOMADSUtil::Logger::L_SevereError,
                        "startThreads() on PacketRouter failed with rc = %d\n", rc);
        return -5;
    }

    // Send ARP request for the MAC address of the gateway and any node in the gateways set
    if (ACMNetProxy::NetProxyApplicationParameters::GATEWAY_MODE) {
        for (const auto & nidExternalInterface : ACMNetProxy::NetProxyApplicationParameters::V_NID_EXTERNAL_INTERFACE_LIST) {
            if (nidExternalInterface.ui32IPv4GatewayAddress == 0U) {
                checkAndLogMsg ("main", NOMADSUtil::Logger::L_LowDetailDebug,
                                "no default gateway configured for the interface <%s>\n",
                                nidExternalInterface.sInterfaceName.c_str());
                continue;
            }
            auto spNetworkInterface = ACMNetProxy::pr.getExternalNetworkInterfaceWithIP (nidExternalInterface.ui32IPv4Address);
            if (0 != (rc = ACMNetProxy::pr.sendARPRequestForGatewayMACAddress (spNetworkInterface.get(), nidExternalInterface))) {
                checkAndLogMsg ("main", NOMADSUtil::Logger::L_Warning,
                                "sendARPRequestForGatewayMACAddress() failed with rc = %d\n", rc);
            }
        }
    }

    //#if defined (WIN32)
    if (0 != (rc = ACMNetProxy::pr.joinThreads())) {
        checkAndLogMsg ("main", NOMADSUtil::Logger::L_SevereError,
                        "joinThreads() on PacketRouter failed with rc = %d\n", rc);
        return -6;
    }
    else {
        checkAndLogMsg ("main", NOMADSUtil::Logger::L_Info,
                        "Execution terminated successfully! Exiting program...\n");
        NOMADSUtil::sleepForMilliseconds (500);
    }

    return 0;
}

