/*
 * AVList.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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
 * authors : Silvia Rota         srota@ihmc.us
 *           Giacomo Benincasa   gbenincasa@ihmc.us
 */

#include "AVList.h"

#include "NLFLib.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace NOMADSUtil;

AVList::AVList (unsigned int uiInitialSize)
    : _pPair (uiInitialSize > 0U ? static_cast<Pair *>(calloc (uiInitialSize, sizeof (Pair))) : NULL),
      _uiMaxSize (uiInitialSize),
      _uiCurrSize (0U)
{
}

AVList::~AVList (void)
{
    emptyList();
}

int AVList::addPair (const char *pszAttrName, const char *pszValue)
{
    if (_uiMaxSize == _uiCurrSize) {
        _uiMaxSize++;
        _uiCurrSize++;
        _pPair = (Pair *) realloc (_pPair, _uiMaxSize * sizeof (Pair));
        if (_pPair == NULL) {
            return 1;
        }
    }
    else {
        _uiCurrSize++;
    }
    if (pszAttrName == NULL) {
        _pPair[_uiCurrSize - 1]._pszAttribute = NULL;
    }
    else {
        _pPair[_uiCurrSize - 1]._pszAttribute = strDup (pszAttrName);
    }
    if (pszValue == NULL) {
        _pPair[_uiCurrSize - 1]._pszAttribute = NULL;
    }
    else {
        _pPair[_uiCurrSize - 1]._pszValue = strDup (pszValue);
    }
    return 0;
}

int AVList::addPair (const char *pszAttrName, float fValue)
{
    if (sprintf (_pszNumber, "%f", fValue) == 0) {
        return 1;
    }
    return addPair (pszAttrName, _pszNumber);
}

int AVList::addPair (const char *pszAttrName, double dValue)
{
    if (sprintf (_pszNumber, "%f", dValue) == 0) {
        return 1;
    }
    return addPair (pszAttrName, _pszNumber);
}

int AVList::addPair (const char *pszAttrName, int iValue)
{
    if (sprintf (_pszNumber, "%d", iValue) == 0) {
        return 1;
    }
    return addPair(pszAttrName, _pszNumber);
}

int AVList::addPair (const char *pszAttrName, int64 i64Value)
{
    if (sprintf (_pszNumber, "%lld", i64Value) == 0) {
        return 1;
    }
    return addPair(pszAttrName, _pszNumber);
}

int AVList::deletePair (unsigned int uiIndex)
{
    if (uiIndex < _uiCurrSize) {
        if (_pPair[uiIndex]._pszAttribute != NULL) {
            free(_pPair[uiIndex]._pszAttribute);
        }
        if (_pPair[uiIndex]._pszValue != NULL) {
            free(_pPair[uiIndex]._pszValue);
        }
        for (unsigned int i = uiIndex; i < _uiCurrSize - 1; i++) {
            _pPair[i]._pszAttribute = _pPair[i + 1]._pszAttribute;
            _pPair[i]._pszValue = _pPair[i + 1]._pszValue;
        }
        _pPair[_uiCurrSize-1]._pszAttribute = NULL;
        _pPair[_uiCurrSize-1]._pszValue = NULL;
        _uiCurrSize--;
        return 0;
    }
    return -1;
}

const char * AVList::getAttribute (unsigned int uiIndex) const
{
    if (uiIndex < _uiCurrSize) {
        return _pPair[uiIndex]._pszAttribute;
    }
    return NULL;
}

const char * AVList::getValueByIndex (unsigned int uiIndex) const
{
    if (uiIndex < _uiCurrSize) {
        return _pPair[uiIndex]._pszValue;
    }
    return NULL;
}

const char * AVList::getValue (const char *pszAttrName) const
{
    if (pszAttrName != NULL) {
        for (unsigned int i = 0; i < _uiCurrSize; i++) {
            if (_pPair[i]._pszAttribute != NULL &&
                (0 == strcmp(_pPair[i]._pszAttribute, pszAttrName))) {
                return _pPair[i]._pszValue;
            }
        }
    }
    return NULL;
}

void AVList::concatLists (AVList *pFirst, AVList *pSecond)
{
    for (unsigned int i = 0; i < pSecond->getLength(); i++) {
        pFirst->addPair (pSecond->getAttribute (i), pSecond->getValueByIndex (i));
    }
}

AVList * AVList::concatListsInNewOne (AVList *pFirst, AVList *pSecond)
{
    AVList *pNew = pFirst->copyList();
    for(unsigned int i = 0; i < pSecond->getLength(); i++) {
        pFirst->addPair (pSecond->getAttribute (i), pSecond->getValueByIndex (i));
    }
    return pNew;
}

AVList * AVList::copyList (void)
{
    AVList *pNew = new AVList (_uiCurrSize);
    for(unsigned int i = 0; i < _uiCurrSize; i++) {
        pNew->addPair (_pPair[i]._pszAttribute, _pPair[i]._pszValue);
    }
    return pNew;
}

void AVList::emptyList (void)
{
    if (_pPair != NULL) {
        for (unsigned int i = 0; i < _uiCurrSize; i++) {
            if (_pPair[i]._pszAttribute != NULL) {
                free (_pPair[i]._pszAttribute);
                _pPair[i]._pszAttribute = NULL;
            }
            if (_pPair[i]._pszValue != NULL) {
                free (_pPair[i]._pszValue);
                _pPair[i]._pszValue = NULL;
            }
        }
        free (_pPair);
        _pPair = NULL;
    }
    _uiMaxSize = 0;
    _uiCurrSize = 0;
}

