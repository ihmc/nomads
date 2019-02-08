/*
 * StorageController.h
 *
 * This class should be extended by external components,
 * registered through the proper API, that need to access
 * the _pDataStore and the _pInfoStore.
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
 * Created on February 4, 2013, 11:25 PM
 */

#ifndef INCL_STORAGE_CONTROLLER_H
#define INCL_STORAGE_CONTROLLER_H

#include "StrClass.h"

namespace IHMC_ACI
{
    class DataStore;
    class DSProImpl;
    class InformationStore;

    class StorageController
    {
        public:
            StorageController (DSProImpl *pDSPro, DataStore *pDataStore, InformationStore *pInfoStore);
            virtual ~StorageController (void);

        protected:
            DSProImpl * getDSPro (void) const;
            DataStore * getDataStore (void);
            InformationStore * getInformationStore (void);
            NOMADSUtil::String getMetadataTableName (void);

        private:
            DSProImpl * const _pDSPro;
            DataStore *_pDataStore;
            InformationStore *_pInfoStore;
    };
}

#endif    /* INCL_STORAGE_CONTROLLER_H */
