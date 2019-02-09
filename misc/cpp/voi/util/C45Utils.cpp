/*
 * C45Utils.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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
 * Created on February 7, 2017, 12:43 AM
 */

#include "C45Utils.h"

#include "C45AVList.h"
#include  "Classifier.h"
#include "MetadataConfiguration.h"
#include "MetadataInterface.h"
#include "NodeContext.h"
#include "Logger.h"
#include "VoiDefs.h"

#include "StringHashset.h"
#include "MetadataRankerConfiguration.h"

using namespace IHMC_VOI;
using namespace IHMC_C45;
using namespace NOMADSUtil;

#define nullMetadataConf Logger::L_Warning, "MetadataConfiguration is not properly initialzied. Can't use the classifier\n"

C45AVList * C45Utils::getMetadataAsDataset (MetadataInterface *pMetadata, MetadataConfiguration *pMetadataCfg)
{
    const char *pszMethodName = "C45Utils::getMetadataAsDataset";
    C45AVList *pValues = new C45AVList (pMetadataCfg->getNumberOfLearningFields() + 1);
    char *pszValue = (char *) "\0";
    int rc;
    for (uint16 i = 0; i < pMetadataCfg->getNumberOfFields(); i++) {
        if (pMetadataCfg->isLearningField (i)) {
            const String fieldName (pMetadataCfg->getFieldName (i));
            rc = pMetadata->getFieldValue (fieldName, &pszValue);
            if (rc < 0) {
                delete pValues;
                return NULL;
            }
            if (fieldName == MetadataInterface::USAGE) {
                if (rc == 1) {
                    delete pValues;
                    return NULL;
                }
                if (rc == 0) {
                    if (0 == strcmp (pszValue, "0")) {
                        pValues->addPair (C45AVList::_CLASS, PredictionClass::NOT_USEFUL);
                    }
                    else if (0 == strcmp (pszValue, "1")) {
                        pValues->addPair (C45AVList::_CLASS, PredictionClass::USEFUL);
                    }
                    free (pszValue);
                }
            }
            else if (rc == 0) {
                    pValues->addPair (fieldName, pszValue);
                    free (pszValue);
            }
            else if (rc == 1) {
                pValues->addPair (fieldName, C45AVList::_UNKNOWN);
            }
        }
    }

    if ((pLogger != NULL) && (pLogger->getDebugLevel() >= Logger::L_HighDetailDebug)) {
        pLogger->logMsg (pszMethodName, Logger::L_HighDetailDebug, "C45AVList at the end is:\n");
        for (unsigned int i = 0; i < pValues->getLength (); i++) {
            pLogger->logMsg (pszMethodName, Logger::L_HighDetailDebug,
                             "attribute[%d] = <%s>, value[%d] = <%s>\n",
                             i, pValues->getAttribute (i), i, pValues->getValueByIndex (i));
        }
    }

    return pValues;
}

C45AVList * C45Utils::getMetadataAsC45List (MetadataInterface *pMetadata, MetadataConfiguration *pMetadataCfg)
{
    C45AVList *pValues = new C45AVList (pMetadataCfg->getNumberOfLearningFields () + 1);
    char *pszValue = NULL;
    int rc;
    for (uint16 i = 0; i < pMetadataCfg->getNumberOfFields(); i++) {
        const String fieldName (pMetadataCfg->getFieldName (i));
        if (fieldName == MetadataInterface::USAGE) {
            continue;
        }
        if (pMetadataCfg->isLearningField (i)) {
            rc = pMetadata->getFieldValue (fieldName, &pszValue);
            if (rc < 0) {
                delete pValues;
                return NULL;
            }
            if (rc == 0) {
                pValues->addPair (fieldName, pszValue);
            }
            else if (rc == 1) {
                pValues->addPair (fieldName, C45AVList::_UNKNOWN);
            }
            if (pszValue != NULL) {
                free (pszValue);
                pszValue = NULL;
            }
        }
    }

    return pValues;
}

IHMC_C45::Prediction * C45Utils::getPrediction (MetadataInterface *pMetadata, NodeContext *pNodeCtxt, MetadataConfiguration *pMetadataCfg)
{
    const char *pszMethodName = "C45Utils::getPrediction";
    if (pMetadataCfg == NULL) {
        checkAndLogMsg (pszMethodName, nullMetadataConf);
        return NULL;
    }
    MetadataRankerConfiguration *pRankerConf = pNodeCtxt->getMetaDataRankerConfiguration();
    if ((pRankerConf != NULL) || (pRankerConf->_fPredRankWeight <= 0.0f)) {
        return NULL;
    }
    C45AVList *pRecord = C45Utils::getMetadataAsC45List (pMetadata, pMetadataCfg);
    if (pRecord != NULL) {
        Prediction *pPrediction = NULL;
        Classifier *pClassifier = pNodeCtxt->getClassifier ();
        if (pClassifier != NULL) {
            pPrediction = pClassifier->consultClassifier (pRecord);
        }
        delete pRecord;
        return pPrediction;
    }
    checkAndLogMsg (pszMethodName, Logger::L_Warning, "Can not retrieve metadata as C45 list from information store\n");
    return NULL;
}

