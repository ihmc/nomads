/* 
 * TagGenerator.h
 *
 * Singleton class for the handling of mockets tags.
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
 * Created on April 15, 2013, 11:58 PM
 */

#ifndef INCL_UNIQUE_TAG_GENERATOR_H
#define	INCL_UNIQUE_TAG_GENERATOR_H

#include "Mutex.h"
#include "StringHashtable.h"

namespace IHMC_ACI
{
    class TagGenerator
    {
        public:
            virtual ~TagGenerator (void);

            static TagGenerator * getInstance (void);

            static uint16 getDefaultTag (void);
            static uint16 getVersionTag (void);

            /**
             * Returns the unique tag 
             */
            uint16 getWaypointTagForPeer (const char *pszPublisherNodeId);

        private:
            TagGenerator (void);
            uint16 getAdHocTag (const char *pszId, NOMADSUtil::StringHashtable<uint16> &tagsById);

        private:
            uint16 _ui16CurrTag;
            static const uint16 _ui16DefaultTag;
            static const uint16 _ui16VersionTag;
            NOMADSUtil::Mutex _m;
            NOMADSUtil::StringHashtable<uint16> _waypointTagByPeerId;

            static TagGenerator *_pTagGenerator;
    };

    inline TagGenerator * TagGenerator::getInstance (void)
    {
        if (_pTagGenerator == NULL) {
            _pTagGenerator = new TagGenerator();
        }
        return _pTagGenerator;
    }

    inline uint16 TagGenerator::getDefaultTag (void)
    {
        return _ui16DefaultTag; // _ui16DefaultTag is constant - no need to synchronize
    }

    inline uint16 TagGenerator::getVersionTag (void)
    {
        return _ui16VersionTag; // _ui16DefaultTag is constant - no need to synchronize
    }

    inline uint16 TagGenerator::getWaypointTagForPeer (const char *pszPublisherNodeId)
    {
        return getAdHocTag (pszPublisherNodeId, _waypointTagByPeerId);
    }

    inline uint16 TagGenerator::getAdHocTag (const char *pszId, NOMADSUtil::StringHashtable<uint16> &tagsById)
    {
        _m.lock();
        uint16 *pui16AdHocTag = tagsById.get (pszId);
        if (pui16AdHocTag == NULL) {
            pui16AdHocTag = new uint16;
            if (pui16AdHocTag == NULL) {
                _m.unlock();
                return _ui16CurrTag++;
            }
            *pui16AdHocTag = _ui16CurrTag;
            _ui16CurrTag++;
            tagsById.put (pszId, pui16AdHocTag);
        }
        _m.unlock();
        return *pui16AdHocTag;
    }
}

#endif	/* INCL_UNIQUE_TAG_GENERATOR_H */

