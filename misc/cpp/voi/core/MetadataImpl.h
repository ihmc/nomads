/*
 * MetadataImpl.h
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on July 5, 2013, 11:52 AM
 */

#ifndef INCL_METADATA_IMPLEMENTATION_H
#define	INCL_METADATA_IMPLEMENTATION_H

#include "MetadataInterface.h"

#include "GeoUtils.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace IHMC_VOI
{
    class MetadataImpl : public MetadataInterface
    {
        public:
            MetadataImpl (void);
            ~MetadataImpl (void);

            int findFieldValue (const char *pszFieldName, NOMADSUtil::String &value) const;
            int resetFieldValue (const char *pszFieldName);
            int setFieldValue (const char *pszAttribute, int64 i64Value);
            int setFieldValue (const char *pszAttribute, const char *pszValue);

            // Minimum range of influence of the object
            NOMADSUtil::BoundingBox getLocation (float fRange = 0.000001f) const;

            int getReferredDataMsgId (NOMADSUtil::String &refersTo) const;

            /*
             * Serialization
             */
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, NOMADSUtil::StringHashset *pFilters = NULL);

        private:
            NOMADSUtil::JsonObject *_pJson;
    };
}

#endif  /* INCL_METADATA_IMPLEMENTATION_H */

