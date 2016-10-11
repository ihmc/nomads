/* 
 * MessageProcessor.h
 *
 * Given the metadata that were selected for the pre-staging, InformationPushPolicy
 * may select further messages to be pre-staged depending on different factors
 * (such as size of the message, state of the network, characteristics of the
 * targeted peer, user/application defined policies, etc.);
 * For instance, InformationPushPolicy may not do anything and let only metadata,
 * to be pushed, or it may select the data referred by the metadata being pushed
 * as well, or my select any combination of metadata and data.
 *
 *  * This file is part of the IHMC DSPro Library/Component
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
 * Created on August 3, 2011, 3:56 PM
 */

#ifndef INCL_MESSAGE_PROCESSOR_H
#define	INCL_MESSAGE_PROCESSOR_H

#include "Rank.h"

namespace IHMC_ACI
{
    class InformationStore;
    class MetaDataInterface;

    class InformationPushPolicy
    {
        public:
            static const char *PROACTIVE_REPLICATOR;
            static const char *REACTIVE_REPLICATOR;

            virtual ~InformationPushPolicy (void);

            /** 
             * Returns a new PtrLList of ranks with the elements to be pre-staged
             */
            Ranks * process (Ranks *pRanks);
            virtual Ranks * process (Rank *pRank) = 0;

            static InformationPushPolicy * getPolicy (const char *pszType, InformationStore *pInformationStore);

        protected:
            explicit InformationPushPolicy (InformationStore *pInformationStore);
            MetadataInterface * getMetadata (const char *pszMessageID);

        protected:
            InformationStore *_pInformationStore;
    };

    /**
     * For each metadata to be pre-staged the ProactivePush policy selects the
     * referred data as well.
     */
    class ProactivePush : public InformationPushPolicy
    {
        public:
            explicit ProactivePush (InformationStore *pInformationStore);
            virtual ~ProactivePush (void);

            Ranks * process (Rank *pRank);
    };

    /**
     * ReactivePush selects only metadata for the pre-stage.
     */
    class ReactivePush : public InformationPushPolicy
    {
        public:
            explicit ReactivePush (InformationStore *pInformationStore);
            virtual ~ReactivePush (void);

            Ranks * process (Rank *pRank);
    };
}

#endif	// INCL_MESSAGE_PROCESSOR_H

