/*
 * Reset.h
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
 * Created on June 26, 2012, 10:42 PM
*/

#ifndef INCL_DSPRO_RESET_H
#define INCL_DSPRO_RESET_H

#include "DSProListener.h"

#include <mutex>

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_ACI
{
    class Reset : public DSProListener
    {
        public:
            Reset (void);
            ~Reset (void);

            bool pathRegistered (IHMC_VOI::NodePath *pNodePath, const char *pszNodeId,
                                 const char *pszTeam, const char *pszMission);

            bool positionUpdated (float fLatitude, float fLongitude, float fAltitude, const char *pszNodeId);

            void newPeer (const char *pszPeerNodeId);
            void deadPeer (const char *pszDeadPeer);

            int dataArrived (const char *pszId, const char *pszGroupName, const char *pszObjectId,
                             const char *pszInstanceId, const char *pszAnnotatedObjMsgId, const char *pszMimeType,
                             const void *pBuf, uint32 ui32Len, uint8 ui8NChunks, uint8 ui8TotNChunks,
                             const char *pszCallbackParameter);

            int metadataArrived (const char *pszId, const char *pszGroupName, const char *pszReferredDataObjectId,
                                 const char *pszReferredDataInstanceId, const char *pszXMLMetadada,
                                 const char *pszReferredDataId, const char *pszQueryId);

            int dataAvailable (const char *pszId, const char *pszGroupName,
                               const char *pszReferredDataObjectId, const char *pszReferredDataInstanceId,
                               const char *pszReferredDataId, const char *pszMimeType,
                               const void *pMetadata, uint32 ui32MetadataLength,
                               const char *pszQueryId);

        private:
            void handle (NOMADSUtil::JsonObject &json);

        private:
            int64 _i64Timestamp;
            std::mutex _m;
    };
}

#endif  /* INCL_DSPRO_RESET_H */
