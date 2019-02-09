/*
 * AMTDictator.h
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

#ifndef INCL_AMT_DICTATOR_H
#define INCL_AMT_DICTATOR_H

#include "StrClass.h"

namespace NOMADSUtil
{
    class JsonObject;
}

namespace AMT_DICTATOR
{
    class AsyncDictator;
}

namespace IHMC_ACI
{
    class DSProImpl;

    class AMTDictator
    {
        public:
            AMTDictator (DSProImpl *pDSPro);
            ~AMTDictator (void);

            void messageArrived (const void *pBuf, uint32 ui32Len, const char *pszMIMEType);
            void messageArrived (NOMADSUtil::JsonObject *pJson);

            /**
             * Return JsonObject if the message is in the proper format, and if it's targeted
             * for the local node, null otherwise.
             */
            static NOMADSUtil::JsonObject * parseAndValidate (const void *pBuf, uint32 ui32Len,
                                                              const char *pszMIMEType,
                                                              const char *pszNodeId);

        private:
            AMT_DICTATOR::AsyncDictator *_pDictator;
            const NOMADSUtil::String _nodeId;
    };
}

#endif  /* INCL_AMT_DICTATOR_H */
