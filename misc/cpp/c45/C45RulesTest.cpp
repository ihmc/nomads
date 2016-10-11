/*
 * C45RulesTest.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45RulesTest.h"

#include <stdlib.h>

using namespace IHMC_C45;

C45RulesTest::C45RulesTest(processRulesResults * rules)
{
    _noRules = rules->set[0].SNRules;
    _pTest = (_testRules *) calloc(_noRules, sizeof(_testRules));
    for(int i = 0; i < _noRules; i ++) {
        _pTest[i]._advantage = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].better -
                                                        rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].worse;
        _pTest[i]._errorRate = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].errorRate;
        _pTest[i]._estimate = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].Error;
        _pTest[i]._incorrect = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].Incorrect;
        _pTest[i]._ruleSize = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].Size;
        _pTest[i]._used = rules->set[0].SRule[rules->set[0].SRuleIndex[i+1]].Used;
    }
}

void C45RulesTest::copyTest(C45RulesTest * rulesTest)
{
    _noRules = rulesTest->_noRules;
    if(_pTest != NULL) free(_pTest);
    _pTest = (_testRules *) calloc(_noRules, sizeof(_testRules));
    for(int i = 0; i < _noRules; i ++) {
        _pTest[i]._advantage = rulesTest->_pTest[i]._advantage;
        _pTest[i]._errorRate = rulesTest->_pTest[i]._errorRate;
        _pTest[i]._estimate = rulesTest->_pTest[i]._estimate;
        _pTest[i]._incorrect = rulesTest->_pTest[i]._incorrect;
        _pTest[i]._ruleSize = rulesTest->_pTest[i]._ruleSize;
        _pTest[i]._used = rulesTest->_pTest[i]._used;
    }
}

C45RulesTest::~C45RulesTest()
{
    if(_pTest != NULL) {
        free(_pTest);
    }
}
