/*
 * C45RuleSetInfo.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45RuleSetInfo.h"

#include <stdlib.h>
#include <string.h>

using namespace IHMC_C45;

C45RuleSetInfo::C45RuleSetInfo(int noRules, const char * defaultClass, bool isComposite, uint16 ui16Version)
{
    if(noRules > 0) {
        _noRules = noRules;
    }
    else {
        _noRules = 0;
    }
    _pszDefaultClass = (char *) calloc(strlen(defaultClass)+1, sizeof(char));
    strcpy(_pszDefaultClass, defaultClass);
    _isComposite = isComposite;
    _ui16Version = ui16Version;
}
	
void C45RuleSetInfo::copyInfo(C45RuleSetInfo * info)
{
    _noRules = info->_noRules;
    if(_pszDefaultClass != NULL) {
        free(_pszDefaultClass);
    }
    _pszDefaultClass = (char *) calloc(strlen(info->_pszDefaultClass)+1, sizeof(char));
    strcpy(_pszDefaultClass, info->_pszDefaultClass);
    _isComposite = info->_isComposite;
    _ui16Version = info->_ui16Version;
}

C45RuleSetInfo::C45RuleSetInfo()
{
    _noRules = 0;
    _pszDefaultClass = NULL;
    _isComposite = false;
    _ui16Version = 0;
}

int C45RuleSetInfo::getNoRules(void)
{
    return _noRules;
}

const char * C45RuleSetInfo::getDefaultClass(void)
{
    return _pszDefaultClass;
}

bool C45RuleSetInfo::isCompositeRuleSet(void)
{
    return _isComposite;
}
uint16 C45RuleSetInfo::getVersion(void)
{
    return _ui16Version;
}

C45RuleSetInfo::~C45RuleSetInfo()
{
    if(_pszDefaultClass != NULL) {
        free(_pszDefaultClass);
    }
}
