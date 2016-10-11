/* 
 * PreviousMessageIds.h
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
 * Created on August 21, 2014, 1:23 AM
 */

#ifndef INCL_PREVIOUS_MESSAGE_IDS_H
#define	INCL_PREVIOUS_MESSAGE_IDS_H

#include "StringStringHashtable.h"

namespace IHMC_ACI
{
    class PreviousMessageIds
    {
        public:
            PreviousMessageIds (void);
            virtual ~PreviousMessageIds (void);

            void add (const char *pszNodeId, const char *pszPreviosMessageId);
            NOMADSUtil::String getPreviousMessageIdForPeer (const char *pszNodeId);
            bool isEmpty (void);

            operator const char * (void);
            PreviousMessageIds & operator = (const char *pszStr);

        private:            
            mutable NOMADSUtil::String _stringRepresentation;
            NOMADSUtil::StringStringHashtable _nodeIdToPrevMsgId;
    };

    inline PreviousMessageIds::PreviousMessageIds (void)
        : _nodeIdToPrevMsgId (true,  // bCaseSensitiveKeys
                              true,  // bCloneKeys ,
                              true,  // bDeleteKeys,
                              false, // bCloneValues,
                              true)  // bDeleteValues
    {
    }

    inline PreviousMessageIds::~PreviousMessageIds (void)
    {
    }

    inline void PreviousMessageIds::add (const char *pszNodeId, const char *pszPreviosMessageId)
    {
        if ((pszNodeId != NULL) && (pszPreviosMessageId != NULL)) {
            _nodeIdToPrevMsgId.put (pszNodeId, NOMADSUtil::strDup (pszPreviosMessageId));
        }
    }

    inline NOMADSUtil::String PreviousMessageIds::getPreviousMessageIdForPeer (const char *pszNodeId)
    {
        if (pszNodeId == NULL) {
            return NOMADSUtil::String();
        }
        return NOMADSUtil::String (_nodeIdToPrevMsgId.get (pszNodeId));
    }

    inline bool PreviousMessageIds::isEmpty (void)
    {
        return (_nodeIdToPrevMsgId.getCount() == 0);
    }

    inline PreviousMessageIds::operator const char * (void)
    {
        _stringRepresentation = _nodeIdToPrevMsgId.toString();
        return _stringRepresentation.c_str();
    }

    inline PreviousMessageIds & PreviousMessageIds::operator = (const char *pszStr)
    {
        _nodeIdToPrevMsgId.removeAll();
        NOMADSUtil::StringStringHashtable *ptmp = NOMADSUtil::StringStringHashtable::parseStringStringHashtable (pszStr);
        if (ptmp != NULL) {
            NOMADSUtil::StringStringHashtable::Iterator iter = ptmp->getAllElements();
            for (; !iter.end(); iter.nextElement()) {
                add (iter.getKey(), iter.getValue());
            }
        }
        delete ptmp;
        return *this;
    }
}

#endif	/* INCL_PREVIOUS_MESSAGE_IDS_H */

