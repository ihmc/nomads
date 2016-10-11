/* 
 * NMSHelper.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2016 IHMC.
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
 * Created on June 24, 2015, 4:30 PM
 */

#include "NMSHelper.h"

#include "DisServiceDefs.h"

#include "NMSProperties.h"
#include "NetworkMessageServiceProxy.h"
#include "NetworkMessageServiceProxyServer.h"

#include "ConfigManager.h"
#include "Logger.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

namespace IHMC_ACI
{
    String toSemicolomnSeparatedString (const char **ppszString)
    {
        if (ppszString == NULL) {
            return String ();
        }
        String ret;
        for (unsigned int i = 0; ppszString[i] != NULL; i++) {
            if (ret.length() > 0) {
                ret += ';';
            }
            ret += ppszString[i];
        }
        return ret;
    }

    const char * obsoletePropertiesSanityCheck (NOMADSUtil::ConfigManager &cfgMgr, const char *pszObsoleteProperty, const char *pszNewProperty)
    {
        String obsoletePropValue (cfgMgr.getValue (pszObsoleteProperty));
        String newPropValue (cfgMgr.getValue (pszNewProperty));
        if (newPropValue.length() > 0) {
            // If the new property is set, keep the value of the new property
            if ((obsoletePropValue.length() > 0) && (newPropValue != obsoletePropValue)) {
                // Warning!
            }
            return cfgMgr.getValue (pszNewProperty);
        }
        if (obsoletePropValue.length() > 0) {
            // otherwise, if the old property is set, also store it as new property
            cfgMgr.setValue (pszObsoleteProperty, obsoletePropValue);
        }
        // otherwise keep DisService's default
        return NULL;
    }

    void disserviceToNMSProperties (NOMADSUtil::PROPAGATION_MODE mode, uint16 ui16NMSSvcPort, bool bAsyncDelivery,
                                    bool bAsyncTransmission, uint8 ui8MessageVersion, bool bReplyViaUnicast,
                                    const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces,
                                    const char **ppszAddedInterfaces, const char *pszDestAddr, uint8 ui8McastTTL,
                                    NOMADSUtil::ConfigManager &cfgMgr)
    {
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.propagationMode", NMSProperties::NMS_TRANSMISSION_MODE)) {
            cfgMgr.setValue (NMSProperties::NMS_TRANSMISSION_MODE, static_cast<int>(mode));
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.port", NMSProperties::NMS_PORT)) {
            cfgMgr.setValue (NMSProperties::NMS_PORT, ui16NMSSvcPort);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.delivery.async", NMSProperties::NMS_DELIVERY_ASYNC)) {
            cfgMgr.setValue (NMSProperties::NMS_DELIVERY_ASYNC, bAsyncDelivery);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.transmission.async", NMSProperties::NMS_TRANSMISSION_ASYNC)) {
            cfgMgr.setValue (NMSProperties::NMS_TRANSMISSION_ASYNC, bAsyncTransmission);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.networkMessageVersion", NMSProperties::NMS_MSG_VERSION)) {
            cfgMgr.setValue (NMSProperties::NMS_MSG_VERSION, ui8MessageVersion);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.replyViaUnicast", NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY)) {
            cfgMgr.setValue (NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY, bReplyViaUnicast);
        }
        const String reqIfaces (toSemicolomnSeparatedString (ppszBindingInterfaces));
        if (reqIfaces.length() > 0) {
            // Use DisService defaults
            cfgMgr.setValue (NMSProperties::NMS_REQUIRED_INTERFACES, reqIfaces);
        }
        const String ignIfaces (toSemicolomnSeparatedString (ppszIgnoredInterfaces));
        if (reqIfaces.length() > 0) {
            // Use DisService defaults
            cfgMgr.setValue (NMSProperties::NMS_IGNORED_INTERFACES, ignIfaces);
        }
        const String optIfaces (toSemicolomnSeparatedString (ppszAddedInterfaces));
        if (reqIfaces.length() > 0) {
            // Use DisService defaults
            cfgMgr.setValue (NMSProperties::NMS_OPTIONAL_INTERFACES, optIfaces);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.bcastAddr", NMSProperties::NMS_OUTGOING_ADDR)) {
            cfgMgr.setValue (NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY, pszDestAddr);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.mcastAddr", NMSProperties::NMS_OUTGOING_ADDR)) {
            cfgMgr.setValue (NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY, pszDestAddr);
        }
        if (NULL == obsoletePropertiesSanityCheck (cfgMgr, "aci.disService.networkMessageService.mcastTTL", NMSProperties::NMS_TTL)) {
            cfgMgr.setValue (NMSProperties::NMS_TTL, ui8McastTTL);
        }
    }
}

NMSHelper::NMSHelper (void)
    : _pNMS (NULL),
      _pProxClient (NULL),
      _pProxySrv (NULL)
{
}

NMSHelper::~NMSHelper (void)
{
}

NetworkMessageServiceInterface * NMSHelper::getNetworkMessageService (NOMADSUtil::PROPAGATION_MODE mode, uint16 ui16NMSSvcPort, bool bAsyncDelivery,
                                                                      bool bAsyncTransmission, uint8 ui8MessageVersion, bool bReplyViaUnicast,
                                                                      const char **ppszBindingInterfaces, const char **ppszIgnoredInterfaces,
                                                                      const char **ppszAddedInterfaces, const char *pszDestAddr, uint8 ui8McastTTL,
                                                                      ConfigManager *pCfgMgr)
{
    if (pCfgMgr == NULL) {
        return NULL;
    }
    const char *pszMethodName = "NMSHelper::getNetworkMessageService";
    disserviceToNMSProperties (mode, ui16NMSSvcPort, bAsyncDelivery, bAsyncTransmission, ui8MessageVersion, bReplyViaUnicast,
                               ppszBindingInterfaces, ppszIgnoredInterfaces, ppszAddedInterfaces, pszDestAddr, ui8McastTTL, *pCfgMgr);
    mode = static_cast<NOMADSUtil::PROPAGATION_MODE>(pCfgMgr->getValueAsInt (NMSProperties::NMS_TRANSMISSION_MODE));
    bAsyncDelivery = pCfgMgr->getValueAsBool(NMSProperties::NMS_DELIVERY_ASYNC);
    bAsyncTransmission = pCfgMgr->getValueAsBool (NMSProperties::NMS_TRANSMISSION_ASYNC);
    ui8MessageVersion = static_cast<uint8>(pCfgMgr->getValueAsInt (NMSProperties::NMS_MSG_VERSION));
    bReplyViaUnicast = pCfgMgr->getValueAsBool (NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY);

    const bool bLocal = pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.local", true);
    String sNMSProxyServerHost = pCfgMgr->getValue ("aci.disService.networkMessageService.proxy.host", "127.0.0.1");
    sNMSProxyServerHost.convertToLowerCase();
    bool bDefaultHostNMSSvc = true;
    const bool bIslocalHost = ((sNMSProxyServerHost.length() <= 0) || (sNMSProxyServerHost == "127.0.0.1") || (sNMSProxyServerHost == "localhost"));
    if (!bIslocalHost) {
        bDefaultHostNMSSvc = false;
    }
    const bool bHostNMSSvc = pCfgMgr->getValueAsBool ("aci.disService.networkMessageService.proxy.instantiateSvc", bDefaultHostNMSSvc);
    if (!bIslocalHost && bHostNMSSvc) {
        return NULL;
    }

    const uint16 ui16NMSProxyServerPort = static_cast<uint16>(pCfgMgr->getValueAsUInt32 (NMSProperties::NMS_PROXY_PORT, NMSProperties::DEFAULT_NMS_PROXY_PORT));
    if (bHostNMSSvc) {
        _pNMS = new NetworkMessageService (mode, bAsyncDelivery, bAsyncTransmission, ui8MessageVersion, bReplyViaUnicast);
        int rc = _pNMS->init (pCfgMgr);
        if (rc < 0) {
            delete _pNMS;
            _pNMS = NULL;
            checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not initialize network message service. Returned code: %d\n", rc);
            return NULL;
        }

        if (bHostNMSSvc) {
            _pProxySrv = new NetworkMessageServiceProxyServer (_pNMS, true);
            rc = _pProxySrv->init (ui16NMSProxyServerPort);
            if (rc < 0) {
                delete _pNMS;
                _pNMS = NULL;
                delete _pProxySrv;
                _pProxySrv = NULL;
                checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not initialize network message service proxy server. Returned code: %d\n", rc);
                return NULL;
            }

            _pProxySrv->start();
        }
        _pNMS->start();
    }

    if (bLocal) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "returning NetworkMessageService.\n");
        return _pNMS;
    }

    _pProxClient = new NetworkMessageServiceProxy (0, true);
    int rc = _pProxClient->init (sNMSProxyServerHost, ui16NMSProxyServerPort);
    if (rc < 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Warning, "could not initialize network message service proxy. Returned code: %d\n", rc);
        return NULL;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Info, "returning NetworkMessageServiceProxy.\n");
    return _pProxClient;
}

bool  NMSHelper::isConnected (void)
{
    if (_pProxClient != NULL) {
        return _pProxClient->isConnected();
    }
    return true;
}

int NMSHelper::start (void)
{
    if ((_pNMS != NULL) && (_pNMS->start() < 0)) {
        return -1;
    }
    if ((_pProxClient != NULL) && (_pProxClient->start() < 0)) {
        return -2;
    }
    return 0;
}

int NMSHelper::stop (void)
{
    // TODO: implement this
    return 0;
}

int NMSHelper::startAdaptors (void)
{
    // TODO: implement this
    return 0;
}

int NMSHelper::stopAdaptors (void)
{
    // TODO: implement this
    return 0;
}

void NMSHelper::requestTerminationAndWait (void)
{
    if (_pProxClient != NULL) {
       // _pProxClient->requestTerminationAndWait();
    }
    if (_pProxySrv != NULL) {
        _pProxySrv->requestTerminationAndWait();
    }
    if (_pNMS != NULL) {
        _pNMS->stop();
    }
}

