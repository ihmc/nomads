/* 
 * NetworkInterfaceManager.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on May 16, 2015, 7:30 AM
 */

#include "NetworkInterfaceManager.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "NetUtils.h"
#include "NLFLib.h"
#include "NMSProperties.h"
#include "ProxyDatagramSocket.h"
#include "StringTokenizer.h"

#include "ManycastForwardingNetworkInterface.h"
#include "ManycastNetworkMessageReceiver.h"
#include "NetworkMessageReceiver.h"
#include "NetworkMessageV2.h"
#include "NetworkInterfaceFactory.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg
#include "ifaces/LocalNetworkInterface.h"

namespace NOMADSUtil
{
    const char **parseListOfStrings (const char *pszValue)
    {
        if (pszValue == NULL) {
            return NULL;
        }
        // Count separators to infer the number of interfaces
        const unsigned int uiLen = strlen (pszValue);
        if (uiLen == 0) {
            return NULL;
        }
        const char SEPARATOR = ';';
        unsigned int uiNIfaces = 1U;
        for (unsigned int i = 0; i < uiLen; i++) {
            if (pszValue[i] == SEPARATOR) {
                ++uiNIfaces;
            }
        }
        if (uiNIfaces == 0) {
            return NULL;
        }
        const char *pszToken;
        const char **ppszIfaces = static_cast<const char **>(calloc (uiNIfaces + 1, sizeof (char*)));
        if (ppszIfaces == NULL) {
            return NULL;
        }
        StringTokenizer tokenizer (pszValue, SEPARATOR, SEPARATOR);
        for (unsigned int i = 0; (pszToken = tokenizer.getNextToken ()) != NULL;) {
            if ((ppszIfaces[i] = strDup (pszToken)) != NULL) {
                ++i;
            }
        }
        return ppszIfaces;
    }

    void parseLocalInterfaces (ConfigManager *pCfgMgr, const char **&ppszBindingInterfaces,
                               const char **&ppszIgnoredInterfaces, const char **&ppszAddedInterfaces)
    {
        if (pCfgMgr == NULL) {
            return;
        }
        ppszBindingInterfaces = parseListOfStrings (pCfgMgr->getValue (NMSProperties::NMS_REQUIRED_INTERFACES));
        ppszIgnoredInterfaces = parseListOfStrings (pCfgMgr->getValue (NMSProperties::NMS_IGNORED_INTERFACES));
        ppszAddedInterfaces = parseListOfStrings (pCfgMgr->getValue (NMSProperties::NMS_OPTIONAL_INTERFACES));
    }

    void selectInterfaces (const char **ppIfaces, StringHashset &set, StringHashset *pFilter = NULL)
    {
        if (ppIfaces != NULL) {
            for (uint16 i = 0; ppIfaces[i] != NULL; i++) {
                const String ifaceAddr (ppIfaces[i]);
                const bool bSelect = (pFilter == NULL) || (!pFilter->containsKeyWild (ppIfaces[i]));
                if (bSelect) {
                    set.put (ppIfaces[i]);
                }
                checkAndLogMsg ("selectInterfaces", Logger::L_Info, "%s %s %s.\n",
                                bSelect ? "selecting" : "ignoring",
                                ifaceAddr.endsWith ("*") ? "interfaces matching" : "interface",
                                ifaceAddr.c_str());
            }
        }
    }

    int sendInternal (const NetworkMessage *pNetMsg, NetworkInterface *pNetInt, const char *pszSrcAddr, uint32 ui32DstAddr, bool bExpedited, const char *pszHints)
    {
        if ((pNetMsg == NULL) || (pNetInt == NULL) || (!pNetInt->canSend()) ||
            (pszSrcAddr == NULL) || (0 == stricmp (pszSrcAddr, NetworkInterface::IN_ADDR_ANY_STR))) {
            return -1;
        }
        checkAndLogMsg ("NetworkInterfaceManager::sendInternal", Logger::L_HighDetailDebug, "interface: %s\n", pszSrcAddr);
        if (ui32DstAddr == NetworkMessageService::EMPTY_RECIPIENT) {
            // Broadcast (or multicast) to the whole network (or group)
            pNetInt->sendMessage (pNetMsg, bExpedited, pszHints);
        }
        else {
            // Unicast the specified address
            pNetInt->sendMessage (pNetMsg, ui32DstAddr, bExpedited, pszHints);
        }
        return 0;
    }
}

using namespace NOMADSUtil;

NetworkInterfaceManager::NetworkInterfaceManager (PROPAGATION_MODE mode, bool bReplyViaUnicast, bool bAsyncTransmission)
    : _mode (mode),
      _bPrimaryInterfaceIdSet (false),
      _bReplyViaUnicast (bReplyViaUnicast),
      _bAsyncTransmission (bAsyncTransmission),
      _bRejoinMcastGrp (true),
      _ui8MTTL (1),
      _ui16MTU (NetworkMessageService::DEFAULT_MTU),
      _ui16Port (6669),
      _ui32PrimaryInterface (0U),
      _ui32SampleInterval (500),
      _ui32MaxMsecsInOutgoingQueue (5000U),
      _pListener (NULL),
      _interfaces (false,  // bCaseSensitiveKeys
                   true,   // bCloneKeys
                   true,   // bDeleteKeys
                   false)  // bDeleteValues

{
}

NetworkInterfaceManager::~NetworkInterfaceManager (void)
{
}

void NetworkInterfaceManager::run (void)
{
    while (!terminationRequested()) {
        sleepForMilliseconds (15000);
        _m.lock();
        for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
            NetworkInterface *pIface = iter.getValue();
            if (MULTICAST == pIface->getMode()) {
                static_cast<LocalNetworkInterface *>(pIface)->rejoinMulticastGroup ();
            }
        }
        _m.unlock();
    }
}

int NetworkInterfaceManager::init (ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return -1;
    }
    const uint16 ui16Port = static_cast<uint16>(pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_PORT, NetworkMessageService::DEFAULT_PORT));
    const String outgoingGroupAddr (pCfgMgr->getValue (NMSProperties::NMS_OUTGOING_ADDR, (_mode == BROADCAST) ? NMS_BROADCAST_ADDRESS : NMS_MULTICAST_ADDRESS));
    const uint32 ui32TTL = pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_TTL, NetworkMessageService::DEFAULT_MCAST_TTL);
    if (ui32TTL > 0xFF) {
        return -2;
    }
    if (pCfgMgr->hasValue (NMSProperties::NMS_PRIMARY_INTERFACE)) {
        const String primaryInterface = pCfgMgr->getValue (NMSProperties::NMS_PRIMARY_INTERFACE);
        setPrimaryInterface (primaryInterface);
    }
    if (_mode != MULTICAST) {
        _bRejoinMcastGrp = false;
    }
    else {
        _bRejoinMcastGrp = pCfgMgr->getValueAsBool (NMSProperties::NMS_PERIODIC_MULTICAST_GROUP_REJOIN, true);
    }

    const char **ppszBindingInterfaces = NULL;
    const char **ppszIgnoredInterfaces = NULL;
    const char **ppszAddedInterfaces = NULL;
    parseLocalInterfaces (pCfgMgr, ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces);
    return init (ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces,
                 outgoingGroupAddr, ui16Port, static_cast<uint8>(ui32TTL));
}

int NetworkInterfaceManager::init (const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces,
                                   const char **ppszAddedInterfaces, const char *pszDestinationAddr,
                                   uint16 ui16Port, uint8 ui8TTL)
{
    const char *pszMethodName = "NetworkInterfaceManager::init";
    if (pszDestinationAddr == NULL) {
        if (_mode == BROADCAST) {
            _dstAddr = NMS_BROADCAST_ADDRESS;
        }
        else if (_mode == NORM) {
            _dstAddr = NMS_MULTICAST_ADDRESS;
        }
        else {
            _dstAddr = NMS_MULTICAST_ADDRESS;
        }
    }

    reset();

    _ui8MTTL = ui8TTL;
    _ui16Port = ui16Port;
    if (_dstAddr.length() <= 0) {
        _dstAddr = pszDestinationAddr;
    }

    selectInterfaces (ppszIgnoredInterfaces, _forbiddenInterfaces);
    selectInterfaces (ppszBindingInterfaces, _requieredInterfaces, &_forbiddenInterfaces);
    selectInterfaces (ppszAddedInterfaces, _optionalInterfaces, &_optionalInterfaces);

    if (_requieredInterfaces.getCount() == 0) {
        // If no binding interfaces were specified, use all the available interfaces, except those
        // specified in ppszIgnoredInterfaces, and adding any specified in ppszAddedInterfaces
        NICInfo **ppIfaces = NetUtils::getNICsInfo (false, false);
        if (ppIfaces != NULL) {
            for (uint16 i = 0; ppIfaces[i] != NULL; i++) {
                const String iface (ppIfaces[i]->getIPAddrAsString());
                if (!_forbiddenInterfaces.containsKeyWild (iface)) {
                    _requieredInterfaces.put (iface);
                }
            }
			NetUtils::freeNICsInfo (ppIfaces);
		}
    }
    else {
        // otherwise, look for all the available interfaces that match the required ones
        for (StringHashset::Iterator iter = _requieredInterfaces.getAllElements(); !iter.end(); iter.nextElement()) {
            const String ifaceAddr (iter.getKey());
            NICInfo *pIface = NetUtils::getNICInfo (ifaceAddr);
            if (pIface == NULL) {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not find required interface %s.\n",
                                ifaceAddr.c_str());
            }
            delete pIface;
        }
    }

    checkAndLogMsg (pszMethodName, Logger::L_Info, "initializing required interfaces.\n");
    bindInterfaces (_requieredInterfaces);
    checkAndLogMsg (pszMethodName, Logger::L_Info, "initializing optional interfaces.\n");
    bindInterfaces (_optionalInterfaces);

    // Initialize the primary interface, if it has not been done yet
    if (!_bPrimaryInterfaceIdSet) {
        for (Interfaces::Iterator iter = _interfaces.getAllElements();!iter.end(); iter.nextElement()) {
            const String addr (iter.getValue()->getBindingInterfaceSpec());
            if (addr.length() >= 0 && !addr.startsWith (ProxyDatagramSocket::ADDRESS_PREFIX)) {
                InetAddr primary (addr);
                _ui32PrimaryInterface = primary.getIPAddress();
                _bPrimaryInterfaceIdSet = true;
                break;
            }
        }
    }

#ifndef WIN32
    // When binding to a specific address, the UDP socket is not receiving
    // broadcast packets any more. This is the de-facto behavior for UDP
    // sockets ever. Win32 takes a stance that diverges from the standard,
    // allowing the reception of broadcasts anyways. To receive broadcasts
    // in Linux (and UNIX) systems we have to bind an additional UDP socket
    // to the wildcard address.
    if (this->_mode != NORM) {
        checkAndLogMsg(pszMethodName, Logger::L_Info, "initializing any_addr interface.\n");
        bindInterface(NetworkInterface::IN_ADDR_ANY_STR);
    }
#endif

    setSampleRate (_interfaces);
    return 0;
}

void NetworkInterfaceManager::addFwdingAddrToManycastIface (uint32 ui32SourceAddr, const char *pszIncomingInterface)
{
    if (pszIncomingInterface == NULL) {
        return;
    }
    static const String BROADCAST_ADDR_STR ("255.255.255.255");
    if (BROADCAST_ADDR_STR != pszIncomingInterface) {
        NetworkInterface *pNetIf = _interfaces.get (pszIncomingInterface);
        if ((pNetIf != NULL) && (pNetIf->getType() == ManycastForwardingNetworkInterface::MCAST_FWD_IF) && (isUnicast (ui32SourceAddr, pszIncomingInterface))) {
            ManycastForwardingNetworkInterface *pMcastFwdIf = static_cast<ManycastForwardingNetworkInterface*>(pNetIf);
            pMcastFwdIf->addForwardingIpAddress (ui32SourceAddr);
        }
    }
}

bool NetworkInterfaceManager::clearToSend (const char *pszInterface)
{
    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    if (pNetIf != NULL) {
        return pNetIf->clearToSend();
    }
    return false;
}

bool NetworkInterfaceManager::clearToSendOnAllInterfaces (void)
{
    for (StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        NetworkInterface *pNetIf = iter.getValue();
        if ((pNetIf->canSend()) && (!pNetIf->clearToSend())) {
            return false;
        }
    }
    return true;
}

char ** NetworkInterfaceManager::getActiveNICsInfoAsString (void)
{
    resolveProxyDatagramSocketAddresses();

    unsigned int ulLen = _interfaces.getCount() + 1;
    if (ulLen == 1) {
        return NULL;
    }
    char **ppszInterfaces = static_cast<char **>(calloc (ulLen, sizeof (char *)));
    if (ppszInterfaces != NULL) {
        StringHashtable<NetworkInterface>::Iterator iter = _interfaces.getAllElements();
        for (unsigned int i = 0; !iter.end() && i < ulLen; iter.nextElement()) {
            NetworkInterface *pNetInt = iter.getValue();
            if ((pNetInt != NULL) && (pNetInt->getNetworkAddr() != NULL) && (pNetInt->isAvailable()) && (!pNetInt->boundToWildcardAddr())) {
                ppszInterfaces[i] = strDup (pNetInt->getNetworkAddr());
                if (ppszInterfaces[i] != NULL) {
                    i++;
                }
            }
        }
    }
    return ppszInterfaces;
}

char ** NetworkInterfaceManager::getActiveNICsInfoAsStringForDestinationAddr (const char *pszDestination)
{
    const String sAddr (pszDestination);
    if (sAddr.startsWith (ProxyDatagramSocket::ADDRESS_PREFIX)) {
        // TODO: implement this case
    }
    else if (InetAddr::isIPv4Addr (pszDestination)) {
        InetAddr addr (pszDestination);
        return getActiveNICsInfoAsStringForDestinationAddr (addr.getIPAddress());
    }
    return NULL;
}

char ** NetworkInterfaceManager::getActiveNICsInfoAsStringForDestinationAddr (uint32 ulSenderRemoteIPv4Addr)
{
    unsigned int ulLen = _interfaces.getCount() + 1;
    if (ulLen == 1) {
        return NULL;
    }
    char **ppszInterfaces = static_cast<char **>(calloc (ulLen, sizeof (char *)));
    if (ppszInterfaces != NULL) {
        Interfaces::Iterator iter = _interfaces.getAllElements();
        for (unsigned int i = 0; !iter.end() && i < ulLen; iter.nextElement()) {
            NetworkInterface *pNetInt = iter.getValue();
            if ((pNetInt != NULL) && (pNetInt->getNetworkAddr() != NULL) && (pNetInt->isAvailable()) && (!pNetInt->boundToWildcardAddr())) {
                InetAddr senderRemoteAddr (ulSenderRemoteIPv4Addr);
                if (NetUtils::areInSameNetwork (pNetInt->getNetworkAddr(), pNetInt->getNetmask(),
                                                senderRemoteAddr.getIPAsString(), pNetInt->getNetmask())) {
                    ppszInterfaces[i] = strDup (pNetInt->getNetworkAddr());
                    if (ppszInterfaces[i] != NULL) {
                        i++;
                    }
                }
            }
        }
    }
    if ((ppszInterfaces != NULL) && (ppszInterfaces[0] == NULL)) {
        free (ppszInterfaces);
        ppszInterfaces = NULL;
    }
    return ppszInterfaces;
}

uint16 NetworkInterfaceManager::getMinMTU (void)
{
    uint16 ui16MinMTU = _ui16MTU;    // Set it to the default MTU
    for (Interfaces::Iterator i = _interfaces.getAllElements(); !i.end(); i.nextElement()) {
        NetworkInterface *pNIC = i.getValue();
        if ((pNIC != NULL) && (pNIC->canSend()) && (pNIC->isAvailable())) {
            if (ui16MinMTU == 0) {
                ui16MinMTU = pNIC->getMTU();
            }
            else if (pNIC->getMTU() < ui16MinMTU) {
                ui16MinMTU = pNIC->getMTU();
            }
        }
    }
    return (ui16MinMTU < NetworkMessageV2::FIXED_HEADER_LENGTH) ?
        0U : (ui16MinMTU - NetworkMessageV2::FIXED_HEADER_LENGTH);
}

uint16 NetworkInterfaceManager::getMTU (void) const
{
    return _ui16MTU;
}

String NetworkInterfaceManager::getOutgoingInterfaceForAddr (unsigned long int ulRemoteAddr)
{
    for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        NetworkInterface *pNetIf = iter.getValue();
        if ((pNetIf != NULL) && (pNetIf->getNetworkAddr() != NULL) && (pNetIf->isAvailable())) {
            InetAddr senderRemoteAddr (ulRemoteAddr);
            if (NetUtils::areInSameNetwork (pNetIf->getNetworkAddr(), pNetIf->getNetmask(),
                                            senderRemoteAddr.getIPAsString(), pNetIf->getNetmask())) {
                return String (pNetIf->getNetworkAddr());
            }
        }
    }
    return String();
}

bool NetworkInterfaceManager::isPrimaryIfaceSet (void) const
{
    return _bPrimaryInterfaceIdSet;
}

String NetworkInterfaceManager::tryToGuessIncomingIface (void)
{
    String guess;
    for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        NetworkInterface *pNetInt = iter.getValue();
        if (pNetInt != NULL && pNetInt->isAvailable()) {
            const char *pszInterfaceAddr = pNetInt->getNetworkAddr();
            if (pszInterfaceAddr != NULL && (strcmp (pszInterfaceAddr, "0.0.0.0") != 0)) {
                if (guess.length() <= 0) {
                    guess = pszInterfaceAddr;
                }
                else {
                    // There are more than one connected network interface, I can't guess
                    return String();
                }
            }
        }
    }
    return guess;
}

uint32 NetworkInterfaceManager::getLinkCapacity (const char *pszInterface)
{
    const char *pszMethodName = "NetworkInterfaceManager::getLinkCapacity";
    if (pszInterface == NULL) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "pszInterface is NULL\n");
        return 0U;
    }
    else {
        NetworkInterface *pNetIf = _interfaces.get (pszInterface);
        if (pNetIf == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "interface %s not found\n", pszInterface);
            return 0U;
        }
        else {
            return pNetIf->getLinkCapacity();
        }
    }
}

uint32 NetworkInterfaceManager::getPrimaryInterface (void) const
{
    return _ui32PrimaryInterface;
}

int64 NetworkInterfaceManager::getReceiveRate (const char *pszAddr)
{
    if (pszAddr == NULL) {
        return -1;
    }
    _m.lock();
    NetworkInterface *pNetIF = _interfaces.get (pszAddr);
    if (pNetIF == NULL) {
        _m.unlock();
        return -2;
    }
    if (!InetAddr::isIPv4Addr (pszAddr)) {
        _m.unlock();
        return -3;
    }
    
    uint32 ui32Bytes = pNetIF->getReceiveRate();

    // In UNIX the INADDR_ANY address must be bound in order to receive broadcast
    // messages (refer at the comment in init() for more information); thus it
    // is necessary to check whether traffic for the interface identified by
    // ui32Addr was received by the socket listening on 0.0.0.0.
    pNetIF = _interfaces.get (NetworkInterface::IN_ADDR_ANY_STR);
    if (pNetIF != NULL) {
        NetworkMessageReceiver *pRec = pNetIF->getReceiver();
        if (pRec != NULL && pRec->getType() == NetworkMessageReceiver::MCAST_NET_RCV) {
            ui32Bytes += static_cast<ManycastNetworkMessageReceiver*>(pRec)->getRateByBcastAddr (pNetIF->getBroadcastAddr());
        }
    }

    _m.unlock();
    return ui32Bytes;
}

uint8 NetworkInterfaceManager::getRescaledTransmissionQueueSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    _m.lock();
    uint32 ui32RescaledQueueSize = 0U; 
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf != NULL) {
        ui32RescaledQueueSize = pNetIf->getRescaledTransmissionQueueSize();
    }
    _m.unlock();
    return ui32RescaledQueueSize;
}

uint32 NetworkInterfaceManager::getTransmissionQueueMaxSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    uint32 ui32MaxQueueSize = 0U; 
    _m.lock();
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf != NULL) {
        ui32MaxQueueSize = pNetIf->getTransmissionQueueMaxSize();
    }
    return ui32MaxQueueSize;
}

uint32 NetworkInterfaceManager::getTransmissionQueueSize (const char *pszOutgoingInterface)
{
    if (pszOutgoingInterface == NULL) {
        return 0;
    }
    uint32 ui32QueueSize = 0U;
    _m.lock();
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf != NULL) {
        ui32QueueSize = pNetIf->getTransmissionQueueSize();
    }
    _m.unlock();
    return ui32QueueSize;
}

uint32 NetworkInterfaceManager::getTransmitRateLimit (const char *pszInterface)
{
    if (pszInterface == NULL) {
        return 0U;
    }
    uint32 ui32Rate = 0U;
    _m.lock();
    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    if (pNetIf != NULL) {
        ui32Rate = pNetIf->getTransmitRateLimit();
    }
    _m.unlock();
    return ui32Rate;
}

bool NetworkInterfaceManager::isUnicast (uint32 ui32Address, const char *pszIncomingInterface)
{
    if (ui32Address == INADDR_BROADCAST || pszIncomingInterface == NULL) {
        return false;
    }

    _m.lock();

    if (NetUtils::isMulticastAddress (inet_ntoa (*(struct in_addr *)&ui32Address))) {
        _m.unlock();
        return false;
    }

    NetworkInterface *pNetIf = _interfaces.get (pszIncomingInterface);
    if (pNetIf != NULL && !pNetIf->boundToWildcardAddr()) {          /*!!*/ // Check how this will work for the CSR
        const char *pszNetmask = pNetIf->getNetmask();
        if (pszNetmask != NULL) {
            // TODO: this assumes an IPv4 address - fix it
            InetAddr mask (pszNetmask);
            if (NetUtils::isBroadcastAddress (ui32Address, mask.getIPAddress())) {
                _m.unlock();
                return false;
            }
        }
    }

    _m.unlock();
    return true;
}

bool NetworkInterfaceManager::isSupportedManycastAddr (uint32 ui32Address)
{
    if (ui32Address == INADDR_BROADCAST) {
        return true;
    }

    _m.lock();
    if (NetUtils::isMulticastAddress (inet_ntoa (*(struct in_addr *)&ui32Address))) {
        _m.unlock();
        return true;
    }

    for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        NetworkInterface *pNetIf = iter.getValue();
        if (pNetIf != NULL && !pNetIf->boundToWildcardAddr()) {          /*!!*/ // Check how this will work for the CSR
            const char *pszNetmask = pNetIf->getNetmask();
            if (pszNetmask != NULL) {
                // TODO: this assumes an IPv4 address - fix it
                InetAddr mask (pszNetmask);
                if (NetUtils::isBroadcastAddress (ui32Address, mask.getIPAddress())) {
                    _m.unlock();
                    return true;
                }
            }
        }
    }

    _m.unlock();
    return false;
}

int NetworkInterfaceManager::registerListener (NetworkInterfaceManagerListener *pListener)
{
    _m.lock();
    _pListener = pListener;
    _m.unlock();
    return 0;
}

void NetworkInterfaceManager::reset (void)
{
    _m.lock();
    _requieredInterfaces.removeAll();
    _optionalInterfaces.removeAll();
    _forbiddenInterfaces.removeAll();
    _proxyInterfacesToResolve.removeAll();
    _interfaces.removeAll();
    _m.unlock();
}

bool NetworkInterfaceManager::send (NetworkMessage *pNetMsg, const char **ppszOutgoingInterfaces,
                                    uint32 ui32Address, bool bExpedited, const char *pszHints)
{
    const char *pszMethodName = "NetworkInterfaceManager::sendNetworkMessage";
    bool bAtLeastOneIF = false;
    if (ppszOutgoingInterfaces == NULL || ppszOutgoingInterfaces[0] == NULL) {
        // No interface selected: send out on all interfaces (eventually pick one)
        checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "sending from all interfaces\n");
        _m.lock();
        for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
            if (sendInternal (pNetMsg, iter.getValue(), iter.getKey(), ui32Address, bExpedited, pszHints) == 0) {
                bAtLeastOneIF = true;
            }
        }
        _m.unlock();
    }
    else {
        _m.lock();
        for (uint8 i = 0; ppszOutgoingInterfaces[i] != NULL; i++) {
            checkAndLogMsg (pszMethodName, Logger::L_HighDetailDebug, "sending from "
                            "interface %s\n", ppszOutgoingInterfaces[i]);
            if (sendInternal (pNetMsg, _interfaces.get (ppszOutgoingInterfaces[i]),
                      ppszOutgoingInterfaces[i], ui32Address, bExpedited, pszHints) == 0) {
                bAtLeastOneIF = true;
            }
            else {
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not send from "
                                "specified interface: %s\n", ppszOutgoingInterfaces[i]);
            }            
        }
        _m.unlock();
    }

    return bAtLeastOneIF;
}

void NetworkInterfaceManager::setLinkCapacity (const char *pszInterface, uint32 ui32Capacity)
{
    const char *pszMethodName = "NetworkInterfaceManager::setLinkCapacity";
    if (pszInterface != NULL) {
        _m.lock();
        NetworkInterface *pNetIf = _interfaces.get (pszInterface);
        if (pNetIf == NULL) {
            checkAndLogMsg (pszMethodName, Logger::L_Warning,
                            "interface %s not found\n", pszInterface);
        }
        else {
            pNetIf->setLinkCapacity (ui32Capacity);
        }
        _m.unlock();
    }
}

int NetworkInterfaceManager::setPrimaryInterface (const char *pszInterfaceAddr)
{
    if (pszInterfaceAddr == NULL) {
        return -1;
    }
    _m.lock();
    if (_bPrimaryInterfaceIdSet) {
        // Once set, the primary interface can't be changed
        _m.unlock();
        return -2;
    }
    InetAddr primary (pszInterfaceAddr);
    _ui32PrimaryInterface = primary.getIPAddress();
    _bPrimaryInterfaceIdSet = true;
    _m.unlock();
    return 0;
}

int NetworkInterfaceManager::setTransmissionQueueMaxSize (const char *pszOutgoingInterface, uint32 ui32MaxSize)
{
    if (pszOutgoingInterface == NULL) {
        return -1;
    }
    _m.lock();
    NetworkInterface *pNetIf = _interfaces.get (pszOutgoingInterface);
    if (pNetIf != NULL) {
        pNetIf->setTransmissionQueueMaxSize (ui32MaxSize);
    }
    _m.unlock();
    return (pNetIf == NULL ? -2 : 0);
}

int NetworkInterfaceManager::setTransmitRateLimit (uint32 ui32RateLimit)
{
    _m.lock();
    for (Interfaces::Iterator i = _interfaces.getAllElements(); !i.end(); i.nextElement()) {
        i.getValue()->setTransmitRateLimit (ui32RateLimit);
    }
    _m.unlock();
    return 0;
}

int NetworkInterfaceManager::setTransmitRateLimit (const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    if (pszDestinationAddress == NULL) {
        return -1;
    }
    _m.lock();
    for (Interfaces::Iterator i = _interfaces.getAllElements(); !i.end(); i.nextElement()) {
        i.getValue()->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
    }
    _m.unlock();
    return 0;
}

int NetworkInterfaceManager::setTransmitRateLimit (const char *pszInterface, const char *pszDestinationAddress, uint32 ui32RateLimit)
{
    if (pszInterface == NULL) {
        return -1;
    }
    _m.lock();
    NetworkInterface *pNetIf = _interfaces.get (pszInterface);
    int rc;
    if (pNetIf == NULL) {
        rc = -2;
    }
    else if (pszDestinationAddress == NULL) {
        rc = pNetIf->setTransmitRateLimit (ui32RateLimit);
    }
    else {
        rc = pNetIf->setTransmitRateLimit (pszDestinationAddress, ui32RateLimit);
    }
    _m.unlock();
    return rc;
}

void NetworkInterfaceManager::start (void)
{
    _m.lock();
    for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        iter.getValue()->start();
    }
    _m.unlock();
    ManageableThread::start();
}

void NetworkInterfaceManager::stop (void)
{
    _m.lock();
    for (Interfaces::Iterator iter = _interfaces.getAllElements(); !iter.end(); iter.nextElement()) {
        iter.getValue()->stop();
    }
    _m.unlock();
    ManageableThread::requestTermination();
}

void NetworkInterfaceManager::bindInterface (const String &ifaceAddr)
{
    if ((_pListener == NULL) || (_dstAddr.length() <= 0)) {
        return;
    }
    const char *pszMethodName = "NetworkInterfaceManager::bindInterface";
    const bool bReceiveOnly = (ifaceAddr ==  NetworkInterface::IN_ADDR_ANY_STR);
    NetworkInterface *pNetInt = NetworkInterfaceFactory::getNetworkInterface (ifaceAddr, _mode, _bAsyncTransmission, _bReplyViaUnicast);
    if (pNetInt == NULL) {
        return;
    }

    int rc = pNetInt->init (_ui16Port, ifaceAddr, _pListener, bReceiveOnly, false, _dstAddr, _ui8MTTL);
    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "initialized %s for network address %s/%s/%s. Current receive buffer size is %d\n",
                        (pNetInt->getType() == ManycastForwardingNetworkInterface::MCAST_FWD_IF ? "ManycastForwardingNetworkInterface" : "NetworkInterface"),
                        pNetInt->getBindingInterfaceSpec(), pNetInt->getNetmask(), pNetInt->getBroadcastAddr(), pNetInt->getReceiveBufferSize());
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_MildError, "could not initialize NetworkInterface for network address %s; rc = %d\n",
                        (const char *) ifaceAddr, rc);
    }

    if (ifaceAddr.startsWith (ProxyDatagramSocket::ADDRESS_PREFIX)) {
        _proxyInterfacesToResolve.append (pNetInt);
    }
    else {
        const int iBufSize = 2 * 1024 * 1024; // 2 MB
        pNetInt->setReceiveBufferSize (iBufSize);
        checkAndLogMsg (pszMethodName, Logger::L_Info, "set receive receive buffer size for %s to %d; "
                        "current UDP receive buffer size is %d\n", pNetInt->getBindingInterfaceSpec(),
                        iBufSize, pNetInt->getReceiveBufferSize());
    }

    _interfaces.put (ifaceAddr, pNetInt);
}

void NetworkInterfaceManager::bindInterfaces (StringHashset &ifaces)
{
    for (StringHashset::Iterator iter = ifaces.getAllElements(); !iter.end(); iter.nextElement()) {
        const String iface (iter.getKey());
        bindInterface (iface);
    }
}

int NetworkInterfaceManager::resolveProxyDatagramSocketAddresses (void)
{
    const char *pszMethodName = "NetworkInterfaceManager::resolveProxyDatagramSocketAddresses";
    _mProxyInterfacesToResolve.lock();
    if (_proxyInterfacesToResolve.isEmpty()) {
        _mProxyInterfacesToResolve.unlock();
        return 0;
    }
    NetworkInterface *pNetInt;
    // bool bResolved = false;
    _proxyInterfacesToResolve.resetGet();
    while (NULL != (pNetInt = _proxyInterfacesToResolve.getNext())) {
        if (pNetInt->getNetworkAddr() != NULL) {
            InetAddr actualAddr (pNetInt->getNetworkAddr());
            _interfaces.put (actualAddr.getIPAsString(), pNetInt);
            checkAndLogMsg (pszMethodName, Logger::L_Info, "resolved the address for %s to be %s\n",
                            pNetInt->getBindingInterfaceSpec(), pNetInt->getNetworkAddr());
            if (!_bPrimaryInterfaceIdSet) {
                InetAddr actualAddr (pNetInt->getNetworkAddr());
                _ui32PrimaryInterface = actualAddr.getIPAddress();
                _bPrimaryInterfaceIdSet = true;
                checkAndLogMsg (pszMethodName, Logger::L_Info, "set the primary "
                                "interface address to be %s\n", pNetInt->getNetworkAddr());
            }
			pNetInt->start();
            _proxyInterfacesToResolve.remove (pNetInt);
            _proxyInterfacesToResolve.resetGet();      // Must restart the iteration when deleting elements from the list
        }
        else {
            checkAndLogMsg (pszMethodName, Logger::L_LowDetailDebug, "not yet "
                            "resolved address for %s\n", pNetInt->getBindingInterfaceSpec());
        }
    }
    _mProxyInterfacesToResolve.unlock();
    return 0;
}

void NetworkInterfaceManager::setSampleRate (NetworkInterface *pIface)
{
    if (pIface == NULL) {
        return;
    }
    pIface->setReceiveRateSampleInterval (_ui32SampleInterval);
    if ((_ui32MaxMsecsInOutgoingQueue > 0) && (_bAsyncTransmission)) {
        pIface->setAutoResizeQueue (true, _ui32MaxMsecsInOutgoingQueue);
        checkAndLogMsg ("NetworkInterfaceManager::setSampleRate", Logger::L_Info,
                        "transmission queue auto-resizing enabled for interface %s\n",
                        pIface->getBindingInterfaceSpec());
    }
}

void NetworkInterfaceManager::setSampleRate (Interfaces &ifaces)
{
    for (Interfaces::Iterator iter = ifaces.getAllElements(); !iter.end(); iter.nextElement()) {
        setSampleRate (iter.getValue());
    }
}

