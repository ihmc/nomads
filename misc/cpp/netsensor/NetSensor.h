/*
 * NetSensor.h
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
* NetSensor.h
*
* Author:                   Roberto Fronteddu
* Year of creation:         2015/2016
* Last Revision by:
* Year of last Revision:
* Version: 1.0
*/


#ifndef INCL_NET_SENSOR_H
#define INCL_NET_SENSOR_H

#include "ConfigurationParameters.h"
#include "Queue.h"
#include "Mutex.h"
#include "DataStructures.h"

#include "TimeIntervalAverage.h"
#include "UDPDatagramSocket.h"
#include "UInt32Hashtable.h"
#include "LList.h"
#include "topology.pb.h"
#include "traffic.pb.h"
#include "Constants.h"
#include "container.pb.h"
#include "PacketStructure.h"

namespace IHMC_MISC
{
    class NetworkInterface;
    //pcap timeout
    #define MS_PACKET_VALID 0
    #define IP_PROTO_IGMP 0x2 
    #define DEFAULT 0
    
	enum UsefullStatus
    {
        INCOMING = 0,
        OUTGOING = 1,
        INTERNAL = 3,
        EXTERNAL = 4,
        MULTICAST = 5,
        END = 6
    };
    enum gwSearchStatus
    {
        UNKNOWN = 0,
        GWFOUND = 1,
        NORMAL = 2,
        ERRORA = -1,
        ERRORB = -2,
        ERRORC = -3,
        ERRORD = -4,
        ERRORE = -5,
    };
	
	enum topologyComponents
	{
		INTERNALS = 0,
		EXTERNALS = 1,
		LOCALGWS = 2,
		REMOTEGWS = 3
	};

	
	class ARPCache 
    {
        public:
            ARPCache(void);
            virtual ~ARPCache(void);
            int insert(uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &pMACAddr);
            const NOMADSUtil::EtherMACAddr * const lookup(uint32 ui32IPAddr) const;

        private:
            NOMADSUtil::UInt32Hashtable<NOMADSUtil::EtherMACAddr> _arpCache;
    };
    inline ARPCache::ARPCache(void) : _arpCache(true) {}
    inline ARPCache::~ARPCache(void) {}  
    inline const NOMADSUtil::EtherMACAddr * const ARPCache::lookup(uint32 ui32IPAddr) const
    {
        return _arpCache.get(ui32IPAddr);
    }
    inline int ARPCache::insert(uint32 ui32IPAddr, const NOMADSUtil::EtherMACAddr &pMACAddr)
    {
        delete _arpCache.put(ui32IPAddr, new NOMADSUtil::EtherMACAddr(pMACAddr));
        return 0;
    }

    //IGMP HEADER V.1 WORK IN PROGRESS
    #pragma pack( push, netSensor,1 )
    //#pragma pack (1)
    struct ICMPFrame
    {//TIME STAMP REPLY MESSAGE
        uint16 identifier;
        uint16 sequenceNumber;
        uint32 OriginateTimeStamp;
        uint32 receiveTimestamp;
        uint32 trasmitTimestamp;

        //void ntohNEW(void);
        //void htonNEW(void);
    };
    struct IGMPV1Header
    {
        enum Type
        {
            Host_Membership_Query = 17,
            Host_Membership_Report = 19,
            DVMRP = 19
        };


        uint8 type;//Version is always 1
        uint8 unused;
        uint16 checksum;
        uint32 groupAddress;

        //void ntoh(void);
        //void hton(void);
    };
    struct IGMPV2Header
    {
        enum Type
        {
            Host_Membership_Query = 0x11,
            Host_Membership_Report = 0x12,
            DVMRP = 0x13,
            PIMv1 = 0x14,
            Cisco_Trace_Messages = 0x15,
            IGMPv2_Membership_Report = 0x16,
            IGMPv2_Leave_Group = 0x17,
            Multicast_Traceroute_Response = 0x1E,
            Multicast_Traceroute = 0x1F,
            IGMPv3_Membership_Report = 0x22,
            Multicast_Router_Advertisement = 0x30,
            Multicast_Router_Solicitation = 0x31,
            Multicast_Router_Termination = 0x32
        };
        uint8 type;
        uint8 maxResponseTime;
        uint16 checksum;
        uint32 groupAddress;

        //todo:
        //void computeChecksum(uint16 ui16Len);
        //handle checksun ntoh and hton when needed
        //void ntoh(void);
        //void hton(void);


    };
    #pragma pack( pop, netSensor )
    //Struc for the read queue

    class NetSensor : public NOMADSUtil::ManageableThread
    {
        private:
          //  struct Packet
		//	{
           //     uint8 *ui8Buf;
			//	uint8 ui8Buf[ETHERNET_MAXIMUM_MFS];
           //     int64 receivedTimeStamp;
           //     int classification;
          //      int received;
         //   };

        public:			
			/*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    Initializes NetSensor in StandAlone mode.

            Parameters: const char *pConfigFile: Path to configuration file.
            *************************************************************************************/
            int init(const char *pConfigFile);

			int init(const int mode, const char *passedParameter);
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    Initializes NetSensor in Built-In mode

            Parameters:     None.
            *************************************************************************************/
            int initAsAComponent(const char *pConfigFile);
			int getTopologyComponentSize(int componentID);
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    General constructor for the NetSensor Class.

            Parameters:      None.
            *************************************************************************************/
            NetSensor (void);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    General destructor for the NetSensor Class.

            Parameters:      None.
            *************************************************************************************/
	        virtual ~NetSensor (void);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    Used in Built-In Mode to pass a packet to NetSensor.

            Parameters:     const uint8 ui8Buf[9038U]: Buffer containing packet data.
                            int received: Data size in Bytes.
							bool bIsInternal: True if the packet comes from the internal interface.
							bool bCountPacketForStat: True if the packet as to be counted for traffic statistic.
            *************************************************************************************/
			int netSensorIteration(const uint8 ui8Buf[9038U], int received, int classification);
			int sendSerializedData(const char* serializedData, const int size);
			ddam::Host * getHostCopy(int componentID, int index);
			ddam::Host* getNewTopologyHost(int componentID, ddam::TopologyParts* splitTopology);
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description: Used in Built-In Mode to enqueue a packet in the RTT queue.

            Parameters: const uint8 ui8Buf[9038U]: Buffer containing packet data.
                        int received: Data size in Bytes.
                        int64 usTime: Network Interface packet received timestamp in micro seconds.
            *************************************************************************************/
            int netSensorIterationRTT(const uint8 ui8Buf[9038U], int received,int64 usTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description: Handles sending of all Protobuf packets.
            
			Parameters: None.
            *************************************************************************************/
            int prepareAndSendUpdates ();
  
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Public
            Description:    Start NetSensor.

            Parameters:     None.
            *************************************************************************************/
            void run(void);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Public
			Description: This function is used in Built-in mode since NetSensor cannot directly query the network interface.

			Parameters:     -const uint8 * emacNetProxyInternalMacAddr
							-const uint8 * emacNetProxyExternalMacAddr
							-uint32 uint32NetProxyExternalInterfaceIp
							-uint32 uint32GatewayIp 
							-uint32 uint32Netmask
			*************************************************************************************/
			int setNetProxyValues(const uint8 * emacNetProxyInternalMacAddr,
				const uint8 * emacNetProxyExternalMacAddr, uint32 uint32NetProxyExternalInterfaceIp, uint32 uint32GatewayIp, uint32 uint32Netmask);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Public
			Description: Used to pass to NetSensor a list of the remote NetProxy IP addresses.

			Parameters: NOMADSUtil::LList<uint32> *uint32listNetProxyRemoteAddress
			*************************************************************************************/
			int setRemoteNetProxyList(NOMADSUtil::LList<uint32> *uint32listNetProxyRemoteAddress);

			int setDefaultStandAloneConfigurationValues(const char* cpcNetworkInterface);


			





        protected:
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Add GW MAC to possible gateway table

            Parameters:     -pEthHeader pointer to packet's ethernet header
            **************************************************************************************/
            int addToPossibleGwTable(NOMADSUtil::String mac, bool bIsLax = false);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Convert string to MAC address.

            Parameters:     NOMADSUtil::EtherMACAddr &eMACAddrToUpdate: MAC to fill.
							const uint8 *pszMACAddr: String that will be used to fill the EtherMACAddr object.
            *************************************************************************************/
            void buildEthernetMACAddressFromString (NOMADSUtil::EtherMACAddr &eMACAddrToUpdate, const uint8 *pszMACAddr);
               
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Clean internal and external tables


            Parameters:     -const int64 i64CurrTime    : Current Time
                            -const int64 i64CleaningTime: Cleaning period
            **************************************************************************************/
            int cleanTablesInternalExternal(const int64 i64CurrTime, const int64 i64CleaningTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Clear the traffic by Source Dest IP table.

            Parameters:     -const int64 i64CurrTime:       Current time.
                            -const int64 i64CleaningTime:   Lifetime of table's entries.
            *************************************************************************************/
            int cleanTablesTraffic(const int64 i64CurrTime, const int64 i64CleaningTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Clear the traffic by MAC table.

            Parameters:     -const int64 i64CurrTime:       Current time.
                            -const int64 i64CleaningTime:   Lifetime of table's entries.
            *************************************************************************************/
            int cleanTablesMAC(const int64 i64CurrTime, const int64 i64CleaningTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Clear the roundtrip table.

            Parameters:     -const int64 i64CurrTime:       Current time.
                            -const int64 i64CleaningTime:   Lifetime of table's entries.
            *************************************************************************************/
            int cleanTableRoundTrip(const int64 i64CurrTime, const int64 i64CleaningTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Main function to clean NetSensor HashTables.

            Parameters:     -const int64 i64CurrTime: Current time, must be consistent with the
            timestamp associated with the has table entry.
            *************************************************************************************/
            int clearTables(const int64 i64CurrTime);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Set common variables

            Parameters:     -None
            **************************************************************************************/
            int commonConfiguration();

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Set standAlone variables

            Parameters:     -None
            **************************************************************************************/
            int configureForStandAlone();

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Main function to handle packets stored in the packet queue.

            Parameters:     -const uint8 * const pPacket:   Pointer to buffer containing
                                                            the packet.
                            -uint16 ui16PacketLen:          Packet size in Bytes.
                            -int64 i64CurrTime:             Current timestamp.
            *************************************************************************************/
            int handlePacket(const int classification,const uint8 * const pPacket, uint16 ui16PacketLen, int64 i64CurrTime);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Used to add IP entry to remote NetProxy IP table.

			Parameters:     uint32 uint32Ip: Add the IP to the remote gateways IP table.
			*************************************************************************************/
			void handleRemoteNetProxyAddresses(uint32 uint32Ip);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Helper function used to populate correctly a gateway entry for which we don’t have a MAC.

			Parameters:     uint32 uint32Ip: IP of the local gateway (NetProxy).
			*************************************************************************************/
			void handleLocalNetProxyAddresses(uint32 uint32Ip);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Utility function to decide whether or not a node is internal/external using the NetProxyTopology mechanism.

			Parameters:     -char* cSourceMac: Source MAC of the node under examination.
							-uint32 ui32SourceAddr: Source IP of the node under examination.
							-const int64 i64CurrTime: Current time.
							-bool bIsInternal: True if information comes from internal interface.
			*************************************************************************************/
			int internalExternalNetProxyTopologyHelper(NOMADSUtil::EtherFrameHeader * const pEthHeader, char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime, const int classification);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:	Utility function to decide whether or not a node is internal/external using the NetMask mechanism.

			Parameters:     -char* cSourceMac: Source MAC of the node under examination.
							-uint32 ui32SourceAddr: Source IP of the node under examination.
							-const int64 i64CurrTime: Current time.
							-bool bIsInternal: True if information comes from internal interface.
			*************************************************************************************/
			int internalExternalNetMaskTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:	Utility function to decide whether or not a node is internal/external using the LaxTopology mechanism.

			Parameters:     -char* cSourceMac: Source MAC of the node under examination.
							-uint32 ui32SourceAddr: Source IP of the node under examination.
							-const int64 i64CurrTime: Current time.
							-bool bIsInternal: True if information comes from internal interface.
			*************************************************************************************/
			int internalExternalLaxTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:	Utility function to decide whether or not a node is internal/external using the ArpTopology mechanism.

			Parameters:     -char* cSourceMac: Source MAC of the node under examination.
							-uint32 ui32SourceAddr: Source IP of the node under examination.
							-const int64 i64CurrTime: Current time.
							-bool bIsInternal: True if information comes from internal interface.
			*************************************************************************************/
			int internalExternalArpTopologyHelper(char* cSourceMac, uint32 ui32SourceAddr, const int64 i64CurrTime);
			
			bool isInMulticastRange(const int ui32DestAddr);



			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Utility function to add a possible gw to the gw table using the LaxTopology mechanism.

			Parameters:    -NOMADSUtil::String sMac: String sMac: Possible gateway MAC
						   -uint32 uint32Ip: Possible gateway IP.
			*************************************************************************************/
			int laxTopologyHelper(NOMADSUtil::String sMac, uint32 uint32Ip);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:	Handle the population of the gateway table using the LaxTopology mechanism.

			Parameters:		const int64 i64CurrTime: Current time.
							const uint8 * const pPacket: Pointer to packet buffer.
							uint16 ui16PacketLen: Packet length.

			*************************************************************************************/
			int laxTopologyMechanism(const int64 i64CurrTime, const uint8 * const pPacket, uint16 ui16PacketLen);
			
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Gateway detection function.

            Parameters:     -const uint8 * const pPacket:   Pointer to buffer containing
                                                            the packet.
                            -uint16 ui16PacketLen:          Packet size in Bytes.
                            -int64 i64CurrTime:             Current timestamp.
            *************************************************************************************/
            int lookForGWandPrepareARPCACHE(const int64 i64CurrTime, const uint8 * const pPacket, uint16 ui16PacketLen);
            
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Look for MACs with multiple IPs associated

            Parameters:     -pIPHeader pointer to packet's IP header
							-pEthHeader pointer to packet's ethernet header
            **************************************************************************************/
            int lookForMACwithMultipleIps(NOMADSUtil::IPHeader *pIPHeader, NOMADSUtil::EtherFrameHeader * const pEthHeader);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Used to populate the Internal and External hash tables.

            Parameters:     -const int64 i64CurrTime: Current timestamp.
							-const uint8 * const pPacket: Pointer to packet buffer.
							-NOMADSUtil::EtherFrameHeader * const pEthHeader: Pointer to Ethernet header.
							-NOMADSUtil::IPHeader *pIPHeader: Pointer to IP header.
							-uint16 ui16PacketLen: Packet lenght in bytes.
            *************************************************************************************/
            int populateInternalAndExternalTables(const int64 i64CurrTime, const int classification, const uint8 * const pPacket, NOMADSUtil::EtherFrameHeader * const pEthHeader, NOMADSUtil::IPHeader *pIPHeader, uint16 ui16PacketLen);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Used to populate the traffic by Source/Dest MAC hash table.

            Parameters: 
							const int64 i64CurrTime: Current timestamp.
							const uint8 * const pPacket: Pointer to packet buffer.
							NOMADSUtil::EtherFrameHeader * const pEthHeader: Pointer to Ethernet header.
							NOMADSUtil::IPHeader *pIPHeader: Pointer to IP header.
							uint16 ui16PacketLen: Packet lenght in bytes.
            *************************************************************************************/
            int populateMACtrafficTable(const int64 i64CurrTime, const uint8 * const pPacket, NOMADSUtil::EtherFrameHeader * const pEthHeader, uint16 ui16PacketLen);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016

            Description:    Used to populate the Multicast hash tables.
			Scope:			Protected
            Parameters:     
						const int64 i64CurrTime: Current timestamp.
						Packet  pPackees: Pointer to packet buffer..
            *************************************************************************************/
			int populateRoundTripHash(const int64 i64CurrTime, NET_SENSOR::PacketStructure  pPacket);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Used to populate the traffic by Source/Dest IP hash table.

			Parameters:     const int64 i64CurrTime: Current timestamp.
							const uint8 * const pPacket: Pointer to packet buffer.
							NOMADSUtil::EtherFrameHeader * const pEthHeader: Pointer to Ethernet header.
							NOMADSUtil::IPHeader *pIPHeader: Pointer to IP header.
							uint16 ui16PacketLen: Packet lenght in bytes.
			*************************************************************************************/
			int populateTrafficTable(const int64 i64CurrTime, const uint8 * const pPacket, NOMADSUtil::EtherFrameHeader *pEtHeader, 
				NOMADSUtil::IPHeader *pIPHeader, uint16 ui16IPHeaderLen, uint8 uint8Protocol, uint16 ui16PacketLen, int classification);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Utility function to check if an entry is already present and handle the insert/update process of an entry.

			Parameters:     const int64 i64CurrTime: Current timestamp.
							const uint8 * const pPacket: Pointer to packet buffer.
							NOMADSUtil::EtherFrameHeader * const pEthHeader: Pointer to Ethernet header.
							NOMADSUtil::IPHeader *pIPHeader: Pointer to IP header.
							uint16 ui16PacketLen: Packet lenght in bytes.
			*************************************************************************************/
			int populateTrafficTableHelper(uint32 ui32SrcAddr, uint32 ui32DestAddr, uint8 uint8Protocol, uint16 ui16DestPort, uint16 ui16PacketLen, const int64 i64CurrTime);
           
			/*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Prepare and sends traffic proto message

            Parameters:     -None
            Returns:        True if successfull
                            False if needs to be repeated
            *************************************************************************************/
            bool protoTraffic(void);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Prepare and sends topology proto message

            Parameters:     None
            Returns:        True if successfull
                            False if needs to be repeated
            *************************************************************************************/
            bool protoTopology(void);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016

			Description:    Prepare the Protobuf topology packet.

			Parameters:
			*************************************************************************************/
			bool prepareTopologyPacket();
			bool prepareTrafficPacket();

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Read Configuration file.

            Parameters:     -const char *configFile: Pointer to configuration file.
            *************************************************************************************/
            int readCfgFile(const char *configFile);

			char * retrieveProtocol(int protocolNumber);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016

			Description:    Send the Protobuf topology packet.

			Parameters:
			*************************************************************************************/
			void sendTopologyPacket();

			void sendTrafficPacket();
			void splitTrafficPacket();
			void splitTopologyPacket();
			bool isInternal(uint32 uint32addr);

			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    Add an entry to the default gateway table.

			Parameters:     NOMADSUtil::String mac: Mac of the gateway.
							NOMADSUtil::String ip: Ip of the gateway.

			**************************************************************************************/
			int setMacAdrrOfDefaultGW(NOMADSUtil::String mac, NOMADSUtil::String ip);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Send the string and append to the packet the ID.

            Parameters:     -const char* message pointer to the message to send.
                            -ID of the packet
            **************************************************************************************/
            int sendSerializedData(const char* message, const int size, const char* ID);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Store stringJson in the file at path filename

            Parameters:     pMessage: pointer to char containing the message file
                            pFilename: Pointer to char  containing the path
            **************************************************************************************/
            int writeOnFile(const char* pMessage,const char *pFilename);

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Actually update the ARP table and check for match with possible gw

            Parameters:     pARPPacket pointer to arp packet
            **************************************************************************************/
            int updateArpChacheAndCheckForMatch(NOMADSUtil::ARPPacket *pARPPacket);
			
			/*************************************************************************************
			Author:         Roberto Fronteddu rfronteddu@ihmc.us
			Year:           2016
			Scope:			Protected
			Description:    

			Parameters:     -google::protobuf::Timestamp *ts
			**************************************************************************************/
			void setProtobufTimestamp(google::protobuf::Timestamp *ts);
            
//THREADS
        private:
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Receiver Thread: Tasked to read packets from Pcap interface and put them
            in the packet queue.

            Parameters:     None.
            *************************************************************************************/
            class ReceiverThread : public NOMADSUtil::ManageableThread
            {
            public:
				NetSensor* _pNetSensor;
                NetworkInterface *_pNetInterface;
                ReceiverThread();
				int restartHandler();
                ~ReceiverThread();
				bool isInternal;
				uint32 queueMaxSize;
                void run();
            };
            NetSensor::ReceiverThread _ReceiverThread;
			NetSensor::ReceiverThread _InternalReceiverThread;
			
			int configureInternalInterface();
			int configureExternalInterface();
            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Rtt Handler Thread: Tasked to read packets from RTT packet queue and
            process them.

            Parameters:     None.
            *************************************************************************************/
            class RTTHandlerThread : public NOMADSUtil::ManageableThread
            {
            public:
				NetSensor* _pNetSensor;
                RTTHandlerThread();
                ~RTTHandlerThread();
                void run();
            };
            NetSensor::RTTHandlerThread _RTTHandlerThread;

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Handler Thread: Tasked to read packets from packet queue and
            process them.

            Parameters:     None.
            *************************************************************************************/
            class HandlerThread : public NOMADSUtil::ManageableThread
            {
            public:
				NetSensor* _pNetSensor;
                HandlerThread();
                ~HandlerThread();
                void run();
            };
            NetSensor::HandlerThread _HandlerThread;

            /*************************************************************************************
            Author:         Roberto Fronteddu rfronteddu@ihmc.us
            Year:           2016
			Scope:			Protected
            Description:    Cleaning Thread: Tasked to clean NetSensor has tables.

            Parameters:     None.
            *************************************************************************************/
            class CleanerThread : public NOMADSUtil::ManageableThread
            {
            public:
                NetSensor* _pNetSensor;
                CleanerThread();
                ~CleanerThread();

                void run();
            };
            NetSensor::CleanerThread _CleanerThread;

//VARIABLES
        protected:
            NetworkInterface *_pNetInterface;
			NetworkInterface *_pInternalNetInterface;
        private:   
            //period of stats sending
            uint32 _uint32Stat_update_time;
            
            //period of cleaning service
            uint32 _uint32Clean_time;

            //life of table entries
            uint32 _uint32Cleaning_time_t;        
            uint32 _uint32Cleaning_time_ie;       
            uint32 _uint32Cleaning_time_rt;         
            uint32 _uint32Cleaning_time_mac;     
            uint32 _uint32Cleaning_time_mc;       

            //statistic activation flags
			bool _bGwArpMechanism;
			bool _bArpTopology;
            bool _bTraffic;
            bool _bTopology;
            bool _bLaxTopology;
			bool _bNMaskTopology;
			bool _bNPSpecialTopology;
            bool _bMac;
            bool _bMulticast;
            bool _bRtt;
            bool _bDefaultGwAccountedFor;
			bool _bPrintTopology;
			bool _bPrintTraffic;
			bool _bDisableMulticastCount;
            //Notified address port
            uint32 _uint32Port;
			uint32 _uint32Mtu;
            //Log proto packet on file
            bool _bWriteOnFile;

			char *pLocalGwIP1;
			char *pLocalGwIP2;
			char *pLocalMAC1;
			char *pLocalMAC2;
			char *pDefaultIp;
			char *pDefaulMac;
			char *pLocalGwName1;
			char *pLocalGwName2;

            //notified address IP 
            char _cIp[80];

            //Interface to sniff
            char _cInterface[80];
			char _cInternalInterface[80];
            //self explanatory global variables
            bool _bBuiltInMode;
            bool _bDefaultGWalreadyAccountedFor;

            //NP adresses TODO: Switch to a list model
            uint32 _uint32Local_NP_IP;      //local NP address
            uint32 _uint32R1_NP_IP;         //Remote NP address
            uint32 _uint32R2_NP_IP;         //Remote NP address
			uint32 _uint32ExternalInterfaceGwIp;
            
            int64  _int64StartTime;

            //Local GW address
            NOMADSUtil::EtherMACAddr _emacGwMacAddr;    
            uint32 _uint32GwIPAddr;

            //NS mac
            NOMADSUtil::EtherMACAddr _emacNetSensorMAC;
            //NP mac
			NOMADSUtil::EtherMACAddr _emacProxyInternalMAC;
			NOMADSUtil::EtherMACAddr _emacProxyExternalMAC;

			//NetProxy variables
			char* _pProxyInternalMacAddr;
			char* _pProxyExternalMacAddr;
			uint32 _uint32NetProxyExternalInterfaceIp;
			NOMADSUtil::LList<uint32> *_uint32listNetProxyRemoteAddress;



			//Multicast utilities
			uint32 _minMulticastAddress;
			uint32 _maxMulticastAddress;


        private:
			ddam::Container _trafficContainer;
			ddam::Container _topologyContainer;

            ddam::TopologyParts _topologyInstance;

            NOMADSUtil::UDPDatagramSocket *_pNotifierSocket;
            NOMADSUtil::InetAddr _notifyAddr;
			
			NOMADSUtil::Mutex _macStatsMUTEX;
            NOMADSUtil::StringHashtable<SourceMac> _macTrafficStats;
            
            NOMADSUtil::Mutex _IETrafficStatsMUTEX;
            NOMADSUtil::UInt32Hashtable<Mac> _internalTrafficStats;
            NOMADSUtil::UInt32Hashtable<Mac> _externalTrafficStats;

            NOMADSUtil::Mutex _IOTrafficStatsMUTEX;
            NOMADSUtil::UInt32Hashtable<PerNodeTrafficStatsIP1> _trafficTable;
            
            NOMADSUtil::StringHashtable<macPerIp> _ipsPerMacTable;

            NOMADSUtil::StringHashtable<IpPerGW> _gwTable;

            NOMADSUtil::Mutex _RTTrafficStatsMUTEX;
            NOMADSUtil::UInt32Hashtable<RoundTripStat> _roundTripStatTable;

            //Packets reading queue
            NOMADSUtil::Mutex _QueueMUTEX;
			NOMADSUtil::Queue<NET_SENSOR::PacketStructure> _packetQueue;

            NOMADSUtil::Mutex _RTTQueueMUTEX;
			NOMADSUtil::Queue<NET_SENSOR::PacketStructure> _RTTpacketQueue;
            ARPCache _pARPCache;
            ARPCache _gwIpMacCache;   
			NOMADSUtil::LList<uint32> *_forwardedIP;

	};
}
#endif   // #ifndef INCL_NET_SENSOR_H

