/*
 * PeerStatusListener.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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
 * Author:  Giacomo Benincasa (gbenincasa@ihmc.us)
 */

#ifndef INCL_PEER_STATUS_LISTENER
#define INCL_PEER_STATUS_LISTENER

#include "Listener.h"

namespace IHMC_ACI 
{
    class PeerStatusListener : public PeerStateListener
    {
        public:
            PeerStatusListener (void);
            virtual ~PeerStatusListener (void);

            virtual bool newPeer (const char *pszPeerNodeId) = 0;
            virtual bool deadPeer (const char *pszPeerNodeId) = 0;

            virtual void newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                      const char *pszIncomingInterface);
            virtual void deadNeighbor (const char *pszNodeUID);

            virtual void newLinkToNeighbor (const char*, const char*, const char*);
            virtual void droppedLinkToNeighbor (const char*, const char*);

            virtual void stateUpdateForPeer (const char *pszNodeUID,
                                             PeerStateListener::PeerStateUpdateInterface *pUpdate);
    };

    inline PeerStatusListener::PeerStatusListener() {}
    inline PeerStatusListener::~PeerStatusListener() {}

    inline void PeerStatusListener::newNeighbor (const char *pszNodeUID, const char *pszPeerRemoteAddr,
                                                 const char *pszIncomingInterface)
    {
        newPeer (pszNodeUID);
    }

    inline void PeerStatusListener::deadNeighbor (const char *pszNodeUID)
    {
        deadPeer (pszNodeUID);
    }

    inline void PeerStatusListener::newLinkToNeighbor (const char*, const char*, const char*)
    {        
    }

    inline void PeerStatusListener::droppedLinkToNeighbor (const char*, const char*)
    {
    }

    inline void PeerStatusListener::stateUpdateForPeer (const char *pszNodeUID,
                                                        PeerStateListener::PeerStateUpdateInterface *pUpdate)
    {
    }
}


#endif // INCL_PEER_STATUS_LISTENER

