/*
 * C45AVList.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45AVList.h"

#include "StrClass.h"

using namespace NOMADSUtil;
using namespace IHMC_C45;

const NOMADSUtil::String C45AVList::_CLASS = "CLASS";
const NOMADSUtil::String C45AVList::_UNKNOWN = "UNKNOWN";
const NOMADSUtil::String C45AVList::_CONTINUOUS = "CONTINUOUS";
const NOMADSUtil::String C45AVList::_IGNORE = "IGNORE";
const NOMADSUtil::String C45AVList::_DISCRETE = "DISCRETE";
const NOMADSUtil::String C45AVList::_DEFAULT_CLASS = "DEFAULTCLASS";

C45AVList::C45AVList (unsigned int uiInitialSize)
: AVList (uiInitialSize)
{
}

C45AVList * C45AVList::concatListsInNewOne (C45AVList *pFirst, C45AVList *pSecond)
{
    C45AVList *pNewList = pFirst->copyList();
    for (unsigned int i = 0; i < pSecond->getLength(); i++) {
        pFirst->addPair (pSecond->getAttribute (i), pSecond->getValueByIndex (i));
    }
    return pNewList;
}

C45AVList * C45AVList::copyList()
{
    C45AVList *pNewList = new C45AVList (AVList::getLength());
    for (int i = 0; i < AVList::getLength(); i++) {
        pNewList->addPair (AVList::getAttribute (i), AVList::getValueByIndex (i));
    }
    return pNewList;
}

C45AVList::~C45AVList()
{
}

