/* 
 * SQLAVList.cpp
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

#include "SQLAVList.h"

#include "MetadataConfiguration.h"
#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_ACI;

namespace NOMADSUtil
{
    bool notNullAndEqual (const char *pszFieldName, const char *pszAttribute)
    {
        return ((pszAttribute != NULL) && (0 == strcmp (pszFieldName, pszAttribute)));
    }
}

SQLAVList::SQLAVList (unsigned int uiInitialSize)
    : AVList (uiInitialSize)
{
}

SQLAVList::~SQLAVList (void)
{
}

SQLAVList * SQLAVList::concatListsInNewOne (SQLAVList *pFirst, SQLAVList *pSecond)
{
    SQLAVList *pNewList = pFirst->copyList();
    for (unsigned int i = 0; i < pSecond->getLength(); i++) {
        pFirst->addPair (pSecond->getAttribute (i), pSecond->getValueByIndex (i));
    }
    return pNewList;
}

SQLAVList * SQLAVList::copyList (void)
{
    SQLAVList *pNewList = new SQLAVList (AVList::getLength());
    for (unsigned int i = 0; i < AVList::getLength(); i++) {
        pNewList->addPair (AVList::getAttribute (i), AVList::getValueByIndex (i));
    }
    return pNewList;
}

const char * SQLAVList::getFieldName (unsigned int uiIndex) const
{
    if (uiIndex >= getLength()) {
        return NULL;
    }
    return AVList::getAttribute (uiIndex);
}


const char * SQLAVList::getFieldType (unsigned int uiIndex) const
{
    if (uiIndex >= getLength()) {
        return NULL;
    }
    MetadataConfiguration *pMetadataConf = MetadataConfiguration::getConfiguration();
    if (pMetadataConf == NULL) {
        return NULL;
    }
    return pMetadataConf->getFieldType (getFieldName (uiIndex));
}

const char * SQLAVList::getFieldValueByIndex (unsigned int uiIndex) const
{
    if (uiIndex >= getLength()) {
        return NULL;
    }
    return getValueByIndex (uiIndex);
}

bool SQLAVList::isFieldAtIndexUnknown (unsigned int uiIndex) const
{
    if (uiIndex >= getLength()) {
        return false;
    }
    return MetadataInterface::isFieldValueUnknown (getAttribute (uiIndex));
}

int SQLAVList::setFieldValue (const char *pszFieldName, const char *pszValue)
{
    if (pszFieldName == NULL || pszValue == NULL) {
        return -1;
    }

    for (unsigned int i = 0; i < getLength(); i++) {
        if (notNullAndEqual (pszFieldName, getAttribute (i))) {
            deletePair (i);
            break;
        }
    }
    return addPair (pszFieldName, pszValue);
}

int SQLAVList::resetFieldValue (const char *pszFieldName)
{
    if (pszFieldName == NULL) {
        return -1;
    }
    for (unsigned int i = 0; i < getLength(); i++) {
        if (notNullAndEqual (pszFieldName, getAttribute (i))) {
            return deletePair (i);
        }
    }
    return -2;
}

int SQLAVList::findFieldValue (const char *pszFieldName, const char **ppszValue) const
{
    if (pszFieldName == NULL || ppszValue == NULL) {
        return -1;
    }
    for (unsigned int i = 0; i < getLength(); i++) {
        if (notNullAndEqual (pszFieldName, getAttribute (i))) {
            (*ppszValue) = static_cast<const char *>(getValueByIndex (i));
            if ((*ppszValue) != NULL) {
                return 0;
            }
            break;
        }
    }
    return -2;
}

