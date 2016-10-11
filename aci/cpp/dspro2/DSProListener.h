/* 
 * DisServiceProListener.h
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on September 3, 2010, 10:37 PM
 */

#ifndef INCL_DSPRO_LISTENER_H
#define	INCL_DSPRO_LISTENER_H

#include "FTypes.h"

namespace IHMC_ACI
{
    class NodePath;

    class DSProListener
    {
        public:
            DSProListener (void);
            virtual ~DSProListener (void);

            /**
             * Notify the application that a peer registered a new path.
             */
            virtual bool pathRegistered (NodePath *pNodePath, const char *pszNodeId,
                                         const char *pszTeam, const char *pszMission) = 0;

            /**
             * Notify the listener that peer pszNodeId, moved to a new position.
             */
            virtual bool positionUpdated (float fLatitude, float fLongitude, float fAltitude,
                                          const char *pszNodeId) = 0;

            /**
             * Notify the listener that a new peer is in communication range.
             */
            virtual void newPeer (const char *pszPeerNodeId) = 0;

            /**
             * Notify the listener that peer pszDeadPeer is no longer in communication range.
             */
            virtual void deadPeer (const char *pszDeadPeer) = 0;

            /**
             * Notify the listener of the arrival of a data message.
             * - pszID: the id of the incoming data message.
             * - pszGroupName: the group of the incoming message.
             * - pszObejctId
             * - pszInstanceId
             * - pszMimeType
             * - pBuf: the payload of the message
             * - ui32BufLen
             * - uiNChunks: the number of chunks of the message that have been received so far
             * - ui8TotNChuks: the total number of chunks the data of the message was split into.
             * - pszQueryId: a dspro-opaque, application-defined parameter passed by mean of getData(),
             *               or requestMoreChunks().
             */
            virtual int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                                     const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                                     const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                                     const char *pszCallbackParameter) = 0;

            /**
             * Notify the listener of the arrival of a data message.
             * - pszID: the id of the incoming data message.
             * - pszGroupName: the group of the incoming message.
             * - pszReferredDataObjectId: the object ID of the referred data message
             * - pszReferredDataInstanceId: the object ID of the referred data message
             * - pszXMLMetadata: null-terminated string that contains and XML
             *                   document describing the data
             * - pszReferredDataId: the DSPro id of the referred data message.
             * - pszQueryId: if the data was delivered as a result of a search activity, then the id
             *               of the matching query is returned.
             */
            virtual int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                         const char *pszReferredDataInstanceId, const char *pszXMLMetadada,
                                         const char *pszReferredDataId, const char *pszQueryId) = 0;
    };
}

#endif	/* INCL_DSPRO_LISTENER_H */

