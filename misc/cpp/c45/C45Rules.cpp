/*
 * C45Rules.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45Rules.h"

#include "C45AVList.h"
#include "C45DecisionTree.h"
#include "C45RuleSetInfo.h"
#include "C45RuleSetTestInfo.h"
#include "C45RulesTest.h"

#include "c4.5.h"

#include "Reader.h"
#include "Writer.h"
#include "Mutex.h"
#include "Logger.h"

#include "types.h"
#include "c4.5rules.h"
#include "consultr.h"

#include <stdlib.h>
#include <string.h>

using namespace NOMADSUtil;
using namespace IHMC_C45;

C45Rules::C45Rules()
{
    _m.lock();
    // initialize the options with default value
    _pOptions = (ruleOptions *) malloc(sizeof(ruleOptions));
    _pOptions->annealing = false;
    _pOptions->fisherThresh = 0.05f;
    _pOptions->isFisherInUse = false;
    _pOptions->pruneConfidence = 0.25f;
    _pOptions->redundancy = 1.0f;
    _pOptions->verbosity = false;
    // initialize other variables
    _pResultedRules = NULL;
    _pConsultedRules = NULL;
    _pRuleConfigure = NULL;
    _pszErrorMessage = NULL;
    _pItemTest = NULL;
    _noRuleSetConstructed = 0;
    _pErrOcc = NULL;
    _errorCode = 0;
    _m.unlock();
}

C45Rules::~C45Rules()
{
    _m.lock();
    free(_pOptions);
    if(_pResultedRules != NULL) {
        for(int i = 0; i < _pResultedRules->nSet; i ++) {
            for(int j = 1; j <= _pResultedRules->set[i].SNRules; j ++) {
                for(int l = 1; l <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Size; l ++) {
                    if(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->NodeType == 3) {
                        for(int k = 1; k <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Forks; k ++) {
                            free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset[k]);
                        }
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset);
                    }
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest);
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]);
                }
                free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs);
            }
            free(_pResultedRules->set[i].SRule);
            free(_pResultedRules->set[i].DefaultClassName);
            free(_pResultedRules->set[i].SRuleIndex);
            if(_pResultedRules->set[i].testResults != NULL) {
                free(_pResultedRules->set[i].testResults->confusionMatrix);
                free(_pResultedRules->set[i].testResults);
            }
        }
        free(_pResultedRules->set);
        if(_pResultedRules->codeErrors != NULL) {
            free(_pResultedRules->codeErrors->errorMessage);
            free(_pResultedRules->codeErrors);
        }
        free(_pResultedRules);
    }
    if(_pConsultedRules != NULL) {
        free(_pConsultedRules->className);
        if(_pConsultedRules->codeErrors != NULL) {
            free(_pConsultedRules->codeErrors->errorMessage);
            free(_pConsultedRules->codeErrors);
        }
        free(_pConsultedRules);
    }
    if(_pRuleConfigure != NULL) {
        for(int i = 0; i <= _pRuleConfigure->MaxClass; i ++) free(_pRuleConfigure->ClassName[i]);
        for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pRuleConfigure->MaxAttVal[i]; j ++) free(_pRuleConfigure->AttValName[i][j]);
            free(_pRuleConfigure->AttName[i]);
            free(_pRuleConfigure->AttValName[i]);
        }
        free(_pRuleConfigure->AttName);
        free(_pRuleConfigure->AttValName);
        free(_pRuleConfigure->ClassName);
        free(_pRuleConfigure->MaxAttVal);
        free(_pRuleConfigure->SpecialStatus);
        free(_pRuleConfigure);
    }
    if(_pszErrorMessage != NULL) free((void *)_pszErrorMessage);
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
        free(_pItemTest);
    }
    if(_pErrOcc != NULL) free(_pErrOcc);
    _m.unlock();
}

C45RuleSetInfo * C45Rules::createRuleSet(C45DecisionTree * tree)
{
    _m.lock();
    if(tree == NULL) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::createRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if(tree->_pResultedTree == NULL) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::createRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if(tree->_pResultedTree->nTrees == 0) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::createRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if(tree->_pResultedTree->trees[0]->isPruned != 0) {
        _pszErrorMessage = "error: there isn't any unpruned tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::createRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if(_pRuleConfigure != NULL) {
        if(_pResultedRules != NULL) {
            for(int i = 0; i < _pResultedRules->nSet; i ++) {
                for(int j = 1; j <= _pResultedRules->set[i].SNRules; j ++) {
                    for(int l = 1; l <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Size; l ++) {
                        if(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->NodeType == 3) {
                            for(int k = 1; k <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Forks; k ++) {
                                free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset[k]);
                            }
                            free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset);
                        }
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest);
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]);
                    }
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs);
                }
                free(_pResultedRules->set[i].SRule);
                free(_pResultedRules->set[i].DefaultClassName);
                free(_pResultedRules->set[i].SRuleIndex);
                if(_pResultedRules->set[i].testResults != NULL) {
                    free(_pResultedRules->set[i].testResults->confusionMatrix);
                    free(_pResultedRules->set[i].testResults);
                }
            }
            free(_pResultedRules->set);
            if(_pResultedRules->codeErrors != NULL) {
                free(_pResultedRules->codeErrors->errorMessage);
                free(_pResultedRules->codeErrors);
            }
            free(_pResultedRules);
            _pResultedRules = NULL;
        }
        if(_pItemTest != NULL) {
            for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
            free(_pItemTest);
            _pItemTest = NULL;
            _MaxItemTest = -1;
        }
        if(_pConsultedRules != NULL) {
            free(_pConsultedRules->className);
            if(_pConsultedRules->codeErrors != NULL) {
                free(_pConsultedRules->codeErrors->errorMessage);
                free(_pConsultedRules->codeErrors);
            }
            free(_pConsultedRules);
            _pConsultedRules = NULL;
        }
        for(int i = 0; i <= _pRuleConfigure->MaxClass; i ++) free(_pRuleConfigure->ClassName[i]);
        for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pRuleConfigure->MaxAttVal[i]; j ++) free(_pRuleConfigure->AttValName[i][j]);
            free(_pRuleConfigure->AttName[i]);
            free(_pRuleConfigure->AttValName[i]);
        }
        free(_pRuleConfigure->AttName);
        free(_pRuleConfigure->AttValName);
        free(_pRuleConfigure->ClassName);
        free(_pRuleConfigure->MaxAttVal);
        free(_pRuleConfigure->SpecialStatus);
    }
    if(_pRuleConfigure == NULL) _pRuleConfigure = (Configure *) malloc(sizeof(Configure));
    _pRuleConfigure->MaxDiscrVal = tree->_pTreeConfigure->MaxDiscrVal;
    _pRuleConfigure->MaxClass = tree->_pTreeConfigure->MaxClass;
    _pRuleConfigure->ClassName = (char * *) calloc(_pRuleConfigure->MaxClass + 1, sizeof(char *));
    for(int i = 0; i <= _pRuleConfigure->MaxClass; i ++) {
        _pRuleConfigure->ClassName[i] = (char *) calloc(strlen(tree->_pTreeConfigure->ClassName[i]) + 1, sizeof(char));
        strcat(_pRuleConfigure->ClassName[i], tree->_pTreeConfigure->ClassName[i]);
    }
    _pRuleConfigure->MaxAtt = tree->_pTreeConfigure->MaxAtt;
    _pRuleConfigure->AttName = (char * *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(char *));
    _pRuleConfigure->MaxAttVal = (short *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(short));
    _pRuleConfigure->SpecialStatus = (char *) malloc((_pRuleConfigure->MaxAtt + 1) * sizeof(char));
    _pRuleConfigure->AttValName = (char * * *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(char * *));
    for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
        _pRuleConfigure->AttName[i] = (char *) calloc(strlen(tree->_pTreeConfigure->AttName[i]) + 1, sizeof(char));
        strcat(_pRuleConfigure->AttName[i], tree->_pTreeConfigure->AttName[i]);
        _pRuleConfigure->SpecialStatus[i] = tree->_pTreeConfigure->SpecialStatus[i];
        _pRuleConfigure->MaxAttVal[i] = tree->_pTreeConfigure->MaxAttVal[i];
        _pRuleConfigure->AttValName[i] = (char * *) calloc(_pRuleConfigure->MaxAttVal[i] + 1, sizeof(char *));
        for(int j = 0; j <= _pRuleConfigure->MaxAttVal[i]; j ++) {
            if(j == 0) {
                if(_pRuleConfigure->SpecialStatus[i] == 2) {
                    _pRuleConfigure->AttValName[i][0] = (char *) calloc(10, sizeof(char));
                    strcat(_pRuleConfigure->AttValName[i][0], tree->_pTreeConfigure->AttValName[i][0]);
                }
                else _pRuleConfigure->AttValName[i][0] = NULL;
            }
            else {
                _pRuleConfigure->AttValName[i][j] = (char *) calloc(strlen(tree->_pTreeConfigure->AttValName[i][j]) + 1,
                    sizeof(char));
                strcat(_pRuleConfigure->AttValName[i][j], tree->_pTreeConfigure->AttValName[i][j]);
            }
        }
    }
    Description * pItem;
    if(tree->_iterate == 0) {
        pItem = (Description *) calloc(tree->_MaxItem + 1, sizeof(Description));
        for(int i = 0; i <= tree->_MaxItem; i ++) pItem[i] = tree->_pItem[i];
        _pResultedRules = createRules(_pOptions, _pRuleConfigure, tree->_MaxItem, pItem, tree->_pResultedTree);
    }
    else {
        pItem = (Description *) calloc(tree->_MaxItemTree + 1, sizeof(Description));
        for(int i = 0; i <= tree->_MaxItemTree; i ++) pItem[i] = tree->_pItemTree[i];
        _pResultedRules = createRules(_pOptions, _pRuleConfigure, tree->_MaxItemTree, pItem, tree->_pResultedTree);
    }
    free(pItem);
    if(_pResultedRules->codeErrors != NULL) {
        _pszErrorMessage = _pResultedRules->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45Rules::createRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pResultedRules->codeErrors->errorCode;
        _m.unlock();
        return NULL;
    }
    _noRuleSetConstructed ++;
    C45RuleSetInfo * info = new C45RuleSetInfo(_pResultedRules->set[0].SNRules, _pResultedRules->set[0].DefaultClassName,
        (_pResultedRules->set[0].isComposite == 0 ? false : true), _noRuleSetConstructed);
    _errorCode = 0;
    _m.unlock();
    return info;
}

C45RulesPrediction * C45Rules::consultClassifier(C45AVList * record)
{
    _m.lock();
    if(_pResultedRules == NULL) {
        _pszErrorMessage = "error: the rule set is not created yet. Unable to consult the rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedRules->codeErrors != NULL) {
        _pszErrorMessage = "error: the rule set is not created yet, because there was an error during the "
            "construction. Unable to consult the rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(record->getLength() != _pRuleConfigure->MaxAtt+1) {
        _pszErrorMessage = "error: the passed AVList must contains a number of pairs equal to the number of "
            "attributes in the rule set. Unable to consult the rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return NULL;
    }
    int i;
    char * * list = (char * *) calloc(record->getLength(), sizeof(char *));
    for(i = 0; i < record->getLength(); i ++) {
        list[i] = (char *) calloc(strlen(record->getAttribute(i)) + strlen(record->getValueByIndex(i)) + 2, sizeof(char));
        strcat(list[i], record->getAttribute(i));
        strcat(list[i], ",");
        strcat(list[i], record->getValueByIndex(i));
    }
    if(_pConsultedRules != NULL) {
        free(_pConsultedRules->className);
        if(_pConsultedRules->codeErrors != NULL) {
            free(_pConsultedRules->codeErrors->errorMessage);
            free(_pConsultedRules->codeErrors);
        }
        free(_pConsultedRules);
        _pConsultedRules = NULL;
    }
    _pConsultedRules = consultRules(_pOptions, _pRuleConfigure, _pResultedRules->set[0], list);
    if(_pConsultedRules->codeErrors != NULL) {
        _pszErrorMessage = _pConsultedRules->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45Rules::consultClassifier", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pConsultedRules->codeErrors->errorCode;
        _m.unlock();
        return NULL;
    }
    C45RulesPrediction * pred = new C45RulesPrediction(_pConsultedRules->className, _pConsultedRules->probability,
        (_pConsultedRules->isDefault == 0 ? false : true));
    _errorCode = 0;
    _m.unlock();
    return pred;
}

C45RuleSetTestInfo * C45Rules::testClassifierOnData(C45AVList * dataset, bool drop)
{
    _m.lock();
    if(_pResultedRules == NULL) {
        _pszErrorMessage = "error: the tree is not created yet. Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(_pResultedRules->codeErrors != NULL) {
        _pszErrorMessage = "error: the tree is not created yet, because there was an error during the construction. "
            "Unable to test the tree. \0";
        if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 9;
        _m.unlock();
        return NULL;
    }
    if(dataset == NULL) {
        _pszErrorMessage = "error: the passed AVList pointer is NULL. Unable to process the new data. \0";
        if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return NULL;
    }
    if((dataset->getLength() % (_pRuleConfigure->MaxAtt + 2)) != 0) {
        _pszErrorMessage = "error: the passed AVList data contains a partial record, so can't be accepted. "
            "Unable to process the new data. \0";
        if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return NULL;
    }
    if(_pItemTest != NULL) {
        for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
        free(_pItemTest);
        _pItemTest = NULL;
    }
    _MaxItemTest = -1;
    _pItemTest = (Description *) calloc(dataset->getLength() / (_pRuleConfigure->MaxAtt + 2), sizeof(Description));
    for(int i = 0; i < dataset->getLength(); i += (2 + _pRuleConfigure->MaxAtt)) {
        _MaxItemTest ++;
        _pItemTest[_MaxItemTest] = (Description) calloc(_pRuleConfigure->MaxAtt + 2, sizeof(AttValue));
        for(int l = 0; l < _pRuleConfigure->MaxAtt + 2; l ++) {
            _pErrOcc = getDataset(_pRuleConfigure, dataset->getAttribute(i+l), dataset->getValueByIndex(i+l),
                _MaxItemTest, _pItemTest);
            if(_pErrOcc != NULL) {
                _pszErrorMessage = _pErrOcc->errorMessage;
                if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                _errorCode = _pErrOcc->errorCode;
                _m.unlock();
                return NULL;
            }
        }
    }
    testRuleset(_pOptions, _pRuleConfigure, _MaxItemTest, _pItemTest, _pResultedRules, drop);
    if(_pResultedRules->codeErrors != NULL) {
        _pszErrorMessage = _pResultedRules->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45Rules::testClassifierOnData", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pResultedRules->codeErrors->errorCode;
        _m.unlock();
        return NULL;
    }
    C45RuleSetTestInfo * test = new C45RuleSetTestInfo(_pResultedRules->set[0].SNRules,
        _pResultedRules->set[0].testResults->noItems, _pResultedRules->set[0].testResults->noErrors,
        _pResultedRules->set[0].testResults->percErrors, _pResultedRules->set[0].testResults->confusionMatrix,
        (_pResultedRules->set[0].testResults->noClasses+1)*(_pResultedRules->set[0].testResults->noClasses+1));
    _errorCode = 0;
    _m.unlock();
    return test;
}

C45RuleSetInfo * C45Rules::getRuleSetInfo(void)
{
    _m.lock();
    if(_pResultedRules != NULL) {
        if(_pResultedRules->codeErrors == NULL) {
            C45RuleSetInfo * info = new C45RuleSetInfo(_pResultedRules->set[0].SNRules,
                _pResultedRules->set[0].DefaultClassName, (_pResultedRules->set[0].isComposite == 0 ? false : true), _noRuleSetConstructed);
            _errorCode = 0;
            _m.unlock();
            return info;
        }
    }
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

C45RulesTest * C45Rules::getTestResultsForRules(void)
{
    _m.lock();
    if(_pResultedRules != NULL) {
        if(_pResultedRules->codeErrors == NULL) {
            if(_pResultedRules->set[0].testResults != NULL) {
                C45RulesTest * test = new C45RulesTest(_pResultedRules);
                _errorCode = 0;
                _m.unlock();
                return test;
            }
        }
    }
    _errorCode = 9;
    _m.unlock();
    return NULL;
}

uint16 C45Rules::getVersion(void)
{
    return _noRuleSetConstructed;
}

void C45Rules::deleteTestData(void)
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

int C45Rules::configureRuleSet (C45DecisionTree * tree)
{
    _m.lock();
    if(tree == NULL) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::configureRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return -1;
    }
    if(tree->_pResultedTree == NULL) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::configureRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if(tree->_pResultedTree->nTrees == 0) {
        _pszErrorMessage = "error: there isn't any input tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::configureRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if(tree->_pResultedTree->trees[0]->isPruned != 0) {
        _pszErrorMessage = "error: there isn't any unpruned tree. Unable to create a rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::configureRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = 1;
        _m.unlock();
        return 1;
    }
    if(_pRuleConfigure != NULL) {
        if(_pResultedRules != NULL) {
            for(int i = 0; i < _pResultedRules->nSet; i ++) {
                for(int j = 1; j <= _pResultedRules->set[i].SNRules; j ++) {
                    for(int l = 1; l <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Size; l ++) {
                        if(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->NodeType == 3) {
                            for(int k = 1; k <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Forks; k ++) {
                                free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset[k]);
                            }
                            free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset);
                        }
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest);
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]);
                    }
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs);
                }
                free(_pResultedRules->set[i].SRule);
                free(_pResultedRules->set[i].DefaultClassName);
                free(_pResultedRules->set[i].SRuleIndex);
                if(_pResultedRules->set[i].testResults != NULL) {
                    free(_pResultedRules->set[i].testResults->confusionMatrix);
                    free(_pResultedRules->set[i].testResults);
                }
            }
            free(_pResultedRules->set);
            if(_pResultedRules->codeErrors != NULL) {
                free(_pResultedRules->codeErrors->errorMessage);
                free(_pResultedRules->codeErrors);
            }
            free(_pResultedRules);
            _pResultedRules = NULL;
        }
        if(_pItemTest != NULL) {
            for(int i = 0; i <= _MaxItemTest; i ++) free(_pItemTest[i]);
            free(_pItemTest);
            _pItemTest = NULL;
            _MaxItemTest = -1;
        }
        if(_pConsultedRules != NULL) {
            free(_pConsultedRules->className);
            if(_pConsultedRules->codeErrors != NULL) {
                free(_pConsultedRules->codeErrors->errorMessage);
                free(_pConsultedRules->codeErrors);
            }
            free(_pConsultedRules);
            _pConsultedRules = NULL;
        }
        for(int i = 0; i <= _pRuleConfigure->MaxClass; i ++) free(_pRuleConfigure->ClassName[i]);
        for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pRuleConfigure->MaxAttVal[i]; j ++) free(_pRuleConfigure->AttValName[i][j]);
            free(_pRuleConfigure->AttName[i]);
            free(_pRuleConfigure->AttValName[i]);
        }
        free(_pRuleConfigure->AttName);
        free(_pRuleConfigure->AttValName);
        free(_pRuleConfigure->ClassName);
        free(_pRuleConfigure->MaxAttVal);
        free(_pRuleConfigure->SpecialStatus);
    }
    if(_pRuleConfigure == NULL) _pRuleConfigure = (Configure *) malloc(sizeof(Configure));
    _pRuleConfigure->MaxDiscrVal = tree->_pTreeConfigure->MaxDiscrVal;
    _pRuleConfigure->MaxClass = tree->_pTreeConfigure->MaxClass;
    _pRuleConfigure->ClassName = (char * *) calloc(_pRuleConfigure->MaxClass + 1, sizeof(char *));
    for(int i = 0; i <= _pRuleConfigure->MaxClass; i ++) {
        _pRuleConfigure->ClassName[i] = (char *) calloc(strlen(tree->_pTreeConfigure->ClassName[i]) + 1, sizeof(char));
        strcat(_pRuleConfigure->ClassName[i], tree->_pTreeConfigure->ClassName[i]);
    }
    _pRuleConfigure->MaxAtt = tree->_pTreeConfigure->MaxAtt;
    _pRuleConfigure->AttName = (char * *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(char *));
    _pRuleConfigure->MaxAttVal = (short *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(short));
    _pRuleConfigure->SpecialStatus = (char *) malloc((_pRuleConfigure->MaxAtt + 1) * sizeof(char));
    _pRuleConfigure->AttValName = (char * * *) calloc(_pRuleConfigure->MaxAtt + 1, sizeof(char * *));
    for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
        _pRuleConfigure->AttName[i] = (char *) calloc(strlen(tree->_pTreeConfigure->AttName[i]) + 1, sizeof(char));
        strcat(_pRuleConfigure->AttName[i], tree->_pTreeConfigure->AttName[i]);
        _pRuleConfigure->SpecialStatus[i] = tree->_pTreeConfigure->SpecialStatus[i];
        _pRuleConfigure->MaxAttVal[i] = tree->_pTreeConfigure->MaxAttVal[i];
        _pRuleConfigure->AttValName[i] = (char * *) calloc(_pRuleConfigure->MaxAttVal[i] + 1, sizeof(char *));
        for(int j = 0; j <= _pRuleConfigure->MaxAttVal[i]; j ++) {
            if(j == 0) {
                if(_pRuleConfigure->SpecialStatus[i] == 2) {
                    _pRuleConfigure->AttValName[i][0] = (char *) calloc(10, sizeof(char));
                    strcat(_pRuleConfigure->AttValName[i][0], tree->_pTreeConfigure->AttValName[i][0]);
                }
                else _pRuleConfigure->AttValName[i][0] = NULL;
            }
            else {
                _pRuleConfigure->AttValName[i][j] = (char *) calloc(strlen(tree->_pTreeConfigure->AttValName[i][j]) + 1,
                    sizeof(char));
                strcat(_pRuleConfigure->AttValName[i][j], tree->_pTreeConfigure->AttValName[i][j]);
            }
        }
    }
    if(_pResultedRules->codeErrors != NULL) {
        _pszErrorMessage = _pResultedRules->codeErrors->errorMessage;
        if(pLogger) pLogger->logMsg("C45Rules::configureRuleSet", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        _errorCode = _pResultedRules->codeErrors->errorCode;
        _m.unlock();
        return _errorCode;
    }
    _errorCode = 0;
    _m.unlock();
    return _errorCode;
}

int64 C45Rules::read(Reader * pReader, uint32 ui32MaxSize)
{
    _m.lock();
    if(pReader == NULL) return -1;
    if(_pRuleConfigure == NULL) {
        _pszErrorMessage = "error: the rule set is not configured yet. Unable to read the rule set. \0";
        if(pLogger) pLogger->logMsg("C45Rules::read", Logger::L_MildError, "%s\n", _pszErrorMessage);
        _errorCode = 3;
        _m.unlock();
        return -1;
    }
    // clean previous data
    if(_pResultedRules != NULL) {
        for(int i = 0; i < _pResultedRules->nSet; i ++) {
            for(int j = 1; j <= _pResultedRules->set[i].SNRules; j ++) {
                for(int l = 1; l <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Size; l ++) {
                    if(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->NodeType == 3) {
                        for(int k = 1; k <= _pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Forks; k ++) {
                            free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset[k]);
                        }
                        free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest->Subset);
                    }
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]->CondTest);
                    free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs[l]);
                }
                free(_pResultedRules->set[i].SRule[_pResultedRules->set[i].SRuleIndex[j]].Lhs);
            }
            free(_pResultedRules->set[i].SRule);
            free(_pResultedRules->set[i].DefaultClassName);
            free(_pResultedRules->set[i].SRuleIndex);
            if(_pResultedRules->set[i].testResults != NULL) {
                free(_pResultedRules->set[i].testResults->confusionMatrix);
                free(_pResultedRules->set[i].testResults);
            }
        }
        free(_pResultedRules->set);
        _pResultedRules->set = NULL;
        if(_pResultedRules->codeErrors != NULL) {
            free(_pResultedRules->codeErrors->errorMessage);
            free(_pResultedRules->codeErrors);
            _pResultedRules->codeErrors = NULL;
        }
    }
    if(_pConsultedRules != NULL) {
        free(_pConsultedRules->className);
        if(_pConsultedRules->codeErrors != NULL) {
            free(_pConsultedRules->codeErrors->errorMessage);
            free(_pConsultedRules->codeErrors);
        }
        free(_pConsultedRules);
    }
    if(_pErrOcc != NULL) free(_pErrOcc);
    // read a rule set
    if(_pResultedRules == NULL) _pResultedRules = (processRulesResults *) calloc(1, sizeof(processRulesResults));
    _pResultedRules->set = (RuleSet *) calloc(1, sizeof(RuleSet));
    int64 totLength = 0;
    int8 retValue;

    if(ui32MaxSize < (totLength + sizeof(RuleNo))) {
        _m.unlock();
        return -2;
    }
    retValue = pReader->read16 (&(_pResultedRules->set[0].SNRules));
    if(retValue < 0) {
        _m.unlock();
        return retValue;
    }
    totLength += sizeof(RuleNo);

    if(ui32MaxSize < (totLength + sizeof(ClassNo))) {
        _m.unlock();
        return -2;
    }
    retValue = pReader->read16 (&(_pResultedRules->set[0].SDefaultClass));
    if(retValue < 0) {
        _m.unlock();
        return retValue;
    }
    totLength += sizeof(ClassNo);

    _pResultedRules->set[0].SRule = (PR *) calloc(_pResultedRules->set[0].SNRules + 1, sizeof(PR));
    _pResultedRules->set[0].SRuleIndex = (RuleNo *) calloc(_pResultedRules->set[0].SNRules + 1, sizeof(RuleNo));
    for(int i = 1; i <= _pResultedRules->set[0].SNRules; i ++) {
        _pResultedRules->set[0].SRuleIndex[i] = i;

        if(ui32MaxSize < (totLength + sizeof(short))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Size));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(short);

        _pResultedRules->set[0].SRule[i].Lhs = (Condition *) calloc(1, sizeof(Condition));
        for(short j = 1; j <= _pResultedRules->set[0].SRule[i].Size; j ++) {
            _pResultedRules->set[0].SRule[i].Lhs[j] = (Condition) calloc(1, sizeof(CondRec));
            _pResultedRules->set[0].SRule[i].Lhs[j]->CondTest = (Test) calloc(1, sizeof(TestRec));

            if(ui32MaxSize < (totLength + sizeof(short))) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->NodeType));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(short);

            if(ui32MaxSize < (totLength + sizeof(Attribute))) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Tested));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(Attribute);

            if(ui32MaxSize < (totLength + sizeof(short))) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Forks));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(short);

            if(ui32MaxSize < (totLength + sizeof(float))) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Cut));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(float);

            if(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->NodeType == BrSubset) {
                _pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Subset = (Set *) calloc(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Forks + 1, sizeof(Set));
                short bytes = (_pRuleConfigure->MaxAttVal[_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Tested]>>3) + 1;
                for(int k = 1; k <= _pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Forks; k ++) {
                    _pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Subset[k] = (Set) malloc(bytes);
                    if(ui32MaxSize < (totLength + bytes)) {
                        _m.unlock();
                        return -2;
                    }
                    retValue = pReader->readBytes(_pResultedRules->set[0].SRule[i].Lhs[j]->CondTest->Subset[k], bytes);
                    if(retValue < 0) {
                        _m.unlock();
                        return retValue;
                    }
                    totLength += bytes;
                }
            }

            if(ui32MaxSize < (totLength + sizeof(short))) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Lhs[j]->TestValue));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(short);
        }

        if(ui32MaxSize < (totLength + sizeof(ClassNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read16 (&(_pResultedRules->set[0].SRule[i].Rhs));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ClassNo);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].Error));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].Bits));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);

        if(ui32MaxSize < (totLength + sizeof(ItemNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].Used));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ItemNo);

        if(ui32MaxSize < (totLength + sizeof(ItemNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].Incorrect));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ItemNo);

        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].better));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(int);

        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].worse));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(int);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read32 (&(_pResultedRules->set[0].SRule[i].errorRate));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);
    }

    // Read discrete values for "discrete attributes with unknown values"
    uint16 length;
    for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
        if(_pRuleConfigure->SpecialStatus[i] != DISCRETE) {
            continue;
        }

        if(ui32MaxSize < (totLength + sizeof(short))) {
            _m.unlock();
            return -2;
        }
        retValue = pReader->read16 (&(_pRuleConfigure->MaxAttVal[i]));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }

        for(int j = 1; j <= _pRuleConfigure->MaxAttVal[i]; j ++) {
            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->read16(&length);
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            _pRuleConfigure->AttValName[i][j] = (char *) malloc(length + 1);
            if(ui32MaxSize < (totLength + length)) {
                _m.unlock();
                return -2;
            }
            retValue = pReader->readBytes(_pRuleConfigure->AttValName[i][j], length);
            if(retValue < 0) {
                free(_pRuleConfigure->AttValName[i][j]);
                _pRuleConfigure->AttValName[i][j] = NULL;
                _m.unlock();
                return retValue;
            }
            _pRuleConfigure->AttValName[i][j][length] = '\0';
            totLength += length;
        }
    }
    _noRuleSetConstructed ++;
    _m.unlock();
    return totLength;
}

int64 C45Rules::write(Writer * pWriter, uint32 ui32MaxSize)
{
    _m.lock();
    if(pWriter == NULL) {
        _m.unlock();
        return -1;
    }
    if(_pResultedRules == NULL) {
        _m.unlock();
        return -3;
    }
    if(_pResultedRules->nSet != 1) {
        _m.unlock();
        return -3;
    }
    int64 totLength = 0;
    int8 retValue;

    if(ui32MaxSize < (totLength + 2)) {
        _m.unlock();
        return -2;
    }
    retValue = pWriter->write16 (&(_pResultedRules->set[0].SNRules));
    if(retValue < 0) {
        _m.unlock();
        return retValue;
    }
    totLength += 2;

    if(ui32MaxSize < (totLength + 2)) {
        _m.unlock();
        return -2;
    }
    retValue = pWriter->write16(&(_pResultedRules->set[0].SDefaultClass));
    if(retValue < 0) {
        _m.unlock();
        return retValue;
    }
    totLength += 2;

    for(int i = 1; i <= _pResultedRules->set[0].SNRules; i ++) {
        if(ui32MaxSize < (totLength + 2)) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write16(&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Size));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(short);

        for(short j = 1; j <= _pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Size; j ++) {
            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write16(&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->NodeType));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += 2;

            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write16(&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Tested));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += 2;

            if(ui32MaxSize < (totLength + 2)) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write16 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Forks));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += 2;

            if(ui32MaxSize < (totLength + sizeof(float))) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Cut));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(float);

            if(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->NodeType == BrSubset) {
                short bytes = (_pRuleConfigure->MaxAttVal[_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Tested]>>3) + 1;
                for(int k = 1; k <= _pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Forks; k ++) {
                    if(ui32MaxSize < (totLength + bytes)) {
                        _m.unlock();
                        return -2;
                    }
                    retValue = pWriter->writeBytes((const char *) _pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Subset[k], bytes);
                    if(retValue < 0) {
                        _m.unlock();
                        return retValue;
                    }
                    totLength += bytes;
                }
            }

            if(ui32MaxSize < (totLength + sizeof(short))) {
                _m.unlock();
                return -2;
            }
            retValue = pWriter->write16 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->TestValue));
            if(retValue < 0) {
                _m.unlock();
                return retValue;
            }
            totLength += sizeof(short);
        }

        if(ui32MaxSize < (totLength + sizeof(ClassNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write16(&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Rhs));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ClassNo);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Error));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Bits));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);

        if(ui32MaxSize < (totLength + sizeof(ItemNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Used));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ItemNo);

        if(ui32MaxSize < (totLength + sizeof(ItemNo))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Incorrect));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(ItemNo);

        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].better));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(int);

        if(ui32MaxSize < (totLength + sizeof(int))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].worse));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(int);

        if(ui32MaxSize < (totLength + sizeof(float))) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write32 (&(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].errorRate));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += sizeof(float);
    }

    // Write attibutes values discovered for "discrete attributes with unknown values"
    uint16 length;
    for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
        if(_pRuleConfigure->SpecialStatus[i] != DISCRETE) {
            continue;
        }

        if(ui32MaxSize < (totLength + 2)) {
            _m.unlock();
            return -2;
        }
        retValue = pWriter->write16 (&(_pRuleConfigure->MaxAttVal[i]));
        if(retValue < 0) {
            _m.unlock();
            return retValue;
        }
        totLength += 2;

        for(int j = 1; j <= _pRuleConfigure->MaxAttVal[i]; j ++) {
            length = strlen(_pRuleConfigure->AttValName[i][j]);
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
            retValue = pWriter->writeBytes((const char *) _pRuleConfigure->AttValName[i][j], length);
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

int64 C45Rules::getWriteLength(void)
{
    _m.lock();
    if(_pResultedRules == NULL) {
        _m.unlock();
        return -1;
    }
    if(_pResultedRules->nSet != 1) {
        _m.unlock();
        return -1;
    }
    int64 totLength = 0;
    totLength += sizeof(RuleNo) + sizeof(ClassNo);
    for(int i = 1; i <= _pResultedRules->set[0].SNRules; i ++) {
        totLength += sizeof(short);
        for(int j = 1; j <= _pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Size; j ++) {
            totLength += 2 * sizeof(short) + sizeof(Attribute) + sizeof(float);
            if(_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->NodeType == BrSubset) {
                totLength += _pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Forks *
                    ((_pRuleConfigure->MaxAttVal[_pResultedRules->set[0].SRule[_pResultedRules->set[0].SRuleIndex[i]].Lhs[j]->CondTest->Tested]>>3) + 1);
            }
            totLength += sizeof(short);
        }
        totLength += sizeof(ClassNo) + 3 * sizeof(float) + 2 * sizeof(ItemNo) + 2 * sizeof(int);
    }
    for(int i = 0; i <= _pRuleConfigure->MaxAtt; i ++) {
        if(_pRuleConfigure->SpecialStatus[i] != DISCRETE) {
            continue;
        }
        totLength += sizeof(int);
        for(int j = 1; j <= _pRuleConfigure->MaxAttVal[i]; j ++) {
            totLength += 2 + strlen(_pRuleConfigure->AttValName[i][j]);
        }
    }
    _m.unlock();
    return totLength;
}
