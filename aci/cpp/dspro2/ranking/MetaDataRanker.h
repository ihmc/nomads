/**
 * MetaDataRanker.h
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

#ifndef INCL_METADATA_RANKER_H
#define INCL_METADATA_RANKER_H

#include "Rank.h"

namespace IHMC_C45
{
    class Prediction;
}

namespace IHMC_ACI
{
    class MetadataInterface;
    class MetadataConfiguration;
    struct MetadataRankerConfiguration;
    struct MetadataRankerLocalConfiguration;
    class MetaDataRankerTest;
    class NodeContext;
    class SQLAVList;

    class MetaDataRanker
    {
        public:
            static const float DEFAULT_COORD_WEIGHT;
            static const float DEFAULT_TIME_WEIGHT;
            static const float DEFAULT_EXP_WEIGHT;
            static const float DEFAULT_IMP_WEIGHT;
            static const float DEFAULT_SRC_REL_WEIGHT;
            static const float DEFAULT_INFO_CONTENT_WEIGHT;
            static const float DEFAULT_PRED_WEIGHT;
            static const float DEFAULT_TARGET_WEIGHT;

            static const bool DEFAULT_STRICT_TARGET;
            static const bool DEFAULT_CONSIDER_FUTURE_PATH_SEGMENTS;

            static const char * COORDINATES_RANK_PROPERTY;
            static const char * TIME_RANK_PROPERTY;
            static const char * EXPIRATION_RANK_PROPERTY;
            static const char * IMPORTANCE_RANK_PROPERTY;
            static const char * SOURCE_RELIABILITY_RANK_PROPERTY;
            static const char * INFORMATION_CONTENT_RANK_PROPERTY;
            static const char * PREDICTION_RANK_PROPERTY;
            static const char * TARGET_RANK_PROPERTY;
            static const char * TIME_RANK_CONSIDER_FUTURE_SEG_PROPERTY;
            static const char * TARGET_RANK_STRICT_PROPERTY;

            static const char * COORDINATES_RANK_DESCRIPTOR;
            static const char * TIME_RANK_DESCRIPTOR;
            static const char * EXPIRATION_RANK_DESCRIPTOR;
            static const char * IMPORTANCE_RANK_DESCRIPTOR;
            static const char * SOURCE_RELIABILITY_RANK_DESCRIPTOR;
            static const char * INFORMATION_CONTENT_RANK_DESCRIPTOR;
            static const char * PREDICTION_RANK_DESCRIPTOR;
            static const char * TARGET_RANK_DESCRIPTOR;

            /**
             * Determines the rank of a single metadata using the ranks computed
             * by the methods in "MetaDataRanker". This is a specific rank for
             * pushing a data. The pull could have a different way to combine
             * the ranks. If it was not possible to compute some of the ranks
             * (because the metadata had unknown values), those ranks are set to
             * 5 (average value).
             */
            static Rank * rank (MetadataInterface *pMetaData, NodeContext *pCurrPeer,
                                MetadataConfiguration *pMetadataConf,
                                MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);

            /**
             * Determines the ranks of each metadata currently present in the
             * cache by matching it against the given NodeContext.
             * The method returns the list of the message IDs of the matched
             * metadata.
             * The rank of the matchmaking is stored in "ranks" at the
             * corresponding index.
             *
             * NOTE: ppInstrumentations is being allocated in this function and
             *       MUST be deallocated by the caller
             *
             * - pFields: is the list of metadata that have to be matched against
             *            the node context
             * - noFileds: is the number of elements of pFields
             * - messageIDsNumber: is the number of elements of the returned array
             * - pNodeContext: the node context of the peer which interests have
             *   to be matched against the metadata of the cached data.
             */
            static Ranks * rank (MetadataList *pMetadataList, NodeContext *pNodeContext,
                                 MetadataConfiguration *pMetadataConf,
                                 MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);

        private:
            friend class MetaDataRankerTest;

            /**
             * Returns a Rank if the matchmaking for the metadata should be skipped,
             * NULL otherwise
             */
            static Rank * filter (MetadataInterface *pMetaData, NodeContext *pNodeContext,
                                  MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);       

            /*
             * Calculate the rank for a single metadata for the push mode.
             */
            static Rank * rankInternal (MetadataInterface *pMetaData, NodeContext *pNodeContext,
                                        IHMC_C45::Prediction *pPrediction,
                                        MetadataRankerLocalConfiguration *pMetadataRankerLocalConf);
    };
}

#endif // INCL_METADATA_RANKER_H

