/*
 * ManycastNetworkMessageReceiver.h
 *
 *  This file is part of the IHMC Network Message Service Library
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
 * ManycastNetworkMessageReceiver collects the bytes received from the socket by
 * the subnetwork the message came from. This is useful when the socket binds
 * the wildcard address. Joining the wildcard address is commun practice on unix
 * in order to receive broadcast messages.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on April 14, 2011, 1:34 PM
 */

#ifndef INCL_MANYCAST_NETWORK_MESSAGE_RECEIVER_H
#define INCL_MANYCAST_NETWORK_MESSAGE_RECEIVER_H

#include "NetworkMessageReceiver.h"
#include "UInt32Hashtable.h"
#include "NetUtils.h"
#include "NICInfo.h"

namespace NOMADSUtil
{
    class NetworkMessageService;
    class MulticastUDPDatagramSocket;
    class DatagramSocket;

    class ManycastNetworkMessageReceiver : public NetworkMessageReceiver
    {
        public:
            /**
             * NOTE: ManycastNetworkMessageReceiver needs to store ppNICInfos
             * but it does not make a copy of it.  The caller must not deallocate,
             * nor modify ppNICInfos.
             */
            ManycastNetworkMessageReceiver (NetworkInterfaceManagerListener *pNMSParent, NetworkInterface *pNetIface,
                                            bool bReceive, NICInfo **ppNICInfos);
            virtual ~ManycastNetworkMessageReceiver (void);

            virtual uint32 getReceiveRate (uint32 ui32IPAddr, uint32 ui32NetMask);
            virtual uint32 getRateByBcastAddr (const char *pszBcastAddr);
            virtual uint32 getRateByBcastAddr (uint32 ui32BcastAddr);

        protected:
            class ManycastRateEstimator : public RateEstimator
            {
                public:
                    ManycastRateEstimator (NICInfo **ppNICInfos);
                    virtual ~ManycastRateEstimator (void);

                    virtual void reset (void);
                    virtual void setInterval (uint32 ui32IntervalInMS);
                    virtual void update (uint32 ui32RemoteIPAddress, uint32 ui32BytesReceived);

                    virtual uint32 getRateByBcastAddr (uint32 ui32BcastAddr);

                private:
                    NICInfo **_ppNICInfos;
                    UInt32Hashtable<RateEstimator> _rateEstimatorsByNetwork;    // broadcast address is the key
            };
    };

    inline uint32 ManycastNetworkMessageReceiver::getReceiveRate (uint32 ui32IPAddr, uint32 ui32NetMask)
    {
        return getRateByBcastAddr ((ui32IPAddr | (~ui32NetMask)));
    }

    inline uint32 ManycastNetworkMessageReceiver::getRateByBcastAddr (uint32 ui32BcastAddr)
    {
        return ((ManycastNetworkMessageReceiver::ManycastRateEstimator *)(this->_pRateEstimator))->getRateByBcastAddr (ui32BcastAddr);
    }

    inline void ManycastNetworkMessageReceiver::ManycastRateEstimator::reset (void)
    {
        NetworkMessageReceiver::RateEstimator::reset();
        UInt32Hashtable<RateEstimator>::Iterator it = _rateEstimatorsByNetwork.getAllElements();
        for (; !it.end(); it.nextElement()) {
            it.getValue()->reset();
        }
    }

    inline void ManycastNetworkMessageReceiver::ManycastRateEstimator::setInterval (uint32 ui32IntervalInMS)
    {
        NetworkMessageReceiver::RateEstimator::setInterval (ui32IntervalInMS);
        UInt32Hashtable<RateEstimator>::Iterator it = _rateEstimatorsByNetwork.getAllElements();
        for (; !it.end(); it.nextElement()) {
            it.getValue()->setInterval (ui32IntervalInMS);
        }
    }

    inline void ManycastNetworkMessageReceiver::ManycastRateEstimator::update (uint32 ui32RemoteIPAddress, uint32 ui32BytesReceived)
    {
        NetworkMessageReceiver::RateEstimator::update (ui32RemoteIPAddress, ui32BytesReceived);
        for (int i = 0; _ppNICInfos[i] != NULL; i++) {
            if (NetUtils::areInSameNetwork (_ppNICInfos[i]->ip.s_addr, _ppNICInfos[i]->netmask.s_addr,
                                            ui32RemoteIPAddress, _ppNICInfos[i]->netmask.s_addr)) {
                RateEstimator *pRateEst = _rateEstimatorsByNetwork.get (_ppNICInfos[i]->broadcast.s_addr);
                if (pRateEst != NULL) {
                    pRateEst->update (ui32RemoteIPAddress, ui32BytesReceived);
                }
            }
        }
    }

    inline uint32 ManycastNetworkMessageReceiver::ManycastRateEstimator::getRateByBcastAddr (uint32 ui32BcastAddr)
    {
        RateEstimator *pRateEst = _rateEstimatorsByNetwork.get (ui32BcastAddr);
        if (pRateEst != NULL) {
            return pRateEst->getRate();
        }
        return 0;
    }
}

#endif    // INCL_MANYCAST_NETWORK_MESSAGE_RECEIVER_H

