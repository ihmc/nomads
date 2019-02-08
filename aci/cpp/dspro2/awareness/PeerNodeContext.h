/*
 * PeerNodeContext.h
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#ifndef INCL_PEER_NODE_CONTEXT_H
#define INCL_PEER_NODE_CONTEXT_H

#include "NodeContextImpl.h"

#include "CommAdaptor.h"
#include "FTypes.h"
#include "PtrLList.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
}

namespace IHMC_C45
{
    class Classifier;
    class C45AVList;
}

namespace IHMC_VOI
{
    class NodePath;
}

namespace IHMC_ACI
{
    class PeerNodeContext : public NodeContextImpl
    {
        public:
            PeerNodeContext (const char *pszNodeID,
                             IHMC_C45::C45AVList *pClassifierConfiguration,
                             double dTooFarCoeff, double dApproxCoeff);
            virtual ~PeerNodeContext (void);

            int addAdaptor (AdaptorId adaptorId);
            int removeAdaptor (AdaptorId adaptorId);
            bool isReacheableThrough (AdaptorId adaptorId);

            // set methods
            void setPeerPresence (bool active);

            // get methods
            uint16 getClassifierVersion (void);

            int getCurrentLatitude (float &latitude);
            int getCurrentLongitude (float &longitude);
            int getCurrentTimestamp (uint64 &timestamp);
            int getCurrentPosition (float &latitude, float &longitude, float &altitude);
            int getCurrentPosition (float &latitude, float &longitude, float &altitude,
                                    const char *&pszLocation, const char *&pszNote,
                                    uint64 &timeStamp);

            /**
             * Returns the current path. If it does not exist it returns nullptr.
             */
            IHMC_VOI::NodePath * getPath (void);

            bool isPeerActive (void);

            bool operator == (PeerNodeContext &rhsPeerNodeContext);

            int fromJson (const NOMADSUtil::JsonObject *pJson);

        private:
            struct CurrentActualPosition {
                CurrentActualPosition (void);
                virtual ~CurrentActualPosition (void);

                float latitude;
                float longitude;
                float altitude;
                const char *pszLocation;
                const char *pszNote;
                uint64 timeStamp;
            };

            bool _bIsPeerActive;
            uint16 _ui16ClassifierVersion;
            NOMADSUtil::DArray<bool> _reacheableThrough;
    };

    typedef NOMADSUtil::PtrLList<PeerNodeContext> PeerNodeContextList;
}

#endif  // INCL_PEER_NODE_CONTEXT_H
