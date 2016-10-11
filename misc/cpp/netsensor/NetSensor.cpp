/*
 * NetSensor.cpp
 *
 * This file is part of the IHMC NetSensor Library/Component
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

/*
* NetSensor.cpp
*
* Author:                   Roberto Fronteddu
* Year of creation:         2015/2016
* Last Revision by:
* Year of last Revision:
* Version: 1.0
*/

#if defined (WIN32)
    #include <stdio.h>
	#include <winsock2.h>
	#include <Winbase.h>
    #define snprintf _snprintf
#define PROTOBUF_USE_DLLS
#elif defined (LINUX)
    #include <stdio.h>
	//#include <time.h>
	#include <sys/time.h>
    #include <sys/types.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>
    #include <linux/if_tun.h>
    #include <fcntl.h>
    #include <errno.h>
#endif

#ifdef ANDROID
	#include <arpa/inet.h>
	#include <linux/if.h>
	#include <linux/in.h>
	#include <linux/sockios.h>
	#include <stdint.h>
#endif

#include "Logger.h"
#include "NetSensor.h"
#include "DataStructures.h"

#include "traffic.pb.h"
#include "topology.pb.h"

#include "container.pb.h"

#include "ConfigManager.h"


#include "NLFLib.h"
#include "PCapInterface.h"
#include "TimeIntervalAverage.h"
#include <fstream>
#include <sstream>
#include <iostream>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;
using namespace ddam;

namespace IHMC_MISC
{
    int NetSensor::addToPossibleGwTable(String mac, bool bIsLax)
    {    
		if (bIsLax) {
			IpPerGW *pa = NULL;
			if (NULL == (pa = _gwTable.get(mac))) {
				checkAndLogMsg("NetSensor::AddGWLax", Logger::L_HighDetailDebug, "Added a new entry in the GW table: %s\n", mac.c_str());
				pa = new IpPerGW();
				pa->_IpEntry = "ND";
				_gwTable.put(mac, pa);
			}
			return 0;
		}
		else {
			IpPerGW *pa = NULL;
			if (NULL == (pa = _gwTable.get(mac))) {
				checkAndLogMsg("NetSensor::addToPossibleGwTable", Logger::L_HighDetailDebug, "New possible MAC address found : %s\n", mac.c_str());
				pa = new IpPerGW;
				_gwTable.put(mac, pa);
			}
			return 0;
		}
    }

    void NetSensor::buildEthernetMACAddressFromString(NOMADSUtil::EtherMACAddr &eMACAddrToUpdate, const uint8 * pszMACAddr)
    {
        eMACAddrToUpdate.ui8Byte1 = pszMACAddr[0];
        eMACAddrToUpdate.ui8Byte2 = pszMACAddr[1];
        eMACAddrToUpdate.ui8Byte3 = pszMACAddr[2];
        eMACAddrToUpdate.ui8Byte4 = pszMACAddr[3];
        eMACAddrToUpdate.ui8Byte5 = pszMACAddr[4];
        eMACAddrToUpdate.ui8Byte6 = pszMACAddr[5];

		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Device mac is is %02X:%02X:%02X:%02X:%02X:%02X\n",
			eMACAddrToUpdate.ui8Byte1,
			eMACAddrToUpdate.ui8Byte2,
			eMACAddrToUpdate.ui8Byte3,
			eMACAddrToUpdate.ui8Byte4,
			eMACAddrToUpdate.ui8Byte5,
			eMACAddrToUpdate.ui8Byte6);
    }

    NetSensor::CleanerThread::CleanerThread() {
    }

    NetSensor::CleanerThread::~CleanerThread() {
		_pNetSensor = NULL;
        checkAndLogMsg("~Cleaner", Logger::L_Info, "Cleaner Thread terminated\n");
    }

    int NetSensor::commonConfiguration()
    { 
		_HandlerThread._pNetSensor = this;
		_RTTHandlerThread._pNetSensor = this;
		_CleanerThread._pNetSensor = this; 

        _trafficContainer = ddam::Container::default_instance();
		_topologyContainer = ddam::Container::default_instance();
        _topologyInstance = ddam::TopologyParts::default_instance();

		if ((pLocalGwIP1 != NULL) || (pLocalMAC1 != NULL)) {
			setMacAdrrOfDefaultGW(pLocalMAC1, pLocalGwIP1);
		}

		if ((pLocalGwIP2 != NULL) || (pLocalMAC2 != NULL)) {
			setMacAdrrOfDefaultGW(pLocalMAC2, pLocalGwIP2);
		}
        return 0;
    }

    int NetSensor::configureForStandAlone()
    {
        int rc = 0;
		checkAndLogMsg("NetSensor::configureForStandAlone;", Logger::L_Info, " Stand-Alone Mode\n");
        if (_cInterface == NULL) {
            checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError, "external interface name not specified\n", rc);
            return -1;
        }
		//MS_PACKET_VALID INSTEAD OF 0
        if (NULL == (_pNetInterface = PCapInterface::getPCapInterface(_cInterface, 1))) {
            checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError, " getPCapInterface(); Could not open external network interface <%s>\n", _cInterface);
            return -2;
        }

        //run-time checks to see that pcap is working correctly
        if (NETSENSOR_IP_ADDR == 0) {
            // Try and retrieve the IP address of the network interface by querying the device itself
            NETSENSOR_IP_ADDR = _pNetInterface->getIPv4Addr() ? _pNetInterface->getIPv4Addr()->ui32Addr : 0;

            if (NETSENSOR_IP_ADDR == 0) {
                checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError, "could not determine IP address of network interface <%s>\n", _cInterface);
                delete _pNetInterface;
                return -3;
            }
            else {
                checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_Info, "retrieved IP address %s for network interface <%s>\n", InetAddr(NETSENSOR_IP_ADDR).getIPAsString(), _cInterface);
            }

            const uint8  *pszExternalMACAddr = NULL;

	#ifndef ANDROID
            if (NULL == (pszExternalMACAddr = _pNetInterface->getMACAddr())) {
                checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
                    "could not obtain MAC address for network interface <%s>\n\n", _cInterface);
                delete _pNetInterface;
                return -4;
            }
	
			buildEthernetMACAddressFromString(_emacNetSensorMAC, pszExternalMACAddr);
	#endif
			// Retrieve IPv4 Netmask
        if (NETSENSOR_NETWORK_NETMASK == 0) {
                // Try and retrieve netmask from the device itself
                NETSENSOR_NETWORK_NETMASK = _pNetInterface->getNetmask() ? _pNetInterface->getNetmask()->ui32Addr : 0;
                if (NETSENSOR_NETWORK_NETMASK == 0) {
                    checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
                        "could not determine Netmask of the network interface\n");
                    delete _pNetInterface;
                    return -10;
                }
                else {
                    checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_Info,
                        "retrieved netmask %s for network interface <%s>\n\n",
                        InetAddr(NETSENSOR_NETWORK_NETMASK).getIPAsString(),
                        _cInterface);
                }
            }
        }

        const IPv4Addr *gwAddr = NULL;
        if (NULL == (gwAddr = _pNetInterface->getDefaultGateway())) {
            checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_Info,
                "getDefaultGateway(); Could not retrieve gwDefaultIPAddr for network interface <%s>, NetSensor will try to obtain that on his own\n\n",
                _cInterface);
            _uint32GwIPAddr = 0;
        }
        else {
            _uint32GwIPAddr = gwAddr->ui32Addr;
        }

        const uint8 *tmpMAC = NULL;
        if (NULL == (tmpMAC = _pNetInterface->getMACAddr())) {
            checkAndLogMsg("NetSensor:configureForStandAlone", Logger::L_Info,
                " getMACAddr(); Could not retrieve gwDefaultMACaddr for network interface <%s>, NetSensor will try to obtain that on his own\n\n",
                _cInterface);

			#ifdef ANDROID
				checkAndLogMsg("NetSensor::init", Logger::L_Info,
					"Trying to get mac android mode...\n");
				struct ifreq *ifr;
				struct ifconf ifc;
				int s, i;
				int numif;

				// find number of interfaces. 
				memset(&ifc, 0, sizeof(ifc));
				ifc.ifc_ifcu.ifcu_req = NULL;
				ifc.ifc_len = 0;

				if ((s = ::socket(PF_INET, SOCK_STREAM, 0)) < 0) {
					checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
						"Could not obtain socket!\n");
					return -5;
				}

				if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
					checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
						"ioctl SIOCGIFCONF error!\n");
					return -6;
				}

				if ((ifr = (ifreq*)malloc(ifc.ifc_len)) == NULL) {
					checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
						"Could not malloc ifreq!\n");
					return -7;
				}

				ifc.ifc_ifcu.ifcu_req = ifr;

				if (ioctl(s, SIOCGIFCONF, &ifc) < 0) {
					checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
						"ioctl SIOCGIFCONF error!\n");
					return -8;
				}

				numif = ifc.ifc_len / sizeof(struct ifreq);

				for (i = 0; i < numif; i++) {
					struct ifreq *r = &ifr[i];
					struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
					if (strcmp(r->ifr_name, _cInterface))
						continue; // skip loopback interface 

					// get MAC address 
					if (ioctl(s, SIOCGIFHWADDR, r) < 0) {
						checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_SevereError,
							"ioctl(SIOCGIFHWADDR) error!\n");
						return -9;
					}

					char macaddr[18];
					sprintf(macaddr, " %02X:%02X:%02X:%02X:%02X:%02X",
						(unsigned char)r->ifr_hwaddr.sa_data[0],
						(unsigned char)r->ifr_hwaddr.sa_data[1],
						(unsigned char)r->ifr_hwaddr.sa_data[2],
						(unsigned char)r->ifr_hwaddr.sa_data[3],
						(unsigned char)r->ifr_hwaddr.sa_data[4],
						(unsigned char)r->ifr_hwaddr.sa_data[5]);

					_emacNetSensorMAC.ui8Byte1 = (unsigned char)r->ifr_hwaddr.sa_data[0];
					_emacNetSensorMAC.ui8Byte2 = (unsigned char)r->ifr_hwaddr.sa_data[1];
					_emacNetSensorMAC.ui8Byte3 = (unsigned char)r->ifr_hwaddr.sa_data[2];
					_emacNetSensorMAC.ui8Byte4 = (unsigned char)r->ifr_hwaddr.sa_data[3];
					_emacNetSensorMAC.ui8Byte5 = (unsigned char)r->ifr_hwaddr.sa_data[4];
					_emacNetSensorMAC.ui8Byte6 = (unsigned char)r->ifr_hwaddr.sa_data[5];

					checkAndLogMsg("NetSensor::init", Logger::L_Info, "MAC is %02X:%02X:%02X:%02X:%02X:%02X\n",
						_emacNetSensorMAC.ui8Byte1,
						_emacNetSensorMAC.ui8Byte2,
						_emacNetSensorMAC.ui8Byte3,
						_emacNetSensorMAC.ui8Byte4,
						_emacNetSensorMAC.ui8Byte5,
						_emacNetSensorMAC.ui8Byte6);
					/*
					checkAndLogMsg("NetSensor::init", Logger::L_Info, "MAC of %s is %02X:%02X:%02X:%02X:%02X:%02X\n", _cInterface,
						(unsigned char)r->ifr_hwaddr.sa_data[0],
						(unsigned char)r->ifr_hwaddr.sa_data[1],
						(unsigned char)r->ifr_hwaddr.sa_data[2],
						(unsigned char)r->ifr_hwaddr.sa_data[3],
						(unsigned char)r->ifr_hwaddr.sa_data[4],
						(unsigned char)r->ifr_hwaddr.sa_data[5]);
					buildEthernetMACAddressFromString(_emacNetSensorMAC, (uint8*)macaddr);
					*/
				}
				close(s);

				free(ifr);
			#else
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "NetSensor MAC set to 0\n");
				_emacNetSensorMAC.ui8Byte1 = 0;
				_emacNetSensorMAC.ui8Byte2 = 0;
				_emacNetSensorMAC.ui8Byte3 = 0;
				_emacNetSensorMAC.ui8Byte4 = 0;
				_emacNetSensorMAC.ui8Byte5 = 0;
				_emacNetSensorMAC.ui8Byte6 = 0;


			#endif
        }
        else {
            _emacNetSensorMAC.ui8Byte1 = tmpMAC[0];
            _emacNetSensorMAC.ui8Byte2 = tmpMAC[1];
            _emacNetSensorMAC.ui8Byte3 = tmpMAC[2];
            _emacNetSensorMAC.ui8Byte4 = tmpMAC[3];
            _emacNetSensorMAC.ui8Byte5 = tmpMAC[4];
            _emacNetSensorMAC.ui8Byte6 = tmpMAC[5];
        }

		_emacProxyInternalMAC.ui8Byte1 = 0;
		_emacProxyInternalMAC.ui8Byte2 = 0;
		_emacProxyInternalMAC.ui8Byte3 = 0;
		_emacProxyInternalMAC.ui8Byte4 = 0;
		_emacProxyInternalMAC.ui8Byte5 = 0;
		_emacProxyInternalMAC.ui8Byte6 = 0;

		_emacProxyExternalMAC.ui8Byte1 = 0;
		_emacProxyExternalMAC.ui8Byte2 = 0;
		_emacProxyExternalMAC.ui8Byte3 = 0;
		_emacProxyExternalMAC.ui8Byte4 = 0;
		_emacProxyExternalMAC.ui8Byte5 = 0;
		_emacProxyExternalMAC.ui8Byte6 = 0;
		if (_bBuiltInMode) {
			checkAndLogMsg("NetSensor::configureForStandAlone", Logger::L_Info, "Receiver Threads will be started later\n");
		}
		else {
			_ReceiverThread._pNetInterface = _pNetInterface;
			_ReceiverThread._pNetSensor = this;
		}


        return 0;
    }

	int NetSensor::configureInternalInterface()
	{
		int rc = 0;
		//strcpy(_cInternalInterface, "eth1");
		if (_cInternalInterface == NULL) {
			checkAndLogMsg("NetSensor::configureInternalInterface", Logger::L_SevereError, "external interface name not specified in netsensor cfg\n", rc);
			return -1;
		}

		if (NULL == (_pInternalNetInterface = PCapInterface::getPCapInterface(_cInternalInterface, MS_PACKET_VALID))) {
			checkAndLogMsg("NetSensor::configureInternalInterface", Logger::L_SevereError, 
				" getPCapInterface(); Could not open  network interface <%s>\n", _cInternalInterface);
			return -2;
		}

		_InternalReceiverThread._pNetInterface = _pInternalNetInterface;
		_InternalReceiverThread._pNetSensor = this;
		_InternalReceiverThread.isInternal = true;

		checkAndLogMsg("NetSensor::configureInternalInterface", Logger::L_Info, "Internal Receiver Thread launched\n");
		return 0;
	}
	
	int NetSensor::configureExternalInterface()
	{
		int rc = 0;
		if (_cInterface == NULL) {
			checkAndLogMsg("NetSensor::configureExternalInterface()", Logger::L_SevereError, "external interface name not specified\n", rc);
			return -1;
		}

		if (NULL == (_pNetInterface = PCapInterface::getPCapInterface(_cInterface, MS_PACKET_VALID))) {
			checkAndLogMsg("NetSensor::configureExternalInterface", Logger::L_SevereError, " getPCapInterface(); Could not open external network interface <%s>\n", _cInterface);
			return -2;
		}
	
		_ReceiverThread._pNetInterface = _pNetInterface;
		_ReceiverThread._pNetSensor = this;
		_ReceiverThread.isInternal = false;
		return 0;
	}

    int NetSensor::cleanTablesInternalExternal(const int64 i64CurrTime, const int64 i64CleaningTime)
    {
        UsefullStatus status = INTERNAL;
        NOMADSUtil::UInt32Hashtable<Mac> *supportTablePointer = NULL;
        while (status == END) {
            switch (status) {
            case INTERNAL:
                status = EXTERNAL;
                supportTablePointer = &_internalTrafficStats;
                break;
            case EXTERNAL:
                status = END;
                supportTablePointer = &_externalTrafficStats;
                break;
            default:
                checkAndLogMsg("NetSensor::CleanerThread", Logger::L_Info, "cleanTablesInternalExternal() anomalous status.\n");
                return -1;
            }
            
            uint32 ui32DirtyAddr[CLEANING_PER_CICLE];
            int numberOfDirtyEntries = 0;
            Mac *pa = NULL;

            for (UInt32Hashtable<Mac>::Iterator i = supportTablePointer->getAllElements(); !i.end(); i.nextElement()) {
                pa = i.getValue();
                if ((pa->i64TimeOfLastChange - i64CurrTime) > i64CleaningTime) {
                    ui32DirtyAddr[numberOfDirtyEntries] = i.getKey();
                    if (numberOfDirtyEntries < CLEANING_PER_CICLE)
                        numberOfDirtyEntries++;
                }
                if (numberOfDirtyEntries >= CLEANING_PER_CICLE)
                    break;
            }

            //delete dirty entries
            for (int index = 0; index < numberOfDirtyEntries; index++) {
                supportTablePointer->remove(ui32DirtyAddr[index]);
            }
        }
        return 0;
    }

	int NetSensor::cleanTablesTraffic(const int64 i64CurrTime, const int64 i64CleaningTime)
    {
        uint32 ui32DirtyAddrS1[CLEANING_PER_CICLE];
        uint32 ui32DirtyAddrS2[CLEANING_PER_CICLE];
        uint32 dirtyProtocol[CLEANING_PER_CICLE];
        uint32 dirtyport[CLEANING_PER_CICLE];

        int numberOfDirtyEntries = 0;

        PerNodeTrafficStatsIP1          *p1 = NULL; //SOURCE
        PerNodeTrafficStatsIP2          *p2 = NULL; //DEST
        PerNodeTrafficStatsbyProtocol   *p3 = NULL; //PROTOCOL LEVEL
        PerNodeTrafficStats             *p4 = NULL; //PORT/ENTRIES LEVEL

		for (UInt32Hashtable<PerNodeTrafficStatsIP1>::Iterator i = _trafficTable.getAllElements(); !i.end(); i.nextElement()) {
            p1 = i.getValue();
            ui32DirtyAddrS1[numberOfDirtyEntries] = i.getKey();
            for (UInt32Hashtable<PerNodeTrafficStatsIP2>::Iterator ii = p1->_IPB.getAllElements(); !ii.end(); ii.nextElement()) {
                p2 = ii.getValue();
                ui32DirtyAddrS2[numberOfDirtyEntries] = ii.getKey();
                for (UInt32Hashtable<PerNodeTrafficStatsbyProtocol>::Iterator iii = p2->_byProtocol.getAllElements(); !iii.end(); iii.nextElement()) {
                    p3 = iii.getValue();
                    dirtyProtocol[numberOfDirtyEntries] = iii.getKey();
                    for (UInt32Hashtable<PerNodeTrafficStats>::Iterator iv = p3->_PerNodeTrafficStatsbyPort.getAllElements(); !iv.end(); iv.nextElement()) {
                        p4 = iv.getValue();
						if ((p4->tiaFiveSecs.getAverage() == 0) && (p4->tiaOneMinute.getAverage() == 0)) {
							dirtyport[numberOfDirtyEntries] = iv.getKey();
							numberOfDirtyEntries++;
						}
						if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
							break;
						}
                    }
					if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
						break;
					}
                }
				if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
					break;
				}
            }
			if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
				break;
			}
        }

        //delete dirty entries
        for (int index = 0; index < numberOfDirtyEntries; index++) {
			UInt32Hashtable<PerNodeTrafficStatsIP1>         p1 = _trafficTable.get(ui32DirtyAddrS1[index]);
            UInt32Hashtable<PerNodeTrafficStatsIP2>         p2 = p1.get(ui32DirtyAddrS2[index]);
            UInt32Hashtable<PerNodeTrafficStatsbyProtocol>  p3 = p2.get(dirtyProtocol[index]);

            p3.remove(dirtyport[index]);
            if (p3.getCount() == 0) {
                p2.remove(dirtyProtocol[index]);
                if (p2.getCount() == 0) {
                    p3.remove(ui32DirtyAddrS2[index]);
                    if (p1.getCount() == 0) {
						_trafficTable.remove(ui32DirtyAddrS1[index]);
                    }
                }
            }
        }
		if (numberOfDirtyEntries > 0) {
			checkAndLogMsg("NetSensor::cleanTablesTraffic", Logger::L_LowDetailDebug, "Deleted %d obsolete entries from traffic table\n", numberOfDirtyEntries);
		}

        return 0;
    }

    int NetSensor::cleanTablesMAC(const int64 i64CurrTime, const int64 i64CleaningTime)
    {
        SourceMac *pa;
        DestMac *pb;
        String sources[CLEANING_PER_CICLE];
        String dests[CLEANING_PER_CICLE];
        int numberOfDirtyEntries = 0;

        for (StringHashtable<SourceMac>::Iterator i = _macTrafficStats.getAllElements(); !i.end(); i.nextElement()) {
            pa = i.getValue();
            for (StringHashtable<DestMac>::Iterator ii = pa->_destMac.getAllElements(); !ii.end(); ii.nextElement()) {
                pb = ii.getValue();
                if ((i64CurrTime - pb->i64TimeOfLastChange) > i64CleaningTime) {
                    sources[numberOfDirtyEntries] = i.getKey();
                    dests[numberOfDirtyEntries] = ii.getKey();
                    numberOfDirtyEntries++;
                }
                if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
                    break;
                }
            }
            if (numberOfDirtyEntries >= CLEANING_PER_CICLE) {
                break;
            }
        }

        //delete dirty entries
        for (int index = 0; index < numberOfDirtyEntries; index++) {
            SourceMac *pa;
            pa = _macTrafficStats.get(sources[index]);
            pa->_destMac.remove(dests[index]);
            if (pa->_destMac.getCount() == 0)
            {
                _macTrafficStats.remove(dests[index]);
            }
        }
        return 0;
    }

    int NetSensor::cleanTableRoundTrip(const int64 i64CurrTime, const int64 i64CleaningTime)
    {
        int numberOfDirtyEntries = 0;
        uint32 ui32DirtyAddrS[300];
        uint32 ui32DirtyAddrD[300];
        uint32 ui32Dirtyid[300];
        uint32 ui32Dirtyseq[300];

        RoundTripStat *p1;
        DestField *p2;
        PerID *p3;
        PerSequenceNumber *p4;

        for (UInt32Hashtable<RoundTripStat>::Iterator i = _roundTripStatTable.getAllElements(); !i.end(); i.nextElement()) {
            p1 = i.getValue();
            for (NOMADSUtil::UInt32Hashtable<DestField>::Iterator ii = p1->_PerDest.getAllElements(); !ii.end(); ii.nextElement()) {
                p2 = ii.getValue();
                for (NOMADSUtil::UInt32Hashtable<PerID>::Iterator iii = p2->_perID.getAllElements(); !iii.end(); iii.nextElement()) {
                    p3 = iii.getValue();
                    for (NOMADSUtil::UInt32Hashtable<PerSequenceNumber>::Iterator iiii = p3->_PerSequenceNumber.getAllElements(); !iiii.end(); iiii.nextElement()) {
                        p4 = iiii.getValue();

                        if ((i64CurrTime - p4->i64TimeOfLastChange) >  i64CleaningTime) {
                            ui32DirtyAddrS[numberOfDirtyEntries] = i.getKey();
                            ui32DirtyAddrD[numberOfDirtyEntries] = ii.getKey();
                            ui32Dirtyid[numberOfDirtyEntries] = iii.getKey();
                            ui32Dirtyseq[numberOfDirtyEntries] = iiii.getKey();
                            if (numberOfDirtyEntries < CLEANING_PER_CICLE)
                                numberOfDirtyEntries++;
                            else {
                                checkAndLogMsg("NetSensor::CleanerThread::ClearTables::cleanTableRoundTrip", Logger::L_HighDetailDebug, "Number of maximum entry to clean reached");
                                break;
                            }
                        }
                    }
                    if (numberOfDirtyEntries >= CLEANING_PER_CICLE)
                        break;
                }
                if (numberOfDirtyEntries >= CLEANING_PER_CICLE)
                    break;
            }
        }

        p1 = NULL;
        p2 = NULL;
        p3 = NULL;
        p4 = NULL;

        for (int counter = 0; counter < numberOfDirtyEntries; counter++) {
            p1 = _roundTripStatTable.get(ui32DirtyAddrS[counter]);
            p2 = p1->_PerDest.get(ui32DirtyAddrD[counter]);
            p3 = p2->_perID.get(ui32Dirtyid[counter]);
            p3->_PerSequenceNumber.remove(ui32Dirtyseq[counter]);

            if (p3->_PerSequenceNumber.getCount() == 0) {
                p2->_perID.remove(ui32Dirtyid[counter]);
                if ((p2->_perID.getCount()) == 0) {
                    p1->_PerDest.remove(ui32DirtyAddrD[counter]);
                    if (p1->_PerDest.getCount() == 0) {
                        _roundTripStatTable.remove(ui32DirtyAddrS[counter]);
                    }
                }
            }
        }
        return 0;
    }

    int NetSensor::clearTables(const int64 i64CurrTime)
    {
        int rc;
		int starvingCounter = 0;
        bool bRtt_CleaningHandled = false;
        bool bMac_CleaningHandled = false;
        bool bTraffic_CleaningHandled = false;
        bool bIE_CleaningHandled = false;

        while (!bMac_CleaningHandled && !bRtt_CleaningHandled && !bIE_CleaningHandled) {

            if (_bTopology) {
                if (!bIE_CleaningHandled) {
                    if (_IETrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {             
                        if ((rc = cleanTablesInternalExternal(i64CurrTime, _uint32Cleaning_time_ie)) < 0) {
                            checkAndLogMsg("NetSensor::CleanerThread::clearTables", Logger::L_Warning,
                                "cleanTablesInternalExternal() failed; rc = %d\n", rc);
                            _IETrafficStatsMUTEX.unlock();
                            return rc;
                        }
                        bIE_CleaningHandled = true;
                        _IETrafficStatsMUTEX.unlock();
                    }
                }
            }
            else {
                bIE_CleaningHandled = true;
            }

            if (_bTraffic) {
                if (!bTraffic_CleaningHandled) {
                    if (_IOTrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
						if ((rc = cleanTablesTraffic(i64CurrTime, _uint32Cleaning_time_t)) < 0) {
                            checkAndLogMsg("NetSensor::CleanerThread::clearTables", Logger::L_Warning,
                                "cleanTablesLink() failed; rc = %d\n", rc);
                            _IOTrafficStatsMUTEX.unlock();
                            return rc;
                        }
                        bTraffic_CleaningHandled = true;
                        _IOTrafficStatsMUTEX.unlock();
                    }
					else {
						starvingCounter++;
							if (starvingCounter > 10000) {
								checkAndLogMsg("NetSensor::clearTables", Logger::L_Warning, "IO mutex is starving\n");
								starvingCounter = 0;
								sleepForMilliseconds(100);
							}

					}
                    
                }
            }
            else {
                bTraffic_CleaningHandled = true;
            }
           
            if (_bMac) {
                if (!bMac_CleaningHandled) {
                    if (_macStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
                        if ((rc = cleanTablesMAC(i64CurrTime, _uint32Cleaning_time_mac)) < 0) {
                            checkAndLogMsg("NetSensor::CleanerThread::clearTables", Logger::L_Warning,
                                "cleanTablesMAC() failed; rc = %d\n", rc);
                            _macStatsMUTEX.unlock();
                            return rc;
                        }
                        bMac_CleaningHandled = true;
                        _macStatsMUTEX.unlock();
                    }
                }
            }
            else {
                bMac_CleaningHandled = true;
            }

            if (_bRtt) {
                if (!bRtt_CleaningHandled) {
                    if (_RTTrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
                        if ((rc = cleanTableRoundTrip(i64CurrTime, _uint32Cleaning_time_rt)) < 0) {
                            checkAndLogMsg("NetSensor::CleanerThread::clearTables", Logger::L_Warning,
                                "cleanTableRoundTrip() failed; rc = %d\n", rc);
                            _RTTrafficStatsMUTEX.unlock();
                            return rc;
                        }
                        bRtt_CleaningHandled = true;
                        _RTTrafficStatsMUTEX.unlock();
                    }
                }
            }
            else {
                bRtt_CleaningHandled = true;
            }

            sleepForMilliseconds(100);
        }
        return 0;
    }

    DestField::DestField(void)
        : _perID(true)
    {
    }

    DestMac::DestMac(void)
        : tiaFiveSecs(5000), tiaOneMinute(60000), i64TimeOfLastChange(0)
    {

    }

	int NetSensor::handlePacket(const int classification, const uint8 * const pPacket, uint16 ui16PacketLen, const int64 i64CurrTime)
    {
        int rc = 0;

        EtherFrameHeader * const pEthHeader = (EtherFrameHeader*)pPacket;
		pEthHeader->ntoh();
		//pEthHeader->ntoh();

		bool bIOHandled = false;
		bool bGwArpHandled = false;
		bool bGwLaxHandled = false;
		bool bIEHandled = false;
		bool bMUHandled = false;
		bool bMACHandled = false;
		int lockCount = 0;

        //Check if all the tables have been populated
		while (!bIEHandled && !bMUHandled && !bGwArpHandled && !bGwLaxHandled && !bMACHandled) {
			if (_bMac) {
                if (!bMACHandled) {
                    if (_macStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
                        if ((rc = populateMACtrafficTable(i64CurrTime, pPacket, pEthHeader, ui16PacketLen)) < 0) {
                            checkAndLogMsg("NetSensor::handlePacketFromInterface", Logger::L_Warning, "populateMACtrafficTable() failed; rc = %d\n", rc);
                            return rc;
                        }
                        _macStatsMUTEX.unlock();
                        bMACHandled = true;
                    }
                }
            }
            else {
                bMACHandled = true;
            }

            //Is this an IP or ARP packet?
            if ((pEthHeader->ui16EtherType == ET_IP) || (pEthHeader->ui16EtherType == ET_ARP)) {
				if (_bGwArpMechanism) {			
					if (!bGwArpHandled) {
                        if ((rc = lookForGWandPrepareARPCACHE(i64CurrTime, pPacket, ui16PacketLen)) < 0) {
                            checkAndLogMsg("NetSensor::handlePacketFromInterface", Logger::L_Warning,
                                "lookForGWandPrepareARPCACHE() failed; rc = %d\n", rc);
                            return rc;
                        }
						bGwArpHandled = true;
                    }
                }

				if (_bTopology && _bLaxTopology) {
					if ((rc = laxTopologyMechanism(i64CurrTime, pPacket, ui16PacketLen)) < 0) {
						checkAndLogMsg("NetSensor::handlePacketFromInterface", Logger::L_Warning,
							"laxTopologyMechanism() failed; rc = %d\n", rc);
						return rc;
					}
					bGwLaxHandled = true;
				}
				else {
					bGwLaxHandled = true;
				}
			}
            else {
				bGwLaxHandled = true;
				bGwArpHandled = true;
            }

            //Is this an IP packet?
            if (pEthHeader->ui16EtherType == ET_IP) {
                IPHeader *pIPHeader = (IPHeader*)(pPacket + sizeof(EtherFrameHeader));
                uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
                uint8 uint8Protocol = pIPHeader->ui8Proto;

                //Check if we have to handle this table
				if (_bTraffic) {
                    //do this only once per cycle
                    if (!bIOHandled) {
						
						if (_IOTrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
							if (_IETrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
								if ((rc = populateTrafficTable(i64CurrTime, pPacket, pEthHeader, pIPHeader, ui16IPHeaderLen, uint8Protocol, ui16PacketLen, classification)) < 0) {
									checkAndLogMsg("NetSensor::HandlerThread::handlePacketFromInterface", Logger::L_Warning,
										"populateIncomingAndOutgoingTables() failed; rc = %d\n", rc);
									_IOTrafficStatsMUTEX.unlock();
									_IETrafficStatsMUTEX.unlock();

									return rc;
								}
								
								_IETrafficStatsMUTEX.unlock();
								//table has been handled
								bIOHandled = true;
							}
							_IOTrafficStatsMUTEX.unlock();
                        }
						else {
							lockCount++;
							if (lockCount == 10000) {
								lockCount = 0;

								checkAndLogMsg("NetSensor::HandlerThread::handlePacketFromInterface", Logger::L_Warning,
									"couldn't get IO and IE locks");
								sleepForMilliseconds(100);
							}
							
						}
                    }
                }
                else {
                    //If not set the handled flag
                    bIOHandled = true;
                }

                //Check if we have to andle this table
                if (_bTopology) {
                    if (!bIEHandled) {
                        if (_IETrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
                            if ((rc = populateInternalAndExternalTables(i64CurrTime, classification, pPacket, pEthHeader, pIPHeader, ui16PacketLen)) < 0) {
                                checkAndLogMsg("NetSensor::HandlerThread::handlePacketFromInterface", Logger::L_Warning,
                                    "populateInternalAndExternalTables() failed; rc = %d\n", rc);
                                _IETrafficStatsMUTEX.unlock();
                                return rc;
                            }
                            _IETrafficStatsMUTEX.unlock();
                            bIEHandled = true;
                        }
                    }
                }
                else {
                    //If not set the handled flag
                    bIEHandled = true;
                }
            }
            else {
                //If not we don't need to populate IO, IE, and MU tables
                bIOHandled = true;
                bIEHandled = true;
                bMUHandled = true;
            }
        }
        return 0;
    }
    
    NetSensor::HandlerThread::HandlerThread() {
    }
    
    NetSensor::HandlerThread::~HandlerThread() 
	{
		_pNetSensor = NULL;
        checkAndLogMsg("~HandlerThread", Logger::L_Info, "Handler Thread terminated\n");
    }
  
	void NetSensor::handleLocalNetProxyAddresses(uint32 uint32Ip)
	{
		String localNetProxyMAC = " ";
		ddam::Host *localGW = _topologyInstance.add_localgws();
		localGW->set_mac(localNetProxyMAC);

		NOMADSUtil::IPv4Addr ip;
		ip.ui32Addr = uint32Ip;
		ip.ntoh();

		localGW->set_ip(ip.ui32Addr);
		if (_bPrintTopology) {
			checkAndLogMsg("NetSensor::protoTopology", Logger::L_HighDetailDebug, "GW: Local NP: IP: %s, MAC: %s\n", InetAddr(uint32Ip).getIPAsString(), localNetProxyMAC.c_str());
		}
	}

	void NetSensor::handleRemoteNetProxyAddresses(uint32 uint32Ip)
	{
		ddam::Host *remoteGW = _topologyInstance.add_remotegws();
		remoteGW->set_mac("ND");
		NOMADSUtil::IPv4Addr ip;
		ip.ui32Addr = uint32Ip;
		ip.ntoh();
		remoteGW->set_ip(ip.ui32Addr);
		if (_bPrintTopology) {
			checkAndLogMsg("NetSensor::protoTopology", Logger::L_HighDetailDebug, "GW: Remote NP: IP: %s, MAC: ND\n", InetAddr(uint32Ip).getIPAsString());
		}
	}

    void NetSensor::HandlerThread::run()
    {
        //Set started variable
        started();
		checkAndLogMsg("NetSensor::HandlerThread::run", Logger::L_Info, "Handler thread started\n");
        int rc;
		int counter = 0;
		int starvationCounter = 0;
		int emptyQueueTest = 0;
        //Check termination
        while (!terminationRequested()) {     
            //Check if there are packets in the reading queue
			if (isRunning()) {
				if (_pNetSensor->_packetQueue.isEmpty()) {
					emptyQueueTest++;
					if (emptyQueueTest == 1000) {
						checkAndLogMsg("NetSensor::HandlerThread::run", Logger::L_Warning, "Queue is not being filled?\n");
						emptyQueueTest = 0;
					}			
					sleepForMilliseconds(100);
				}
				else {
					
					if (_pNetSensor->_QueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
						
						int64 i64CurrTime = getTimeInMilliseconds();
						while (!_pNetSensor->_packetQueue.isEmpty()) {
							NET_SENSOR::PacketStructure packet;
							_pNetSensor->_packetQueue.dequeue(packet);
							//if (false) {
								if ((rc = _pNetSensor->handlePacket(packet.classification, packet.ui8Buf, (uint16)packet.received, i64CurrTime)) < 0) {
									checkAndLogMsg("NetSensor::HandlerThread::run", Logger::L_SevereError, "Handle Packet From Interface failed\n");
								}
							//}

						}
						_pNetSensor->_QueueMUTEX.unlock();
					}
				}
            }
			else {
				//If not sleep
				checkAndLogMsg("NetSensor::HandlerThread::run", Logger::L_Warning, "Handler thread is not running\n");
				sleepForMilliseconds(100);
			}
        }
        checkAndLogMsg("NetSensor::HandlerThread::run", Logger::L_Info, " Handler Thread is terminating\n");
        terminating();
    }

	int NetSensor::setDefaultStandAloneConfigurationValues(const char* cpcNetworkInterface)
	{
		checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues()", Logger::L_Info, "Setting standard configurations\n");
		
		//IP
		strcpy(_cIp, NOTIFIED_IP);
		checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "Notified IP: %s\n", _cIp);

		//PORT
		_uint32Port = NOTIFIED_PORT;
		checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "Notified PORT: %d\n", _uint32Port);

		//LOCAL INTERFACE
		strcpy(_cInterface, cpcNetworkInterface);
		checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "Sniffing Interface: %s\n", _cInterface);

	//Enabled features
		//INCOMING_OUTGOING_STAT
		_bTraffic = TRAFFIC_DETECTION_ENABLED;

		//GW DETECTION MECHANISMS
		_bGwArpMechanism = ARP_GW_DETECTION_MECHANISM_ENABLED;

		//TOPOLOGY
		_bTopology = TOPOLOGY_DETECTION_ENABLED;
		_bLaxTopology = LAX_TOPOLOGY_DETECTION_ENABLED;
		_bArpTopology = ARP_TOPOLOGY_DETECTION_;
		_bNMaskTopology = NETMASK_TOPOLOGY_DETECTION_ENABLED;
		_bNPSpecialTopology = NETPROXY_TOPOLOGY_DETECTION;

		//MAC_STAT
		_bMac = TRAFFIC_BY_MAC_DETECTION_ENABLED;
		
		//ROUND_TRIP_STAT
		_bRtt = RTT_DETECTION_ENABLED;

		//COUNT MULTICAST
		_bDisableMulticastCount = TRAFFIC_MULTICAST_DETECTION_DISABLED;

	//TIMINGS
		//STAT_UPDATE_TIME
		_uint32Stat_update_time = STAT_UPDATE_TIME;
		//CLEAN_TIME
		_uint32Clean_time = 500;
		//CLEANING_TIME_IO
		_uint32Cleaning_time_t = CLEANING_TIME_IO;
		//CLEANING_TIME_IE
		_uint32Cleaning_time_ie = CLEANING_TIME_IE;
		//CLEANING_TIME_RT
		_uint32Cleaning_time_rt = CLEANING_TIME_RT;
		//CLEANING_TIME_MAC
		_uint32Cleaning_time_mac = CLEANING_TIME_MAC;
		//CLEANING_TIME_MC
		_uint32Cleaning_time_mc = CLEANING_TIME_MC;

		//DEBUG MODE
		_bPrintTopology = DEBUG_MODE;
		_bPrintTraffic = DEBUG_MODE;
		_bWriteOnFile = DEBUG_MODE;

		//OTHERS
		_uint32Mtu = PACKETMAXSIZE;

		//DEBUG LEVEL
		pLogger->setDebugLevel(DEBUG_LEVEL);

		switch (pLogger->getDebugLevel()) {
			case 1:
				checkAndLogMsg("\nNetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Severe Error Message\n");
				break;
			case 2:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Mild Error Messag\n");
				break;
			case 3:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Warning\n");
				break;
			case 4:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Info\n");
				break;
			case 5:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Net\n");
				break;
			case 6:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Low Detail Debug Message\n");
				break;
			case 7:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: Medium Detail Debug Message\n");
				break;
			case 8:
				checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Debug level: High Detail Debug Message\n");
				break;
		}

		if (_bTraffic) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Traffic\n");
		}
		if (_bTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Topology\n");
		}
		int count = 0;
		if (_bLaxTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Lax Topology\n");
			count++;
		}
		if (_bArpTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); ARP Topology\n");
			count++;
		}
		if (_bNMaskTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); NETMASK Topology\n");
			count++;
		}
		if (_bNPSpecialTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); NetProxy Topology\n");
			count++;
		}
		if (_bMac) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Mac\n");
		}
		if (_bRtt) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); RoundTrip\n");
		}
		if (_bDisableMulticastCount) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Do not count multicast\n");
		}
		if (_bWriteOnFile) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Write on file\n");
		}
		if (_bPrintTopology) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Print Topology\n");
		}
		if (_bPrintTraffic) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_Info, "readCfgFile(); Print Traffic\n");
		}
		//CONFIGURATION LOGIC CHECK
		if (_bNPSpecialTopology && !_bBuiltInMode) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_SevereError, "You can't use NPSpecial algorithm without being a built in component!\n");
			return -1;
		}
		if (count > 1) {
			checkAndLogMsg("NetSensor::setDefaultStandAloneConfigurationValues", Logger::L_SevereError, "You are not supposed to use more than one topology detection mechanism!\n");
			return -2;
		}
		return 0;
	}

    int NetSensor::initAsAComponent(const char *configFile)
    {
        _bBuiltInMode = true;
        return init(configFile);
    }

	int NetSensor::init(const int mode, const char *cpcParameter) {
		int rc;
		if (mode == 0) {
			//cpcParameter is interface
			if ((rc = setDefaultStandAloneConfigurationValues(cpcParameter)) != 0) {
				checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " setDefaultStandAloneConfigurationValues() failed; rc = %d\n", rc);
				return -1;
			}
		}
		
		if (mode == 1) {
			//cpcParameter is cfg file
			if ((rc = readCfgFile(cpcParameter)) != 0) {
				checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " readCfgFile() failed; rc = %d\n", rc);
				return -1;
			}
		}

		// create notification socket
		if (NULL == (_pNotifierSocket = new UDPDatagramSocket())) {
			checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " UDPDatagramSocket() failed;\n");
			return -2;
		}

		// Initialize notification socket
		if (0 != (rc = _pNotifierSocket->init())) {
			checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " _pNotifierSocket->init() failed to initialize UDPDatagramSocket; rc = %d\n", rc);
			return -3;
		}

		//set up addr and port
		_notifyAddr = InetAddr(_cIp, _uint32Port);

		if (!_bBuiltInMode) {
			//Stand alone mode
			rc = configureForStandAlone();
			if (rc != 0) {
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "configureForStandAlone() failed; rc = %d\n", rc);
				return -4;
			}
		}
		else {
			// no need for the receiving thread
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Built-in Mode\n");
		}

		if ((rc = commonConfiguration()) != 0) {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "commonConfiguration() failed; rc = %d\n", rc);
			return -5;
		}

		return 0;
	}

    int NetSensor::init(const char *configFile)
    {
        int rc = 0;

        if (readCfgFile(configFile) != 0) {
            checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " readCfgFile() failed; rc = %d\n", rc);
            return -1;
        }

        // create notification socket
        if (NULL == (_pNotifierSocket = new UDPDatagramSocket())) {
            checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " UDPDatagramSocket() failed;\n");
            return -2;
        }
        
		// Initialize notification socket
        if (0 != (rc = _pNotifierSocket->init())) {
            checkAndLogMsg("NetSensor::init", Logger::L_SevereError, " _pNotifierSocket->init() failed to initialize UDPDatagramSocket; rc = %d\n", rc);
            return -3;
        }

        //set up addr and port
        _notifyAddr = InetAddr(_cIp, _uint32Port);

        if (_bBuiltInMode) {
			// no need for the receiving thread
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Built-in Mode\n");
			_forwardedIP = new NOMADSUtil::LList < uint32 >;

			configureInternalInterface();
			configureExternalInterface();
        }
        else {
			//Stand alone mode
			rc = configureForStandAlone();
			if (rc != 0) {
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "configureForStandAlone() failed; rc = %d\n", rc);
				return -4;
			}
        }
        
		rc = commonConfiguration();
        if (rc != 0) {
            checkAndLogMsg("NetSensor::init", Logger::L_Info, "commonConfiguration() failed; rc = %d\n", rc);
            return -5;
        }

        return 0;
    }
   
	int NetSensor::internalExternalNetProxyTopologyHelper(EtherFrameHeader * const pEthHeader, char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime, const int classification)
	{
		NOMADSUtil::UInt32Hashtable<Mac> *supportTable;
		IpPerGW				*pa = NULL;
		IpEntry				*pb = NULL;
		IpEntry             *p4 = NULL;
		PerNodeTrafficStats *p5 = NULL;
		Mac                 *p6 = NULL;

		if ((classification == 0) || (classification == 1)) {
			//printf("DEBUG: sourceMac: %s, sourceIP: %s, classification %d\n", cSourceMac, InetAddr(ui32SourceAddr).getIPAsString(), classification);
			if (classification == 0) {
				//we see the packet in the internal interface
				if ((pEthHeader->dest == _emacProxyExternalMAC) || (pEthHeader->dest == _emacProxyInternalMAC)) {
					//the packet doesn't come from the netproxy
					supportTable = &_internalTrafficStats;
				}
				else {
					//don't count the packet
					return 0; 
				}
			}			
			if (classification == 1) {
				//we see the packet in the external interface	
				if (((ui32SourceAddr == _uint32NetProxyExternalInterfaceIp) || (pEthHeader->src != _emacProxyExternalMAC) && (pEthHeader->src != _emacProxyInternalMAC))) {
					//consider only the netproxy external interface and all the external ip that doesn't originate from netproxy
					supportTable = &_externalTrafficStats;
				}
				else {
					//if ((pEthHeader->src == _emacProxyExternalMAC) || (pEthHeader->src != _emacProxyInternalMAC)) {
					//	supportTable = &_internalTrafficStats;
					//}

					//don't count the packet
					return 0; 
				}
			}	
			
			if ((p6 = supportTable->get(ui32SourceAddr)) == NULL) {
				p6 = new Mac();
				p6->mac = cSourceMac;
				p6->i64TimeOfLastChange = i64CurrTime;
				supportTable->put(ui32SourceAddr, p6);
			}
			p6->i64TimeOfLastChange = i64CurrTime;
		}
		return 0;
	}

	int NetSensor::internalExternalNetMaskTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime)
	{
		bool internal = false;
		bool external = false;
		NOMADSUtil::UInt32Hashtable<Mac> *supportTable;
		IpPerGW *pa = NULL;
		IpEntry *pb = NULL;
		IpEntry             *p4 = NULL;
		PerNodeTrafficStats *p5 = NULL;
		Mac                 *p6 = NULL;

		//check if packet source IP is in subnet range
		((NETSENSOR_IP_ADDR & NETSENSOR_NETWORK_NETMASK) == (ui32SourceAddr & NETSENSOR_NETWORK_NETMASK)) ? (supportTable = &_internalTrafficStats) : (supportTable = &_externalTrafficStats);

		if ((p6 = supportTable->get(ui32SourceAddr)) == NULL) {
			p6 = new Mac();
			p6->mac = cSourceMac;
			p6->i64TimeOfLastChange = i64CurrTime;
			supportTable->put(ui32SourceAddr, p6);
		}
		p6->i64TimeOfLastChange = i64CurrTime;

		return 0;
	}

	int NetSensor::internalExternalArpTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime)
	{
		bool internal = false;
		bool external = false;
		NOMADSUtil::UInt32Hashtable<Mac> *supportTable;
		IpPerGW *pa = NULL;
		IpEntry *pb = NULL;
		IpEntry             *p4 = NULL;
		PerNodeTrafficStats *p5 = NULL;
		Mac                 *p6 = NULL;

		if (_gwTable.getCount() >= 1) {
			if (_bDefaultGwAccountedFor) {
				uint32 uint32SupportAddr = 0; //may be the gw or the local np address

				//do we have a local NP addres?
				if (_uint32Local_NP_IP == 0) {
					//no --> the local address is the closest point out of the network
					uint32SupportAddr = _uint32GwIPAddr;
				}
				else {
					//Yes, we are the closest point out of the local network
					uint32SupportAddr = _uint32Local_NP_IP;
				}

				//check if Source MAC is the same of supportAddr, note that since we are in the internal network the only entry we can found is the one from the GW
				if (NULL == (pa = _gwTable.get(cSourceMac))) {
					internal = true;
					external = false;
				}
				else {
					//MAC is the one of the GW what about IP?
					if (ui32SourceAddr != uint32SupportAddr) {
						//no
						internal = false;
						external = true;
					}
				}

				if (internal) {
					supportTable = &_internalTrafficStats;
				}
				if (external) {
					supportTable = &_externalTrafficStats;
				}
				if (internal || external) {
					if ((p6 = supportTable->get(ui32SourceAddr)) == NULL) {
						p6 = new Mac();
						p6->mac = cSourceMac;
						p6->i64TimeOfLastChange = i64CurrTime;
						supportTable->put(ui32SourceAddr, p6);
					}
					p6->i64TimeOfLastChange = i64CurrTime;
				}
			}
		}
		return 0;
	}

	int NetSensor::internalExternalLaxTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime)
	{
		bool internal = false;
		bool external = false;
		NOMADSUtil::UInt32Hashtable<Mac> *supportTable;
		IpPerGW *pa = NULL;
		IpEntry *pb = NULL;
		IpEntry             *p4 = NULL;
		PerNodeTrafficStats *p5 = NULL;
		Mac                 *p6 = NULL;

		if (_gwTable.getCount() >= 1) {
			if (_bDefaultGwAccountedFor) {
				uint32 uint32SupportAddr = 0; //may be the gw or the local np address

				//do we have a local NP addres?
				if (_uint32Local_NP_IP == 0) {
					//no --> the local address is the closest point out of the network
					uint32SupportAddr = _uint32GwIPAddr;
				}
				else {
					//Yes, we are the closest point out of the local network
					uint32SupportAddr = _uint32Local_NP_IP;
				}

				//check if Source MAC is the same of supportAddr, note that since we are in the internal network the only entry we can found is the one from the GW
				if (NULL == (pa = _gwTable.get(cSourceMac))) {
					internal = true;
					external = false;
				}
				else {
					//MAC is the one of the GW what about IP?
					if (ui32SourceAddr != uint32SupportAddr) {
						//no
						internal = false;
						external = true;
					}
				}

				if (internal) {
					supportTable = &_internalTrafficStats;
				}
				if (external) {
					supportTable = &_externalTrafficStats;
				}
				if (internal || external) {
					if ((p6 = supportTable->get(ui32SourceAddr)) == NULL) {
						p6 = new Mac();
						p6->mac = cSourceMac;
						p6->i64TimeOfLastChange = i64CurrTime;
						supportTable->put(ui32SourceAddr, p6);
					}
					p6->i64TimeOfLastChange = i64CurrTime;
				}
			}
		}

		return 0;
	}

    IpEntry::IpEntry(void)
    {
    }

    IpPerGW::IpPerGW(void)
        :_IpEntry(true)
    {
    }

	bool NetSensor::isInternal(uint32 uint32addr)
	{
		if (_internalTrafficStats.get(uint32addr)) {
			return true;
		}
		return false;
	}

	bool NetSensor::isInMulticastRange(const int ui32DestAddr)
	{
		IPv4Addr timpIP;
		timpIP.ui32Addr = ui32DestAddr;
		timpIP.ntoh();
		if ((_minMulticastAddress <= timpIP.ui32Addr) && (timpIP.ui32Addr <= _maxMulticastAddress)) {
			return true;
		}
		return false;
	}

    int NetSensor::lookForMACwithMultipleIps(IPHeader *pIPHeader, EtherFrameHeader * pEthHeader)
    {
        int rc = 0;
        macPerIp *pa = NULL;
        uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;

        char supportMac[30];
        sprintf(supportMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, 
			pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
        
        String destMAC = supportMac;
        if (NULL == (pa = _ipsPerMacTable.get(destMAC))) {
            pa = new macPerIp();
            pa->ips.add(ui32DestAddr);
            _ipsPerMacTable.put(destMAC, pa);
        }
        else {
            if (pa->ips.search(ui32DestAddr) == 0)
                pa->ips.add(ui32DestAddr);
        }

        if (pa->ips.getCount() > GWTHRESHOLD) {
            rc = addToPossibleGwTable(destMAC);
        }
        return rc;
    }

	int NetSensor::laxTopologyMechanism(const int64 i64CurrTime, const uint8 * const pPacket, uint16 ui16PacketLen) 
	{
		int rc = 0;
		IpEntry *p4 = NULL;
		EtherFrameHeader * const pEthHeader = (EtherFrameHeader*)pPacket;
		IPHeader *pIPHeader = (IPHeader*)(pPacket + sizeof(EtherFrameHeader));
		if (pEthHeader->ui16EtherType == ET_IP) {
			uint32 uint32SupportAddress;
			char cSupportMac[30];
			String sSupportMAC;

			for (int count = 0; count < 2; count++) {
				if (count == 0) {
					sprintf(cSupportMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3,
						pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6);
					sSupportMAC = cSupportMac;
					uint32SupportAddress = pIPHeader->srcAddr.ui32Addr;
					laxTopologyHelper(sSupportMAC, uint32SupportAddress);
				}
				if (count == 1) {
					sprintf(cSupportMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3,
						pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
					sSupportMAC = cSupportMac;
					uint32SupportAddress = pIPHeader->srcAddr.ui32Addr;
					laxTopologyHelper(sSupportMAC, uint32SupportAddress);
				}
			}
		}
		return 0;
	}

	int NetSensor::laxTopologyHelper(NOMADSUtil::String sMac, uint32 uint32Ip)
	{
		macPerIp *pa = NULL;
		if (NULL == (pa = _ipsPerMacTable.get(sMac))) {
			pa = new macPerIp();
			pa->ips.add(uint32Ip);
			_ipsPerMacTable.put(sMac, pa);
		}
		else {
			if (pa->ips.search(uint32Ip) == 0)
				pa->ips.add(uint32Ip);
		}

		if (pa->ips.getCount() > GWTHRESHOLD) {
			addToPossibleGwTable(sMac, true);
		}
		return 0;
	}

    int NetSensor::lookForGWandPrepareARPCACHE(const int64 i64CurrTime, const uint8 * const pPacket, uint16 ui16PacketLen)
    {
        int rc = 0;
        IpEntry *p4 = NULL;
        EtherFrameHeader * const pEthHeader = (EtherFrameHeader*)pPacket;
        IPHeader *pIPHeader = (IPHeader*)(pPacket + sizeof(EtherFrameHeader));

        if (pEthHeader->ui16EtherType == ET_IP) {
            if (rc = lookForMACwithMultipleIps(pIPHeader, pEthHeader) != 0) {
                return rc;
            }
        }

        if (pEthHeader->ui16EtherType == ET_ARP) {
            ARPPacket *pARPPacket = (ARPPacket*)(pPacket + sizeof(EtherFrameHeader));

            //Update the ARP Cache and check mac match to find GWs ip
            if (rc = updateArpChacheAndCheckForMatch(pARPPacket) != 0) {
                return rc;
            }
        }
        return 0;
    }

    Mac::Mac(void) 
    {
    }

    macPerIp::macPerIp(void)
    {
    }

    NetSensor::NetSensor(void)
        :
        _uint32Local_NP_IP(0),
        _uint32R1_NP_IP(0),
        _uint32R2_NP_IP(0),
        _uint32GwIPAddr(0),
		_uint32ExternalInterfaceGwIp(0)
    {
		_bTraffic = false;
		_bTopology = false;
		_bMac = false;
		_bRtt = false;

		_bWriteOnFile = false;
        _bDefaultGWalreadyAccountedFor = false;
		_bGwArpMechanism = false;
		_bArpTopology = false;
		_bLaxTopology = false;
		_bNMaskTopology = false;
		_bNPSpecialTopology = false;

		_bPrintTopology = false;
        _pNetInterface = NULL;
		_bBuiltInMode = false;
		_bPrintTraffic = false;
		_bDisableMulticastCount = false;
		_uint32listNetProxyRemoteAddress = NULL;
		_forwardedIP = NULL;
		pLocalGwIP1 = NULL;
		pLocalGwIP2 = NULL;
		pLocalMAC1 = NULL;
		pLocalMAC2 = NULL;
		pDefaultIp = NULL;
		pDefaulMac = NULL;
		pLocalGwName1 = NULL;
		pLocalGwName2 = NULL;
		_pProxyInternalMacAddr = NULL;
		_pProxyExternalMacAddr = NULL;
        _pNotifierSocket = NULL;
        _int64StartTime = getTimeInMilliseconds();	

		//standalone
		_minMulticastAddress = InetAddr("0.0.0.224").getIPAddress();
		_maxMulticastAddress = InetAddr("255.255.255.239").getIPAddress();

    }
    
    NetSensor::~NetSensor(void)
    {
        if (_pNetInterface != NULL) {
			delete[] _pNetInterface;
            _pNetInterface = NULL;
        }
        if (_pNotifierSocket != NULL) {
			delete[] _pNotifierSocket;
            _pNotifierSocket = NULL;
        }

		if (pLocalGwIP1 != NULL) {
			delete[](pLocalGwIP1);
		}

		if (pLocalGwIP2 != NULL) {
			delete[](pLocalGwIP2);
		}

		if (pLocalMAC1 != NULL) {
			delete[](pLocalMAC1);
		}

		if (pLocalMAC2 != NULL) {
			delete[](pLocalMAC2);
		}

		if (pLocalGwName1 != NULL) {
			delete[](pLocalGwName1);
		}

		if (pLocalGwName2 != NULL) {
			delete[](pLocalGwName2);
		}

		if (_pProxyInternalMacAddr != NULL) {
			delete[](_pProxyInternalMacAddr);
		}

		if (_pProxyExternalMacAddr != NULL) {
			delete[](_pProxyExternalMacAddr);
		}

        google::protobuf::ShutdownProtobufLibrary();
    }
   
	int NetSensor::netSensorIteration(const uint8 ui8Buf[9038U], int received, int classification)
    {
        //TODO: add a flag to check if the caller has ntoed the packet
		NET_SENSOR::PacketStructure packet;
        memcpy(packet.ui8Buf, ui8Buf, received);
        packet.received = received;
		packet.classification = classification;
        int done = false;
        while (!done) {
			printf("SHOULD NOT BE RUNNING!!!!\n");
            if (_QueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
                _packetQueue.enqueue(packet);
                _QueueMUTEX.unlock();
                done = true;
            }
        }
        return 0;
    }
    
    int NetSensor::netSensorIterationRTT(const uint8 ui8Buf[9038U], int received, int64 usTime) {
        //TODO: add a flag to check if the caller has ntoed the packet
		NET_SENSOR::PacketStructure packet;
        memcpy(packet.ui8Buf, ui8Buf, sizeof(ui8Buf));
        packet.received = received;
        packet.receivedTimeStamp = usTime;
        int doneRTT = false;
        while (!doneRTT) {
            if (_RTTQueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
                _RTTpacketQueue.enqueue(packet);
                _RTTQueueMUTEX.unlock();
                doneRTT = true;
            }
        }
        return 0;
    }

    PerID::PerID(void)
    {
    }

    PerNodeTrafficStats::PerNodeTrafficStats(void)
        : tiaNumberOfPacketsInFiveSec(5000), tiaFiveSecs(5000), tiaOneMinute(60000), i64TimeOfLastChange(0)
    {
    }

    PerNodeTrafficStatsbyProtocol::PerNodeTrafficStatsbyProtocol(void)
        : _PerNodeTrafficStatsbyPort(true)
    {
    }

    PerNodeTrafficStatsContainer::PerNodeTrafficStatsContainer(void)
        : _PerNodeTrafficStatsbyProtocol(true)
    {
    }

    PerNodeTrafficStatsIP1::PerNodeTrafficStatsIP1(void)
        : _IPB(true)
    {
    }

    PerNodeTrafficStatsIP2::PerNodeTrafficStatsIP2(void)
        : _byProtocol(true)
    {
    }

    PerNodeTrafficStatsIPA::PerNodeTrafficStatsIPA(void)
        : _PerNodeTrafficStatsbyIPB(true)
    {
    }

    PerSequenceNumber::PerSequenceNumber(void)
    {
    }

	int NetSensor::populateInternalAndExternalTables(const int64 i64CurrTime, const int classification, const uint8 * const pPacket, EtherFrameHeader * const pEthHeader, 
		IPHeader *pIPHeader, uint16 ui16PacketLen)
	{
		IpPerGW *pa = NULL;
		IpEntry *pb = NULL;

		IpEntry             *p4 = NULL;
		PerNodeTrafficStats *p5 = NULL;
		Mac                 *p6 = NULL;

		bool internal = false;
		bool external = false;

		char cSourceMac[30];
		sprintf(cSourceMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, 
			pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6);
		
		String sSourceMac = cSourceMac;
		pIPHeader->ntoh();
		pIPHeader->ntoh();
		uint32 ui32SourceAddr = pIPHeader->srcAddr.ui32Addr;

		if (!_bBuiltInMode) {
			//standalone
			if (_bLaxTopology) {
				internalExternalLaxTopologyHelper(cSourceMac, ui32SourceAddr, i64CurrTime);
			}

			if (_bArpTopology) {
				internalExternalArpTopologyHelper(cSourceMac, ui32SourceAddr, i64CurrTime);
			}

			if (_bNMaskTopology) {
				internalExternalNetMaskTopologyHelper(cSourceMac, ui32SourceAddr, i64CurrTime);
			}
		}
		else {
			if (_bNPSpecialTopology) {
				internalExternalNetProxyTopologyHelper(pEthHeader, cSourceMac, ui32SourceAddr, i64CurrTime, classification);
			}
		}
		return 0;
	}
	
	int NetSensor::populateTrafficTable(const int64 i64CurrTime, const uint8 * const pPacket, NOMADSUtil::EtherFrameHeader *pEtHeader, NOMADSUtil::IPHeader *pIPHeader, 
		uint16 ui16IPHeaderLen, uint8 uint8Protocol, uint16 ui16PacketLen, int classification)
    {
        pIPHeader->ntoh();
        pIPHeader->ntoh();
        uint32 ui32SrcAddr = pIPHeader->srcAddr.ui32Addr;
        uint32 ui32DestAddr = pIPHeader->destAddr.ui32Addr;
        uint16 ui16DestPort = 0;
        
		if (_bDisableMulticastCount && isInMulticastRange(ui32DestAddr)) {
			checkAndLogMsg("NetSensor::populateTrafficTable", Logger::L_HighDetailDebug, "Ignoring multicast packet as configured\n");
			return 0;
		}
		
		UDPHeader *pUDPHeader = NULL;
        TCPHeader *pTCPHeader = NULL;
        ICMPHeader *pICMPHeader = NULL;
        IGMPV2Header *pIGMPHeader = NULL;

        switch (uint8Protocol) {
        case IP_PROTO_UDP:
			
			pUDPHeader = (UDPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
			pUDPHeader->hton();
            ui16DestPort = pUDPHeader->ui16DPort;
			pUDPHeader->ntoh();
            break;

        case IP_PROTO_TCP:
            pTCPHeader = (TCPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
            pTCPHeader->hton();
            ui16DestPort = pTCPHeader->ui16DPort;
            pTCPHeader->ntoh();
            break;

        case IP_PROTO_ICMP:
            pICMPHeader = (ICMPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
            pICMPHeader->hton();
            ui16DestPort = 0;
            pICMPHeader->ntoh();
            break;

        case IP_PROTO_IGMP:
            ui16DestPort = 0;
            break;

        default:
            checkAndLogMsg("NetSensor::HandlingThread::populateTrafficTable", Logger::L_HighDetailDebug,
                "Packed ignored: Protocol: %d unsupported\n", uint8Protocol);
            return 0;
        }
		
		if (!_bBuiltInMode) {
			//standalone mode
			populateTrafficTableHelper(ui32SrcAddr, ui32DestAddr, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
			if (pEtHeader->src == _emacNetSensorMAC) {
				populateTrafficTableHelper(0, ui32DestAddr, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
			}
			if (pEtHeader->dest == _emacNetSensorMAC || isInMulticastRange(ui32DestAddr)) {
				populateTrafficTableHelper(ui32SrcAddr, 0, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
			}
		}
		else {
			if (classification == 0) {
				//traffic in the internal network		
				populateTrafficTableHelper(ui32SrcAddr, ui32DestAddr, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
			}

			//EI
			if (classification == 1) {
				//traffic in the external network	
				if ((ui32SrcAddr == _uint32NetProxyExternalInterfaceIp) || (ui32DestAddr == _uint32NetProxyExternalInterfaceIp)) {
					populateTrafficTableHelper(ui32SrcAddr, ui32DestAddr, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
				}

				if (pEtHeader->src == _emacProxyExternalMAC) {
					bool countIt = false;
					//packet generated by the external interface
					//if (_IETrafficStatsMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
						if (!_internalTrafficStats.contains(ui32DestAddr)) {
							countIt = true;			
						}
					//	_IETrafficStatsMUTEX.unlock();
					//}

					if (countIt && (ui32DestAddr != _uint32NetProxyExternalInterfaceIp)) {
						populateTrafficTableHelper(0, ui32DestAddr, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
					}
				}

				if (pEtHeader->dest == _emacProxyExternalMAC && pEtHeader->src != _emacProxyExternalMAC) {
					bool countIt = false;
					//packet generated by the external interface
					//if (_IETrafficStatsMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
						if (!_internalTrafficStats.contains(ui32SrcAddr)) {
							countIt = true;
						}
					//	_IETrafficStatsMUTEX.unlock();
					
					//}

					//packet headed to the external interface
					if (countIt) {
						populateTrafficTableHelper(ui32SrcAddr, 0, uint8Protocol, ui16DestPort, ui16PacketLen, i64CurrTime);
					}
				}
			}
		}
		return 0;
    }

	int NetSensor::populateTrafficTableHelper(uint32 ui32SrcAddr, uint32 ui32DestAddr, 
		uint8 uint8Protocol, uint16 ui16DestPort, uint16 ui16PacketLen, const int64 i64CurrTime) 
	{
		//_trafficTable
		PerNodeTrafficStatsIP1          *p1 = NULL; //SOURCE
		PerNodeTrafficStatsIP2          *p2 = NULL; //DEST
		PerNodeTrafficStatsbyProtocol   *p3 = NULL; //PROTOCOL LEVEL
		PerNodeTrafficStats             *p4 = NULL; //PORT/ENTRIES LEVEL
		
		if (NULL == (p1 = _trafficTable.get(ui32SrcAddr))) {
			//no source address
			p1 = new PerNodeTrafficStatsIP1();
			p2 = new PerNodeTrafficStatsIP2();
			p3 = new PerNodeTrafficStatsbyProtocol();
			p4 = new PerNodeTrafficStats();

			p4->tiaFiveSecs.add(ui16PacketLen);
			p4->tiaOneMinute.add(ui16PacketLen);
			p4->tiaNumberOfPacketsInFiveSec.add(1);
			p4->i64TimeOfLastChange = i64CurrTime;

			p3->_PerNodeTrafficStatsbyPort.put(ui16DestPort, p4);
			p2->_byProtocol.put(uint8Protocol, p3);
			p1->_IPB.put(ui32DestAddr, p2);
			_trafficTable.put(ui32SrcAddr, p1);
		}
		else {
			if (NULL == (p2 = p1->_IPB.get(ui32DestAddr))) {
				//no dest address
				p2 = new PerNodeTrafficStatsIP2();
				p3 = new PerNodeTrafficStatsbyProtocol();
				p4 = new PerNodeTrafficStats();

				p4->tiaFiveSecs.add(ui16PacketLen);
				p4->tiaOneMinute.add(ui16PacketLen);
				p4->tiaNumberOfPacketsInFiveSec.add(1);
				p4->i64TimeOfLastChange = i64CurrTime;

				p3->_PerNodeTrafficStatsbyPort.put(ui16DestPort, p4);
				p2->_byProtocol.put(uint8Protocol, p3);
				p1->_IPB.put(ui32DestAddr, p2);
			}
			else {
				if (NULL == (p3 = p2->_byProtocol.get(uint8Protocol))) {
					//no protocol
					p3 = new PerNodeTrafficStatsbyProtocol();
					p4 = new PerNodeTrafficStats();

					p4->tiaFiveSecs.add(ui16PacketLen);
					p4->tiaOneMinute.add(ui16PacketLen);
					p4->tiaNumberOfPacketsInFiveSec.add(1);
					p4->i64TimeOfLastChange = i64CurrTime;

					p3->_PerNodeTrafficStatsbyPort.put(ui16DestPort, p4);
					p2->_byProtocol.put(uint8Protocol, p3);
				}
				else {
					if (NULL == (p4 = p3->_PerNodeTrafficStatsbyPort.get(ui16DestPort))) {
						//no port
						p4 = new PerNodeTrafficStats();

						p4->tiaFiveSecs.add(ui16PacketLen);
						p4->tiaOneMinute.add(ui16PacketLen);
						p4->tiaNumberOfPacketsInFiveSec.add(1);
						p4->i64TimeOfLastChange = i64CurrTime;

						p3->_PerNodeTrafficStatsbyPort.put(ui16DestPort, p4);
					}
					else {
						//Needed to reduce load on sending phase	
						uint32 avgS = p4->tiaFiveSecs.getAverage();
						uint32 avgM = p4->tiaOneMinute.getAverage();
						if (avgS > 4000000000) {
							checkAndLogMsg("NetSensor::prepareAndSendUpdates()", Logger::L_Warning, "tiaFiveSec is dangerously high...\n");
						}
						if (avgM > 4000000000) {
							checkAndLogMsg("NetSensor::prepareAndSendUpdates()", Logger::L_Warning, "tiaOneMinute is dangerously high...\n");
						}

						p4->tiaFiveSecs.add(ui16PacketLen);
						p4->tiaOneMinute.add(ui16PacketLen);
						p4->tiaNumberOfPacketsInFiveSec.add(1);
						p4->i64TimeOfLastChange = i64CurrTime;
					}
				}
			}
		}
		return 0;
	}

    int NetSensor::populateMACtrafficTable(const int64 i64CurrTime, const uint8 * const pPacket, 
		EtherFrameHeader * const pEthHeader, uint16 ui16PacketLen)
    {
        SourceMac *pa;
        DestMac *pb;
        char supportMac[30];

        sprintf(supportMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->src.ui8Byte1, pEthHeader->src.ui8Byte2, pEthHeader->src.ui8Byte3, pEthHeader->src.ui8Byte4, pEthHeader->src.ui8Byte5, pEthHeader->src.ui8Byte6);
        String sourceMAC = supportMac;

        sprintf(supportMac, "%02x:%02x:%02x:%02x:%02x:%02x", pEthHeader->dest.ui8Byte1, pEthHeader->dest.ui8Byte2, pEthHeader->dest.ui8Byte3, pEthHeader->dest.ui8Byte4, pEthHeader->dest.ui8Byte5, pEthHeader->dest.ui8Byte6);
        String destMAC = supportMac;

        if (NULL != (pa = _macTrafficStats.get(sourceMAC))) {
            if (NULL != (pb = pa->_destMac.get(destMAC))) {
                pb->tiaFiveSecs.getAverage();
                pb->tiaOneMinute.getAverage();

                pb->tiaFiveSecs.add(ui16PacketLen);
                pb->tiaOneMinute.add(ui16PacketLen);
                pb->i64TimeOfLastChange = i64CurrTime;
            }
            else {
                pb = new DestMac;
                pb->tiaFiveSecs.add(ui16PacketLen);
                pb->tiaOneMinute.add(ui16PacketLen);
                pb->i64TimeOfLastChange = i64CurrTime;
                pa->_destMac.put(destMAC, pb);
            }
        }
        else {
            pa = new SourceMac;
            pb = new DestMac;
            pb->tiaFiveSecs.add(ui16PacketLen);
            pb->tiaOneMinute.add(ui16PacketLen);
            pb->i64TimeOfLastChange = i64CurrTime;
            pa->_destMac.put(destMAC, pb);
            _macTrafficStats.put(sourceMAC, pa);

        }
         return 0;
    }

	int NetSensor::populateRoundTripHash(const int64 i64CurrTime, NET_SENSOR::PacketStructure packet)
    {
        const uint8 * const pPacket = packet.ui8Buf;
        int rc = 0;
        EtherFrameHeader * const pEthHeader = (EtherFrameHeader*)pPacket;
        pEthHeader->ntoh();
        if (pEthHeader->ui16EtherType == ET_IP) {
            IPHeader *pIPHeader = (IPHeader*)(pPacket + sizeof(EtherFrameHeader));
            uint16 ui16IPHeaderLen = (pIPHeader->ui8VerAndHdrLen & 0x0F) * 4;
            uint8 uint8Protocol = pIPHeader->ui8Proto;
            if (uint8Protocol == IP_PROTO_ICMP) {
                ICMPHeader *pICMPHeader = (ICMPHeader*)(((uint8*)pIPHeader) + ui16IPHeaderLen);
                uint8 ui8Type = pICMPHeader->ui8Type;
                if ((ui8Type == ICMPHeader::T_Echo_Request) || (ui8Type == ICMPHeader::T_Echo_Reply)) {
                    uint32 s32Addr = pIPHeader->srcAddr.ui32Addr;
                    uint32 d32Addr = pIPHeader->destAddr.ui32Addr;
                    RoundTripStat       *p1;
                    DestField           *p2;
                    PerID               *p3;
                    PerSequenceNumber   *p4;

                    uint32 supportAddrA;
                    uint32 supportAddrB;

                    if (ui8Type == ICMPHeader::T_Echo_Request) {
                        supportAddrA = s32Addr;
                        supportAddrB = d32Addr;
                    }
                    if (ui8Type == ICMPHeader::T_Echo_Reply) {
                        supportAddrA = d32Addr;
                        supportAddrB = s32Addr;
                    }

                    if (NULL == (p1 = _roundTripStatTable.get(supportAddrA))) {
                        //no entry for that addr
                        p1 = new  RoundTripStat();
                        p2 = new  DestField();
                        p3 = new  PerID();
                        p4 = new  PerSequenceNumber();

						p2->i64LastMeasuredRTT = 0;
                        p4->i64TimeOfLastChange = i64CurrTime;

                        switch (ui8Type) {
                        case ICMPHeader::T_Echo_Reply:
                            //ignore reply since we missed the request
                            break;

                        case ICMPHeader::T_Echo_Request:
                            p4->gotAReply = false;
                            p4->gotARequest = true;
                            p4->i64TRequestTime = packet.receivedTimeStamp;
                            break;
                        }
                        p3->_PerSequenceNumber.put(uint32(pICMPHeader->ui16RoHWord2), p4);
                        p2->_perID.put(uint32(pICMPHeader->ui16RoHWord1), p3);
                        p1->_PerDest.put(supportAddrB, p2);
                        _roundTripStatTable.put(supportAddrA, p1);
                    }
                    else{
                        //we have an entry for that address!
                        if (NULL == (p2 = p1->_PerDest.get(supportAddrB))) {
                            //we don't have an entry for the second address
                            p2 = new  DestField();
                            p3 = new  PerID();
                            p4 = new  PerSequenceNumber();
							p2->i64LastMeasuredRTT = 0;
                            p4->i64TimeOfLastChange = packet.receivedTimeStamp;

                            switch (ui8Type) {
                            case ICMPHeader::T_Echo_Reply:
                                //ignore packet, we missed the request
                                break;

                            case ICMPHeader::T_Echo_Request:
                                p4->gotAReply = false;
                                p4->gotARequest = true;
                                p4->i64TRequestTime = packet.receivedTimeStamp;
                                break;
                            }
                            p3->_PerSequenceNumber.put(uint32(pICMPHeader->ui16RoHWord2), p4);
                            p2->_perID.put(uint32(pICMPHeader->ui16RoHWord1), p3);
                            p1->_PerDest.put(supportAddrB, p2);
                        }
                        else {
                            if (NULL == (p3 = p2->_perID.get(uint32(pICMPHeader->ui16RoHWord1)))) {
                                //no ID entry
								p3 = new  PerID();
								p4 = new  PerSequenceNumber();
								p4->i64TimeOfLastChange = packet.receivedTimeStamp;
								switch (ui8Type) {
									case ICMPHeader::T_Echo_Reply:
										//we missed the request
										break;
									case ICMPHeader::T_Echo_Request:
										p4->gotAReply = false;
										p4->gotARequest = true;
										p4->i64TRequestTime = packet.receivedTimeStamp;
										break;
								}
								p3->_PerSequenceNumber.put(uint32(pICMPHeader->ui16RoHWord2), p4);
								p2->_perID.put(uint32(pICMPHeader->ui16RoHWord1), p3);
							}
							else {
								if (NULL == (p4 = p3->_PerSequenceNumber.get(uint32(pICMPHeader->ui16RoHWord2)))) {
									//no entry
									p4 = new  PerSequenceNumber();
									p4->i64TimeOfLastChange = packet.receivedTimeStamp;
									switch (ui8Type) {
										case ICMPHeader::T_Echo_Reply:
											//we missed the request
											break;
										case ICMPHeader::T_Echo_Request:
											p4->gotAReply = false;
											p4->gotARequest = true;
											p4->i64TRequestTime = packet.receivedTimeStamp;
											break;
									}
									p3->_PerSequenceNumber.put(uint32(pICMPHeader->ui16RoHWord2), p4);
								}
								else {
									//entry present
									switch (ui8Type) {
										case ICMPHeader::T_Echo_Reply:
											p4->gotAReply = true;
											p4->i64TReplyTime = packet.receivedTimeStamp;
											p2->i64LastMeasuredRTT = (p4->i64TReplyTime - p4->i64TRequestTime) / 1000;								
											break;

										case ICMPHeader::T_Echo_Request:
											p4->gotAReply = false;
											p4->gotARequest = true;
											p4->i64TRequestTime = packet.receivedTimeStamp;
											break;
									}
								}
							}
						}
					}
				}
			}
		}
		return 0;
	}

	int NetSensor::prepareAndSendUpdates()
	{
		int rc = 0;
		checkAndLogMsg("NetSensor::prepareAndSendUpdates()", Logger::L_HighDetailDebug, "Sending data...\n");
		bool bIOSent = false;
		bool bTOPSent = false;

		//check if you have sent all the data
		while (!bIOSent && !bTOPSent) {
			//do this one time per cicle.
			if (_bTraffic) {
				bIOSent = protoTraffic();
			}
			else {
				bIOSent = true;
			}

			if (_bTopology) {
				bTOPSent = protoTopology();
			}
			else {
				bTOPSent = true;
			}
		}
		return rc;
	}
	
	bool NetSensor::prepareTopologyPacket()
	{
		Mac *pa = NULL;
		IpPerGW *pb = NULL;
		String localNetProxyMAC = " ";
		String RMAC = " ";
		String topologySupport;

		_topologyInstance.set_networkname(InetAddr((NETSENSOR_NETWORK_NETMASK & NETSENSOR_IP_ADDR)).getIPAsString());
		_topologyInstance.set_subnetmask(InetAddr(NETSENSOR_NETWORK_NETMASK).getIPAsString());

		if (_bPrintTopology) {
			checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "NetSensor IP: %s; NetMask: %s; Network Name: %s\n",
				InetAddr(NETSENSOR_IP_ADDR).getIPAsString(),
				InetAddr(NETSENSOR_NETWORK_NETMASK).getIPAsString(),
				InetAddr((NETSENSOR_NETWORK_NETMASK & NETSENSOR_IP_ADDR)).getIPAsString());
			checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "Internals:\n");
		}

		if (_IETrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
			if (_internalTrafficStats.getCount() > 0) {
				for (UInt32Hashtable<Mac>::Iterator i = _internalTrafficStats.getAllElements(); !i.end(); i.nextElement()) {
					pa = i.getValue();
					if (pa != NULL) {
						ddam::Host *internal = _topologyInstance.add_internals();
						internal->set_mac(pa->mac);
						NOMADSUtil::IPv4Addr ip;
						ip.ui32Addr = i.getKey();
						if (ip.ui32Addr == NETSENSOR_IP_ADDR) {
							internal->set_isdefault(true);
						}
						ip.ntoh();
						internal->set_ip(ip.ui32Addr);
						if (_bPrintTopology) {
							checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " IP: %s, MAC: %s\n", InetAddr(i.getKey()).getIPAsString(), pa->mac.c_str());
						}
					}
					else {
						checkAndLogMsg("NetSensor::protoTopology", Logger::L_Warning, "InternalTrafficStat map has null entries?\n");
					}
				}
			}


			if (_bPrintTopology) {
				checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "Externals:\n");
			}

			for (UInt32Hashtable<Mac>::Iterator i = _externalTrafficStats.getAllElements(); !i.end(); i.nextElement()) {
				pa = i.getValue();
				ddam::Host *external = _topologyInstance.add_externals();		
				external->set_mac(pa->mac);

				NOMADSUtil::IPv4Addr ip;
				ip.ui32Addr = i.getKey();
				if (ip.ui32Addr == NETSENSOR_IP_ADDR) {
					external->set_isdefault(true);
				}
				ip.ntoh();

				external->set_ip(ip.ui32Addr);
				if (_bPrintTopology) {

					checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " IP: %s, MAC: %s\n", InetAddr(i.getKey()).getIPAsString(), pa->mac.c_str());
				}
			}

			if (_uint32Local_NP_IP == 0) {
				bool bExitPointHandled = false;
				if (_bPrintTopology) {
					checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "GWs:\n");
				}

				for (StringHashtable<IpPerGW>::Iterator i = _gwTable.getAllElements(); !i.end(); i.nextElement()) {
					pb = i.getValue();
					ddam::Host *localGW = _topologyInstance.add_localgws();
					localGW->set_mac(i.getKey());
					if ((pDefaulMac != NULL) && !(strcmp(pDefaulMac, i.getKey()))) {
						localGW->set_isdefault(true);
						IPv4Addr ip;
						ip.ui32Addr = InetAddr(pDefaultIp).getIPAddress();
						ip.ntoh();
						localGW->set_ip(ip.ui32Addr);

						bExitPointHandled = true;
					}
					else {
						if (!strcmp(pb->_IpEntry, "ND")) {
							//we don't have an IP since we used the lax topology mechanism
							localGW->set_ip(0);
							int a = 0;
						}
						else {
							if ((pLocalGwIP1 != NULL) && !(strcmp(pLocalGwIP1, pb->_IpEntry.c_str()))) {
								if (pLocalGwName1 != NULL) {
									localGW->set_gatewayname(pLocalGwName1);
								}
								else {
									localGW->set_gatewayname("unknown");
								}

								if (_bPrintTopology) {
									checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " Local GW name: %s\n", pLocalGwName1);
								}
							}
							if ((pLocalGwIP2 != NULL) && !(strcmp(pLocalGwIP2, pb->_IpEntry.c_str()))) {
								if (pLocalGwName1 != NULL) {
									localGW->set_gatewayname(pLocalGwName2);
								}
								else {
									localGW->set_gatewayname("unknown");
								}
								if (_bPrintTopology) {
									checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " Local GW name: %s\n", pLocalGwName2);
								}
							}

							NOMADSUtil::IPv4Addr ip;
							ip.ui32Addr = InetAddr(pb->_IpEntry).getIPAddress();
							ip.ntoh();
							localGW->set_ip(ip.ui32Addr);
						}
					}
					if (_bPrintTopology) {
						checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " IP: %s, MAC: %s\n", pb->_IpEntry.c_str(), i.getKey());
					}
				}
				if (_uint32ExternalInterfaceGwIp != 0) {
					ddam::Host *localGW = _topologyInstance.add_localgws();
					localGW->set_ip(_uint32ExternalInterfaceGwIp);
					checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, " IP: %s, MAC: %s\n",
						InetAddr(_uint32ExternalInterfaceGwIp).getIPAsString(), "");
				}

				if (!bExitPointHandled) {
					checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "Exit point was not found on the MAC list, forcing value if present\n");
					if (pDefaulMac != NULL) {
						ddam::Host *localGW = _topologyInstance.add_localgws();
						localGW->set_mac(pDefaulMac);
						if (pDefaultIp != NULL) {
							IPv4Addr ip;
							ip.ui32Addr = InetAddr(pDefaultIp).getIPAddress();
							checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "Forced IP: %s, MAC: %s\n", pDefaultIp, pDefaulMac);
							ip.ntoh();
							localGW->set_ip(ip.ui32Addr);
						}
						localGW->set_isdefault(true);
					}
				}
			}
			else {
				handleLocalNetProxyAddresses(_uint32Local_NP_IP);
			}

			if (_uint32R1_NP_IP != 0) {
				handleRemoteNetProxyAddresses(_uint32R1_NP_IP);
			}

			if (_uint32R2_NP_IP != 0) {
				handleRemoteNetProxyAddresses(_uint32R2_NP_IP);
			}

			if (_uint32listNetProxyRemoteAddress != NULL) {
				for (int count = 0; count < _uint32listNetProxyRemoteAddress->getCount(); count++) {
					uint32 remoteAddr;
					_uint32listNetProxyRemoteAddress->getNext(remoteAddr);
					handleRemoteNetProxyAddresses(remoteAddr);
				}
				_uint32listNetProxyRemoteAddress->resetGet();
			}
			else {
				if (_bPrintTopology) {
					checkAndLogMsg("NetSensor::protoTopology", Logger::L_LowDetailDebug, "Remote GWs: List is null\n");
				}
			}
			
			
			_IETrafficStatsMUTEX.unlock();

			ddam::TopologyParts* _newTopologyInstance = new ddam::TopologyParts();
			_newTopologyInstance->CopyFrom(_topologyInstance);
			_topologyInstance.Clear();

			_topologyContainer.set_allocated_topologyparts(_newTopologyInstance);
			google::protobuf::Timestamp* ts = _topologyContainer.mutable_timestamp();
			setProtobufTimestamp(ts);
			_topologyContainer.set_datatype(TOPOLOGY_PARTS);


			if (_bWriteOnFile) {
				writeOnFile(_topologyInstance.DebugString().c_str(), "topology.txt");
			}

			return true;
		}
		return false;
	}

	bool NetSensor::prepareTrafficPacket()
	{
		char* mode;
		PerNodeTrafficStatsIP1          *pSources = NULL; //SOURCE
		PerNodeTrafficStatsIP2          *pDests = NULL; //DEST
		PerNodeTrafficStatsbyProtocol   *pProtocols = NULL; //PROTOCOL LEVEL
		PerNodeTrafficStats             *pPorts = NULL; //PORT/ENTRIES LEVEL
		uint32 uint32SensorAddress = 0;
		if (NETSENSOR_IP_ADDR != 0) {
			uint32SensorAddress = NETSENSOR_IP_ADDR;
		}
		else {
			if (_uint32NetProxyExternalInterfaceIp != 0) {
				uint32SensorAddress = _uint32NetProxyExternalInterfaceIp;
			}
			else {
				checkAndLogMsg("NetSensor::protoLink", Logger::L_HighDetailDebug,
					"Something went wrong, could not recover primary sending interface IP\n");
			}
		}



		if (_IOTrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
			if (_IETrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok) {
				checkAndLogMsg("NetSensor::protoLink", Logger::L_LowDetailDebug, "IE and IO locks obtained\n");
				if (_bRtt) {
					if (!(_RTTrafficStatsMUTEX.tryLock() == NOMADSUtil::Mutex::RC_Ok)) {
						_IOTrafficStatsMUTEX.unlock();
						_IETrafficStatsMUTEX.unlock();
						return false;
					}
				}

				if (_trafficTable.getTableSize() == 0) {
					printf("_trafficTable Something went wrong!\n");
				}
				for (UInt32Hashtable<PerNodeTrafficStatsIP1>::Iterator i = _trafficTable.getAllElements(); !i.end(); i.nextElement()) {
					//SOURCE
					pSources = i.getValue();
					uint32 ui32SrcAddr = i.getKey();
					uint32 ui32DestAddr = 0;
					bool bDetEntries = false;

					for (UInt32Hashtable<PerNodeTrafficStatsIP2>::Iterator ii = pSources->_IPB.getAllElements(); !ii.end(); ii.nextElement()) {
						if (pSources->_IPB.getCount() == 0) {
							printf("pSources->_IPB Something went wrong!\n");
						}

						//DEST
						pDests = ii.getValue();
						ui32DestAddr = ii.getKey();
						ddam::Link supportLink;
						NOMADSUtil::IPv4Addr ip;
						ip.ui32Addr = ui32DestAddr;
						ip.ntoh();
						supportLink.set_ipdst(ip.ui32Addr);

						ip.ui32Addr = ui32SrcAddr;
						ip.ntoh();
						supportLink.set_ipsrc(ip.ui32Addr);

						for (UInt32Hashtable<PerNodeTrafficStatsbyProtocol>::Iterator iii = pDests->_byProtocol.getAllElements(); !iii.end(); iii.nextElement()) {
							if (pDests->_byProtocol.getCount() == 0) {
								printf("pDests->_byProtocol Something went wrong!\n");
							}


							pProtocols = iii.getValue();
							for (UInt32Hashtable<PerNodeTrafficStats>::Iterator iv = pProtocols->_PerNodeTrafficStatsbyPort.getAllElements(); !iv.end(); iv.nextElement()) {
								//PORT
								if (pProtocols->_PerNodeTrafficStatsbyPort.getCount() == 0) {
									printf("pDests->_byProtocol Something went wrong!\n");
								}

								pPorts = iv.getValue();

								uint32 supf = pPorts->tiaFiveSecs.getAverage();
								uint32 supm = pPorts->tiaOneMinute.getAverage();

								if ((supf > 0) || (supm > 0)) {
									bDetEntries = true;

									ddam::Stat *stat;
									stat = supportLink.add_stats();

									if ((ui32SrcAddr == uint32SensorAddress) || (ui32SrcAddr == 0)) {
										//We are sending this traffic
										stat->set_sentfivesec(supf);
										stat->set_sentminute(supm);
										mode = "Sent";
									}
									else {
										if (((ui32DestAddr == uint32SensorAddress) || (isInMulticastRange(ui32DestAddr) && !_bBuiltInMode) || (ui32DestAddr == 0))) {
											//We are receiving this traffic
											stat->set_receivedfivesec(supf);
											stat->set_receivedminute(supm);
											mode = "Received";
										}
										else {
											//we are observing this traffic
											stat->set_observedfivesec(supf);
											stat->set_observedminute(supm);
											mode = "Observed";
										}

									}

									NOMADSUtil::String sProtocolName;
									sProtocolName = retrieveProtocol(iii.getKey());

									stat->set_protocol(sProtocolName.c_str());
									stat->set_port(iv.getKey());

									if (_bPrintTraffic) {
										checkAndLogMsg("NetSensor::protoLink", Logger::L_LowDetailDebug,
											"Traffic Flagged as: %s src: %s, dst: %s, prot: %s, port: %d, fives: %d, mins: %d\n",
											mode, InetAddr(ui32SrcAddr).getIPAsString(), InetAddr(ui32DestAddr).getIPAsString(), sProtocolName.c_str(), iv.getKey(), supf, supm);
									}
								}
								//else {
								//	printf("entry is 0 ");
								//}
								//
								//else {
								//pProtocols->_PerNodeTrafficStatsbyPort.remove(iv.getKey());
								//if (pProtocols->_PerNodeTrafficStatsbyPort.getCount() == 0) {
								//	pDests->_byProtocol.remove(iii.getKey());
								//}
								//}
							}
						}
						if (bDetEntries) {
							ddam::Link *newLink = _trafficContainer.add_links();
							newLink->CopyFrom(supportLink);

							if (_bRtt) {
								RoundTripStat *pRoundTripStat;
								DestField *pDestField;
								ddam::Description* description;
								description = new ddam::Description();
								newLink->set_allocated_description(description);

								if ((pRoundTripStat = _roundTripStatTable.get(ui32SrcAddr)) != NULL) {
									if ((pDestField = pRoundTripStat->_PerDest.get(ui32DestAddr)) != NULL) {
										description->set_latency(pDestField->i64LastMeasuredRTT);
										if (_bPrintTraffic) {
											checkAndLogMsg("NetSensor::protoLink", Logger::L_LowDetailDebug, "Traffic Debug: ts :%lld\n", pDestField->i64LastMeasuredRTT);
										}
									}
								}
							}
						}
					}

				}

				_IOTrafficStatsMUTEX.unlock();
				_IETrafficStatsMUTEX.unlock();

				checkAndLogMsg("NetSensor::protoLink", Logger::L_LowDetailDebug, "IE and IO locks released\n");
				if (_bRtt) {
					_RTTrafficStatsMUTEX.unlock();
				}

				google::protobuf::Timestamp* ts = new google::protobuf::Timestamp();
				_trafficContainer.set_allocated_timestamp(ts);
				setProtobufTimestamp(ts);
				_trafficContainer.set_datatype(LINK);
				//could break averages calculation, use it only for debug!
				if (_bWriteOnFile) {
					writeOnFile(_trafficContainer.DebugString().c_str(), "linkHR.txt");
				}

				return true;
			}
			_IOTrafficStatsMUTEX.unlock();
		}
		return false;
	}

	bool NetSensor::protoTopology(void)
	{
		if (prepareTopologyPacket()) {
			sendTopologyPacket();
			_topologyContainer.Clear();
			return true;
		}
		return false;
	}

	bool NetSensor::protoTraffic(void)
	{
		if (prepareTrafficPacket()) {
			sendTrafficPacket();
			_trafficContainer.Clear();
			return true;
		}
		return false;
	}

	int NetSensor::readCfgFile(const char *configFile)
	{
		char supStr[80];

		checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); Reading configuration from file: %s\n", configFile);
		//check if file exists
		FILE *pFile;
		if ((pFile = fopen(configFile, "r")) == NULL) {
			checkAndLogMsg("NetSensor::init", Logger::L_SevereError, "readCfgFile(); can't find netSensor cfg file on path: %s\n", configFile);
			return -1;
		}
		fclose(pFile);

		//Load properties
		NOMADSUtil::ConfigManager cfg;
		cfg.init(1024);

		cfg.readConfigFile(configFile, true);

		//IP
		strcpy(_cIp, cfg.getValue("IP"));
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Notified IP: %s\n", _cIp);

		//PORT
		_uint32Port = cfg.getValueAsInt("PORT");
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Notified PORT: %d\n", _uint32Port);

		//LOCAL_NP
		strcpy(supStr, cfg.getValue("LOCAL_NP"));
		_uint32Local_NP_IP = InetAddr(supStr).getIPAddress();
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Local Proxy IP: %s\n", supStr);

		//REMOTE NP1
		strcpy(supStr, cfg.getValue("REMOTE_NP1"));
		_uint32R1_NP_IP = InetAddr(supStr).getIPAddress();
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Remote Proxy IP: %s\n", supStr);

		//REMOTE NP2
		strcpy(supStr, cfg.getValue("REMOTE_NP2"));
		_uint32R2_NP_IP = InetAddr(supStr).getIPAddress();
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "Remote Proxy IP: %s\n", supStr);

		//LOCAL GW
		if (cfg.hasValue("LOCAL_GW1_IP")) {
			pLocalGwIP1 = new char[20];
			strcpy(pLocalGwIP1, cfg.getValue("LOCAL_GW1_IP"));
		}

		if (cfg.hasValue("LOCAL_GW2_IP")) {
			pLocalGwIP2 = new char[20];
			strcpy(pLocalGwIP2, cfg.getValue("LOCAL_GW2_IP"));
		}

		if (cfg.hasValue("LOCAL_GW1_MAC")) {
			pLocalMAC1 = new char[20];
			strcpy(pLocalMAC1, cfg.getValue("LOCAL_GW1_MAC"));
		}

		if (cfg.hasValue("LOCAL_GW2_MAC")) {
			pLocalMAC2 = new char[20];
			strcpy(pLocalMAC2, cfg.getValue("LOCAL_GW2_MAC"));
		}

		if (cfg.hasValue("LOCAL_GW1_NAME")) {
			pLocalGwName1 = new char[20];
			strcpy(pLocalGwName1, cfg.getValue("LOCAL_GW1_NAME"));
		}

		if (cfg.hasValue("LOCAL_GW2_NAME")) {
			pLocalGwName2 = new char[20];
			strcpy(pLocalGwName2, cfg.getValue("LOCAL_GW2_NAME"));
		}

		if (cfg.hasValue("DEFAULT_IP")) {
			pDefaultIp = new char[20];
			strcpy(pDefaultIp, cfg.getValue("DEFAULT_IP"));
		}

		if (cfg.hasValue("DEFAULT_MAC")) {
			pDefaulMac = new char[20];
			strcpy(pDefaulMac, cfg.getValue("DEFAULT_MAC"));
		}
		if (!_bBuiltInMode) {
			pLogger->setDebugLevel(cfg.getValueAsInt("DEBUG_LEVEL"));
		}


		//LOCAL INTERFACE
		strcpy(_cInterface, cfg.getValue("INTERFACE"));


		if (_bBuiltInMode) {
			if (!cfg.hasValue("INTERNAL_INTERFACE")) {
				checkAndLogMsg("NetSensor::init", Logger::L_SevereError, "Please specify the internal interface in the netsensor cfg file by adding INTERNAL_INTERFACE=<interface name>\n");
				exit(-1);
			}

			if (!cfg.hasValue("INTERFACE")) {
				checkAndLogMsg("NetSensor::init", Logger::L_SevereError, "Please specify the external interface in the netsensor cfg file by adding INTERFACE=<interface name>\n");
				exit(-1);
			}

			strcpy(_cInternalInterface, cfg.getValue("INTERNAL_INTERFACE"));
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "External Interface: %s\n", _cInterface);
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Internal Interface: %s\n", _cInternalInterface);
		}
		else {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Internal Interface: %s\n", _cInternalInterface);
		}
			

		//INCOMING_OUTGOING_STAT
		_bTraffic = cfg.getValueAsBool("INCOMING_OUTGOING_STAT");

		//GW DETECTION MECHANISMS
		_bGwArpMechanism = cfg.getValueAsBool("ARP_GW_FINDING_MECHANISM");

		//TOPOLOGY
		_bTopology = cfg.getValueAsBool("TOPOLOGY_STAT");
		_bLaxTopology = cfg.getValueAsBool("LAX_TOPOLOGY_MECHANISM");
		_bArpTopology = cfg.getValueAsBool("ARP_TOPOLOGY_MECHANISM");
		_bNMaskTopology = cfg.getValueAsBool("NETMASK_TOPOLOGY_MECHANISM");
		_bNPSpecialTopology = cfg.getValueAsBool("NETPROXY_TOPOLOGY_MECHANISM");

		//MAC_STAT
		_bMac = cfg.getValueAsBool("MAC_STAT");
		//ROUND_TRIP_STAT
		_bRtt = cfg.getValueAsBool("ROUND_TRIP_STAT");

		//STAT_UPDATE_TIME
		_uint32Stat_update_time = cfg.getValueAsInt("STAT_UPDATE_TIME");
		//CLEAN_TIME
		_uint32Clean_time = cfg.getValueAsInt("CLEAN_TIME");
		//CLEANING_TIME_IO
		_uint32Cleaning_time_t = cfg.getValueAsInt("CLEANING_TIME_IO");
		//CLEANING_TIME_IE
		_uint32Cleaning_time_ie = cfg.getValueAsInt("CLEANING_TIME_IE");
		//CLEANING_TIME_RT
		_uint32Cleaning_time_rt = cfg.getValueAsInt("CLEANING_TIME_RT");
		//CLEANING_TIME_MAC
		_uint32Cleaning_time_mac = cfg.getValueAsInt("CLEANING_TIME_MAC");
		//CLEANING_TIME_MC
		_uint32Cleaning_time_mc = cfg.getValueAsInt("CLEANING_TIME_MC");

		if (cfg.hasValue("FORCED_NETMASK_VALUE")) {
			char *_pForcedNetMask = new char[20];
			strcpy(_pForcedNetMask, cfg.getValue("FORCED_NETMASK_VALUE"));
			NETSENSOR_NETWORK_NETMASK = InetAddr(_pForcedNetMask).getIPAddress();
		}

		if (cfg.getValueAsInt("GWTHRESHOLD") != 0)
			GWTHRESHOLD = cfg.getValueAsInt("GWTHRESHOLD");

		_bPrintTopology = cfg.getValueAsBool("PRINT_TOPOLOGY");
		_bPrintTraffic = cfg.getValueAsBool("PRINT_TRAFFIC");
		_bWriteOnFile = cfg.getValueAsBool("WRITE_ON_FILE");


		_uint32Mtu = cfg.getValueAsInt("MTU");
		if (_uint32Mtu == 0) {
			_uint32Mtu = PACKETMAXSIZE;
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Forced MTU to: %d\n", _uint32Mtu);
		}
		else {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "Set MTU to: %d\n", _uint32Mtu);
		}
		
		if(cfg.hasValue("DO_NOT_COUNT_MULTICAST"))
			_bDisableMulticastCount = cfg.getValueAsBool("DO_NOT_COUNT_MULTICAST");
		
		checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); Active options status:\n");

		switch (pLogger->getDebugLevel()) {
			case 1:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Severe Error Message\n");
				break;
			case 2:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Mild Error Messag\n");
				break;
			case 3:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Warning\n");
				break;
			case 4:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Info\n");
				break;
			case 5:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Net\n");
				break;
			case 6:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Low Detail Debug Message\n");
				break;
			case 7:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: Medium Detail Debug Message\n");
				break;
			case 8:
				checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDebug level: High Detail Debug Message\n");
				break;
		}


		if (_bTraffic)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tTraffic\n");
		if (_bTopology)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tTopology\n");

		int count = 0;
		if (_bLaxTopology) {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tLax Topology\n");
			count++;
		}

		if (_bArpTopology) {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tARP Topology\n");
			count++;
		}

		if (_bNMaskTopology) {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tNETMASK Topology\n");
			count++;
		}
		if (_bNPSpecialTopology) {
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tNetProxy Topology\n");
			count++;
		}

		if (_bMac)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tMac\n");
		if (_bRtt)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tRoundTrip\n");
		if (_bDisableMulticastCount)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tDo not count multicast\n");
		if (_bWriteOnFile)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tWrite on file\n");
		if (_bPrintTopology)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tPrint Topology\n");
		if (_bPrintTraffic)
			checkAndLogMsg("NetSensor::init", Logger::L_Info, "readCfgFile(); \tPrint Traffic\n");

		//CONFIGURATION LOGIC CHECK
		if (_bNPSpecialTopology && !_bBuiltInMode) {
			checkAndLogMsg("NetSensor::init", Logger::L_SevereError, "You can't use NPSpecial algorithm without being a built in component!\n");
			return -1;
		}
		if (count > 1) {
			checkAndLogMsg("NetSensor::init", Logger::L_SevereError, "You are not supposed to use more than one topology detection mechanism!\n");
			return -2;
		}
		return 0;
	}

	NetSensor::ReceiverThread::ReceiverThread(void)
	{
	}

	NetSensor::ReceiverThread::~ReceiverThread(void)
	{
		_pNetSensor = NULL;
		_pNetInterface = NULL;
		checkAndLogMsg("~ReceiverThread", Logger::L_Info, "Receiver Thread terminated\n");
	}

	NetSensor::RTTHandlerThread::RTTHandlerThread(void)
	{
	}

	NetSensor::RTTHandlerThread::~RTTHandlerThread(void)
	{
		_pNetSensor = NULL;
		checkAndLogMsg("~_RTTHandlerThread", Logger::L_Info, "Handler Thread terminated\n");
	}

	char * NetSensor::retrieveProtocol(int protocolNumber)
	{
		switch (protocolNumber)
		{
			case IP_PROTO_UDP:
				return "UDP";
			case IP_PROTO_TCP:
				return "TCP";
			case IP_PROTO_ICMP:
				return "ICMP";
			case IP_PROTO_IGMP:
				return  "IGMP";
			default:
				return "UNKNOWN";
		}
	}

	RoundTripStat::RoundTripStat(void)
		: _PerDest(true)
	{
	}

	void NetSensor::run(void)
	{
		this->started();
		checkAndLogMsg("NetSensor::run", Logger::L_Info, "NetSensor Main Thread started\n");

		if (_bBuiltInMode) {
			_InternalReceiverThread.start();
			_ReceiverThread.start();
		}
		else {
			_ReceiverThread.start();
		}

		if (_bRtt) {
			_RTTHandlerThread.start();
			
		}
		_HandlerThread.start();
		_CleanerThread.start();

		int rc;
		int64 i64CurrTime = 0;
		int64 i64LastOutputTime = 0;
		int64 i64FiveSecTimer = 0;
		int64 i64MinTimer = 0;
		int minutes = 0;
		i64CurrTime = getTimeInMilliseconds();
		i64LastOutputTime = i64CurrTime;
		i64MinTimer = i64CurrTime;
		//Check for termination request
		while (!terminationRequested()) {
			i64CurrTime = getTimeInMilliseconds();
			//If it is time to send the updates
			if ((i64CurrTime - i64LastOutputTime) > _uint32Stat_update_time) {
				i64LastOutputTime = i64CurrTime;

				if ((rc = prepareAndSendUpdates()) < 0) {
					checkAndLogMsg("NetSensor::run", Logger::L_Warning,
						"prepareAndSendJSon () failed; rc = %d\n", rc);
					break;
				}
			}

			if ((i64CurrTime - i64MinTimer) > 60000) {
				i64MinTimer = i64CurrTime;
				minutes++;
				checkAndLogMsg("NetSensor::run", Logger::L_Info, "Minutes from start : %d\n", minutes);
			}
			// do not clog cpu
			sleepForMilliseconds(100);
		}

		//terminate thread
		_HandlerThread.requestTermination();
		_ReceiverThread.requestTermination();
		if (_bBuiltInMode) {
			_InternalReceiverThread.requestTermination();
		}
		_CleanerThread.requestTermination();
		checkAndLogMsg("NetSensor::run", Logger::L_Info, "Waiting for Cleaner, Receiver and Handler threads...\n");
		while (!_InternalReceiverThread.hasTerminated() && !_HandlerThread.hasTerminated() && !_ReceiverThread.hasTerminated() && !_CleanerThread.hasTerminated() && !_RTTHandlerThread.hasTerminated()){
			if (!_HandlerThread.hasTerminated())
				checkAndLogMsg("NetSensor::run", Logger::L_Info, "_HandlerThread is terminating\n");

			if (_bBuiltInMode) {
				if (!_InternalReceiverThread.hasTerminated())
					checkAndLogMsg("NetSensor::run", Logger::L_Info, "_InternalReceiverThread is terminating\n");
			}

			if (!_ReceiverThread.hasTerminated())
				checkAndLogMsg("NetSensor::run", Logger::L_Info, "_ReceiverThread is terminating\n");

			if (!_RTTHandlerThread.hasTerminated())
				checkAndLogMsg("NetSensor::run", Logger::L_Info, "_RTTHandlerThread is terminating\n");

			if (!_CleanerThread.hasTerminated())
				checkAndLogMsg("NetSensor::run", Logger::L_Info, "_CleanerThread is terminating\n");

			sleepForMilliseconds(1000);
		}
		checkAndLogMsg("NetSensor::run", Logger::L_Info, "NetSensor is terminating\n");
		terminating();
	}

	void NetSensor::CleanerThread::run(void)
	{
		//Set started variable
		started();
		checkAndLogMsg("NetSensor::CleanerThread::run()", Logger::L_Info, "Cleaner Thread launched\n\n");
		int rc;
		int64 i64CurrTime = getTimeInMilliseconds();
		//check termination request
		while (!terminationRequested()) {
			if (getTimeInMilliseconds() - i64CurrTime > _pNetSensor->_uint32Clean_time) {
				i64CurrTime = getTimeInMilliseconds();
				rc = _pNetSensor->clearTables(i64CurrTime);
				if (rc != 0) {
					checkAndLogMsg("NetSensor::CleanerThread::run", Logger::L_SevereError, " _NetSensor->clearTables failed; rc = %d\n", rc);
					break;
				}
			}
			sleepForMilliseconds(300);
		}
		checkAndLogMsg("NetSensor::CleanerThread::run", Logger::L_Info, " Cleaner Thread is terminating\n");
		terminating();
	}

	int NetSensor::ReceiverThread::restartHandler()
	{
		if (this->isInternal) {
			checkAndLogMsg("NetSensor::InternalReceiverThread::restartHandler",
				Logger::L_Warning, " Restarting the pcapHandler\n");
		}
		else {
			checkAndLogMsg("NetSensor::ExternalReceiverThread::restartHandler",
				Logger::L_Warning, " Restarting the pcapHandler\n");
		}

		return 0;
	}


	void NetSensor::ReceiverThread::run(void)
	{
		//Set started variable
		started();
		if (this->isInternal) {
			checkAndLogMsg("NetSensor::InternalReceiverThread::run", Logger::L_Info, "Receiver thread started\n");
		}
		else {
			checkAndLogMsg("NetSensor::ExternalReceiverThread::run", Logger::L_Info, "Receiver thread started\n");
		}

		//check termination
		int counter = 0;
		int starvationCounter = 0;
		int size = 0; 
		
		while (!terminationRequested()) {
			NET_SENSOR::PacketStructure packet;
			int received;
			
			queueMaxSize = 500;

			this->isInternal ? packet.classification = 0 : packet.classification = 1;
			//packet.ui8Buf = new uint8[ETHERNET_MAXIMUM_MFS];

			received = _pNetInterface->readPacket(packet.ui8Buf, sizeof(packet.ui8Buf), &packet.receivedTimeStamp);
			if (received > 0) {
				packet.received = received;
				//printf("Received: %d\n", received);
				bool Done = false;
				bool DoneRTT = false;

				//if round trip detection is not active we don't need to add packets to rtt queue
				if (!_pNetSensor->_bRtt) {
					DoneRTT = true;
				}

				while (!Done || !DoneRTT) {
					if (!DoneRTT) {
						if (_pNetSensor->_RTTQueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
							if (_pNetSensor->_RTTpacketQueue.size() < queueMaxSize) {
								_pNetSensor->_RTTpacketQueue.enqueue(packet);
							}
							else {
								checkAndLogMsg("NetSensor::ReceiverThread::run", Logger::L_Warning, "Incoming  rtt queue full, dropping the packet\n");
							}
							_pNetSensor->_RTTQueueMUTEX.unlock();
							DoneRTT = true;
						}
					}
					if (!Done) {
						if (_pNetSensor->_QueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
							if ((size = _pNetSensor->_packetQueue.size()) < queueMaxSize) {						
								_pNetSensor->_packetQueue.enqueue(packet);
								_pNetSensor->_QueueMUTEX.unlock();
							}
							else {
								_pNetSensor->_QueueMUTEX.unlock();						
								sleepForMilliseconds(100);
							}
							Done = true;
						}
					}
				}
			}
			else {
				if (received == 0) {
					if (this->isInternal) {
						checkAndLogMsg("NetSensor::InternalReceiverThread::run", Logger::L_Warning, "Received was 0\n");
					}
					else {
						checkAndLogMsg("NetSensor::ExternalReceiverThread::run", Logger::L_Warning, "Received was 0\n");
					}		
				}

				if (received < 0) {
					if (this->isInternal) {
						checkAndLogMsg("NetSensor::InternalReceiverThread::run", Logger::L_Warning, "Received was 0%d\n", received);
					}
					else {
						checkAndLogMsg("NetSensor::ExternalReceiverThread::run", Logger::L_Warning, "Received was %d\n", received);
					}
				}
				
				sleepForMilliseconds(100);
			}
		}
		checkAndLogMsg("NetSensor::ReceiverThread::run", Logger::L_Info, "Terminating\n");
		terminating();
	}

	void NetSensor::RTTHandlerThread::run(void)
	{
		//Set started variable
		started();
		checkAndLogMsg("NetSensor::RTTHandlerThread::run()", Logger::L_Info, "RTT Handler thread started\n");
		int rc;
		//Check termination
		while (!terminationRequested()) {
			//Check if there are packets in the reading queue
			if (_pNetSensor->_RTTpacketQueue.isEmpty()) {
				sleepForMilliseconds(100);
			}
			else {
				//check if we are running
				if (isRunning()) {
					//If it is
					NET_SENSOR::PacketStructure packet;
					bool done = false;

					if (_pNetSensor->_RTTQueueMUTEX.lock() == NOMADSUtil::Mutex::RC_Ok) {
						_pNetSensor->_RTTpacketQueue.dequeue(packet);
						_pNetSensor->_RTTQueueMUTEX.unlock();
						done = true;
					}
					if (done) {
						//Handle the packet
						if ((rc = _pNetSensor->populateRoundTripHash(getTimeInMilliseconds(), packet)) < 0) {
							checkAndLogMsg("NetSensor::RTTHandlerThread::run", Logger::L_SevereError, "populateRoundTripHash failed\n");
						}
					}
				}
				else {
					//If not sleep
					sleepForMilliseconds(100);
				}
			}
		}
		checkAndLogMsg("NetSensor::RTTHandlerThread::run", Logger::L_Info, " RttHandler Thread is terminating\n");
		terminating();
	}

	void NetSensor::sendTrafficPacket()
	{
		int size = 0;
		size = _trafficContainer.ByteSize();
		if (size < _uint32Mtu) {
			char * buffer = new char[size];
			_trafficContainer.SerializeToArray(buffer, size);
			checkAndLogMsg("NetSensor::RunThread::sendTrafficPacket", Logger::L_Info, "About to send: %dB\n", size);
			if (size < 30) {
				checkAndLogMsg("NetSensor::RunThread::sendTrafficPacket", Logger::L_Warning, "Traffic packet size is low: %dB\n", size);
			}
			
			sendSerializedData(buffer, size);

			//printf("Output:\n%s\n\n",_trafficContainer.ShortDebugString().c_str());

			if (DEBUG_MODE) {
				ddam::Container debugContainer;
				if (!debugContainer.ParseFromArray(buffer, size)) {
					checkAndLogMsg("NetSensor::RunThread::sendTrafficPacket", Logger::L_SevereError, "Failed to parse traffic message of size: %dB\n", size);
				}
				else {
					checkAndLogMsg("NetSensor::RunThread::sendTrafficPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
				}
			}
			delete[](buffer);
		}
		else {
			checkAndLogMsg("NetSensor::RunThread::sendTrafficPacket", Logger::L_Info, "Splitting Traffic packet of size: %dB\n", size);
			splitTrafficPacket();
		}
	}

	int NetSensor::getTopologyComponentSize(int componentID)
	{
		switch (componentID) 
		{
			case INTERNALS:
				return _topologyContainer.topologyparts().internals_size();
			case EXTERNALS:
				return _topologyContainer.topologyparts().externals_size();
			case LOCALGWS:
				return _topologyContainer.topologyparts().localgws_size();
			case REMOTEGWS:
				return _topologyContainer.topologyparts().remotegws_size();
		}
		return NULL;
	}
	
	ddam::Host* NetSensor::getHostCopy(int componentID, int index)
	{
		ddam::Host *hostCopy = new ddam::Host();
		switch (componentID)
		{
		case INTERNALS:
			hostCopy->CopyFrom(_topologyContainer.topologyparts().internals(index));
			return hostCopy;
		case EXTERNALS:
			hostCopy->CopyFrom(_topologyContainer.topologyparts().externals(index));
			return hostCopy;
		case LOCALGWS:
			hostCopy->CopyFrom(_topologyContainer.topologyparts().localgws(index));
			return hostCopy;
		case REMOTEGWS:
			hostCopy->CopyFrom(_topologyContainer.topologyparts().remotegws(index));
			return hostCopy;
		}
		return NULL;
	}

	ddam::Host* NetSensor::getNewTopologyHost(int componentID, ddam::TopologyParts* splitTopology)
	{
		switch (componentID)
		{
		case INTERNALS:
			return splitTopology->add_internals();
		case EXTERNALS:
			return splitTopology->add_externals();
		case LOCALGWS:
			return splitTopology->add_localgws();
		case REMOTEGWS:
			return splitTopology->add_remotegws();
		}
		return NULL;
	}

	void NetSensor::splitTopologyPacket()
	{
		int size = 0;
		ddam::Container _newtopologyContainer;
		
		ddam::TopologyParts *splitTopology = new ddam::TopologyParts();	
		_newtopologyContainer.set_datatype(TOPOLOGY_PARTS);
		google::protobuf::Timestamp* ts = new google::protobuf::Timestamp();
		_newtopologyContainer.set_allocated_timestamp(ts);
		setProtobufTimestamp(ts);
		
		splitTopology->set_networkname(InetAddr((NETSENSOR_NETWORK_NETMASK & NETSENSOR_IP_ADDR)).getIPAsString());
		splitTopology->set_subnetmask(InetAddr(NETSENSOR_NETWORK_NETMASK).getIPAsString());
		
		//splitTopology->set_networkname(_topologyContainer.topologyparts().networkname());
		//splitTopology->set_subnetmask(_topologyContainer.topologyparts().subnetmask());
	
		_newtopologyContainer.set_allocated_topologyparts(splitTopology);

		for (int hostType = INTERNALS; hostType <= REMOTEGWS; hostType++) {
			int arrayElements = 0;
			arrayElements = getTopologyComponentSize(hostType);
			for (int index = 0; index < arrayElements; index++) {
				ddam::Host* hostCopy; 
				ddam::Host* newHost;
				hostCopy = getHostCopy(hostType, index);

				if ((_newtopologyContainer.ByteSize() + hostCopy->ByteSize()) < (_uint32Mtu - 3)) {
					newHost = getNewTopologyHost(hostType, splitTopology);
					newHost->CopyFrom(*hostCopy);
					delete(hostCopy);
				}
				else {
					size = _newtopologyContainer.ByteSize();
					char * buffer = new char[size];
					_newtopologyContainer.SerializeToArray(buffer, size);
					checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_Info, "About to send: %dB\n", size);
					sendSerializedData(buffer, size);

					if (DEBUG_MODE) {
						ddam::Container debugContainer;
						if (!debugContainer.ParseFromArray(buffer, size)) {
							checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_SevereError, "Failed to parse split topology message of size: %dB\n", size);
						}
						else {
							checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
						}
					}

					delete[](buffer);

					_newtopologyContainer.Clear();
					
					_newtopologyContainer.set_datatype(TOPOLOGY_PARTS);
					
					google::protobuf::Timestamp* ts = new google::protobuf::Timestamp();
					_newtopologyContainer.set_allocated_timestamp(ts);
					setProtobufTimestamp(ts);

					splitTopology = new ddam::TopologyParts();
					splitTopology->set_networkname(InetAddr((NETSENSOR_NETWORK_NETMASK & NETSENSOR_IP_ADDR)).getIPAsString());
					splitTopology->set_subnetmask(InetAddr(NETSENSOR_NETWORK_NETMASK).getIPAsString());

					_newtopologyContainer.set_allocated_topologyparts(splitTopology);

					newHost = getNewTopologyHost(hostType, splitTopology);
					newHost->CopyFrom(*hostCopy);
					delete(hostCopy);
				}
			}
		}
		size = _newtopologyContainer.ByteSize();
		if (size > 0) {
			char * buffer = new char[size];
			_newtopologyContainer.SerializeToArray(buffer, size);
			checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_Info, "About to send: %dB\n", size);
			sendSerializedData(buffer, size);

			if (DEBUG_MODE) {
				ddam::Container debugContainer;
				if (!debugContainer.ParseFromArray(buffer, size)) {
					checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_SevereError, "Failed to parse split topology message of size: %dB\n", size);
				}
				else {
					checkAndLogMsg("NetSensor::RunThread::splitTopologyPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
				}
			}

			delete[](buffer);
			splitTopology->Clear();
		}

	}

	void NetSensor::splitTrafficPacket()
	{
		int size = 0;
		ddam::Container splitTrafficContainer;
		splitTrafficContainer.set_datatype(LINK);

		google::protobuf::Timestamp* ts2 = new google::protobuf::Timestamp();
		splitTrafficContainer.set_allocated_timestamp(ts2);
		setProtobufTimestamp(ts2);
		for (int index = 0; index < _trafficContainer.links_size(); index++) {
			ddam::Link linkCopy;
			linkCopy.CopyFrom(_trafficContainer.links(index));
			if (splitTrafficContainer.ByteSize() + linkCopy.ByteSize() < (_uint32Mtu - 3)) {
				ddam::Link *newLink = splitTrafficContainer.add_links();
				newLink->CopyFrom(linkCopy);
			}
			else {
				size = splitTrafficContainer.ByteSize();
				char *buffer = new char[size];
				splitTrafficContainer.SerializeToArray(buffer, size);
				checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_LowDetailDebug, "About to send: %dB\n", size);
				sendSerializedData(buffer, size);

				if (DEBUG_MODE) {
					ddam::Container debugContainer;
					if (!debugContainer.ParseFromArray(buffer, size)) {
						checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_SevereError, "Failed to parse split traffic message of size: %dB\n", size);
					}
					else {
						checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
					}
				}


				delete[](buffer);
				splitTrafficContainer.Clear();

				google::protobuf::Timestamp* ts2 = new google::protobuf::Timestamp();
				splitTrafficContainer.set_allocated_timestamp(ts2);
				setProtobufTimestamp(ts2);
				splitTrafficContainer.set_datatype(LINK);

				index--;
			}
		}
		if (splitTrafficContainer.ByteSize() > 0) {
			size = splitTrafficContainer.ByteSize();
			char * buffer = new char[size];
			splitTrafficContainer.SerializeToArray(buffer, size);
			checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_LowDetailDebug, "About to send: %dB\n", size);
			sendSerializedData(buffer, size);

			if (DEBUG_MODE) {
				ddam::Container debugContainer;
				if (!debugContainer.ParseFromArray(buffer, size)) {
					checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_SevereError, "Failed to parse split traffic message of size: %dB\n", size);
				}
				else {
					checkAndLogMsg("NetSensor::RunThread::splitTrafficPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
				}
			}

			delete[](buffer);
			splitTrafficContainer.Clear();
		}
	}

	//returns the number of bytes sent
	int NetSensor::sendSerializedData(const char* serializedData, const int size)
	{
		int rc;
		if (size < _uint32Mtu) {
			if (0 >(rc = _pNotifierSocket->sendTo(_notifyAddr.getIPAddress(), _notifyAddr.getPort(), serializedData, size))) {
				checkAndLogMsg("NetSensor::RunThread::prepareAndSendJSon", Logger::L_MildError, "failed to send mac UDP packet with status message; rc = %d\n", rc);
				return rc;
			}
		}
		else {
			checkAndLogMsg("NetSensor::RunThread:prepareAndSendJSon", Logger::L_Warning,
				"Message too large to be sent over UDP - it is %d, which is greater than %d\n", size, _uint32Mtu);
			return 0;
		}

		return size;
	}

	int NetSensor::sendSerializedData(const char* stringJson, const int size, const char * ID)
	{
		int rc;
		char* temp_buffer = new char[size + 1];
		memcpy(temp_buffer, ID, 1);
		memcpy(temp_buffer + 1, stringJson, size);
		checkAndLogMsg("NetSensor::RunThread:sendSerializedData()", Logger::L_HighDetailDebug, "About to send %d Bytes, message ID: %s\n", (size + 1), ID);
		if ((size + 1) > _uint32Mtu) {
			checkAndLogMsg("NetSensor::RunThread:prepareAndSendJSon", Logger::L_Warning, "Outgoing message too large to be sent over UDP, sending anyway - it is %d, ID: %s which is greater than %d\n", (size + 1), ID, _uint32Mtu);
		}
		if (0 > (rc = _pNotifierSocket->sendTo(_notifyAddr.getIPAddress(), _notifyAddr.getPort(), temp_buffer, (size + 1)))) {
			checkAndLogMsg("NetSensor::RunThread::prepareAndSendJSon", Logger::L_MildError, "failed to send mac UDP packet with status message; rc = %d\n", rc);
			delete[](temp_buffer);
			return rc;
		}
		delete[](temp_buffer);
		return 0;
	}

	void NetSensor::sendTopologyPacket()
	{
		int size = 0;
		if ((size = _topologyContainer.ByteSize()) < _uint32Mtu) {
			char * buffer = new char[size];
			checkAndLogMsg("NetSensor::RunThread::sendTopologyPacket", Logger::L_Info, "About to send: %dB\n\n", size);
			_topologyContainer.SerializeToArray(buffer, size);
			sendSerializedData(buffer, size);

			if (DEBUG_MODE) {
				ddam::Container debugContainer;
				if (!debugContainer.ParseFromArray(buffer, size)) {
					checkAndLogMsg("NetSensor::RunThread::sendTopologyPacket", Logger::L_SevereError, "Failed to parse topology message of size: %dB\n", size);
				}
				else {
					checkAndLogMsg("NetSensor::RunThread::sendTopologyPacket", Logger::L_Info, "Packet correctly parsed: %dB\n", size);
				}

			}

			delete[](buffer);
		}
		else {
			checkAndLogMsg("NetSensor::RunThread::sendTopologyPacket", Logger::L_Info, "Splitting Topology packet of size: %dB\n", size);
			splitTopologyPacket();
		}
	}

	int NetSensor::setMacAdrrOfDefaultGW(String mac, String ip)
	{
		IpPerGW *pa = NULL;
		IpEntry *pb = NULL;

		pa = new IpPerGW();
		pb = new IpEntry();
		pa->_IpEntry = ip;
		_gwTable.put(mac, pa);
		_bDefaultGwAccountedFor = true;
		checkAndLogMsg("NetSensor::setMacAdrrOfDefaultGW()", Logger::L_HighDetailDebug, "Default gw with IP: %s and mac: %s added to the gw table\n", ip.c_str(), mac.c_str());
		return 0;
	}

	int NetSensor::setNetProxyValues(const uint8 * emacNetProxyInternalMacAddr, const uint8 * emacNetProxyExternalMacAddr,
		uint32 uint32NetProxyExternalInterfaceIp, uint32 gwIp, uint32 netmask)
	{

		_pProxyInternalMacAddr = new char[30];
		NOMADSUtil::EtherMACAddr eMACAddrToUpdate;
		buildEthernetMACAddressFromString(eMACAddrToUpdate, emacNetProxyInternalMacAddr);
		buildEthernetMACAddressFromString(_emacProxyInternalMAC, emacNetProxyInternalMacAddr); //store the internal mac for later use

		sprintf(_pProxyInternalMacAddr, "%02x:%02x:%02x:%02x:%02x:%02x", eMACAddrToUpdate.ui8Byte1,
			eMACAddrToUpdate.ui8Byte2, eMACAddrToUpdate.ui8Byte3, eMACAddrToUpdate.ui8Byte4,
			eMACAddrToUpdate.ui8Byte5, eMACAddrToUpdate.ui8Byte6);

		_pProxyExternalMacAddr = new char[30];
		buildEthernetMACAddressFromString(eMACAddrToUpdate, emacNetProxyExternalMacAddr);
		buildEthernetMACAddressFromString(_emacProxyExternalMAC, emacNetProxyExternalMacAddr); //store the external mac for later use

		sprintf(_pProxyExternalMacAddr, "%02x:%02x:%02x:%02x:%02x:%02x", eMACAddrToUpdate.ui8Byte1,
			eMACAddrToUpdate.ui8Byte2, eMACAddrToUpdate.ui8Byte3, eMACAddrToUpdate.ui8Byte4,
			eMACAddrToUpdate.ui8Byte5, eMACAddrToUpdate.ui8Byte6);

		NETSENSOR_IP_ADDR = uint32NetProxyExternalInterfaceIp; //TODO: check that this is not needed 

		_uint32NetProxyExternalInterfaceIp = uint32NetProxyExternalInterfaceIp;
		NETSENSOR_NETWORK_NETMASK = netmask;
		IPv4Addr support;
		support.ui32Addr = gwIp;
		support.ntoh();
		_uint32ExternalInterfaceGwIp = support.ui32Addr;


		checkAndLogMsg("NetSensor::setNetProxyValues", Logger::L_Info,
			"NetProxyInternalMacAddr: %s; NetProxyExternalMacAddr: %s; NetProxyExternalIpAddr %s\n",
			_pProxyInternalMacAddr, _pProxyExternalMacAddr, InetAddr(_uint32NetProxyExternalInterfaceIp).getIPAsString());
		return 0;
	}

	void NetSensor::setProtobufTimestamp(google::protobuf::Timestamp *ts)
	{
#if defined (WIN32)
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;

		ts->set_seconds((INT64)((ticks / 10000000) - 11644473600LL));
		ts->set_nanos((INT32)((ticks % 10000000) * 100));
#endif

#if defined (LINUX)
		struct timeval tv;
		gettimeofday(&tv, NULL);
		ts->set_seconds(tv.tv_sec);
		ts->set_nanos(tv.tv_usec * 1000);
#endif
	}

	int NetSensor::setRemoteNetProxyList(NOMADSUtil::LList<uint32> *uint32listNetProxyRemoteAddress)
	{
		_uint32listNetProxyRemoteAddress = uint32listNetProxyRemoteAddress;
		if (_uint32listNetProxyRemoteAddress != NULL) {
			checkAndLogMsg("NetSensor::setRemoteNetProxyList", Logger::L_HighDetailDebug, "Remote proxy list queried correctly\n");
			return 0;
		}
		else {
			checkAndLogMsg("NetSensor::setRemoteNetProxyList", Logger::L_HighDetailDebug, "Remote proxy list query returned NULL\n");
			return -1;
		}

	}

    SourceMac::SourceMac(void)
        : _destMac(true)
    {

    }

    int NetSensor::updateArpChacheAndCheckForMatch(ARPPacket *pARPPacket)
    {
        pARPPacket->ntoh();
        pARPPacket->ntoh();
        String supportMAC;
        char supportChar[30];

        sprintf(supportChar, "%02x:%02x:%02x:%02x:%02x:%02x", pARPPacket->sha.ui8Byte1, pARPPacket->sha.ui8Byte2, pARPPacket->sha.ui8Byte3, pARPPacket->sha.ui8Byte4, pARPPacket->sha.ui8Byte5, pARPPacket->sha.ui8Byte6);
        supportMAC = supportChar;
        
        if (pARPPacket->spa.ui32Addr != 0) {
            //not an arp probe
            if ((pARPPacket->spa.ui32Addr != NETSENSOR_IP_ADDR) && (pARPPacket->sha != _emacNetSensorMAC)) {
                //sender is not the NetSensor
                _pARPCache.insert(pARPPacket->spa.ui32Addr, pARPPacket->sha); //populate the ARPhash
                IpPerGW *pa = NULL;
                if (NULL != (pa = _gwTable.get(supportMAC))) {
                    if (pa->_IpEntry == NULL) {
                        pa->_IpEntry = InetAddr(pARPPacket->spa.ui32Addr).getIPAsString();
                        checkAndLogMsg("NetSensor::lookForGWandPrepareARPCACHE()", Logger::L_HighDetailDebug, "Possible GW IP found!\n");                 
                    }
                }
            }
        }
        return 0;
    }

    int NetSensor::writeOnFile(const char* stringJson, const char *filename) 
    {
        String supBuf = stringJson;
        FILE *pFileHR;
        pFileHR = fopen(filename, "w");
        fprintf(pFileHR, "%s", supBuf.c_str());
        fclose(pFileHR);
        return 0;
    }
}
