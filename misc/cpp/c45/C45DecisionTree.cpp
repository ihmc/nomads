/*
 * C45DecisionTree.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45DecisionTree.h"

#include "C45TreeInfo.h"

#include "c4.5.h"
#include "consult.h"

#include "Logger.h"
#include "Mutex.h"
#include "Reader.h"
#include "StrClass.h"
#include "Writer.h"

#include <string.h>
#include <stdlib.h>

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace IHMC_C45;
using namespace NOMADSUtil;

C45DecisionTree::C45DecisionTree()
{
    _m.lock();
    // initialize the options with default values
    _pOptions = (treeOptions *) malloc(sizeof(treeOptions));
    _pOptions->gainCriterion = true;
    _pOptions->increment = 0;
    _pOptions->initialWindow = 0;
    _pOptions->cycleErrors = 0.20f;
    _pOptions->cycleWindow = 1.0f;
    _pOptions->maxWindow = 0;
    _pOptions->minObjects = 2;
    _pOptions->probThresh = false;
    _pOptions->pruneConfidence = 0.25f;
    _pOptions->subsetting = false;
    _pOptions->verbosity = false;
    // initialize other variables
    _pResultedTree = NULL;
    _pConsultedTree = NULL;
    _pszErrorMessage = NULL;
    _errorCode = -1;
    _pTreeConfigure = NULL;
    _pErrOcc = NULL;
    _pItem = NULL;
    _pItemTest = NULL;
    _pItemTree = NULL;
    _dataFlag = false;
    _iterate = 0;
    _treeCounter = 0;
    _MaxItemIncrement = -1;
    _MaxItemTest = -1;
    _m.unlock();
}

C45DecisionTree::~C45DecisionTree()
{
    _m.lock();
    if(_pConsultedTree != NULL) {
        free(_pConsultedTree->className);
        if(_pConsultedTree->codeErrors != NULL) {
            free(_pConsultedTree->codeErrors->errorMessage);
            free(_pConsultedTree->codeErrors);
        }
        free(_pConsultedTree);
    }
    if(_pResultedTree != NULL) {
        if(_pResultedTree->codeErrors != NULL) {
            free(_pResultedTree->codeErrors->errorMessage);
            free(_pResultedTree->codeErrors);
        }
        if(_pResultedTree->nTrees > 0) {
            for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                if(_pResultedTree->trees[i]->testResults != NULL) {
                    free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                    free(_pResultedTree->trees[i]->testResults);
                }
                freeTree(_pResultedTree->trees[i]->tree);
                free(_pResultedTree->trees[i]);
            }
            free(_pResultedTree->trees);
        }
        free(_pResultedTree);
    }
    if(_pTreeConfigure != NULL) {
        for(int i = 0; i <= _pTreeConfigure->MaxClass; i ++) free(_pTreeConfigure->ClassName[i]);
        for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pTreeConfigure->MaxAttVal[i]; j ++) free(_pTreeConfigure->AttValName[i][j]);
            free(_pTreeConfigure->AttName[i]);
            free(_pTreeConfigure->AttValName[i]);
        }
        free(_pTreeConfigure->AttName);
        free(_pTreeConfigure->AttValName);
        free(_pTreeConfigure->ClassName);
        free(_pTreeConfigure->MaxAttVal);
        free(_pTreeConfigure->SpecialStatus);
        free(_pTreeConfigure);
    }
    if(_pItem != NULL) {
        if(_MaxItemIncrement > 0) {
            for(int i = 0; i <= _MaxItemIncrement; i ++) free(_pItem[i]);
        }
        else {
            for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
        }
        free(_pItem);
    }
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
        free(_pItemTest);
    }
    if(_pItemTree != NULL) free(_pItemTree);
    if(_pErrOcc != NULL) {
        free(_pErrOcc->errorMessage);
        free(_pErrOcc);
    }
    free(_pOptions);
    _m.unlock();
}

int C45DecisionTree::configureClassifier(C45AVList * attributes)
{
    _m.lock();
    if(attributes == NULL) {
        _pszErrorMessage = "error: the passed C45AVList pointer is NULL. Unable to configure the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::configureClassifier", Logger::L_SevereError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    int countClass = 0;
    for (unsigned int i = 0; i < attributes->getLength(); i ++) {
        if(!strcmp(attributes->getAttribute(i), attributes->_CLASS)) countClass ++;
    }
    if(countClass != 1) {
        _pszErrorMessage = "error: there must be 1 row begins with the constant CLASS that specifies the names"
                           " of the classes. Unable to configure the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::configureClassifier", Logger::L_SevereError,
            "%s\n", _pszErrorMessage);
        _errorCode = 2;
        _m.unlock();
        return 2;
    }
    if(_pTreeConfigure != NULL) {
        if(_pConsultedTree != NULL) {
            free(_pConsultedTree->className);
            if(_pConsultedTree->codeErrors != NULL) {
                free(_pConsultedTree->codeErrors->errorMessage);
                free(_pConsultedTree->codeErrors);
            }
            free(_pConsultedTree);
            _pConsultedTree = NULL;
        }
        if(_pResultedTree != NULL) {
            if(_pResultedTree->codeErrors != NULL) {
                free(_pResultedTree->codeErrors->errorMessage);
                free(_pResultedTree->codeErrors);
            }
            if(_pResultedTree->nTrees > 0) {
                for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                    if(_pResultedTree->trees[i]->testResults != NULL) {
                        free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                        free(_pResultedTree->trees[i]->testResults);
                    }
                    freeTree(_pResultedTree->trees[i]->tree);
                    free(_pResultedTree->trees[i]);
                }
                free(_pResultedTree->trees);
            }
            free(_pResultedTree);
            _pResultedTree = NULL;
        }
        if(_pItem != NULL) {
            if(_MaxItemIncrement > 0) {
                for(int i = 0; i <= _MaxItemIncrement; i ++) free(_pItem[i]);
            }
            else {
                for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
            }
            free(_pItem);
            _pItem = NULL;
        }
        if(_pItemTest != NULL) {
            for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
            free(_pItemTest);
            _pItemTest = NULL;
        }
        if(_pItemTree != NULL) {
            free(_pItemTree);
            _pItemTree = NULL;
        }
        _dataFlag = false;
        _iterate = 0;
        _treeCounter = 0;
        for(int i = 0; i <= _pTreeConfigure->MaxClass; i ++) free(_pTreeConfigure->ClassName[i]);
        for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pTreeConfigure->MaxAttVal[i]; j ++) free(_pTreeConfigure->AttValName[i][j]);
            free(_pTreeConfigure->AttName[i]);
            free(_pTreeConfigure->AttValName[i]);
        }
        free(_pTreeConfigure->AttName);
        free(_pTreeConfigure->AttValName);
        free(_pTreeConfigure->ClassName);
        free(_pTreeConfigure->MaxAttVal);
        free(_pTreeConfigure->SpecialStatus);
    }
    if(_pTreeConfigure == NULL) _pTreeConfigure = (Configure *) malloc(sizeof(Configure));
    _pTreeConfigure->MaxDiscrVal = 2;
    _pTreeConfigure->MaxAtt = -1;
    _pTreeConfigure->MaxAttVal = (short *) calloc((attributes->getLength()-1), sizeof(short));
    _pTreeConfigure->AttName = (char * *) calloc((attributes->getLength()-1), sizeof(char *));
    _pTreeConfigure->SpecialStatus = (char *) malloc((attributes->getLength()-1)*sizeof(char));
    _pTreeConfigure->AttValName = (char * * *) calloc((attributes->getLength()-1), sizeof(char * *));
    for(int i = 0; i < attributes->getLength(); i ++) {
        if ((attributes->getAttribute (i) == NULL)||(attributes->getValueByIndex (i) == NULL)) {
            _pszErrorMessage = "error: one of the attributes or the values is NULL. Unable to configure the tree. \0";
            if(pLogger) pLogger->logMsg("C45DecisionTree::configureClassifier", Logger::L_SevereError,
                "%s\n", _pszErrorMessage);
            _errorCode = 1;
            _m.unlock();
            return 1;
        }
        _pErrOcc = getNames (_pTreeConfigure, attributes->getAttribute (i), attributes->getValueByIndex (i));
        if(_pErrOcc != NULL) {
            _pszErrorMessage = _pErrOcc->errorMessage;
            if(pLogger) pLogger->logMsg("C45DecisionTree::configureClassifier", Logger::L_SevereError,
            "%s\n", _pszErrorMessage);
            _errorCode = _pErrOcc->errorCode;
            _m.unlock();
            return _pErrOcc->errorCode;
        }
    }
    // test prints
    checkAndLogMsg ("C45DecisionTree::configureClassifier", Logger::L_Info, "MaxClass = %d\n", _pTreeConfigure->MaxClass);

    /*for(int i = 0; i <= _pTreeConfigure->MaxClass; i ++) printf("ClassName[%d] = %s\n", i, _pTreeConfigure->ClassName[i]);
    printf("MaxDiscrVal = %d\n", _pTreeConfigure->MaxDiscrVal);
    printf("MaxAtt = %d\n", _pTreeConfigure->MaxAtt);
    for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        printf("AttName[%d] = %s\n", i, _pTreeConfigure->AttName[i]);
        printf("MaxAttVal[%d] = %d\n", i, _pTreeConfigure->MaxAttVal[i]);
        printf("SpecialStatus[%d] = %d\n", i, _pTreeConfigure->SpecialStatus[i]);
        if(_pTreeConfigure->SpecialStatus[i] != 2) {
            for(int l = 0; l <= _pTreeConfigure->MaxAttVal[i]; l ++)
                printf("AttValName[%d] = %s\n", l, _pTreeConfigure->AttValName[i][l]);
        }
        else {
            if(_pTreeConfigure->AttValName == NULL) printf("_pAttValName is NULL\n");
            else printf("AttValName is an integer = %d\n", (int) _pTreeConfigure->AttValName[i][0]);
        }
    }*/
    _errorCode = 0;
    _m.unlock();
    return 0;
}

C45TreeInfo * C45DecisionTree::createNewTree(C45AVList * dataset)
{
    _m.lock();
    if(_pTreeConfigure == NULL) {
        _pszErrorMessage = "error: the tree is not configured yet. Unable to create the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::createNewTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return NULL;
    }
    _dataFlag = false;
    _iterate = 0;
    int i = addNewData(dataset);
    if(i != 0) {
        if(pLogger) pLogger->logMsg("C45DecisionTree::createNewTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = i;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree != NULL) {
        if(_pResultedTree->codeErrors != NULL) {
            free(_pResultedTree->codeErrors->errorMessage);
            free(_pResultedTree->codeErrors);
        }
        if(_pResultedTree->nTrees > 0) {
            for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                if(_pResultedTree->trees[i]->testResults != NULL) {
                    free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                    free(_pResultedTree->trees[i]->testResults);
                }
                freeTree(_pResultedTree->trees[i]->tree);
                free(_pResultedTree->trees[i]);
            }
            free(_pResultedTree->trees);
        }
        free(_pResultedTree);
        _pResultedTree = NULL;
    }
    _pResultedTree = constructTree(_pOptions, _pTreeConfigure, _MaxItem, _pItem);
    _treeCounter ++;
    _errorCode = 0;
    // Note: treeDim() is a recursive function and could be slow when the tree (expecially the unpruned one) is big
    C45TreeInfo * info = new C45TreeInfo(treeDim(_pResultedTree->trees[1]->tree), treeDim(_pResultedTree->trees[0]->tree),
        _MaxItem + 1, _treeCounter);
    _errorCode = 0;
    _m.unlock();
    return info;
}

int C45DecisionTree::setupCycleMode(int initialWindow, int maxWindow, int increment, float percCycleErr, float percCycleWin)
{
    _m.lock();
    if((maxWindow < 1)||(maxWindow > 1000000)) {
        _pszErrorMessage = "error: maximum window must be between 1 and 1000000 or is not accepted. Unable "
                           "to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupCycleMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if((initialWindow < 1)||(initialWindow >= maxWindow)) {
        _pszErrorMessage = "error: initial window must be between 1 and maximum window or is not accepted. "
                           "Unable to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupCycleMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if((increment < 1)||(increment + initialWindow > maxWindow)) {
        _pszErrorMessage = "error: increment must be >= 1 and must respect the relation increment + initial"
                           " window <= maximum window. Unable to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupCycleMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    _pOptions->maxWindow = maxWindow;
    _pOptions->initialWindow = initialWindow;
    _pOptions->increment = increment;
    if (percCycleErr > 1.0) {
        percCycleErr = 1.0f;
    }
    else if (percCycleErr == 0.0) {
        percCycleErr = 0.20f;
    }
    _pOptions->cycleErrors = percCycleErr;
    if ((percCycleWin > 1.0) || (percCycleWin == 0.0)) {
        percCycleWin = 1.0f;
    }
    _pOptions->cycleWindow = percCycleWin;
    _iterate = 1;
    _memoryFlag = true;
    _errorCode = 0;
    _m.unlock();
    return 0;
}

int C45DecisionTree::setupWindowMode(int initialWindow, int maxWindow, int increment)
{
    _m.lock();
    if((maxWindow < 1)||(maxWindow > 1000000)) {
        _pszErrorMessage = "error: maximum window must be between 1 and 1000000 or is not accepted. Unable"
                           " to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupWindowMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if((initialWindow < 1)||(initialWindow >= maxWindow)) {
        _pszErrorMessage = "error: initial window must be between 1 and maximum window or is not accepted. "
                           "Unable to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupWindowMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if((increment < 1)||(increment + initialWindow > maxWindow)) {
        _pszErrorMessage = "error: increment must be >= 1 and must respect the relation increment + initial"
                           " window <= maximum window. Unable to iterate the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::setupWindowMode", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    _pOptions->maxWindow = maxWindow;
    _pOptions->initialWindow = initialWindow;
    _pOptions->increment = increment;
    _iterate = 2;
    _memoryFlag = true;
    _errorCode = 0;
    _m.unlock();
    return 0;
}

C45TreeInfo * C45DecisionTree::createCompositeTree(C45DecisionTree * * trees, int noTrees)
{
    _m.lock();
    if(trees == NULL) {
        _pszErrorMessage = "error: the passed C45DecisionTree pointer is NULL. Unable to create a composite"
                           " decision tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::createCompositeTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if(noTrees <= 1) {
        _pszErrorMessage = "error: there must be at least 2 input tree to create a composite tree. Unable "
                           "to create the composite tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::createCompositeTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    for(int i = 1; i < noTrees; i ++) {
        if(!compareConfiguration(trees[0], trees[i])) {
            _pszErrorMessage = "error: the passed trees must have the same configuration. Unable to create "
                               "a composite tree. \0";
            if(pLogger) pLogger->logMsg("C45DecisionTree::createCompositeTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
            _errorCode = 9;
            _m.unlock();
            return NULL;
        }
    }
    if(_pTreeConfigure != NULL) {
        if(_pConsultedTree != NULL) {
            free(_pConsultedTree->className);
            if(_pConsultedTree->codeErrors != NULL) {
                free(_pConsultedTree->codeErrors->errorMessage);
                free(_pConsultedTree->codeErrors);
            }
            free(_pConsultedTree);
            _pConsultedTree = NULL;
        }
        if(_pResultedTree != NULL) {
            if(_pResultedTree->codeErrors != NULL) {
                free(_pResultedTree->codeErrors->errorMessage);
                free(_pResultedTree->codeErrors);
            }
            if(_pResultedTree->nTrees > 0) {
                for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                    if(_pResultedTree->trees[i]->testResults != NULL) {
                        free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                        free(_pResultedTree->trees[i]->testResults);
                    }
                    freeTree(_pResultedTree->trees[i]->tree);
                    free(_pResultedTree->trees[i]);
                }
                free(_pResultedTree->trees);
            }
            free(_pResultedTree);
            _pResultedTree = NULL;
        }
        if(_pItem != NULL) {
            if(_MaxItemIncrement > 0) {
                for(int i = 0; i <= _MaxItemIncrement; i ++) free(_pItem[i]);
            }
            else {
                for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
            }
            free(_pItem);
            _pItem = NULL;
        }
        if(_pItemTest != NULL) {
            for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
            free(_pItemTest);
            _pItemTest = NULL;
        }
        if(_pItemTree != NULL) {
            free(_pItemTree);
            _pItemTree = NULL;
        }
        _dataFlag = false;
        _iterate = 0;
        _treeCounter = 0;
        _errorCode = -1;
        for(int i = 0; i <= _pTreeConfigure->MaxClass; i ++) free(_pTreeConfigure->ClassName[i]);
        for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pTreeConfigure->MaxAttVal[i]; j ++) free(_pTreeConfigure->AttValName[i][j]);
            free(_pTreeConfigure->AttName[i]);
            free(_pTreeConfigure->AttValName[i]);
        }
        free(_pTreeConfigure->AttName);
        free(_pTreeConfigure->AttValName);
        free(_pTreeConfigure->ClassName);
        free(_pTreeConfigure->MaxAttVal);
        free(_pTreeConfigure->SpecialStatus);
    }
    if(_pTreeConfigure == NULL) _pTreeConfigure = (Configure *) malloc(sizeof(Configure));
    _pTreeConfigure->MaxDiscrVal = trees[0]->_pTreeConfigure->MaxDiscrVal;
    _pTreeConfigure->MaxClass = trees[0]->_pTreeConfigure->MaxClass;
    _pTreeConfigure->ClassName = (char * *) calloc(_pTreeConfigure->MaxClass + 1, sizeof(char *));
    for(int i = 0; i <= _pTreeConfigure->MaxClass; i ++) {
        _pTreeConfigure->ClassName[i] = (char *) calloc(strlen(trees[0]->_pTreeConfigure->ClassName[i]) + 1, sizeof(char));
        strcat(_pTreeConfigure->ClassName[i], trees[0]->_pTreeConfigure->ClassName[i]);
    }
    _pTreeConfigure->MaxAtt = trees[0]->_pTreeConfigure->MaxAtt;
    _pTreeConfigure->AttName = (char * *) calloc(_pTreeConfigure->MaxAtt + 1, sizeof(char *));
    _pTreeConfigure->MaxAttVal = (short *) calloc(_pTreeConfigure->MaxAtt + 1, sizeof(short));
    _pTreeConfigure->SpecialStatus = (char *) malloc((_pTreeConfigure->MaxAtt + 1)*sizeof(char));
    _pTreeConfigure->AttValName = (char * * *) calloc(_pTreeConfigure->MaxAtt + 1, sizeof(char * *));
    for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        _pTreeConfigure->AttName[i] = (char *) calloc(strlen(trees[0]->_pTreeConfigure->AttName[i]) + 1, sizeof(char));
        strcat(_pTreeConfigure->AttName[i], trees[0]->_pTreeConfigure->AttName[i]);
        _pTreeConfigure->SpecialStatus[i] = trees[0]->_pTreeConfigure->SpecialStatus[i];
        _pTreeConfigure->MaxAttVal[i] = trees[0]->_pTreeConfigure->MaxAttVal[i];
        _pTreeConfigure->AttValName[i] = (char * *) calloc(_pTreeConfigure->MaxAttVal[i] + 1, sizeof(char *));
        for(int j = 0; j <= _pTreeConfigure->MaxAttVal[i]; j ++) {
            if(j == 0) _pTreeConfigure->AttValName[i][j] = NULL;
            else {
                _pTreeConfigure->AttValName[i][j] = (char *) calloc(strlen(trees[0]->_pTreeConfigure->AttValName[i][j]) + 1,
                                                    sizeof(char));
                strcat(_pTreeConfigure->AttValName[i][j], trees[0]->_pTreeConfigure->AttValName[i][j]);
            }
        }
    }
    _MaxItem = 0;
    for(int i = 0; i < noTrees; i ++) {
        if(trees[i]->_pItem == NULL) continue;
        if(trees[i]->_MaxDelete > 0) _MaxItem += trees[i]->_MaxItem - trees[i]->_MaxDelete - 1;
        else _MaxItem += trees[i]->_MaxItem;
    }
    if(_MaxItem == 0) {
        _pszErrorMessage = "error: can not create a composite tree with trees without saved data. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::createCompositeTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    _pItem = (Description *) calloc(_MaxItem + 1, sizeof(Description));
    int i = 0;
    int h = 0;
    for(int l = 0; l < noTrees; l ++) {
        if(trees[l]->_MaxDelete > 0) {
            for(int j = 0; j < (trees[l]->_MaxItem - trees[l]->_MaxDelete); j ++, h ++)
                _pItem[h] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
            for(int j = trees[l]->_MaxDelete + 1; j <= trees[l]->_MaxItem; j ++, i ++) {
                memcpy(_pItem[i], trees[l]->_pItem[j], (_pTreeConfigure->MaxAtt + 2) * sizeof(AttValue));
            }
        }
        else {
            for(int j = 0; j <= trees[l]->_MaxItem; j ++, h ++)
                _pItem[h] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
            for(int j = 0; j <= trees[l]->_MaxItem; j ++, i ++) {
                memcpy(_pItem[i], trees[l]->_pItem[j], (_pTreeConfigure->MaxAtt + 2) * sizeof(AttValue));
            }
        }
    }
    _pResultedTree = constructTree(_pOptions, _pTreeConfigure, _MaxItem, _pItem);
    _treeCounter ++;
    _errorCode = 0;
    // Note: treeDim() is a recursive function and could be slow when the tree (expecially the unpruned one) is big
    C45TreeInfo * info = new C45TreeInfo(treeDim(_pResultedTree->trees[1]->tree),
                             treeDim(_pResultedTree->trees[0]->tree), (_MaxItem + 1), _treeCounter);
    _errorCode = 0;
    _m.unlock();
    return info;
}

int C45DecisionTree::addNewData(C45AVList * dataset)
{
    _m.lock();
    if(_iterate == 3) {
        _pszErrorMessage = "error: can not insert new data in a read tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if(dataset == NULL) {
        _pszErrorMessage = "error: the passed AVList pointer is NULL. Unable to process the new data. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if(_pTreeConfigure == NULL) {
        _pszErrorMessage = "error: the tree must be configured before you could add some data. Unable to "
            "process the new data. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return 3;
    }
    if((dataset->getLength() % (_pTreeConfigure->MaxAtt + 2)) != 0) {
        _pszErrorMessage = "error: the passed AVList data contains a partial record, so can't be accepted."
            " Unable to process the new data. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return 3;
    }
    if(_dataFlag) {   // test data
        if(_pItemTest != NULL) {
            for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
            free(_pItemTest);
            _pItemTest = NULL;
        }
        _MaxItemTest = -1;
        _pItemTest = (Description *) calloc((dataset->getLength() / (_pTreeConfigure->MaxAtt + 2)), sizeof(Description));
        for(int i = 0; i < dataset->getLength(); i += (_pTreeConfigure->MaxAtt + 2)) {
            _MaxItemTest ++;
            _pItemTest[_MaxItemTest] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
            for(int l = 0; l < _pTreeConfigure->MaxAtt + 2; l ++) {
                _pErrOcc = getDataset(_pTreeConfigure, dataset->getAttribute(i+l), dataset->getValueByIndex (i+l),
                    _MaxItemTest, _pItemTest);
                if(_pErrOcc != NULL) {
                    _dataFlag = false;
                    _pszErrorMessage = _pErrOcc->errorMessage;
                    if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
                    _errorCode = _pErrOcc->errorCode;
                    _m.unlock();
                    return _pErrOcc->errorCode;
                }
            }
        }
        _dataFlag = false;
        _errorCode = 0;
        _m.unlock();
        return 0;
    }
    if(_iterate == 0) {  // normal mode
        if(_pItem == NULL) {
            _MaxItem = -1;
            _pItem = (Description *) calloc((dataset->getLength() / (_pTreeConfigure->MaxAtt + 2)), sizeof(Description));
        }
        else {
            _MaxItem ++;
            _pItem = (Description *) realloc(_pItem,
                (dataset->getLength() / (_pTreeConfigure->MaxAtt + 2) + _MaxItem + 1) * sizeof(Description));
        }
        for(int i = 0; i < dataset->getLength(); i += _pTreeConfigure->MaxAtt + 2) {
            _MaxItem ++;
            _pItem[_MaxItem] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
            for(int l = 0; l < _pTreeConfigure->MaxAtt + 2; l ++) {
                _pErrOcc = getDataset(_pTreeConfigure, dataset->getAttribute(i + l),
                    dataset->getValueByIndex (i + l), _MaxItem, _pItem);
                if(_pErrOcc != NULL) {
                    _pszErrorMessage = _pErrOcc->errorMessage;
                    if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
                    _errorCode = _pErrOcc->errorCode;
                    _m.unlock();
                    return _pErrOcc->errorCode;
                }
            }
        }
        _errorCode = 0;
        _m.unlock();
        return 0;
    }                                  // iterative modes
    int allocated = -1;
    do {
        if(_pItemTree == NULL) {       // initialise
            if(_pItem != NULL) {       // free space if previous normal mode was in use
                for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
                free(_pItem);
                _pItem = NULL;
            }
            _MaxItem = -1;
            _MaxDelete = -1;
            _MaxItemIncrement = -1;
            _MaxItemTree = -1;
            _MaxItemIndex = -1;
            _MaxItemTreeIncrement = -1;  // alloc initialWindow
            _pItem = (Description *) calloc(_pOptions->initialWindow, sizeof(Description));
            _pItemTree = (Description *) calloc(_pOptions->initialWindow, sizeof(Description));
            _MaxItemPos = _pOptions->initialWindow - 1;
        }
        else {
            if(_MaxItemTree == _MaxItemPos) {
                if(_MaxItemTree + _pOptions->increment < _pOptions->maxWindow) {   // alloc increment
                    if(_MaxItemIndex < (_MaxItem + _pOptions->increment)) {
                        _MaxItemIndex = _MaxItem + _pOptions->increment;
                        _pItem = (Description *) realloc(_pItem, (_MaxItem + 1 + _pOptions->increment) * sizeof(Description));
                        _pItemTree = (Description *) realloc(_pItemTree,
                            (_MaxItemTree + 1 + _pOptions->increment) * sizeof(Description));
                    }
                    _MaxItemPos = _MaxItemTree + _pOptions->increment;
                }
                else {
                    _MaxDelete = _MaxItemTree + _pOptions->increment - _pOptions->maxWindow;
                    for(int i = 0; i <= _MaxDelete; i ++) {        // for each item to delete in _ItemTree
                        for(int l = 0; l <= _MaxItemTree; l ++) {  // find the right item in _ItemTree
                            if(_pItem[i] == _pItemTree[l]) {
                                for(int j = l; j < _MaxItemTree; j ++) _pItemTree[j] = _pItemTree[j+1];// compatta
                                _pItemTree[_MaxItemTree] = NULL;
                                _MaxItemTree --;
                                break;
                            }
                        }
                    }
                    if(_MaxItemPos >= (_pOptions->maxWindow - _pOptions->increment)) {
                        if(_memoryFlag) {
                            _pItemTree = (Description *) realloc(_pItemTree, _pOptions->maxWindow * sizeof(Description));
                            _memoryFlag = false;
                        }
                        _MaxItemPos = _pOptions->maxWindow - 1;
                    }
                    if(_MaxItemIndex < (_MaxItem + _pOptions->increment)) {
                        _MaxItemIndex = _MaxItem + _pOptions->increment;
                        _pItem = (Description *) realloc(_pItem, (_MaxItem + 1 + _pOptions->increment) * sizeof(Description));
                    }
                }
            }
        }
        if(_MaxItemTree + 1 < _pOptions->initialWindow) {
            for(int i = _MaxItemTree + 1; i <= _MaxItemPos; i ++) {
                allocated ++;
                if(allocated == dataset->getLength()/(_pTreeConfigure->MaxAtt+2)) break;
                _MaxItemTree ++;
                _pItemTree[_MaxItemTree] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
                _MaxItem ++;
                _pItem[_MaxItem] = _pItemTree[_MaxItemTree];
                for(int l = 0; l < _pTreeConfigure->MaxAtt + 2; l ++) {
                    _pErrOcc = getDataset(_pTreeConfigure, dataset->getAttribute(allocated*(_pTreeConfigure->MaxAtt+2)+l),
                        dataset->getValueByIndex (allocated*(_pTreeConfigure->MaxAtt+2)+l), _MaxItemTree, _pItemTree);
                    if(_pErrOcc != NULL) {
                        _pszErrorMessage = _pErrOcc->errorMessage;
                        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
                            "%s\n", _pszErrorMessage);
                        _errorCode = _pErrOcc->errorCode;
                        _m.unlock();
                        return _pErrOcc->errorCode;
                    }
                }
            }
        }
        else {
            if(_MaxItemTreeIncrement == -1) {
                _MaxItemIncrement = _MaxItem;
                _MaxItemTreeIncrement = _MaxItemTree;
            }
            for(; _MaxItemTreeIncrement < _MaxItemPos;) {
                allocated ++;
                if(allocated == dataset->getLength() / (_pTreeConfigure->MaxAtt+2)) break;
                _MaxItemIncrement ++;
                _MaxItemTreeIncrement ++;
                _pItemTree[_MaxItemTreeIncrement] = (Description) calloc(_pTreeConfigure->MaxAtt + 2, sizeof(AttValue));
                _pItem[_MaxItemIncrement] = _pItemTree[_MaxItemTreeIncrement];
                for(int l = 0; l < _pTreeConfigure->MaxAtt + 2; l ++) {
                    _pErrOcc = getDataset(_pTreeConfigure, dataset->getAttribute(allocated*(_pTreeConfigure->MaxAtt+2)+l),
                        dataset->getValueByIndex (allocated*(_pTreeConfigure->MaxAtt+2)+l), _MaxItemTreeIncrement, _pItemTree);
                    if(_pErrOcc != NULL) {
                        _pszErrorMessage = _pErrOcc->errorMessage;
                        if(pLogger) pLogger->logMsg("C45DecisionTree::addNewData", Logger::L_MildError,
                            "%s\n", _pszErrorMessage);
                        _errorCode = _pErrOcc->errorCode;
                        _m.unlock();
                        return _pErrOcc->errorCode;
                    }
                }
            }
        }
        if(_MaxItemTree == _MaxItemPos) {
            // test prints
            //printf("prima di constructTree, print di _pItemTree\n");
            //for(int i = 0; i <= _MaxItemTree; i ++) {
            //    for(int l = 0; l < _pTreeConfigure->MaxAtt + 2; l ++) {
            //        if((l == 0)||(l == 2)||(l == 4)||(l == 10)||(l == 11)||(l == 12))
            //            printf("ItemTree[%d][%d] = %f\n", i, l, _pItemTree[i][l]._cont_val);
            //        else printf("ItemTree[%d][%d] = %d\n", i, l, _pItemTree[i][l]._discr_val);
            //    }
            //}
            if(_pResultedTree != NULL) {
                if(_pResultedTree->codeErrors != NULL) {
                    free(_pResultedTree->codeErrors->errorMessage);
                    free(_pResultedTree->codeErrors);
                }
                if(_pResultedTree->nTrees > 0) {
                    for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                        if(_pResultedTree->trees[i]->testResults != NULL) {
                            free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                            free(_pResultedTree->trees[i]->testResults);
                        }
                        freeTree(_pResultedTree->trees[i]->tree);
                        free(_pResultedTree->trees[i]);
                    }
                    free(_pResultedTree->trees);
                }
                free(_pResultedTree);
            }
            _pResultedTree = constructTree(_pOptions, _pTreeConfigure, _MaxItemTree, _pItemTree);
            _treeCounter ++;
        }
        if(_MaxItemTreeIncrement == _MaxItemPos) {
            if(_iterate == 1) {
                int cycles = 0;
                if(_pResultedTree != NULL) {
                    if(_pResultedTree->codeErrors != NULL) {
                        free(_pResultedTree->codeErrors->errorMessage);
                        free(_pResultedTree->codeErrors);
                    }
                    if(_pResultedTree->nTrees > 0) {
                        for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                            if(_pResultedTree->trees[i]->testResults != NULL) {
                                free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                                free(_pResultedTree->trees[i]->testResults);
                            }
                            freeTree(_pResultedTree->trees[i]->tree);
                            free(_pResultedTree->trees[i]);
                            _pResultedTree->trees[i] = NULL;
                        }
                    }
                }
                _MaxItemTree = iterativeMode(_pOptions, _pTreeConfigure, _MaxItemTree+1, _MaxItemTreeIncrement,
                    _pItemTree, _pResultedTree, &cycles);
                _treeCounter += cycles;
            }
            if (_iterate == 2) {
                if(_pResultedTree != NULL) {
                    if(_pResultedTree->codeErrors != NULL) {
                        free(_pResultedTree->codeErrors->errorMessage);
                        free(_pResultedTree->codeErrors);
                    }
                    if(_pResultedTree->nTrees > 0) {
                        for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                            if(_pResultedTree->trees[i]->testResults != NULL) {
                                free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                                free(_pResultedTree->trees[i]->testResults);
                            }
                            freeTree(_pResultedTree->trees[i]->tree);
                            free(_pResultedTree->trees[i]);
                        }
                        free(_pResultedTree->trees);
                    }
                    free(_pResultedTree);
                    _pResultedTree = NULL;
                }
                _pResultedTree = constructTree(_pOptions, _pTreeConfigure, _MaxItemTreeIncrement, _pItemTree);
                _treeCounter ++;
                _MaxItemTree = _MaxItemTreeIncrement;
            }
            if(_MaxDelete != -1) {
                for(int i = 0; i <= _MaxItemIncrement; i ++) {
                    if(i <= _MaxDelete) free(_pItem[i]);
                    else _pItem[i - _MaxDelete - 1] = _pItem[i];
                }
                for(int i = _MaxItemIncrement - _MaxDelete; i <= _MaxItemIncrement; i ++) _pItem[i] = NULL;
                _MaxItem = _MaxItem - _MaxDelete - 1;
                _MaxItemIncrement = _MaxItemIncrement - _MaxDelete - 1;
                _MaxDelete = -1;
            }
            if(_MaxItemTree != _MaxItemTreeIncrement) {
                if((_iterate == 1) || ((_iterate == 2) && (_MaxItemTreeIncrement == _pOptions->maxWindow - 1))) {
                    for(int i = _MaxItemTree + 1; i <= _MaxItemTreeIncrement; i ++) {
                        for(int l = 0; l <= _MaxItemIncrement; l ++) {
                            if(_pItem[l] == _pItemTree[i]) {
                                for(int j = l; j < _MaxItemIncrement; j ++) _pItem[j] = _pItem[j+1];
                                _pItem[_MaxItemIncrement] = NULL;
                                _MaxItemIncrement --;
                                break;
                            }
                        }
                    }
                    for(int i = _MaxItemTree+1; i <= _MaxItemTreeIncrement; i ++) free(_pItemTree[i]);
                }
            }
            _MaxItem = _MaxItemIncrement;
            _MaxItemIncrement = -1;
            _MaxItemTreeIncrement = -1;
            _MaxItemPos = _MaxItemTree;
        }
    } while(allocated != dataset->getLength() / (_pTreeConfigure->MaxAtt+2));
    _errorCode = 0;
    _m.unlock();
    return 0;
}

C45TreePrediction * C45DecisionTree::consultClassifier(C45AVList * pRecord)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during"
                           " the construction. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(pRecord->getLength() != _pTreeConfigure->MaxAtt + 1) {
        _pszErrorMessage = "error: the given AVList must contains a number of pairs equal to the"
                           " number of attributes in the tree. Unable to consult the tree. \0";
        printf("\n\t\tAVList length = %d, MaxAttr + 1 = %d\n", pRecord->getLength(), _pTreeConfigure->MaxAtt + 1);
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return NULL;
    }
    int i;
    char * * list = (char * *) calloc(pRecord->getLength(), sizeof(char *));
    for(i = 0; i < pRecord->getLength(); i ++) {
        list[i] = (char *) calloc(strlen(pRecord->getAttribute(i))+strlen(pRecord->getValueByIndex(i))+2, sizeof(char));
        strcat(list[i], pRecord->getAttribute(i));
        strcat(list[i], ",");
        strcat(list[i], pRecord->getValueByIndex(i));
    }
    if(_pResultedTree->nTrees > 0) {
        if(_pResultedTree->trees[0]->isPruned == 1) i = 0;
        else {
            if(_pResultedTree->nTrees == 2) i = 1;
            else {
                _pszErrorMessage = "error: there isn't any pruned tree. Unable to consult the tree. \0";
                if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = 9;
                _m.unlock();
                return NULL;
            }
        }
    }
    else {
        _pszErrorMessage = "error: there isn't any pruned tree. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pConsultedTree != NULL) {
        if(_pConsultedTree->codeErrors != NULL) {
            free(_pConsultedTree->codeErrors->errorMessage);
            free(_pConsultedTree->codeErrors);
        }
        free(_pConsultedTree);
        _pConsultedTree = NULL;
    }
    _pConsultedTree = consultTree(_pOptions, _pTreeConfigure, _pResultedTree->trees[i]->tree, list);
    if(_pConsultedTree->codeErrors != NULL) {
        _pszErrorMessage = _pConsultedTree->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pConsultedTree->codeErrors->errorCode;
        _m.unlock();
        return NULL;
    }
    C45TreePrediction * pred = new C45TreePrediction(_pConsultedTree);
    _errorCode = 0;
    _m.unlock();
    return pred;
}

C45TreePrediction * C45DecisionTree::consultUnprunedTree(C45AVList * record)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during"
                           " the construction. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(record->getLength() != _pTreeConfigure->MaxAtt+1) {
        _pszErrorMessage = "error: the passed AVList must contains a number of pairs equal to the"
                           " number of attributes in the tree. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return NULL;
    }
    char * * list = (char * *) calloc(record->getLength(), sizeof(char *));
    for(int i = 0; i < record->getLength(); i ++) {
        list[i] = (char *) calloc(strlen(record->getAttribute(i))+strlen(record->getValueByIndex(i))+2, sizeof(char));
        strcat(list[i], record->getAttribute(i));
        strcat(list[i], ",");
        strcat(list[i], record->getValueByIndex(i));
    }
    if(_pResultedTree->nTrees < 0) {
        _pszErrorMessage = "error: there isn't any unpruned tree. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->trees[0]->isPruned != 0) {
        _pszErrorMessage = "error: there isn't any unpruned tree. Unable to consult the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;;
    }
    if(_pConsultedTree != NULL) {
        free(_pConsultedTree->className);
        if(_pConsultedTree->codeErrors != NULL) {
            free(_pConsultedTree->codeErrors->errorMessage);
            free(_pConsultedTree->codeErrors);
        }
        free(_pConsultedTree);
        _pConsultedTree = NULL;
    }
    _pConsultedTree = consultTree(_pOptions, _pTreeConfigure, _pResultedTree->trees[0]->tree, list);
    if(_pConsultedTree->codeErrors != NULL) {
        _pszErrorMessage = _pConsultedTree->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45DecisionTree::consultUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pConsultedTree->codeErrors->errorCode;
        _m.unlock();
        return NULL;
    }
    C45TreePrediction * pred = new C45TreePrediction(_pConsultedTree);
    _errorCode = 0;
    _m.unlock();
    return pred;
}

C45TreeTestInfo * C45DecisionTree::testClassifierOnData(C45AVList * dataset)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during"
                           " the construction. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    _dataFlag = true;
    int i = addNewData(dataset);
    if(i != 0) {
        if(pLogger) pLogger->logMsg("C45DecisionTree::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = i;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees > 0) {
        i = -1;
        if(_pResultedTree->trees[0]->isPruned == 1) i = 0;
        if(_pResultedTree->nTrees == 2) i = 1;
        if(i != -1) {
            testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 1);
            if(_pResultedTree->codeErrors != NULL) {
                _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
                if(pLogger) pLogger->logMsg("C45DecisionTree::testClassifierOnData", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = _pResultedTree->codeErrors->errorCode;
                _m.unlock();
                return NULL;
            }
            C45TreeTestInfo * test = new C45TreeTestInfo(_pResultedTree->trees[i]->testResults->treeSize,
                _pResultedTree->trees[i]->testResults->noErrors, _pResultedTree->trees[i]->testResults->noItems,
                _pResultedTree->trees[i]->testResults->percErrors, _pResultedTree->trees[i]->testResults->estimate,
                _pResultedTree->trees[i]->testResults->confusionMatrix,
                _pResultedTree->trees[i]->testResults->noClasses * _pResultedTree->trees[i]->testResults->noClasses);
            _errorCode = 0;
            _m.unlock();
            return test;
        }
    }
    _pszErrorMessage = "error: there isn't the pruned tree. Unable to test it. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testClassifierOnData", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45TreeTestInfo * C45DecisionTree::testPrunedTree(void)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testPrunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during the "
                           "construction. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testPrunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees > 0) {
        int i = -1;
        if(_pResultedTree->trees[0]->isPruned == 1) i = 0;
        if(_pResultedTree->nTrees == 2) i = 1;
        if(i != -1) {
            if(_pItemTest != NULL) testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 1);
            else testTree(_pOptions, _pTreeConfigure, _MaxItem, _pItem, _pResultedTree, 1);
            if(_pResultedTree->codeErrors != NULL) {
                _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
                if(pLogger) pLogger->logMsg("C45DecisionTree::testPrunedTree", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = _pResultedTree->codeErrors->errorCode;
                _m.unlock();
                return NULL;
            }
            C45TreeTestInfo * test = new C45TreeTestInfo(_pResultedTree->trees[i]->testResults->treeSize,
                _pResultedTree->trees[i]->testResults->noErrors, _pResultedTree->trees[i]->testResults->noItems,
                _pResultedTree->trees[i]->testResults->percErrors, _pResultedTree->trees[i]->testResults->estimate,
                _pResultedTree->trees[i]->testResults->confusionMatrix,
                _pResultedTree->trees[i]->testResults->noClasses * _pResultedTree->trees[i]->testResults->noClasses);
            _errorCode = 0;
            _m.unlock();
            return test;
        }
    }
    _pszErrorMessage = "error: there isn't the pruned tree. Unable to test it. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testPrunedTree", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45TreeTestInfo * C45DecisionTree::testUnprunedTreeOnData(C45AVList * dataset)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTreeOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during the "
                           "construction. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTreeOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    _dataFlag = true;
    int i = addNewData(dataset);
    if(i != 0) {
        if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTreeOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = i;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees > 0) {
        if(_pResultedTree->trees[0]->isPruned == 0) {
            testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 0);
            if(_pResultedTree->codeErrors != NULL) {
                _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
                if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTreeOnData", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = _pResultedTree->codeErrors->errorCode;
                _m.unlock();
                return NULL;
            }
            C45TreeTestInfo * test = new C45TreeTestInfo(_pResultedTree->trees[0]->testResults->treeSize,
                _pResultedTree->trees[0]->testResults->noErrors, _pResultedTree->trees[0]->testResults->noItems,
                _pResultedTree->trees[0]->testResults->percErrors, _pResultedTree->trees[0]->testResults->estimate,
                _pResultedTree->trees[0]->testResults->confusionMatrix,
                _pResultedTree->trees[0]->testResults->noClasses * _pResultedTree->trees[0]->testResults->noClasses);
            _errorCode = 0;
            _m.unlock();
            return test;
        }
    }
    _pszErrorMessage = "error: there isn't the unpruned tree. Unable to test it. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTreeOnData", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45TreeTestInfo * C45DecisionTree::testUnprunedTree(void)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during"
                           " the construction. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTree", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees > 0) {
        if(_pResultedTree->trees[0]->isPruned == 0) {
            if(_pItemTest != NULL) testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 0);
            else testTree(_pOptions, _pTreeConfigure, _MaxItem, _pItem, _pResultedTree, 0);
            if(_pResultedTree->codeErrors != NULL) {
                _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
                if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTree", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = _pResultedTree->codeErrors->errorCode;
                _m.unlock();
                return NULL;
            }
            C45TreeTestInfo * test = new C45TreeTestInfo(_pResultedTree->trees[0]->testResults->treeSize,
                _pResultedTree->trees[0]->testResults->noErrors, _pResultedTree->trees[0]->testResults->noItems,
                _pResultedTree->trees[0]->testResults->percErrors, _pResultedTree->trees[0]->testResults->estimate,
                _pResultedTree->trees[0]->testResults->confusionMatrix,
                _pResultedTree->trees[0]->testResults->noClasses * _pResultedTree->trees[0]->testResults->noClasses);
            _errorCode = 0;
            _m.unlock();
            return test;
        }
    }
    _pszErrorMessage = "error: there isn't the unpruned tree. Unable to test it. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testUnprunedTree", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45TreeTestInfo * * C45DecisionTree::testBothTreesOnData(C45AVList * dataset)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the trees are not created yet. Unable to test the trees. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTreesOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the trees are not created yet, because there was an error during the"
                           " construction. Unable to test the trees. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTreesOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    _dataFlag = true;
    int i = addNewData(dataset);
    if(i != 0) {
        if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTreesOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = i;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees == 2) {
        testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 2);
        if(_pResultedTree->codeErrors != NULL) {
            _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
            if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTreesOnData", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
            _errorCode = _pResultedTree->codeErrors->errorCode;
            _m.unlock();
            return NULL;
        }
        C45TreeTestInfo * * test = new C45TreeTestInfo*[2];
        for(i = 0; i < 2; i ++) test[i] = new C45TreeTestInfo(_pResultedTree->trees[i]->testResults->treeSize,
            _pResultedTree->trees[i]->testResults->noErrors, _pResultedTree->trees[i]->testResults->noItems,
            _pResultedTree->trees[i]->testResults->percErrors, _pResultedTree->trees[i]->testResults->estimate,
            _pResultedTree->trees[i]->testResults->confusionMatrix,
            _pResultedTree->trees[i]->testResults->noClasses * _pResultedTree->trees[i]->testResults->noClasses);
        _errorCode = 0;
        _m.unlock();
        return test;
    }
    _pszErrorMessage = "error: there aren't both the trees. Unable to test them. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTreesOnData", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45TreeTestInfo * * C45DecisionTree::testBothTrees(void)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error: the tree are not created yet. Unable to test the trees. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTrees", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->codeErrors != NULL) {
        _pszErrorMessage = "error: the trees are not created yet, because there was an error during the"
                           " construction. Unable to test the trees. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTrees", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedTree->nTrees == 2) {
        if(_pItemTest != NULL) testTree(_pOptions, _pTreeConfigure, _MaxItemTest, _pItemTest, _pResultedTree, 2);
        else testTree(_pOptions, _pTreeConfigure, _MaxItem, _pItem, _pResultedTree, 2);
        if(_pResultedTree->codeErrors != NULL) {
            _pszErrorMessage = _pResultedTree->codeErrors->errorMessage;
            if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTrees", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
            _errorCode = _pResultedTree->codeErrors->errorCode;
            _m.unlock();
            return NULL;
        }
        C45TreeTestInfo * * test = new C45TreeTestInfo*[2];
        for(int i = 0; i < 2; i ++) test[i] = new C45TreeTestInfo(_pResultedTree->trees[i]->testResults->treeSize,
            _pResultedTree->trees[i]->testResults->noErrors, _pResultedTree->trees[i]->testResults->noItems,
            _pResultedTree->trees[i]->testResults->percErrors, _pResultedTree->trees[i]->testResults->estimate,
            _pResultedTree->trees[i]->testResults->confusionMatrix,
            _pResultedTree->trees[i]->testResults->noClasses * _pResultedTree->trees[i]->testResults->noClasses);
        _errorCode = 0;
        _m.unlock();
        return test;
    }
    _pszErrorMessage = "error: there aren't both the trees. Unable to test them. \0";
    if(pLogger) pLogger->logMsg("C45DecisionTree::testBothTrees", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

void C45DecisionTree::deletePrunedTree(void)
{
    _m.lock();
    if(_pResultedTree != NULL) {
        if(_pResultedTree->codeErrors == NULL) {
            if(_pResultedTree->nTrees == 2) {
                freeTree(_pResultedTree->trees[1]->tree);
                if(_pResultedTree->trees[1]->testResults != NULL) {
                    free(_pResultedTree->trees[1]->testResults->confusionMatrix);
                    free(_pResultedTree->trees[1]->testResults);
                }
            }
            else {
                if((_pResultedTree->nTrees == 1)&&(_pResultedTree->trees[0]->isPruned == 1)) {
                    freeTree(_pResultedTree->trees[0]->tree);
                    if(_pResultedTree->trees[0]->testResults != NULL) {
                        free(_pResultedTree->trees[0]->testResults->confusionMatrix);
                        free(_pResultedTree->trees[0]->testResults);
                    }
                }
            }
            _pResultedTree->trees[1] = NULL;
            _pResultedTree->nTrees --;
        }
    }
    _m.unlock();
}

void C45DecisionTree::deleteUnprunedTree(void)
{
    _m.lock();
    if(_pResultedTree != NULL) {
        if(_pResultedTree->codeErrors == NULL) {
            if(_pResultedTree->nTrees > 0) {
                freeTree(_pResultedTree->trees[0]->tree);
                if(_pResultedTree->trees[0]->testResults != NULL) {
                    free(_pResultedTree->trees[0]->testResults->confusionMatrix);
                    free(_pResultedTree->trees[0]->testResults);
                }
                _pResultedTree->trees[0] = _pResultedTree->trees[1];
                _pResultedTree->trees[1] = NULL;
                _pResultedTree->nTrees --;
            }
        }
    }
    _m.unlock();
}

void C45DecisionTree::deleteTestData(void)
{
    _m.lock();
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
        free(_pItemTest);
        _pItemTest = NULL;
        _MaxItemTest = -1;
    }
    _m.unlock();
}

bool C45DecisionTree::setInitialWindow(int initialWindow)
{
    _m.lock();
    if((initialWindow < 1) || (initialWindow > 1000000)) {
        _m.unlock();
        return false;
    }
    if(_pOptions->maxWindow == 0) {
        if(_pOptions->increment == 0) {
            _pOptions->initialWindow = initialWindow;
            _m.unlock();
            return true;
        }
        if(initialWindow + _pOptions->increment <= 1000000) {
            _pOptions->initialWindow = initialWindow;
            _m.unlock();
            return true;
        }
    }
    if((_pOptions->increment == 0) && (initialWindow < _pOptions->maxWindow)) {
        _pOptions->initialWindow = initialWindow;
        _m.unlock();
        return true;
    }
    if((_pOptions->maxWindow != 0) && (_pOptions->increment != 0)) {
        if(initialWindow + _pOptions->increment <= _pOptions->maxWindow) {
            _pOptions->initialWindow = initialWindow;
            _m.unlock();
            return true;
        }
    }
    _m.unlock();
    return false;
}

bool C45DecisionTree::setMaximumWindow(int maxWindow)
{
    _m.lock();
    if((maxWindow < 1) || (maxWindow > 1000000)) {
        _m.unlock();
        return false;
    }
    if(_pOptions->initialWindow == 0) {
        if(_pOptions->increment == 0) {
            _pOptions->maxWindow = maxWindow;
            _m.unlock();
            return true;
        }
        if(_pOptions->increment < maxWindow) {
            _pOptions->maxWindow = maxWindow;
            _m.unlock();
            return true;
        }
    }
    if((_pOptions->increment == 0) && (_pOptions->initialWindow < maxWindow)) {
        _pOptions->maxWindow = maxWindow;
        _m.unlock();
        return true;
    }
    if((_pOptions->initialWindow != 0) && (_pOptions->increment != 0)) {
        if(_pOptions->initialWindow + _pOptions->increment <= maxWindow) {
            _pOptions->maxWindow = maxWindow;
            _m.unlock();
            return true;
        }
    }
    _m.unlock();
    return false;
}

bool C45DecisionTree::setIncrementSize(int increment)
{
    _m.lock();
    if((increment < 1) || (increment > 1000000)) {
        _m.unlock();
        return false;
    }
    if(_pOptions->initialWindow == 0) {
        if(_pOptions->maxWindow == 0) {
            _pOptions->increment = increment;
            _m.unlock();
            return true;
        }
        if(increment < _pOptions->maxWindow) {
            _pOptions->increment = increment;
            _m.unlock();
            return true;
        }
    }
    if((_pOptions->maxWindow == 0) && (increment + _pOptions->initialWindow <= 1000000)) {
        _pOptions->increment = increment;
        _m.unlock();
        return true;
    }
    if((_pOptions->initialWindow != 0) && (_pOptions->maxWindow != 0)) {
        if(_pOptions->initialWindow + increment <= _pOptions->maxWindow) {
            _pOptions->increment = increment;
            _m.unlock();
            return true;
        }
    }
    _m.unlock();
    return false;
}

C45TreeInfo * C45DecisionTree::getTreeInfo(void)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _pszErrorMessage = "error : the tree is not created yet. Is not possible to retrieve information about it. ";
        if(pLogger) pLogger->logMsg("C45DecisionTree::getTreeInfo", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    int pDim = 0, uDim = 0;
    if(_pResultedTree->trees[0]->isPruned == 1) pDim = treeDim(_pResultedTree->trees[0]->tree);
    else {
        if(_pResultedTree->nTrees == 2) pDim = treeDim(_pResultedTree->trees[1]->tree);
    }
    if(_pResultedTree->trees[0]->isPruned == 0) uDim = treeDim(_pResultedTree->trees[0]->tree);
    C45TreeInfo * info = new C45TreeInfo(pDim, uDim, _MaxItem + 1, _treeCounter);
    _errorCode = 0;
    _m.unlock();
    return info;
}

int64 C45DecisionTree::read(Reader * pReader, uint32 ui32MaxSize)
{
    _m.lock();
    if(pReader == NULL) {
        _m.unlock();
        return -1;
    }
    if(_pTreeConfigure == NULL) {
        _pszErrorMessage = "error: the tree is not configured yet. Unable to read the tree. \0";
        if(pLogger) pLogger->logMsg("C45DecisionTree::read", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return -1;
    }
    _dataFlag = false;
    _iterate = 3;

    // delete previous data
    if(_pConsultedTree != NULL) {
        if(_pConsultedTree->codeErrors != NULL) {
            free(_pConsultedTree->codeErrors->errorMessage);
            free(_pConsultedTree->codeErrors);
        }
        free(_pConsultedTree);
        _pConsultedTree = NULL;
    }
    if(_pResultedTree != NULL) {
        if(_pResultedTree->codeErrors != NULL) {
            free(_pResultedTree->codeErrors->errorMessage);
            free(_pResultedTree->codeErrors);
            _pResultedTree->codeErrors = NULL;
        }
        if(_pResultedTree->nTrees > 0) {
            for(int i = 0; i < _pResultedTree->nTrees; i ++) {
                if(_pResultedTree->trees[i]->testResults != NULL) {
                    free(_pResultedTree->trees[i]->testResults->confusionMatrix);
                    free(_pResultedTree->trees[i]->testResults);
                }
                freeTree(_pResultedTree->trees[i]->tree);
                free(_pResultedTree->trees[i]);
            }
            free(_pResultedTree->trees);
            _pResultedTree->trees = NULL;
        }
    }
    if(_pItem != NULL) {
        if(_MaxItemIncrement > 0) {
            for(int i = 0; i <= _MaxItemIncrement; i ++) free(_pItem[i]);
        }
        else {
            for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
        }
        free(_pItem);
        _pItem = NULL;
        _MaxItemIncrement = -1;
    }
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
        free(_pItemTest);
        _pItemTest = NULL;
        _MaxItemTest = -1;
    }
    if(_pItemTree != NULL) {
        free(_pItemTree);
        _pItemTree = NULL;
    }
    if(_pErrOcc != NULL) {
        free(_pErrOcc->errorMessage);
        free(_pErrOcc);
        _pErrOcc = NULL;
    }

    // read a pruned tree
    if(_pResultedTree == NULL) {
        _pResultedTree = (processTreeResults *) calloc(1, sizeof(processTreeResults));
    }
    _pResultedTree->nTrees = 1;
    _pResultedTree->trees = (treeResults *) calloc(1, sizeof(treeResults));
    _pResultedTree->trees[0] = (treeResults) calloc(1, sizeof(_result_));
    _pResultedTree->trees[0]->isPruned = 1;
    _pResultedTree->trees[0]->testResults = NULL;
    _pResultedTree->trees[0]->tree = (Tree) calloc(1, sizeof(_tree_record));

    int64 totLength = 0;
    totLength = read(pReader, ui32MaxSize, totLength, _pResultedTree->trees[0]->tree);
    if(totLength < 0) {
        _m.unlock();
        return totLength;
    }

    // Read discrete values for "discrete attributes with unknown values"
    int8 retValue;
    uint16 length;
    for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        if(_pTreeConfigure->SpecialStatus[i] != DISCRETE) continue;
        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read16 (&(_pTreeConfigure->MaxAttVal[i]));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        for(int j = 1; j <= _pTreeConfigure->MaxAttVal[i]; j ++) {
            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16(&length);
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            _pTreeConfigure->AttValName[i][j] = (char *) malloc(length + 1);
            if(ui32MaxSize < (totLength + length)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->readBytes(_pTreeConfigure->AttValName[i][j], length);
            if(retValue < 0) {
                free(_pTreeConfigure->AttValName[i][j]);
                _pTreeConfigure->AttValName[i][j] = NULL;
                _m.unlock();
                return retValue;
            }
            _pTreeConfigure->AttValName[i][j][length] = '\0';
            totLength += length;
        }
    }
    _treeCounter ++;
    _m.unlock();
    return totLength;
}

int64 C45DecisionTree::skip (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize)
{
    if(pReader == NULL) {
        return -1;
    }
    int64 totLength = 0;
    totLength = skip(pReader, ui32MaxSize, totLength);
    if(totLength < 0) {
        return totLength;
    }
    // Read discrete values for "discrete attributes with unknown values"
    int8 retValue;
    uint16 length;
    uint16 ui16Temp;
    char * pszTemp = NULL;
    _m.lock();
    for(int i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        if(_pTreeConfigure->SpecialStatus[i] != DISCRETE) continue;
        if(ui32MaxSize < (totLength + sizeof(int))) {
	            return -2;
        }
        retValue = pReader->read16 (& ui16Temp);
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        for(int j = 1; j <= _pTreeConfigure->MaxAttVal[i]; j ++) {
            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16(&length);
	            if(retValue < 0) {
               _m.unlock();
                return retValue;
            }
            pszTemp = (char *) malloc(length + 1);
            if(ui32MaxSize < (totLength + length)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->readBytes(pszTemp, length);
            if(retValue < 0) {
        		free(pszTemp);
                pszTemp = NULL;
                _m.unlock();
                return retValue;
            }
            free(pszTemp);
            pszTemp = NULL;
            totLength += length;
        }
    }
    _m.unlock();
    return totLength;
}

int64 C45DecisionTree::write(Writer * pWriter, uint32 ui32MaxSize)
{
    _m.lock();
    if(pWriter == NULL) {
        _m.unlock();
        return -1;
    }
    if(_pResultedTree == NULL) {
        _m.unlock();
        return -3;
    }
    int i = -1, j;
    if(_pResultedTree->nTrees > 0) {
        if(_pResultedTree->trees[0]->isPruned == 1) i = 0;
        if(_pResultedTree->nTrees == 2) i = 1;
    }
    if(i == -1) {
        _m.unlock();
        return -3;
    }
    int64 totLength = 0;
    totLength = write(pWriter, ui32MaxSize, totLength, _pResultedTree->trees[i]->tree);
    if(totLength < 0) {
        _m.unlock();
        return totLength;
    }
    // Write attibutes values discovered for "discrete attributes with unknown values"
    int8 retValue;
    uint16 length;
    for(i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        if(_pTreeConfigure->SpecialStatus[i] != DISCRETE) {
            continue;
        }

        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }

        retValue = pWriter->write16 (&(_pTreeConfigure->MaxAttVal[i]));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += 2;

        for(j = 1; j <= _pTreeConfigure->MaxAttVal[i]; j ++) {
            length = strlen(_pTreeConfigure->AttValName[i][j]);
            if(ui32MaxSize < (uint32) (totLength + length + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write16(&length);
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }

            totLength += 2;
            retValue = pWriter->writeBytes((const char *) _pTreeConfigure->AttValName[i][j], length);
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += length;
        }
    }
    _m.unlock();
    return totLength;
}

int64 C45DecisionTree::getWriteLength(void)
{
    _m.lock();
    if(_pResultedTree == NULL) {
        _m.unlock();
        return 0;
    }
    int i = -1, j;
    if(_pResultedTree->nTrees > 0) {
        if(_pResultedTree->trees[0]->isPruned == 1) i = 0;
        if(_pResultedTree->nTrees == 2) i = 1;
    }
    if(i == -1) {
        _m.unlock();
        return 0;
    }
    int64 totLength = 0;
    totLength = getWriteLength(totLength, _pResultedTree->trees[i]->tree);
    for(i = 0; i <= _pTreeConfigure->MaxAtt; i ++) {
        if(_pTreeConfigure->SpecialStatus[i] != DISCRETE) continue;
        totLength += sizeof(int);
        for(j = 1; j <= _pTreeConfigure->MaxAttVal[i]; j ++) totLength += strlen(_pTreeConfigure->AttValName[i][j]) + 2;
    }
    _m.unlock();
    return totLength;
}

// private methods
bool C45DecisionTree::compareConfiguration(C45DecisionTree * first, C45DecisionTree * second)
{
    if(first->_pTreeConfigure->MaxDiscrVal != second->_pTreeConfigure->MaxDiscrVal) return false;
    if(first->_pTreeConfigure->MaxClass != second->_pTreeConfigure->MaxClass) return false;
    for(int i = 0; i < first->_pTreeConfigure->MaxClass; i ++) {
        if(strcmp(first->_pTreeConfigure->ClassName[i], second->_pTreeConfigure->ClassName[i])) return false;
    }
    if(first->_pTreeConfigure->MaxAtt != second->_pTreeConfigure->MaxAtt) return false;
    for(int i = 0; i <= first->_pTreeConfigure->MaxAtt; i ++) {
        if(strcmp(first->_pTreeConfigure->AttName[i], second->_pTreeConfigure->AttName[i])) return false;
        if(first->_pTreeConfigure->MaxAttVal[i] != second->_pTreeConfigure->MaxAttVal[i]) return false;
        if(first->_pTreeConfigure->SpecialStatus[i] != second->_pTreeConfigure->SpecialStatus[i]) return false;
        for(int l = 1; l <= first->_pTreeConfigure->MaxAttVal[i]; l ++) {
            if(strcmp(first->_pTreeConfigure->AttValName[i][l], second->_pTreeConfigure->AttValName[i][l])) return false;
        }
    }
    return true;
}

void C45DecisionTree::freeTree(Tree tree)
{
    if(tree->NodeType) {
        for(int i = 1; i <= tree->Forks; i ++) {
            freeTree(tree->Branch[i]);
        }
        free(tree->Branch);
        if(tree->NodeType == BrSubset) {
            for(int j = 1; j <= tree->Forks; j ++) free(tree->Subset[j]);
            free(tree->Subset);
        }
    }
    free(tree->ClassDist);
    free(tree);
}

int64 C45DecisionTree::read(Reader * pReader, uint32 ui32MaxSize, int64 totLength, Tree tree)
{
    if(totLength < 0) return totLength;
    int8 retValue;

    if(ui32MaxSize < (totLength + 2)) return -2;
    retValue = pReader->read16 (&(tree->NodeType));
    if(retValue < 0) return retValue;
    totLength += 2;

    if(ui32MaxSize < (totLength + 2)) return -2;
    retValue = pReader->read16 (&(tree->Leaf));
    if(retValue < 0) return retValue;
    totLength += 2;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pReader->read32 (&(tree->Items));
    if(retValue < 0) return retValue;
    totLength += 4;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pReader->read32 (&(tree->Errors));
    if(retValue < 0) return retValue;
    totLength += 4;

    short n = _pTreeConfigure->MaxClass + 1;
    tree->ClassDist = (ItemCount *) calloc(n, sizeof(ItemCount));
    if(ui32MaxSize < (totLength + (n * 4))) return -2;
    for (int i = 0; i < n; i++) {
        if ((retValue = pReader->read32 (&(tree->ClassDist[i]))) < 0) {
            free(tree->ClassDist);
            tree->ClassDist = NULL;
            return retValue;
        }
    }
    totLength += n * 4;

    if(tree->NodeType != NodeTypeLeaf) {
        if(ui32MaxSize < (totLength + 2)) return -2;
        retValue = pReader->read16 (&(tree->Tested));
        if(retValue < 0) return retValue;
        totLength += 2;

        if(ui32MaxSize < (totLength + sizeof(short))) return -2;
        retValue = pReader->read16 (&(tree->Forks));
        if(retValue < 0) return retValue;
        totLength += 2;

        switch(tree->NodeType) {
            case BrDiscr :
                break;

            case ThreshContin :
                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32(&(tree->Cut));
                if(retValue < 0) return retValue;
                totLength += sizeof(float);

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32 (&(tree->Lower));
                if(retValue < 0) return retValue;
                totLength += 4;

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32 (&(tree->Upper));
                if(retValue < 0) return retValue;
                totLength += 4;
                break;

            case BrSubset :
                tree->Subset = (Set *) calloc(tree->Forks + 1, sizeof(Set));
                int bytes = (_pTreeConfigure->MaxAttVal[tree->Tested]>>3) + 1;
                for(int i = 1; i <= tree->Forks; i ++) {
                    tree->Subset[i] = (Set) malloc(bytes);
                    if(ui32MaxSize < (totLength + bytes)) return -2;
                    retValue = pReader->readBytes(tree->Subset[i], bytes);
                    if(retValue < 0) {
                        free(tree->Subset[i]);
                        tree->Subset[1] = NULL;
                        return retValue;
                    }
                    totLength += bytes;
                }
        }
        tree->Branch = (Tree *) calloc(tree->Forks + 1, sizeof(Tree));
        for(int i = 1; i <= tree->Forks; i ++) {
            tree->Branch[i] = (Tree) calloc(1, sizeof(_tree_record));
            totLength = read(pReader, ui32MaxSize, totLength, tree->Branch[i]);
        }
    }
    return totLength;
}

int64 C45DecisionTree::skip(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize, int64 totLength)
{
	if(totLength < 0) return totLength;
    int8 retValue;
    short shTemp, nodeType, tested, forks;
    float flTemp;

    if(ui32MaxSize < (totLength + 2)) return -2;
    retValue = pReader->read16 (& nodeType);
    if(retValue < 0) return retValue;
    totLength += 2;

    if(ui32MaxSize < (totLength + 2)) return -2;
    retValue = pReader->read16 (& shTemp);
    if(retValue < 0) return retValue;
    totLength += 2;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pReader->read32 (& flTemp);
    if(retValue < 0) return retValue;
    totLength += 4;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pReader->read32 (& flTemp);
    if(retValue < 0) return retValue;
    totLength += 4;

    short n = _pTreeConfigure->MaxClass + 1;
    if(ui32MaxSize < (totLength + (n * 4))) return -2;
    for (int i = 0; i < n; i++) {
        if ((retValue = pReader->read32 (& flTemp)) < 0) {
            return retValue;
        }
    }
    totLength += n * 4;

    if(nodeType != NodeTypeLeaf) {
        if(ui32MaxSize < (totLength + 2)) return -2;
        retValue = pReader->read16 (& tested);
        if(retValue < 0) return retValue;
        totLength += 2;

        if(ui32MaxSize < (totLength + sizeof(short))) return -2;
        retValue = pReader->read16 (& forks);
        if(retValue < 0) return retValue;
        totLength += 2;

        switch(nodeType) {
            case BrDiscr :
                break;

            case ThreshContin :
                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32(& flTemp);
                if(retValue < 0) return retValue;
                totLength += sizeof(float);

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32 (& flTemp);
                if(retValue < 0) return retValue;
                totLength += 4;

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pReader->read32 (& flTemp);
                if(retValue < 0) return retValue;
                totLength += 4;
                break;

            case BrSubset :
                int bytes = (_pTreeConfigure->MaxAttVal[tested]>>3) + 1;
                Set setTmp;
                for(int i = 1; i <= forks; i ++) {
                    setTmp = (Set) malloc(bytes);
                    if(ui32MaxSize < (totLength + bytes)) return -2;
                    retValue = pReader->readBytes(setTmp, bytes);
                    if(retValue < 0) {
                        free(setTmp);
                        setTmp = NULL;
                        return retValue;
                    }
                    free(setTmp);
                    setTmp = NULL;
                    totLength += bytes;
                }
        }
        for(int i = 1; i <= forks; i ++) {
            totLength = skip(pReader, ui32MaxSize, totLength);
        }
    }
    return totLength;
}

int64 C45DecisionTree::write(Writer * pWriter, uint32 ui32MaxSize, int64 totLength, Tree tree)
{
    if(totLength < 0) return totLength;
    int8 retValue;

    if(ui32MaxSize < (totLength + sizeof(short))) return -2;
    retValue = pWriter->write16 (&(tree->NodeType));
    if(retValue < 0) return retValue;
    totLength += sizeof(short);

    if(ui32MaxSize < (totLength + 2)) return -2;
    retValue = pWriter->write16 (&(tree->Leaf));
    if(retValue < 0) return retValue;
    totLength += 2;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pWriter->write32(&(tree->Items));
    if(retValue < 0) return retValue;
    totLength += 4;

    if(ui32MaxSize < (totLength + 4)) return -2;
    retValue = pWriter->write32(&(tree->Errors));
    if(retValue < 0) return retValue;
    totLength += 4;

    short n = _pTreeConfigure->MaxClass + 1;
    if(ui32MaxSize < (totLength + (n * 4))) return -2;
    for (int i = 0; i < n; i++) {
        if ((retValue = pWriter->write32 (&(tree->ClassDist[i]))) < 0) {
            return retValue;
        }
    }
    totLength += n * 4;

    if(tree->NodeType != NodeTypeLeaf) {
        if(ui32MaxSize < (totLength + 2)) return -2;
        retValue = pWriter->write16 (&(tree->Tested));
        if(retValue < 0) return retValue;
        totLength += 2;

        if(ui32MaxSize < (totLength + sizeof(short))) return -2;
        retValue = pWriter->write16 (&(tree->Forks));
        if(retValue < 0) return retValue;
        totLength += sizeof(short);

        switch(tree->NodeType) {
            case BrDiscr :
                break;

            case ThreshContin :
                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pWriter->write32 (&(tree->Cut));
                if(retValue < 0) return retValue;
                totLength += 4;

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pWriter->write32 (&(tree->Lower));
                if(retValue < 0) return retValue;
                totLength += 4;

                if(ui32MaxSize < (totLength + 4)) return -2;
                retValue = pWriter->write32 (&(tree->Upper));
                if(retValue < 0) return retValue;
                totLength += 4;

                break;

            case BrSubset :
                int bytes = (_pTreeConfigure->MaxAttVal[tree->Tested] >> 3) + 1;
                for(int j = 1; j <= tree->Forks; j ++) {
                    if(ui32MaxSize < (totLength + bytes)) return -2;
                    retValue = pWriter->writeBytes((const char *) tree->Subset[j], bytes);
                    if(retValue < 0) return retValue;
                    totLength += bytes;
                }
        }

        for(int j = 1; j <= tree->Forks; j ++) {
            totLength = write(pWriter, ui32MaxSize, totLength, tree->Branch[j]);
        }
    }
    return totLength;
}

int64 C45DecisionTree::getWriteLength(int64 totLength, Tree tree)
{
    totLength += sizeof(short) + sizeof(ClassNo) + (_pTreeConfigure->MaxClass + 3) * sizeof(ItemCount);
    if(tree->NodeType) {
        totLength += sizeof(Attribute) + sizeof(short);
        switch(tree->NodeType) {
            case BrDiscr :
                break;
            case ThreshContin :
                totLength += 3 * sizeof(float);
                break;
            case BrSubset :
                totLength += tree->Forks * ((_pTreeConfigure->MaxAttVal[tree->Tested] >> 3) + 1);
                break;
        }
        for(int j = 1; j <= tree->Forks; j ++) {
            totLength = getWriteLength(totLength, tree->Branch[j]);
        }
    }
    return totLength;
}
