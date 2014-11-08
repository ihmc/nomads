/*
 * XLayerWrapper.cpp
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#include "XLayerWrapper.h"
#if defined (USE_XLAYER)
    #include "XLayerProxy.h"
    #include "XLayerMsgPropagationService.h"

    using namespace IHMC;
    using namespace IHMC_ACI;

    static XLayerProxy *XLayerWrapper_pXLayerProxy;

    XLayerWrapper::XLayerWrapper (void)
    {
        XLayerWrapper_pXLayerProxy = XLayerProxy::getInstance();
    }

    XLayerWrapper::XLayerWrapper (const char *pszAddress, uint16 ui16Port)
    {
        XLayerWrapper_pXLayerProxy = XLayerProxy::getInstance (false);
        XLayerWrapper_pXLayerProxy->connect (pszAddress, ui16Port, true);
    }

    XLayerWrapper::~XLayerWrapper (void)
    {
        delete XLayerWrapper_pXLayerProxy;
        XLayerWrapper_pXLayerProxy = NULL;
    }

    MessagePropagationService * XLayerWrapper::getMPS (void)
    {
        XLayerMsgPropagationService *pXLMPS = XLayerWrapper_pXLayerProxy->getMsgPropagationService();
        if (pXLMPS == NULL) {
            printf ("pXLMPS is null\n");
        }
        else {
            printf ("pXLMPS is not null\n");
        }
        MessagePropagationService *pMPS = dynamic_cast<MessagePropagationService*> (pXLMPS);
        if (pMPS == NULL) {
            printf ("pMPS is NULL\n");
        }
        else {
            printf ("pMPS is not null\n");
        }
        return pMPS;
    }
#endif
