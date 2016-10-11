/*
 * C45TreePrediction.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "C45TreePrediction.h"

#include <string.h>
#include <stdlib.h>
#include<stdio.h>//test

using namespace IHMC_C45;

C45TreePrediction::C45TreePrediction()
{
    _pPrediction = NULL;
    _noPred = -1;
}

C45TreePrediction::C45TreePrediction (consultTreeResults *results)
{
    _noPred = results[0].nClasses;
    _pPrediction = (_pred *) calloc(_noPred, sizeof(_pred));
    for(int i = 0; i < results[0].nClasses; i ++) {
        printf("\n\n\nclassName[%d] = <%s>\n\n", i, results[i].className);
        _pPrediction[i]._pszClassName = (char *) calloc(strlen(results[i].className)+1, sizeof(char));
        strcpy(_pPrediction[i]._pszClassName, results[i].className);
        _pPrediction[i]._guessProb = results[i].guessProb;
        _pPrediction[i]._lowProb = results[i].lowProb;
        _pPrediction[i]._upperProb = results[i].upperProb;
    }
}

const char * C45TreePrediction::getPrediction()
{
    if(_pPrediction == NULL) {
        return NULL;
    }
    char * pred = _pPrediction[0]._pszClassName;
    float max = _pPrediction[0]._guessProb;
    for(int i = 1; i < _noPred; i ++) {
        if(_pPrediction[i]._guessProb > max) {
            pred = _pPrediction[i]._pszClassName;
            max = _pPrediction[i]._guessProb;
        }
    }
    return pred;
}

float C45TreePrediction::getProbability()
{
    if (_pPrediction == NULL) {
        return -1;
    }
    float max = _pPrediction[0]._guessProb;
    for (int i = 1; i < _noPred; i ++) {
        if(_pPrediction[i]._guessProb > max) {
            max = _pPrediction[i]._guessProb;
        }
    }
    return max;
}

const char * C45TreePrediction::getClassName (int noPredictions)
{
    if (_pPrediction == NULL) {
        return NULL;
    }
    if ((noPredictions >= _noPred) || (noPredictions < 0)) {
        return NULL;
    }
    return _pPrediction[noPredictions]._pszClassName;
}

float C45TreePrediction::getGuessProbability (int noPredictions)
{
    if (_pPrediction == NULL) {
        return 0.0f;
    }
    if ((noPredictions >= _noPred) || (noPredictions < 0)) {
        return 0.0f;
    }
    return _pPrediction[noPredictions]._guessProb;
}

float C45TreePrediction::getLowProbability (int noPredictions)
{
    if (_pPrediction == NULL) {
        return 0.0f;
    }
    if ((noPredictions >= _noPred) || (noPredictions < 0)) {
        return 0.0f;
    }
    return _pPrediction[noPredictions]._lowProb;
}

float C45TreePrediction::getUpperProbability(int noPredictions)
{
    if (_pPrediction == NULL) {
        return 0.0f;
    }
    if ((noPredictions >= _noPred) || (noPredictions < 0)) {
        return 0.0f;
    }
    return _pPrediction[noPredictions]._upperProb;
}

void C45TreePrediction::copyPrediction (C45TreePrediction * prediction)
{
    _noPred = prediction->_noPred;
    if (_pPrediction != NULL) {
        free(_pPrediction);
        _pPrediction = NULL;
    }
    _pPrediction = (_pred *) calloc(_noPred, sizeof(_pred));
    for (int i = 0; i < _noPred; i ++) {
        _pPrediction[i]._pszClassName = (char *) calloc(strlen(prediction->_pPrediction[i]._pszClassName)+1, sizeof(char));
        strcpy(_pPrediction[i]._pszClassName, prediction->_pPrediction[i]._pszClassName);
        _pPrediction[i]._guessProb = prediction->_pPrediction[i]._guessProb;
        _pPrediction[i]._lowProb = prediction->_pPrediction[i]._lowProb;
        _pPrediction[i]._upperProb = prediction->_pPrediction[i]._upperProb;
    }
}

C45TreePrediction::~C45TreePrediction()
{
    if (_pPrediction == NULL) {
        return;
    }
    free(_pPrediction);
}
