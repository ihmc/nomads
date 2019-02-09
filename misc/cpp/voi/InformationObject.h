/*
 * InformationObject.h
 *
 * This file is part of the IHMC Voi Library/Component
 * Copyright (c) 2008-2017 IHMC.
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
 * Created on February 14, 2017
 */

#ifndef INCL_INFORMATION_OBJECT_H
#define INCL_INFORMATION_OBJECT_H

#include "FTypes.h"
#include "PtrLList.h"

namespace IHMC_VOI
{
    class MetadataInterface;

    class InformationObject
    {
        public:
            InformationObject (MetadataInterface *pMetadata, uint32 ui32DataLen);
            virtual ~InformationObject (void);

            MetadataInterface * getMetadata (void) const;
            virtual const void * getData (uint32 &ui32DataLen) const = 0;

        protected:
            uint32 _ui32DataLen;
            MetadataInterface *_pMetadata;
    };

    class MutableInformationObject : public InformationObject
    {
        public:
            MutableInformationObject (MetadataInterface *pMetadata, void *pData, uint32 ui32DataLen);
            ~MutableInformationObject (void);

            const void * getData (uint32 &ui32DataLen) const;

        private:
            void *_pData;
    };

    class ImmutableInformationObject : public InformationObject
    {
        public:
            ImmutableInformationObject (MetadataInterface *pMetadata, const void *pData = NULL, uint32 ui32DataLen = 0U);
            ~ImmutableInformationObject (void);

            const void * getData (uint32 &ui32DataLen) const;

        private:
            const void *_pData;
    };

    typedef NOMADSUtil::PtrLList<InformationObject> InformationObjects;
}

#endif  /* INCL_INFORMATION_OBJECT_H */

