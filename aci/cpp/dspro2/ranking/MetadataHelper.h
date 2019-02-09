/*
 * MetadataHelper.h
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

#ifndef INCL_METADATA_HELPER_H
#define INCL_METADATA_HELPER_H

#include "FTypes.h"

namespace IHMC_VOI
{
    class MetadataInterface;
}

namespace NOMADSUtil
{
    class Writer;
    class BufferWriter;
    class AVList;
}

namespace  IHMC_ACI
{
    class MetaData;
    class MetadataConfigurationImpl;
    class PreviousMessageIds;

    int getFieldValueAsPrevMessageIds (IHMC_VOI::MetadataInterface *pMetadata, PreviousMessageIds &previousMessageIds);

    /**
     * Returns true if the REFERS_TO property is set and the object
     * pointed by this was published by pszSource.
     *
     * Return false if the REFERS_TO property is null or pszSource is
     * null, or pszSource is not the publisher.
     */
    bool refersToDataFromSource (IHMC_VOI::MetadataInterface *pMetadata, const char *pszSource);

    int setFieldsValues (IHMC_VOI::MetadataInterface *pMetadata, NOMADSUtil::AVList *pFieldsValues);

    /**
     * Set the given fields values just if they where unknown before.
     * Always set the pedigree.
     */
    int setFixedMetadataFields (IHMC_VOI::MetadataInterface *pMetadata, int64 i64ExpirationTime, int64 i64ReceiverTimeStamp,
                                const char *pszSource, int64 i64SourceTimeStamp, const char *pszNodeId);

    MetaData * toMetadata (NOMADSUtil::AVList *pAVList);
    MetaData * toMetadata (const void *pBuf, uint32 ui32Len);

    int toBuffer (IHMC_VOI::MetadataInterface *pMetadata, NOMADSUtil::Writer *pWriter, bool bOmitTransient = true);
}

#endif  /* INCL_METADATA_HELPER_H */

