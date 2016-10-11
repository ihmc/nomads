/*
 * NonClassifyingLocalNodeContext.h
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

#ifndef NON_CLASSIFYING_LOCAL_NODE_CONTEXT_H
#define NON_CLASSIFYING_LOCAL_NODE_CONTEXT_H

#include "LocalNodeContext.h"

namespace IHMC_ACI
{
    class NonClassifyingLocalNodeContext : public LocalNodeContext
    {
        public:
            static const char * TYPE;

            NonClassifyingLocalNodeContext (const char *pszNodeID, double dTooFarCoeff, double dApproxCoeff);
            virtual ~NonClassifyingLocalNodeContext (void);
    };
}

#endif // NON_CLASSIFYING_LOCAL_NODE_CONTEXT_H

