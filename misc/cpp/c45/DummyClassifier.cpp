/*
 * DummyClassifier.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "DummyClassifier.h"

#include <stddef.h>

using namespace NOMADSUtil;
using namespace IHMC_C45;

DummyClassifier::DummyClassifier()
{
    _pConfigure = NULL;
    _pItemTest = NULL;
    _MaxItemTest = -1;
    _pszErrorMessage = NULL;
    _errorCode = 0;
}

DummyClassifier::~DummyClassifier()
{
}

int DummyClassifier::configureClassifier (C45AVList * attributes)
{
    // Not necessary
    return 0;
}

Prediction * DummyClassifier::consultClassifier(C45AVList * record)
{
    return NULL;
}

TestInfo * DummyClassifier::testClassifierOnData(C45AVList * dataset)
{
    return NULL;
}

int DummyClassifier::addNewData(C45AVList * dataset)
{
    return 0;
}

void DummyClassifier::deleteTestData(void)
{
}

int64 DummyClassifier::read(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize)
{
    return 0;
}

int64 DummyClassifier::skip (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    return 0;
}

int64 DummyClassifier::write(NOMADSUtil::Writer * pWriter, uint32 ui32MaxSize)
{
    return 0;
}

int64 DummyClassifier::getWriteLength(void)
{
    return 0;
}

uint16 DummyClassifier::getVersion(void)
{
    return 0;
}
