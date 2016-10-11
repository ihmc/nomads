/*
 * SearchProperties.cpp
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

#include "SearchProperties.h"

#include "Defs.h"
#include "Writer.h"

#include "Logger.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_ACI;
using namespace NOMADSUtil;

void SearchProperties::deallocate (SearchProperties &searchProp)
{
    if (searchProp.pszQueryId != NULL) {
        free ((char *)searchProp.pszQueryId);
    }
    
    if (searchProp.pszGroupName != NULL) {
        free ((char *)searchProp.pszGroupName);
    }
    
    if (searchProp.pszQueryType != NULL) {
        free ((char *)searchProp.pszQueryType);
    }
    
    if (searchProp.pszQueryQualifiers != NULL) {
        free ((char *)searchProp.pszQueryQualifiers);
    }
    
    if (searchProp.pQuery != NULL) {
        free ((char *)searchProp.pQuery);
    }
}

int SearchProperties::read (SearchProperties &searchProp, Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }

    uint16 ui16;
    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            searchProp.pszQueryId = (char *) calloc (ui16+1, sizeof (char));
            if (searchProp.pszQueryId == NULL) {
                return -3;
            }
            if (pReader->readBytes ((char *)searchProp.pszQueryId, ui16) < 0) {
                return -4;
            }
            ((char *)(searchProp.pszQueryId))[ui16] = '\0';
        }
    }
    else {
        return -5;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            searchProp.pszQuerier = (char *) calloc (ui16+1, sizeof (char));
            if (searchProp.pszQuerier == NULL) {
                return -6;
            }
            if (pReader->readBytes ((char *)searchProp.pszQuerier, ui16) < 0) {
                return -7;
            }
            ((char *)(searchProp.pszQuerier))[ui16] = '\0';
        }
    }
    else {
        return -8;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            searchProp.pszGroupName = (char *) calloc (ui16+1, sizeof (char));
            if (searchProp.pszGroupName == NULL) {
                return -9;
            }
            if (pReader->readBytes ((char *)searchProp.pszGroupName, ui16)) {
                return -10;
            }
            ((char *)(searchProp.pszGroupName))[ui16] = '\0';
        }
    }
    else {
        return -11;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            searchProp.pszQueryType = (char *) calloc (ui16+1, sizeof (char));
            if (searchProp.pszQueryType == NULL) {
                return -12;
            }
            if (pReader->readBytes ((char *)searchProp.pszQueryType, ui16)) {
                return -13;
            }
            ((char *)(searchProp.pszQueryType))[ui16] = '\0';
        }
    }
    else {
        return -14;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            searchProp.pszQueryQualifiers = (char *) calloc (ui16+1, sizeof (char));
            if (searchProp.pszQueryQualifiers == NULL) {
                return -15;
            }
            if (pReader->readBytes ((char *)searchProp.pszQueryQualifiers, ui16)) {
                return -16;
            }
            ((char *)(searchProp.pszQueryQualifiers))[ui16] = '\0';
        }
    }
    else {
        return -17;
    }

    if (pReader->read32 (&searchProp.uiQueryLen) == 0) {
        if (searchProp.uiQueryLen > 0) {
            searchProp.pQuery = malloc (searchProp.uiQueryLen);
            if (searchProp.pQuery == NULL) {
                return -18;
            }
            if (pReader->readBytes ((char *)searchProp.pQuery, searchProp.uiQueryLen)) {
                return -19;
            }
        }
    }
    else {
        return -20;
    }

    return 0;
}

int SearchProperties::write (SearchProperties &searchProp, Writer *pWriter)
{
    if (pWriter == NULL) {
        return -1;
    }

    uint16 ui16 = searchProp.pszQueryId == NULL ? 0 : strlen (searchProp.pszQueryId);
    if (pWriter->write16 (&ui16) < 0) {
        return -3;
    }
    if (ui16 > 0 && pWriter->writeBytes (searchProp.pszQueryId, ui16) < 0) {
        return -4;
    }

    ui16 = searchProp.pszQuerier == NULL ? 0 : strlen (searchProp.pszQuerier);
    if (pWriter->write16 (&ui16) < 0) {
        return -5;
    }
    if (ui16 > 0 && pWriter->writeBytes (searchProp.pszQuerier, ui16) < 0) {
        return -6;
    }

    ui16 = searchProp.pszGroupName == NULL ? 0 : strlen (searchProp.pszGroupName);
    if (pWriter->write16 (&ui16) < 0) {
        return -7;
    }
    if (ui16 > 0 && pWriter->writeBytes (searchProp.pszGroupName, ui16) < 0) {
        return -8;
    }

    ui16 = searchProp.pszQueryType == NULL ? 0 : strlen (searchProp.pszQueryType);
    if (pWriter->write16 (&ui16) < 0) {
        return -9;
    }
    if (ui16 > 0 && pWriter->writeBytes (searchProp.pszQueryType, ui16) < 0) {
        return -10;
    }

    ui16 = searchProp.pszQueryQualifiers == NULL ? 0 : strlen (searchProp.pszQueryQualifiers);
    if (pWriter->write16 (&ui16) < 0) {
        return -11;
    }
    if (ui16 > 0 && pWriter->writeBytes (searchProp.pszQueryQualifiers, ui16) < 0) {
        return -12;
    }

    if (pWriter->write32 (&searchProp.uiQueryLen) < 0) {
        return -13;
    }
    if (searchProp.uiQueryLen > 0 && pWriter->writeBytes (searchProp.pQuery, searchProp.uiQueryLen) < 0) {
        return -14;
    }

    return 0;
}

int readCommomSearchProperties (char *&pszQueryId, char *&pszQuerier, char *&pszQueryType,
                                char *&pszMatchingNode, Reader *pReader)
{
    if (pReader == NULL) {
        return -1;
    }

    uint16 ui16;

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            pszQueryId = (char *) calloc (ui16 + 1, sizeof (char));
            if (pszQueryId == NULL) {
                checkAndLogMsg ("SearchProperties::read", memoryExhausted);
                return -2;
            }
            if (pReader->readBytes (pszQueryId, ui16) < 0) {
                return -3;
            }
            pszQueryId[ui16] = '\0';
        }
    }
    else {
        return -4;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            pszQuerier = (char *) calloc (ui16 + 1, sizeof (char));
            if (pszQuerier == NULL) {
                checkAndLogMsg ("SearchProperties::read", memoryExhausted);
                return -5;
            }
            if (pReader->readBytes (pszQuerier, ui16) < 0) {
                return -6;
            }
            pszQuerier[ui16] = '\0';
        }
    }
    else {
        return -7;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            pszQueryType = (char *) calloc (ui16 + 1, sizeof (char));
            if (pszQueryType == NULL) {
                checkAndLogMsg ("SearchProperties::read", memoryExhausted);
                return -8;
            }
            if (pReader->readBytes (pszQueryType, ui16)) {
                return -9;
            }
            pszQueryType[ui16] = '\0';
        }
    }
    else {
        return -10;
    }

    if (pReader->read16 (&ui16) == 0) {
        if (ui16 > 0) {
            pszMatchingNode = (char *) calloc (ui16 + 1, sizeof (char));
            if (pszMatchingNode == NULL) {
                checkAndLogMsg ("SearchProperties::read", memoryExhausted);
                return -12;
            }
            if (pReader->readBytes (pszMatchingNode, ui16)) {
                return -13;
            }
            pszMatchingNode[ui16] = '\0';
        }
    }
    else {
        return -14;
    }

    return 0;
}

int SearchProperties::read (char *&pszQueryId, char *&pszQuerier, char *&pszQueryType, char **&ppszMatchingMsgIds,
                            char *&pszMatchingNode, Reader *pReader)
{
    if (readCommomSearchProperties (pszQueryId, pszQuerier, pszQueryType, pszMatchingNode, pReader) < 0) {
        return -1;
    }

    unsigned int uiCount = 0;
    pReader->read16 (&uiCount);
    if (uiCount == 0) {
        ppszMatchingMsgIds = NULL;
    }
    else {
        ppszMatchingMsgIds = (char **) calloc (uiCount + 1, sizeof (char*));
        if (ppszMatchingMsgIds == NULL) {
            checkAndLogMsg ("SearchProperties::read", memoryExhausted);
            return -11;
        }
        for (unsigned int i = 0; i < uiCount; i++) {
            uint16 ui16 = 0;
            pReader->read16 (&ui16);
            if (ui16 > 0) {
                ppszMatchingMsgIds[i] = (char *) calloc (ui16 + 1, sizeof (char));
                if (ppszMatchingMsgIds[i] != NULL) {
                    pReader->readBytes (ppszMatchingMsgIds[i], ui16);
                    ppszMatchingMsgIds[i][ui16] = '\0';
                }
            }
        }
        ppszMatchingMsgIds[uiCount] = NULL;
    }

    return 0;
}

int writeCommomSearchProperties (const char *pszQueryId, const char *pszQuerier, char *pszQueryType,
                                 const char *pszMatchingNode, Writer *pWriter)
{
    if (pWriter == NULL || pszQueryId == NULL) {
        return -1;
    }
    uint16 ui16 = pszQueryId == NULL ? 0 : strlen (pszQueryId);
    if (pWriter->write16 (&ui16) < 0) {
        return -2;
    }
    if (ui16 > 0 && pWriter->writeBytes (pszQueryId, ui16) < 0) {
        return -3;
    }

    ui16 = pszQuerier == NULL ? 0 : strlen (pszQuerier);
    if (pWriter->write16 (&ui16) < 0) {
        return -4;
    }
    if (ui16 > 0 && pWriter->writeBytes (pszQuerier, ui16) < 0) {
        return -5;
    }

    ui16 = pszQueryType == NULL ? 0 : strlen (pszQueryType);
    if (pWriter->write16 (&ui16) < 0) {
        return -6;
    }
    if (ui16 > 0 && pWriter->writeBytes (pszQueryType, ui16) < 0) {
        return -7;
    }

    ui16 = pszMatchingNode == NULL ? 0 : strlen (pszMatchingNode);
    if (pWriter->write16 (&ui16) < 0) {
        return -8;
    }
    if (ui16 > 0 && pWriter->writeBytes (pszMatchingNode, ui16) < 0) {
        return -9;
    }
    return 0;
}

int SearchProperties::write (const char *pszQueryId, const char *pszQuerier, char *pszQueryType,
                             const char **ppszMatchingMsgIds, const char *pszMatchingNode, Writer *pWriter)
{
    if (writeCommomSearchProperties (pszQueryId, pszQuerier, pszQueryType, pszMatchingNode, pWriter) < 0) {
        return -1;
    }

    if (ppszMatchingMsgIds != NULL) {
        unsigned int uiCount = 0;
        for (; ppszMatchingMsgIds[uiCount] != NULL; uiCount++);
        pWriter->write16 (&uiCount);

        for (unsigned int i = 0; i < uiCount; i++) {
            uint16 ui16 = strlen (ppszMatchingMsgIds[i]);
            pWriter->write16 (&ui16);
            if (ui16 > 0) {
                pWriter->writeBytes (ppszMatchingMsgIds[i], ui16);
            }
        }
    }
    else {
        uint16 ui16 = 0;
        pWriter->write16 (&ui16);
    }

    return 0;
}

int SearchProperties::read (char *&pszQueryId, char *&pszQuerier, char *&pszQueryType, void *&pReply, uint16 &ui16ReplyLen,
                            char *&pszMatchingNode, Reader *pReader)
{
    if (readCommomSearchProperties (pszQueryId, pszQuerier, pszQueryType, pszMatchingNode, pReader) < 0) {
        return -1;
    }
    if (pReader->read16 (&ui16ReplyLen) < 0) {
        return -2;
    }
    if (ui16ReplyLen == 0) {
        return 0;
    }
    pReply = malloc (ui16ReplyLen);
    if (pReply == NULL) {
        return -3;
    }
    if (pReader->readBytes (pReply, ui16ReplyLen) < 0) {
        return -4;
    }
    return 0;
}

int SearchProperties::write (const char *pszQueryId, const char *pszQuerier, char *pszQueryType,
    const void *pReply, uint16 ui16ReplyLen, const char *pszMatchingNode,
    NOMADSUtil::Writer *pWriter)
{
    if (writeCommomSearchProperties (pszQueryId, pszQuerier, pszQueryType, pszMatchingNode, pWriter) < 0) {
        return -1;
    }
    if (pWriter->write16 (&ui16ReplyLen) < 0) {
        return -2;
    }
    if (pWriter->writeBytes (pReply, ui16ReplyLen) < 0) {
        return -3;
    }
    return 0;
}

