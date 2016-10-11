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
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#include "StringTokenizer.h"
#include "Logger.h"
#include "PCapInterface.h"
#include "ConfigurationParameters.h"
#include<iostream>

#if defined (WIN32)
    #define snprintf _snprintf
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

namespace IHMC_MISC
{
	PCapInterface::PCapInterface(const String &sDeviceName) : NetworkInterface(T_PCap)
    {
		_sAdapterName = sDeviceName;
        _pPCapHandle = NULL;
    }

    PCapInterface::~PCapInterface (void) // TO DO
    {
        requestTermination();
        if (_pPCapHandle != NULL) {
            pcap_close (_pPCapHandle);
            _pPCapHandle = NULL;
        }
    }

    PCapInterface * const PCapInterface::getPCapInterface (const char * const pszDevice,int msValidity)
    {
        int rc;
        pcap_if_t *pAllDevs;
        char errbuf[PCAP_ERRBUF_SIZE];

        if (pcap_findalldevs (&pAllDevs, errbuf) == -1) {
            checkAndLogMsg ("PCapInterface::getPCapInterface", Logger::L_MildError, "failed to enumerate interfaces\n");
            return NULL;
        }

        String sDeviceName (NetworkInterface::getDeviceNameFromUserFriendlyName (pszDevice));
        if (sDeviceName.length() <= 0) {
			checkAndLogMsg ("PCapInterface::getPCapInterface", Logger::L_MildError, "impossible to retrieve the network device name from the user friendly name %s \n", pszDevice);
			return NULL;
        }

        PCapInterface *pPCapInterface = new PCapInterface (sDeviceName);
		
        if (0 != (rc = pPCapInterface->init(msValidity)))
		{
            checkAndLogMsg ("PCapInterface::getPCapInterface", Logger::L_MildError, "failed to initialize PCapInterface; rc = %d\n", rc);
            delete pPCapInterface;
            return NULL;
        }
		
        return pPCapInterface;
    }

    int PCapInterface::init(int msPacketValid)
    {
		char szErrorBuf[PCAP_ERRBUF_SIZE];
		if (NULL == (_pPCapHandle = pcap_create(pszNetworkAdapterNamePrefix + _sAdapterName, szErrorBuf))) {
			checkAndLogMsg ("PCapInterface::init", Logger::L_MildError, "pcap_open_live() failed for device %s with error %s\n", _sAdapterName.c_str(), szErrorBuf);
            return -1;
        }

        pcap_set_snaplen (_pPCapHandle, 65535);
        pcap_set_promisc (_pPCapHandle, 1);
        //pcap_set_timeout (_pPCapHandle, msPacketValid); //ms di ritardo nella lettura
		pcap_set_timeout(_pPCapHandle, 1); //ms di ritardo nella lettura
        #if defined (LINUX)
            pcap_set_immediate_mode (_pPCapHandle, 1);
        #endif
        pcap_activate (_pPCapHandle);

        const uint8 * const pszMACAddr = NetworkInterface::getMACAddrForDevice (_sAdapterName);
		
		if (pszMACAddr) {
			memcpy(static_cast<void*> (NetworkInterface::_aui8MACAddr), pszMACAddr, 6);
			NetworkInterface::_bMACAddrFound = true;
            delete[] pszMACAddr;
        }
		else {
			checkAndLogMsg("PCapInterface::init", Logger::L_Warning, "NetSensor was not able to query the MAC address from the device\n");
		}

        retrieveAndSetIPv4Addr();
		IPv4Addr ipv4DefGW = NetworkInterface::getDefaultGatewayForInterface(_sAdapterName);
        if (ipv4DefGW.ui32Addr) {
            _bDefaultGatewayFound = true;
            _ipv4DefaultGateway = ipv4DefGW;
        }
        return 0;
    }

    bool PCapInterface::checkMACAddress (void)
    {
        if ((!_bMACAddrFound) || (!_bIPAddrFound))
		{
            return false;
        }
        uint8 ui8IPAddrOctet3, ui8IPAddrOctet4;
        const char *nextToken = NULL;
        InetAddr virtualIPAddr (NETSENSOR_IP_ADDR);
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

	int PCapInterface::restartHandler()
	{
		if (_pPCapHandle != NULL) {
			pcap_close(_pPCapHandle);
			_pPCapHandle = NULL;
		}
		if (0 != init(1)) {
			checkAndLogMsg("PCapInterface::ReadPacket", Logger::L_SevereError,
				"Failed to restart pcap handler\n");
			return -1;
		}
		else {
			checkAndLogMsg("PCapInterface::ReadPacket", Logger::L_Warning,
				"Handler Restarted\n");
			return 0;
		}

	}



    int PCapInterface::readPacket(uint8 *pui8Buf, uint16 ui16BufSize, int64 *tus)
    {
        struct pcap_pkthdr *pPacketHeader;
        const u_char *pPacketData;
        while (true) {
            int rc = pcap_next_ex (_pPCapHandle, &pPacketHeader, &pPacketData);
            if (rc > 0) {		
                uint32 ui32PacketSize = ui16BufSize < pPacketHeader->caplen ? ui16BufSize : pPacketHeader->caplen;
                memcpy (pui8Buf, pPacketData, ui32PacketSize);    
                *tus = pPacketHeader->ts.tv_usec + (pPacketHeader->ts.tv_sec * 1000000);
                return ui32PacketSize;
            }
            else if (rc == 0) {
                if (_bIsTerminationRequested) {         
                    return 0;
                }
            }
            else if (rc < 0) 
			{
				checkAndLogMsg ("PCapInterface::readPacket", Logger::L_MildError, "pcap_next_ex() returned %d\n", rc);
                return -1;
            }
        }
    }

    int PCapInterface::writePacket (const uint8 * const pui8Buf, uint16 ui16PacketLen)
    {
        _mWrite.lock();
        if (0 != pcap_sendpacket (_pPCapHandle, pui8Buf, ui16PacketLen)) 
		{
			checkAndLogMsg ("PCapInterface::writePacket", Logger::L_MildError, "pcap_sendpacket() failed writing a %hu bytes long packet: %s\n", ui16PacketLen, pcap_geterr (_pPCapHandle));
            _mWrite.unlock();
            return -1;
        }
        _mWrite.unlock();
        return ui16PacketLen;
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

        String adapterName = pszNetworkAdapterNamePrefix + _sAdapterName;
        for (pDevice = pAllDevs; pDevice != NULL; pDevice = pDevice->next) {
            if (adapterName ^= pDevice->name) {

                for (const pcap_addr_t *a = pDevice->addresses; a; a = a->next) {
                    switch (a->addr->sa_family) {
                    case AF_INET:
                        //printf ("Interface %s has address family name: AF_INET\n", pDevice->name);
                        if (a->addr) {
                            _ipv4Addr.ui32Addr = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;
                            _bIPAddrFound = true;
                            //printf ("\t\tIPv4 address: %s\n", InetAddr (_ipv4Addr.ui32Addr).getIPAsString());
                        }
                        if (a->netmask) {
                            _ipv4Netmask.ui32Addr = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;
                            _bNetmaskFound = true;
                            //printf ("\t\tNetmask: %s\n", InetAddr (_ipv4Netmask.ui32Addr).getIPAsString());
                        }
                        return;

                    case AF_INET6:
                        //printf ("Interface %s has address family name: AF_INET6\n", pDevice->name);
                        break;

                    default:
                        printf ("Interface %s has address family name unknown\n", pDevice->name);
                    }
                }
            }
        }
    }
    



}
