/*
 * ACINetProxy
 *
 * A proxy for IHMC Agile Computing Infrastructure components
 * 
 * Transparently interfaces with legacy applications by using packet capture / injection
 * mechanisms and then uses ACI components such as DisService to perform the actual data
 * communications.
 *
 * This file is part of the IHMC Agile Computing Infrastructure
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "TapInterfaceTestAndConfiguration.h"
#include "TapInterface.cpp"
#include "Logger.h"
#include <windows.h>
#include <netcon.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#define stricmp _stricmp

#define checkAndLogMsg if (pLogger) pLogger->logMsg

//ACINetProxy::TapInterface *ACINetProxy::TapInterface::_pTapInterface = NULL;
namespace ACMNetProxy
{
    uint32 ACMNetProxy::NetProxyApplicationParameters::NETPROXY_IP_ADDR = 0U;
    uint16 ACMNetProxy::NetProxyApplicationParameters::TAP_INTERFACE_MTU = ACMNetProxy::NetProxyApplicationParameters::TAP_INTERFACE_DEFAULT_MTU;

    TapInterfaceTestAndConfiguration::TapInterfaceTestAndConfiguration (void) { }

    TapInterfaceTestAndConfiguration::~TapInterfaceTestAndConfiguration (void) { }

    int TapInterfaceTestAndConfiguration::runTapInterfaceTest (void)
    {
        //TapInterface *pTITest = TapInterface::getTAPInterface();
        ACMNetProxy::TapInterface *pTITest = ACMNetProxy::TapInterface::getTAPInterface();
        int rc = pTITest->init();

        switch (rc) {
            case 0:
                if (pTITest->checkMACAddress()) {
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_Info, "SUCCESSFULLY RUN\n");
                    return 0;
                }
                else {
                    checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, 
                                    "THE IP ADDRESS DOES NOT MATCH THE MAC ADDRESS\n");

                    const char * fileName = "C:\\Temp\\netParams.txt";
                    if (!checkFilePermission (fileName)) {
                        int rc = writeTAPConfOnFile (pTITest, fileName);
                    }
                    
                    return -4;
                }
            case -1:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "NO TAP-WIN32 INTERFACE FOUND\n");
                return -5;
            case -2:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, 
                                "FOUND MORE THAN ONE TAP INTERFACE: DO NOT KNOW WHICH ONE TO USE\n");
                return -6;
            case -3:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "FAILED OPENING THE TAP INTERFACE DEVICE\n");
                return -7;
            case -4:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "FAILED CREATING THE READING EVENT\n");
                return -8;
            case -5:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "FAILED CREATING THE WRITING EVENT\n");
                return -9;
            default:
                checkAndLogMsg ("TAP INTERFACE TEST - ", Logger::L_MildError, "UNKNOWN ERROR\n");
                return -10;
        }
    }

    int TapInterfaceTestAndConfiguration::checkFilePermission (const char * fileName)
    {
        HANDLE hFile; 
        char DataBuffer[] = "";
        DWORD dwBytesToWrite = (DWORD)strlen(DataBuffer);
        DWORD dwBytesWritten = 0;
        BOOL bErrorFlag = FALSE;
        hFile = CreateFile (fileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            return -1;
        }
        CloseHandle(hFile);
        return 0;
    }

    int TapInterfaceTestAndConfiguration::writeTAPConfOnFile (ACMNetProxy::TapInterface *pTITest, const char * fileName) 
    {
        FILE * pFile = fopen (fileName,"w");

        const char * adapterDescr = pTITest->getAdapterName();
        if (adapterDescr != NULL) {
            fprintf (pFile, "adapter descriptor = %s\n", adapterDescr);
        }
        else {
            fprintf (pFile, "adapter descriptor = \n");
        }

        const InetAddr ipInetAddr(ACMNetProxy::NetProxyApplicationParameters::NETPROXY_IP_ADDR);
        if (ipInetAddr.getIPAddress() != 0U) {
            const char *pcIPAddr = ipInetAddr.getIPAsString();
            fprintf (pFile, "ip=%s\n", pcIPAddr);
            StringTokenizer st (pcIPAddr, '.');
            const char * octects[3];
            for (int i=0; i<3; i++) {
                octects[i] = st.getNextToken();
            }
            fprintf (pFile, "default gateway=%s.%s.%s.0\n", octects[0], octects[1], octects[2]); 
        }
        else {
            fprintf (pFile, "ip=\n");
            fprintf (pFile, "default gateway=\n");
        }

        fprintf (pFile, "subnet mask=255.255.255.0\n");

        if (pTITest->getMACAddr() != NULL) {
            fprintf (pFile, "MAC=%X:%X:%X:%X:%X:%X\n", pTITest->getMACAddr()[0], pTITest->getMACAddr()[1], 
                pTITest->getMACAddr()[2], pTITest->getMACAddr()[3], pTITest->getMACAddr()[4], pTITest->getMACAddr()[5]);
        }
        else {
            fprintf (pFile, "MAC=\n");
        }

        fclose (pFile);
        return 0;
    }

    /*
     * Allow you to change an adapter's IP, subnet mask and gateway addressess 
     * without needing to restart using the command netsh
     */
    int TapInterfaceTestAndConfiguration::updateIPAddress (int argc, char *argv[]) 
    {
        char *adapterName;
        char *newIPAddr;
        char *newMaskAddr;
        char *newGatewayAddr;
        bool setAdapterName = false;
        bool setIPAddr = false;
        bool setMaskAddr = false;
        bool setGatewayAddr = false;

        int i = 2;
        if ((argc > 2) && (0 == stricmp (argv[i], "--help"))) {
            showIPHelp();
            return -1;
        }

        /*if (argc != 10) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, "wrong number of arguments\n");
            showIPHelp();
            return -2;
        }*/

        while (i < argc) {
            /*if (0 == stricmp (argv[i], "-i")) {
                adapterName = new char[strlen(argv[i+1])+1];
                strcpy (adapterName, argv[i+1]);
                setAdapterName = true;
                i = i+2;
            }*/
            if (0 == stricmp (argv[i], "-d")) {
                char *driverDescriptor = new char[strlen(argv[i+1])+1];
                strcpy (driverDescriptor, argv[i+1]);
                i = i+2;
                adapterName = getAdapterName (driverDescriptor);
                if (NULL == adapterName) {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_SevereError, 
                                    "Error in retrieving the Adapter Name for %s.\n", driverDescriptor);
                    return -7;
                }
            }
            else if (0 == stricmp (argv[i], "-ip")) {
                if (isValidIP (argv[i+1])) {
                    newIPAddr = new char[strlen(argv[i+1])+1];
                    strcpy (newIPAddr, argv[i+1]);
                    setIPAddr = true;
                    i = i+2;
                }
                else {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, 
                                    "IP String %s is not valid. IP addresses must be X.Y.Z.W with every octacte in [0-255]\n", argv[i+1]);
                    return -3;
                }
            }
            else if (0 == stricmp (argv[i], "-mask")) {
                if (isValidMask (argv[i+1])) {
                    newMaskAddr = new char[strlen(argv[i+1])+1];
                    strcpy (newMaskAddr, argv[i+1]);
                    setMaskAddr = true;
                    i = i+2;
                }
                else {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, 
                                    "Subnet Mask String %s is not valid. The addresses must be X.Y.Z.W with every octacte equal either to 0 or to 255\n", argv[i+1]);
                    return -4;
                }
            }
            else if (0 == stricmp (argv[i], "-gateway")) {
                if (isValidIP (argv[i+1])) {
                    newGatewayAddr = new char[strlen(argv[i+1])+1];
                    strcpy (newGatewayAddr, argv[i+1]);
                    setGatewayAddr = true;
                    i = i+2;
                }
                else {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, 
                                    "Default Gateway String %s is not valid. The addresses must be X.Y.Z.W with every octacte in [0-255]\n", argv[i+1]);
                    return -5;
                }
            }
            else {
                checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, "wrong argument.\n");
                showMacHelp();
                return -6;
            }
        }

        /*if (!setAdapterName) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_Warning, "Adapter Name has not been set.\n");
            showMacHelp();
            return -7;
        }*/

        if (!setIPAddr) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, "IP address has not been set.\n");
            showMacHelp();
            return -8;
        }

        if (!setMaskAddr) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, "Subnet mask address has not been set.\n");
            showMacHelp();
            return -9;
        }

        int rc;
        if (!setGatewayAddr) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_Warning, "Default gateway address has not been set.\n");
            showMacHelp();
            rc = setIPWithoutGateway (adapterName, newIPAddr, newMaskAddr);
        }
        else {
            rc = setIP (adapterName, newIPAddr, newMaskAddr, newGatewayAddr);
        }
        if (rc != 0) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateIPAddress: ", Logger::L_MildError, "Error while setting the ip with netsh\n");
            return rc;
        }

        return 0;
    }

    /*
     * Find the adapter name basing on the driver descriptor
     */
    char * TapInterfaceTestAndConfiguration::getAdapterName (char * driverDescriptor)
    {
        HKEY hListKey = NULL;
        HKEY hKey = NULL;
        RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}",0, KEY_READ, &hListKey);
        if (!hListKey) {
            checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_MildError, "Failed to open adapter list key\n");
            return NULL;
        }
        FILETIME writtenTime;
        char adapterKeyBuf[512], adapterDriverDescBuf[512], adapterNameBuf[512];
        DWORD adapterKeyBufSiz = 512;
        DWORD crap;
        int i = 0;
        bool found = false;
        while (RegEnumKeyEx (hListKey, i++, adapterKeyBuf, &adapterKeyBufSiz, 0, NULL, NULL, &writtenTime) == ERROR_SUCCESS) {
            _snprintf (adapterDriverDescBuf, 512, adapterKeyBuf);
            hKey = NULL;
            RegOpenKeyEx (hListKey, adapterDriverDescBuf, 0, KEY_READ, &hKey);
            if (hKey) {
                adapterKeyBufSiz = 512;
                RegQueryValueEx (hKey, "NetCfgInstanceId", 0, &crap, (LPBYTE)adapterKeyBuf, &adapterKeyBufSiz);
                if (RegQueryValueEx (hKey, "DriverDesc", 0, &crap, (LPBYTE)adapterDriverDescBuf, &adapterKeyBufSiz) == ERROR_SUCCESS && strcmp (adapterDriverDescBuf, driverDescriptor) == 0) {
                    checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_Info, "Adapter reg expr key is %s\n", adapterKeyBuf);
                    checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_Info, "Adapter driver descr is %s\n", adapterDriverDescBuf);
                    found = true;
                    break;
                }
                RegCloseKey (hKey);
            }
            adapterKeyBufSiz = 512;
        }
        RegCloseKey(hListKey);
        if (!found) {
            checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_MildError, 
                            "Could not find adapter driver descr '%s'.\n", driverDescriptor);
            return NULL;
        }

        RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}",0, KEY_READ, &hListKey);
        if (!hListKey) {
            checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_MildError, "Failed to open adapter list key in Phase 2\n");
            return NULL;
        }
        char adapterNetworkKeyBuf[512];
        DWORD adapterNetworkKeyBufSiz = 512;
        _snprintf (adapterNetworkKeyBuf, 512, "%s\\Connection", adapterKeyBuf);
        RegOpenKeyEx (hListKey, adapterNetworkKeyBuf, 0, KEY_READ, &hKey);
        if (!hKey) {
            checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_MildError, "Failed to open adapter list key in Phase 3\n");
            return NULL;
        }
        adapterKeyBufSiz = 512;
        RegQueryValueEx(hKey, "Name", 0, &crap, (LPBYTE)adapterNameBuf, &adapterKeyBufSiz);
        checkAndLogMsg ("TAP INTERFACE TEST - getAdapterName: ", Logger::L_Info, "Adapter name is %s\n", adapterNameBuf);

        return adapterNameBuf;
    }

    /*
     * Show the help for the ip address changeing
     */
    void TapInterfaceTestAndConfiguration::showIPHelp() 
    {
        char message[313];
        strcat (message, "\nUsage:\n\0");
        //strcat (message, " -i [adapter-name]                    The adapter name from Network Connections\n\0");
        strcat (message, " -ip [ip-address]                     IP address\n\0");
        strcat (message, " -mask [subnet mask]                  Subnet mask\n\0");
        strcat (message, " -gateway [degault gateway-address]   Degault gateway\n\0");
        strcat (message, " --help                               Shows this screen\n\n\0");
        checkAndLogMsg ("showHelp ", Logger::L_Info, "%d %s\n", strlen(message), (const char *) message);
    }

    /*
     * Check if the IP address is correct
     */
    bool TapInterfaceTestAndConfiguration::isValidIP (char *ip)
    {
        StringTokenizer st (ip, '.');
        int tokensCount = 0;
        const char * octect;
        while (NULL != (octect = st.getNextToken())) {
            int val = atoi (octect);
            if ((val < 0) || (val > 255)) {
                return false;
            }
            tokensCount++;
        }
        if (tokensCount != 4) {
            return false;
        }
        return true;
    }

    /*
     * Check if the IP address is correct
     */
    bool TapInterfaceTestAndConfiguration::isValidMask (char *mask)
    {
        StringTokenizer st (mask, '.');
        int tokensCount = 0;
        const char * octect;
        while (NULL != (octect = st.getNextToken())) {
            int val = atoi (octect);
            if ((val != 0) && (val != 255)) {
                return false;
            }
            tokensCount++;
        }
        if (tokensCount != 4) {
            return false;
        }
        return true;
    }
  
    /*
     * Update the ip, subnet mask and default gateway addresses using the 
     * command netsh
     */
    int TapInterfaceTestAndConfiguration::setIP (char * adapterName, char * newIPAddr, char * newMaskAddr, char * newGatewayAddr)
    {
        char command[313];
        command[0] = '\0';
        strcat (command, "netsh interface ip set address name=\"\0");
        strcat (command, adapterName);
        strcat (command, "\" static \0");
        strcat (command, newIPAddr);
        strcat (command, " \0");
        strcat (command, newMaskAddr);
        strcat (command, " \0");
        strcat (command, newGatewayAddr);
        strcat (command, " 1\0");
        checkAndLogMsg ("TAP INTERFACE TEST - setIP: ", Logger::L_MildError, "netsh command = %s\n", command);
        int rc = system (command);
        if (rc != 0) {
            return 11;
        }
        return rc;
    }

    /*
     * Use the command netsh to update the ip, subnet mask addresses without 
     * setting the default gateway
     */
    int TapInterfaceTestAndConfiguration::setIPWithoutGateway (char * adapterName, char * newIPAddr, char * newMaskAddr)
    {
        char command[313];
        command[0] = '\0';
        strcat (command, "netsh interface ip set address name=\"\0");
        strcat (command, adapterName);
        strcat (command, "\" static \0");
        strcat (command, newIPAddr);
        strcat (command, " \0");
        strcat (command, newMaskAddr);
        strcat (command, " \0");
        strcat (command, "gateway=none\0");
        checkAndLogMsg ("TAP INTERFACE TEST - setIP: ", Logger::L_MildError, "netsh command = %s\n", command);
        int rc = system (command);
        if (rc != 0) {
            return 11;
        }
        return rc;
    }

    /*
     * Allow you to change an adapter's MAC address without needing to restart.
     * When the MAC address is changed, all the connections are closed automatically
     * and the adapter is reset
     */
    int TapInterfaceTestAndConfiguration::updateMacAddress (int argc, char *argv[])
    {
        char *adapterName;
        char *newMacAddr;
        bool setAdapterName = false;
        bool setMacAddr = false;

        int i = 2;
        if ((argc > 2) && (0 == stricmp (argv[i], "--help"))) {
            showMacHelp();
            return -1;
        }

        /*if (argc != 6) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "wrong number of arguments\n");
            showMacHelp();
            return -2;
        }*/

        while (i < argc) {
            /*if (0 == stricmp (argv[i], "-i")) {
                adapterName = new char[strlen(argv[i+1])+1];
                strcpy (adapterName, argv[i+1]);
                setAdapterName = true;
                i = i+2;
            }*/
            if (0 == stricmp (argv[i], "-d")) {
                char *driverDescriptor = new char[strlen(argv[i+1])+1];
                strcpy (driverDescriptor, argv[i+1]);
                i = i+2;
                adapterName = getAdapterName (driverDescriptor);
                if (NULL == adapterName) {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_SevereError, 
                                    "Error in retrieving the Adapter Name for %s.\n", driverDescriptor);
                    return -5;
                }
            }
            else if (0 == stricmp (argv[i], "-a")) {
                if (isValidMAC (argv[i+1])) {
                    newMacAddr = new char[strlen(argv[i+1])+1];
                    strcpy (newMacAddr, argv[i+1]);
                    setMacAddr = true;
                    i = i+2;
                }
                else {
                    checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, 
                                    "MAC String %s is not valid. MAC addresses must be XX:XX:XX:XX:XX:XX with X in [0-9a-fA-F].\n", argv[i+1]);
                    return -3;
                }
            }
            else {
                checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "wrong argument.\n");
                showMacHelp();
                return -4;
            }
        }

        /*if (!setAdapterName) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "Adapter Name has not been set.\n");
            showMacHelp();
            return -5;
        }*/

        if (!setMacAddr) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "MAC address has not been set.\n");
            showMacHelp();
            return -6;
        }

        int rc = setMAC (adapterName, newMacAddr);
        if (rc != 0) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "Error while setting the mac address\n");
            return rc;
        }
        rc = resetAdapter (adapterName);
        if (rc != 0) {
            checkAndLogMsg ("TAP INTERFACE TEST - updateMacAddress: ", Logger::L_MildError, "Error while resetting the network adapter\n");
            return rc;
        }
        return 0;
    }

    /*
     * Check if the mac address is in the form m/^[0-9a-fA-F]{12}$/
     */
    bool TapInterfaceTestAndConfiguration::isValidMAC (char *mac) 
    {
        if (strlen (mac) != 17) {
            return false;
        }
        for (int i = 0; i < 17; i++) {
            if ((i % 3) ==2) {
                if (mac[i] != ':') {
                    return false;
                }
            }
            else {
                if ((mac[i] < '0' || mac[i] > '9') && (mac[i] < 'a' || mac[i] > 'f') && (mac[i] < 'A' || mac[i] > 'F')) {
                    return false;
                }
            }
        }
        return true;
    }

    /*
     * Show the help for the mac address changeing
     */
    void TapInterfaceTestAndConfiguration::showMacHelp() 
    {
        char message[218];
        strcat (message, "\nUsage:\n");
        //strcat (message, " -i [adapter-name]      The adapter name from Network Connections.\n");
        strcat (message, " -a [mac-address]       MAC address in the form XX:XX:XX:XX:XX:XX \n\t\t\twith X in [0-9a-fA-F].\n");
        strcat (message, " --help                 Shows this screen.\n\n");
        checkAndLogMsg ("showHelp ", Logger::L_Info, (const char *) message);
    }

    /*
     * Set the mac address of the component adapterName to the value of newMacAddr. 
     * It also sets the MTU registry to 1500 and the AllowNonAdmin registy to 1.
     */
    int TapInterfaceTestAndConfiguration::setMAC (char * adapterName, char * newMacAddr) 
    {
        HKEY hListKey = NULL;
        HKEY hKey = NULL;
        RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}",0, KEY_READ, &hListKey);
        if (!hListKey) {
            checkAndLogMsg ("TAP INTERFACE TEST - setMAC: ", Logger::L_MildError, "Failed to open adapter list key\n");
            return -7;
        }
        FILETIME writtenTime;
        char adapterKeyBuf[512], adapterNameBuf[512];
        DWORD adapterKeyBufSiz = 512;
        DWORD crap;
        int i = 0;
        bool found = false;
        while (RegEnumKeyEx (hListKey, i++, adapterKeyBuf, &adapterKeyBufSiz, 0, NULL, NULL, &writtenTime) == ERROR_SUCCESS) {
            _snprintf (adapterNameBuf, 512, "%s\\Connection", adapterKeyBuf);
            hKey = NULL;
            RegOpenKeyEx (hListKey, adapterNameBuf, 0, KEY_READ, &hKey);
            if (hKey) {
                adapterKeyBufSiz = 512;
                if (RegQueryValueEx (hKey, "Name", 0, &crap, (LPBYTE)adapterNameBuf, &adapterKeyBufSiz) == ERROR_SUCCESS && strcmp (adapterNameBuf, adapterName) == 0) {
                    checkAndLogMsg ("TAP INTERFACE TEST - setMAC: ", Logger::L_Info, "Adapter reg expr key is %s\n", adapterKeyBuf);
                    checkAndLogMsg ("TAP INTERFACE TEST - setMAC: ", Logger::L_Info, "Adapter name is %s\n", adapterNameBuf);
                    found = true;
                    break;
                }
                RegCloseKey (hKey);
            }
            adapterKeyBufSiz = 512;
        }
        RegCloseKey(hListKey);
        if (!found) {
            checkAndLogMsg ("TAP INTERFACE TEST - setMAC: ", Logger::L_MildError, 
                            "Could not find adapter name '%s'.\nPlease make sure this is the name you gave it in Network Connections.\n", 
                            adapterName);
            return -8;
        }
        RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}", 0, KEY_READ, &hListKey);
        if (!hListKey) {
            checkAndLogMsg ("TAP INTERFACE TEST - setMAC: ", Logger::L_MildError, "Failed to open adapter list key in Phase 2\n");
            return -9;
        }
        i = 0;
        char buf[512];
        while (RegEnumKeyEx (hListKey, i++, adapterNameBuf, &adapterKeyBufSiz, 0, NULL, NULL, &writtenTime) == ERROR_SUCCESS) {
            hKey = NULL;
            RegOpenKeyEx (hListKey, adapterNameBuf, 0, KEY_READ | KEY_SET_VALUE, &hKey);
            if (hKey) {
                adapterKeyBufSiz = 512;
                if ((RegQueryValueEx(hKey, "NetCfgInstanceId", 0, &crap, (LPBYTE)buf, &adapterKeyBufSiz) == ERROR_SUCCESS) && (strcmp (buf, adapterKeyBuf) == 0)) {
                    RegSetValueEx(hKey, "MAC", 0, REG_SZ, (LPBYTE)newMacAddr, strlen(newMacAddr) + 1);
                    RegSetValueEx(hKey, "AllowNonAdmin", 0, REG_SZ, (LPBYTE)"1", 2);
                    RegSetValueEx(hKey, "MediaStatus", 0, REG_SZ, (LPBYTE)"1", 2);
                    RegSetValueEx(hKey, "MTU", 0, REG_SZ, (LPBYTE)"1500", 5);
                }
                RegCloseKey(hKey);
            }
            adapterKeyBufSiz = 512;
        }
        RegCloseKey(hListKey);
        return 0;
    }

    /*
     * Reset the adapter to make the mac address changes effective
     */
    int TapInterfaceTestAndConfiguration::resetAdapter (char * adapterName) 
    {
        struct _GUID guid = {0xBA126AD1,0x2166,0x11D1,0};
        memcpy (guid.Data4, "\xB1\xD0\x00\x80\x5F\xC1\x27\x0E", 8);
        unsigned short * buf = new unsigned short[strlen (adapterName)+1];
  
        void (__stdcall *NcFreeNetConProperties) (NETCON_PROPERTIES *);
        HMODULE NetShell_Dll = LoadLibrary ("Netshell.dll");
        if (!NetShell_Dll) {
            checkAndLogMsg ("TAP INTERFACE TEST - resetAdapter: ", Logger::L_MildError, "Couldn't load Netshell.dll\n");
            return -10;
        }
        NcFreeNetConProperties = (void (__stdcall *)(struct tagNETCON_PROPERTIES *))GetProcAddress (NetShell_Dll, "NcFreeNetconProperties");
        if (!NcFreeNetConProperties) {
            checkAndLogMsg ("TAP INTERFACE TEST - resetAdapter: ", Logger::L_MildError, "Couldn't load required DLL function\n");
            return -11;
        }

        for (unsigned int i = 0; i <= strlen (adapterName); i++) {
            buf[i] = adapterName[i];
        }
        CoInitialize(0);
        INetConnectionManager * pNCM = NULL;    
        HRESULT hr = ::CoCreateInstance (guid, NULL, CLSCTX_ALL, __uuidof (INetConnectionManager), (void**) &pNCM);
        if (!pNCM)
            checkAndLogMsg ("TAP INTERFACE TEST - resetAdapter: ", Logger::L_MildError, "Failed to instantiate required object\n");
        else {
            IEnumNetConnection * pENC;
            pNCM->EnumConnections (NCME_DEFAULT, &pENC);
            if (!pENC) {
                checkAndLogMsg ("TAP INTERFACE TEST - resetAdapter: ", Logger::L_MildError, "Could not enumerate Network Connections\n");
            }
            else {
                INetConnection * pNC;
                ULONG fetched;
                NETCON_PROPERTIES * pNCP;
                do {
                    pENC->Next (1, &pNC, &fetched);
                    if (fetched && pNC) {
                        pNC->GetProperties (&pNCP);
                        if (pNCP) {
                            if (wcscmp (pNCP->pszwName, (const wchar_t *) buf) == 0) {
                                pNC->Disconnect();
                                pNC->Connect();
                            }
                            NcFreeNetConProperties (pNCP);
                        }
                    }
                } while (fetched);
                pENC->Release();
            }
            pNCM->Release();
        }

        FreeLibrary (NetShell_Dll);
        CoUninitialize ();
        return 0;
    }

}
