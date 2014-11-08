/*
 * NetworkMessageReceiver.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#ifndef INCL_NETWORK_MESSAGE_RECEIVER_H
#define	INCL_NETWORK_MESSAGE_RECEIVER_H

#include "ManageableThread.h"

namespace NOMADSUtil
{
    class NetworkInterface;
    class NetworkMessageService;
    class MulticastUDPDatagramSocket;

    /**
     * Simple thread that listens the specified socket and calls NetworkMessageService
     * back upon a well-formed NetworkMessageService's NetworkMessageService
     * arrival.
     */
    class NetworkMessageReceiver : public NOMADSUtil::ManageableThread
    {
        public:
            enum Type {
                NET_RCV,
                MCAST_NET_RCV
            };

            NetworkMessageReceiver (NetworkMessageService *pNMSParent, NetworkInterface *pNetIface,
                                    bool bReceive = true);
            virtual ~NetworkMessageReceiver (void);

            // Enables the estimation of the receive rate by this receiver
            void enableReceiveRateEstimation (void);

            // Disable the estimation of the receive rate by this receiver
            void disableReceiveRateEstimation (void);

            // See NetworkInterface for description
            virtual void setReceiveRateSampleInterval (uint32 ui32IntervalInMS);

            // See NetworkInterface for description
            virtual uint32 getReceiveRate (void);

            virtual Type getType (void);

             // It listens for messages and pass them to the NetworkMessageService
             // by pointer to NetworkMessageV1 object passing.
             // NOTE:The NetworkMessageV1 object is NOT deleted in this method.
            void run (void);

            void setDisconnected (void);

        protected:
            class RateEstimator;

            NetworkMessageReceiver (NetworkMessageService *pNMSParent, NetworkInterface *pNetIface,
                                    bool bReceive, RateEstimator *pRateEstimator, Type type);

            class RateEstimator
            {
                public:
                    RateEstimator (void);
                    virtual ~RateEstimator (void);
                    virtual void reset (void);
                    virtual void setInterval (uint32 ui32IntervalInMS);
                    virtual void update (uint32 ui32RemoteIPAddress, uint32 ui32BytesReceived);

                    // measured in Bps
                    virtual uint32 getRate (void);

                private:
                    uint32 _ui32RateInterval;     // The enforcement interval in milliseconds
                    int64 _i64IntervalStartTime;  // Time in milliseconds when interval started
                    uint32 _ui32BytesReceived;    // The number of bytes received since the start of the interval
                    uint32 _ui32ComputedRate;     // The rate that was computed for the last interval
            };

            RateEstimator *_pRateEstimator;

        private:
            int receive (char *achBuf, int iBufLen);

        private:
            const bool _bReceive;
            NetworkMessageService *_pNMSParent;
            NetworkInterface *_pNetIface;
            bool _bEstimateRate;
            Type _type;
            static const uint16 MAX_MESSAGE_SIZE = 65000;
    };

    inline void NetworkMessageReceiver::enableReceiveRateEstimation (void)
    {
        _pRateEstimator->reset();
        _bEstimateRate = true;
    }

    inline void NetworkMessageReceiver::disableReceiveRateEstimation (void)
    {
        _bEstimateRate = false;
        _pRateEstimator->reset();
    }

    inline void NetworkMessageReceiver::setReceiveRateSampleInterval (uint32 ui32IntervalInMS)
    {
        _pRateEstimator->setInterval (ui32IntervalInMS);
    }

    inline uint32 NetworkMessageReceiver::getReceiveRate (void)
    {
        return _pRateEstimator->getRate();
    }

    inline NetworkMessageReceiver::Type NetworkMessageReceiver::getType (void)
    {
        return _type;
    }
}

#endif  // INCL_NETWORK_MESSAGE_RECEIVER_H
