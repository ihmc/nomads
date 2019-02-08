/*
 * MetaData.h
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
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details..
 */

#ifndef INCL_METADATA_H
#define INCL_METADATA_H

#include "MetadataImpl.h"

namespace NOMADSUtil
{
    class AVList;
    class Reader;
    class Writer;
}

namespace IHMC_VOI
{
    class Pedigree;
}

namespace IHMC_ACI
{
    class PreviousMessageIds;
    class MetaData;
    struct MetadataFieldInfo;

    class MetaData : public IHMC_VOI::MetadataInterface
    {
        public:
            MetaData (void);
            virtual ~MetaData (void);

            MetaData * clone (void);

            NOMADSUtil::BoundingBox getLocation (float fRange = 0.000001f) const;

            /**
             * Set one or more fields values
             * Returns 0 in case of success, a negative number otherwise
             *
             * NOTE: it makes a copy of pszValue
             */
            int setFieldValue (const char *pszAttribute, int64 i64Value);
            int setFieldValue (const char *pszAttribute, const char *pszValue);

            int resetFieldValue (const char *pszFieldName);

            int getReferredDataMsgId (NOMADSUtil::String &refersTo) const;

            int fromString (const char *pszJson);
            int fromJson (const NOMADSUtil::JsonObject *pJson);
            NOMADSUtil::JsonObject * toJson (void) const;

            /**
             * Reads metadata fields with the given Reader.
             * Returns a number > 0 if there are no errors; the returned value
             * is the number of bytes that were read.
             * Returns a negative number otherwise.
             *
             * NOTE: read note in the write() comment.
             */
            int read (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);

            /**
             * Writes metadata fields with the given Writer.
             * Returns a number > 0 if there are no errors. The
             * number specify the number of bytes written.
             *
             * Returns a negative number otherwise.
             *
             * NOTE: only the values are serialized. It is assumed that the order
             *       of the attribute is the same on every machine
             */
            int write (NOMADSUtil::Writer *pWriter, uint32 ui32MaxSize, NOMADSUtil::StringHashset *pFilters);

        protected:
            int findFieldValue (const char *pszFieldName, NOMADSUtil::String &value) const;

        private:
            IHMC_VOI::MetadataImpl _impl;
    };
}

#endif   // INCL_METADATA_H
