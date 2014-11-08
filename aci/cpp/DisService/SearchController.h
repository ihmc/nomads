/* 
 * SearchController.h
 *
 *This file is part of the IHMC DisService Library/Component
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
 *
 * Author: Giacomo Benincasa	(gbenincasa@ihmc.us)
 * Created on April 4, 2014, 6:41 PM
 */

#ifndef INCL_SEARCH_CONTROLLER_H
#define	INCL_SEARCH_CONTROLLER_H

#include "Listener.h"
#include "Services.h"

namespace IHMC_ACI
{
    class DisseminationService;

    class SearchController : public SearchListener, public SearchService
    {
        public:
            SearchController (DisseminationService *pDisService);            
            virtual ~SearchController (void);
    };
}

#endif	/* INCL_SEARCH_CONTROLLER_H */

