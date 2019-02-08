/*
 * RandomClassifier.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "RandomClassifier.h"

#include "C45AVList.h"
#include "Prediction.h"

#include "c4.5.h"

#include "NLFLib.h"
#include "StrClass.h"

#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_C45;

RandomClassifier::RandomClassifier()
{
    _pConfigure = NULL;
    _pItemTest = NULL;
    _MaxItemTest = -1;
    _pszErrorMessage = NULL;
    _errorCode = -1;
    srand ((unsigned int)getTimeInMilliseconds());
}

RandomClassifier::~RandomClassifier()
{
    if(_pConfigure != NULL) {
        for(int i = 0; i <= _pConfigure->MaxClass; i ++) {
            free(_pConfigure->ClassName[i]);
        }
        for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pConfigure->MaxAttVal[i]; j ++) {
                free(_pConfigure->AttValName[i][j]);
            }
            free(_pConfigure->AttName[i]);
            free(_pConfigure->AttValName[i]);
        }
        free(_pConfigure->AttName);
        free(_pConfigure->AttValName);
        free(_pConfigure->ClassName);
        free(_pConfigure->MaxAttVal);
        free(_pConfigure->SpecialStatus);
        free(_pConfigure);
    }
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) {
            free(_pItemTest[i]);
        }
        free(_pItemTest);
    }
    if(_pErrOcc != NULL) {
        free(_pErrOcc->errorMessage);
        free(_pErrOcc);
    }
    if(_pszErrorMessage != NULL) {
        free((void *) _pszErrorMessage);
    }
}

int RandomClassifier::configureClassifier (C45AVList * attributes)
{
    if(attributes == NULL) {
        _pszErrorMessage = "error: the passed C45AVList pointer is NULL. Unable to configure the tree. \0";
        _errorCode = 1;
        return 1;
    }
    int countClass = 0;
    for (unsigned int i = 0; i < attributes->getLength(); i ++) {
        if (strcmp (attributes->getAttribute(i), attributes->_CLASS) == 0) {
            countClass ++;
        }
    }
    if (countClass != 1) {
        _pszErrorMessage = "error: there must be 1 row begins with the constant CLASS that specifies the "
                           "names of the classes. Unable to configure the tree. \0";
        _errorCode = 2;
        return 2;
    }
    if (_pConfigure != NULL) {
        for (int i = 0; i <= _pConfigure->MaxClass; i ++) {
            free(_pConfigure->ClassName[i]);
        }
        for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
            for (int j = 0; j <= _pConfigure->MaxAttVal[i]; j ++) {
                free(_pConfigure->AttValName[i][j]);
            }
            free(_pConfigure->AttName[i]);
            free(_pConfigure->AttValName[i]);
        }
        free(_pConfigure->AttName);
        free(_pConfigure->AttValName);
        free(_pConfigure->ClassName);
        free(_pConfigure->MaxAttVal);
        free(_pConfigure->SpecialStatus);
    }
    if(_pConfigure == NULL) {
        _pConfigure = (Configure *) malloc(sizeof(Configure));
    }
    _pConfigure->MaxDiscrVal = 2;
    _pConfigure->MaxAtt = -1;
    _pConfigure->MaxAttVal = (short *) calloc((attributes->getLength()-1), sizeof(short));
    _pConfigure->AttName = (char * *) calloc((attributes->getLength()-1), sizeof(char *));
    _pConfigure->SpecialStatus = (char *) malloc((attributes->getLength()-1)*sizeof(char));
    _pConfigure->AttValName = (char * * *) calloc((attributes->getLength()-1), sizeof(char * *));
    for(int i = 0; i < attributes->getLength(); i ++) {
        if((attributes->getAttribute(i) == NULL)||(attributes->getValueByIndex(i) == NULL)) {
            _pszErrorMessage = "error: one of the attributes or the values is NULL. Unable to configure the tree. \0";
            return 1;
        }
        _pErrOcc = getNames(_pConfigure, attributes->getAttribute(i), attributes->getValueByIndex(i));
        if(_pErrOcc != NULL) {
            _pszErrorMessage = _pErrOcc->errorMessage;
            _errorCode = _pErrOcc->errorCode;
            return _pErrOcc->errorCode;
        }
    }
    return 0;
}

Prediction * RandomClassifier::consultClassifier(C45AVList * record)
{
    int pred = rand() % (_pConfigure->MaxClass + 1);
    Prediction * pPred = new Prediction(_pConfigure->ClassName[pred], (float) (1 / (_pConfigure->MaxClass + 1)));
    return pPred;
}

RandomTestInfo * RandomClassifier::testClassifierOnData(C45AVList * dataset)
{
    if(_pConfigure == NULL) {
        _pszErrorMessage = "error: the tree must be configured before you could add some data. Unable to "
        "process the new data. \0";
        _errorCode = 3;
        return NULL;
    }
    int pred, errCount = 0;
    int * confMat = (int *) calloc((_pConfigure->MaxClass + 1)*(_pConfigure->MaxClass + 1), sizeof(int));
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) {
            free(_pItemTest[i]);
        }
        free(_pItemTest);
        _pItemTest = NULL;
    }
    _MaxItemTest = -1;
    _pItemTest = (Description *) calloc((dataset->getLength() / (_pConfigure->MaxAtt + 2)), sizeof(Description));
    for(int i = 0; i < dataset->getLength(); i += (_pConfigure->MaxAtt + 2)) {
            _MaxItemTest ++;
            _pItemTest[_MaxItemTest] = (Description) calloc(_pConfigure->MaxAtt + 2, sizeof(AttValue));
            for(int l = 0; l < _pConfigure->MaxAtt + 2; l ++) {
                _pErrOcc = getDataset(_pConfigure, dataset->getAttribute(i+l), dataset->getValueByIndex(i+l), _MaxItemTest, _pItemTest);
                if(_pErrOcc != NULL) {
                    _pszErrorMessage = _pErrOcc->errorMessage;
                    _errorCode = _pErrOcc->errorCode;
                    return NULL;
                }
            }
            pred = rand() % (_pConfigure->MaxClass + 1);
            if(pred != _pItemTest[_MaxItemTest][_pConfigure->MaxAtt + 1]._discr_val) {
                errCount ++;
            }
            confMat[_pItemTest[_MaxItemTest][_pConfigure->MaxAtt + 1]._discr_val*(_pConfigure->MaxClass + 1)+pred]++;
    }
    RandomTestInfo * pTest = new RandomTestInfo(errCount, _MaxItemTest + 1, (float) (errCount / (_MaxItemTest + 1)), confMat,
                                                (_pConfigure->MaxClass + 1)*(_pConfigure->MaxClass + 1));
    free(confMat);
    _errorCode = 0;
    return pTest;
}

int RandomClassifier::addNewData(C45AVList * dataset)
{
    // Not necessary
    return -1;
}

void RandomClassifier::deleteTestData(void)
{
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) {
            free(_pItemTest[i]);
        }
        free(_pItemTest);
        _pItemTest = NULL;
    }
    _MaxItemTest = -1;
}

int64 RandomClassifier::read(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize)
{
    return 0;
}

int64 RandomClassifier::skip (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    return 0;
}

int64 RandomClassifier::write(NOMADSUtil::Writer * pWriter, uint32 ui32MaxSize)
{
    return 0;
}

int64 RandomClassifier::getWriteLength(void)
{
    return 0;
}

uint16 RandomClassifier::getVersion(void)
{
    return 0;
}
