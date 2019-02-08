/*
 * MessageProperties.h
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
 * Created on January 23, 2013, 2:35 PM
 */

#ifndef INCL_MESSAGE_PROPERTIES_H
#define INCL_MESSAGE_PROPERTIES_H

#include "StrClass.h"

namespace IHMC_ACI
{
    struct MessageProperties
    {
        MessageProperties (const char *pszPublisherNodeId,
                           const char *pszId,
                           const char *pszObjectId,
                           const char *pszInstanceId,
                           const char *pszAnnotatedObjMsgId,
                           const void *pAnnotationMetadata,
                           uint32 ui32AnnotationLen,
                           const char *pszMimeType,
                           const char *pszChecksum,
                           const char *pszQueryId,
                           int64 i64ExpirationTime);
        ~MessageProperties (void);

        NOMADSUtil::String toString (void) const;

        const uint32 _ui32AnnotationLen;
        const char * const _pszPublisherNodeId;
        const char * const _pszMsgId;
        const char * const _pszObjectId;
        const char * const _pszInstanceId;
        const char * const _pszAnnotatedObjMsgId;
        const void * const _pAnnotationMetadata;
        const char * const _pszMimeType;
        const char * const _pszChecksum;
        const char * const _pszQueryId;
        const int64 _i64ExpirationTime;
    };

    inline MessageProperties::MessageProperties (const char *pszPublisherNodeId,
                                                 const char *pszId,
                                                 const char *pszObjectId,
                                                 const char *pszInstanceId,
                                                 const char *pszAnnotatedObjMsgId,
                                                 const void *pAnnotationMetadata,
                                                 uint32 ui32AnnotationLen,
                                                 const char *pszMimeType,
                                                 const char *pszChecksum,
                                                 const char *pszQueryId,
                                                 int64 i64ExpirationTime)
        : _ui32AnnotationLen (ui32AnnotationLen),
          _pszPublisherNodeId (pszPublisherNodeId),
          _pszMsgId (pszId),
          _pszObjectId (pszObjectId),
          _pszInstanceId (pszInstanceId),
          _pszAnnotatedObjMsgId (pszAnnotatedObjMsgId),
          _pAnnotationMetadata (pAnnotationMetadata),
          _pszMimeType (pszMimeType),
          _pszChecksum (pszChecksum),
          _pszQueryId (pszQueryId),
          _i64ExpirationTime (i64ExpirationTime)
    {
    }

    inline MessageProperties::~MessageProperties (void)
    {
    }

    inline NOMADSUtil::String MessageProperties::toString (void) const
    {
        NOMADSUtil::String s (_pszMsgId);
        s += " (objectId <";
        s += (_pszObjectId == nullptr ? "null" : _pszObjectId);
        s += "> instanceId <";
        s += (_pszInstanceId == nullptr ? "null" : _pszInstanceId);
        s += ">)";
        return s;
    }
}

#endif    /* INCL_MESSAGE_PROPERTIES_H */
