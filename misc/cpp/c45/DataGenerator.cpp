/*
 * DataGenerator.cpp
 *
 * This file is part of the IHMC Misc Library
 * Copyright (c) IHMC. All Rights Reserved.
 * 
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
 */

#include "DataGenerator.h"

#include "C45DecisionTree.h"

#include "types.h"
#include "c4.5.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

#include <string.h>
#include <stdlib.h>

using namespace IHMC_C45;
using namespace NOMADSUtil;

const NOMADSUtil::String DataGenerator::_XML_HEADER = "<?xml version=\"1.0\"?>";
const NOMADSUtil::String DataGenerator::_XML_DATA_ITEM = "Metadata";
const NOMADSUtil::String DataGenerator::_XML_ATTRIBUTE = "Field";
const NOMADSUtil::String DataGenerator::_XML_ATTRIBUTE_NAME = "FieldName";
const NOMADSUtil::String DataGenerator::_XML_ATTRIBUTE_VALUE = "FieldValue";
const NOMADSUtil::String DataGenerator::_XML_CLASS_NAME = "Usage";

DataGenerator::DataGenerator()
{
    _pConfigure = NULL;
    _pErrOcc = NULL;
    _pszErrorMessage = NULL;
    _pRules = NULL;
    _pAttrRange = NULL;
    _pszDefaultClass = NULL;
    _pItem = NULL;
    srand ((unsigned int)getTimeInMilliseconds());
}

DataGenerator::~DataGenerator()
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
    if(_pErrOcc != NULL) free(_pErrOcc);
    if(_pszErrorMessage != NULL) free(_pszErrorMessage);
    if(_pRules != NULL) {
        for(int i = 0; i <= _MaxRules; i ++) {
            if(_pRules[i]._MaxAttr > -1) {
                for(int j = 0; j < _pRules[i]._MaxAttr; j ++) {
                    if(_pRules[i]._pConditionStatus[j] != NULL) {
                        free(_pRules[i]._pConditionStatus[j]);
                    }
                    if(_pRules[i]._pConditionValue[j] != NULL) {
                        free(_pRules[i]._pConditionValue[j]);
                    }
                    if(_pRules[i]._pConditionRange != NULL) {
                        if(_pRules[i]._pConditionRange[j] != NULL) {
                            free(_pRules[i]._pConditionRange[j]);
                        }
                    }
                }
            }
            if(_pRules[i]._pAttrNo != NULL) {
                free(_pRules[i]._pAttrNo);
            }
            if(_pRules[i]._pMaxCond != NULL) {
                free(_pRules[i]._pMaxCond);
            }
            free(_pRules[i]._pConditionStatus);
            free(_pRules[i]._pConditionValue);
            if(_pRules[i]._pConditionRange != NULL) {
                free(_pRules[i]._pConditionRange);
            }
        }
        free(_pRules);
    }
    if(_pAttrRange != NULL) {
        free(_pAttrRange);
    }
    if(_pszDefaultClass != NULL) {
        free(_pszDefaultClass);
    }
    if(_pItem != NULL) {
        for(int i = 0; i <= _MaxItem; i ++) {
            free(_pItem[i]);
        }
        free(_pItem);
    }
}

int DataGenerator::initializeAttributes(C45AVList * treeAttributes)
{
    if(treeAttributes == NULL) {
        if(_pszErrorMessage != NULL) {
            free(_pszErrorMessage);
        }
        _pszErrorMessage = (char *) calloc(strlen("error: the passed C45AVList pointer is NULL. Unable to initialize "
                                                  "the data generator. \0"), sizeof(char));
        strcat(_pszErrorMessage, "error: the passed C45AVList pointer is NULL. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                                    "%s\n", _pszErrorMessage);
        return 1;
    }
    int countClass = 0;
    for(int i = 0; i < treeAttributes->getLength(); i ++) {
            if(strcmp(treeAttributes->getAttribute(i), treeAttributes->_CLASS) == 0) countClass ++;
    }
    if(countClass != 1) {
            if(_pszErrorMessage != NULL) free(_pszErrorMessage);
            _pszErrorMessage = (char *) calloc(strlen("error: there must be 1 row begins with the constant CLASS that "
        "specifies the names of the classes. Unable to initialize the data generator. \0"), sizeof(char));
            strcat(_pszErrorMessage, "error: there must be 1 row begins with the constant CLASS that specifies the names "
        "of the classes. Unable to initialize the data generator. \0");
    if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
        "%s\n", _pszErrorMessage);
            return 2;
    }
    if(_pConfigure != NULL) {
            for(int i = 0; i <= _pConfigure->MaxClass; i ++) free(_pConfigure->ClassName[i]);
            for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
                    for(int j = 0; j <= _pConfigure->MaxAttVal[i]; j ++) free(_pConfigure->AttValName[i][j]);
                    free(_pConfigure->AttName[i]);
                    free(_pConfigure->AttValName[i]);
            }
            free(_pConfigure->AttName);
            free(_pConfigure->AttValName);
            free(_pConfigure->ClassName);
            free(_pConfigure->MaxAttVal);
            free(_pConfigure->SpecialStatus);
    }
    if(_pConfigure == NULL) _pConfigure = (Configure *) malloc(sizeof(Configure));
    _pConfigure->MaxDiscrVal = 2;
    _pConfigure->MaxAtt = -1;
    _pConfigure->MaxAttVal = (short *) calloc((treeAttributes->getLength()-1), sizeof(short));
    _pConfigure->AttName = (char * *) calloc((treeAttributes->getLength()-1), sizeof(char *));
    _pConfigure->SpecialStatus = (char *) malloc((treeAttributes->getLength()-1) * sizeof(char));
    _pConfigure->AttValName = (char * * *) calloc((treeAttributes->getLength()-1), sizeof(char * *));
    for(int i = 0; i < treeAttributes->getLength(); i ++) {
            if((treeAttributes->getAttribute(i) == NULL)||(treeAttributes->getValueByIndex(i) == NULL)) {
                    if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                    _pszErrorMessage = (char *) calloc(strlen("error: one of the attributes or the values is NULL. Unable to "
            "initialize the data generator. \0"), sizeof(char));;
                    strcat(_pszErrorMessage, "error: one of the attributes or the values is NULL. Unable to initialize the "
            "data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
                    return 1;
            }
            _pErrOcc = getNames(_pConfigure, treeAttributes->getAttribute(i), treeAttributes->getValueByIndex(i));
            if(_pErrOcc != NULL) {
                    _pszErrorMessage = _pErrOcc->errorMessage;
                    return _pErrOcc->errorCode;
            }
    }
    // test prints
    /*printf("MaxClass = %d\n", _pConfigure->MaxClass);
    for(int i = 0; i <= _pConfigure->MaxClass; i ++) printf("ClassName[%d] = %s\n", i, _pConfigure->ClassName[i]);
    printf("MaxDiscrVal = %d\n", _pConfigure->MaxDiscrVal);
    printf("MaxAtt = %d\n", _pConfigure->MaxAtt);
    for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
            printf("AttName[%d] = %s\n", i, _pConfigure->AttName[i]);
            printf("MaxAttVal[%d] = %d\n", i, _pConfigure->MaxAttVal[i]);
            printf("SpecialStatus[%d] = %d\n", i, _pConfigure->SpecialStatus[i]);
            if(_pConfigure->SpecialStatus[i] != 2) {
                    for(int l = 0; l <= _pConfigure->MaxAttVal[i]; l ++)
                            printf("AttValName[%d] = %s\n", l, _pConfigure->AttValName[i][l]);
            }
    }*/
    return 0;
}

int DataGenerator::initializeAttributes(const char * pszFileName)
{
    if(pszFileName == NULL) {
        if(_pszErrorMessage != NULL) free(_pszErrorMessage);
        _pszErrorMessage = (char *) calloc(strlen("error: the passed file name is a NULL pointer. Unable to "
            "initialize the data generator. \0"), sizeof(char));
        strcat(_pszErrorMessage, "error: the passed file name is a NULL pointer. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        return 1;
    }
    FILE * pNFile;
    if(!(pNFile = fopen(pszFileName, "r"))) {
        if(_pszErrorMessage != NULL) free(_pszErrorMessage);
        _pszErrorMessage = (char *) calloc(strlen("error: cannot open the file ''. Unable to initialize the "
            "data generator. \0") + strlen(pszFileName), sizeof(char));
        strcat(_pszErrorMessage, "error: cannot open the file '");
        strcat(_pszErrorMessage, pszFileName);
        strcat(_pszErrorMessage, "'. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        return 1;
    }
    char Buffer[400];
    char delimiter;
    short v;
    int AttCeiling = 2;       // predict 2 different attributes
    int ClassCeiling = 2;     // the default number of classes is 2
    int ValCeiling;           // predict number of value for each attribute
    if(_pConfigure != NULL) {
        for(int i = 0; i <= _pConfigure->MaxClass; i ++) free(_pConfigure->ClassName[i]);
        for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
            for(int j = 0; j <= _pConfigure->MaxAttVal[i]; j ++) free(_pConfigure->AttValName[i][j]);
            free(_pConfigure->AttName[i]);
            free(_pConfigure->AttValName[i]);
        }
        free(_pConfigure->AttName);
        free(_pConfigure->AttValName);
        free(_pConfigure->ClassName);
        free(_pConfigure->MaxAttVal);
        free(_pConfigure->SpecialStatus);
    }
    if(_pConfigure == NULL) _pConfigure = (Configure *) malloc(sizeof(Configure));
    _pConfigure->MaxDiscrVal = 2;
    _pConfigure->MaxAtt = -1;
    _pConfigure->MaxAttVal = (short *) calloc(AttCeiling, sizeof(short));
    _pConfigure->AttName = (char * *) calloc(AttCeiling, sizeof(char *));
    _pConfigure->SpecialStatus = (char *) calloc(AttCeiling, sizeof(char));
    _pConfigure->AttValName = (char * * *) calloc(AttCeiling, sizeof(char * *));
    _pConfigure->ClassName = (char * *) calloc(ClassCeiling, sizeof(char *));
    _pConfigure->MaxClass = -1;
    do {
        readFromFile(pNFile, Buffer, &delimiter);
        if(++(_pConfigure->MaxClass) >= ClassCeiling) {
            ClassCeiling += 1;
            _pConfigure->ClassName = (char * *) realloc(_pConfigure->ClassName, ClassCeiling*sizeof(char *));
        }
        _pConfigure->ClassName[_pConfigure->MaxClass] = (char *) calloc(strlen(Buffer) + 1, sizeof(char));
        strcpy(_pConfigure->ClassName[_pConfigure->MaxClass], Buffer);
    } while(delimiter == ',');
    while(readFromFile(pNFile, Buffer, &delimiter)) {
        if(delimiter != ':') {
            if(_pszErrorMessage != NULL) free(_pszErrorMessage);
            _pszErrorMessage = (char *) calloc(strlen("error: colon expected after attribute name ")
            + strlen(Buffer) + 3, sizeof(char));
            strcat(_pszErrorMessage, "error: colon expected after attribute name ");
            strcat(_pszErrorMessage, Buffer);
            strcat(_pszErrorMessage, ". ");
            if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
            return 3;
        }
        if(++(_pConfigure->MaxAtt) >= AttCeiling) {
            AttCeiling ++;
            _pConfigure->AttName = (char * *) realloc(_pConfigure->AttName, AttCeiling * sizeof(char *));
            _pConfigure->MaxAttVal = (short *) realloc(_pConfigure->MaxAttVal, AttCeiling * sizeof(short));
            _pConfigure->AttValName = (char * * *) realloc(_pConfigure->AttValName, AttCeiling * sizeof(char * *));
            _pConfigure->SpecialStatus = (char *) realloc(_pConfigure->SpecialStatus, AttCeiling * sizeof(char));
        }
        _pConfigure->AttName[_pConfigure->MaxAtt] = (char *) calloc(strlen(Buffer) + 1, sizeof(char));
        strcpy(_pConfigure->AttName[_pConfigure->MaxAtt], Buffer);
        _pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 0;
        _pConfigure->MaxAttVal[_pConfigure->MaxAtt] = 0;
        ValCeiling = 1;
        _pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) calloc(ValCeiling, sizeof(char *));
        do {
            if(!(readFromFile(pNFile, Buffer, &delimiter))) {
                if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                _pszErrorMessage = (char *) calloc(strlen("error: unable to read values of attribute ")
                + strlen(Buffer) + 3, sizeof(char));
                strcat(_pszErrorMessage, "error: unable to read values of attribute ");
                strcat(_pszErrorMessage, Buffer);
                strcat(_pszErrorMessage, ". ");
                if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                return 4;
            }
            if(++(_pConfigure->MaxAttVal[_pConfigure->MaxAtt]) >= ValCeiling) {
                ValCeiling ++;
                _pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) realloc(_pConfigure->AttValName[_pConfigure->MaxAtt],
                    ValCeiling * sizeof(char *));
            }
            if(strcmp(Buffer, "continuous") && memcmp(Buffer, "discrete", 8) && strcmp(Buffer, "ignore")) {
                _pConfigure->AttValName[_pConfigure->MaxAtt][_pConfigure->MaxAttVal[_pConfigure->MaxAtt]] =
                    (char *) calloc(strlen(Buffer) + 1, sizeof(char));
                strcpy(_pConfigure->AttValName[_pConfigure->MaxAtt][_pConfigure->MaxAttVal[_pConfigure->MaxAtt]], Buffer);
            }
        } while(delimiter == ',');
        if(_pConfigure->MaxAttVal[_pConfigure->MaxAtt] == 1) {
            if(!strcmp(Buffer, "continuous")) {}  // no special status in this case
            else if(!memcmp(Buffer, "discrete", 8)) {
                _pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 2;
                v = atoi(&Buffer[8]);
                if(v < 2) {
                    if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                    _pszErrorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")
                        + strlen(_pConfigure->AttName[_pConfigure->MaxAtt]) + 3, sizeof(char));
                    strcat(_pszErrorMessage, "error: illegal number of discrete values (< 2) for attribute ");
                    strcat(_pszErrorMessage, _pConfigure->AttName[_pConfigure->MaxAtt]);
                    strcat(_pszErrorMessage, ". ");
                    if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
                    return 5;
                }
                _pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) realloc(_pConfigure->AttValName[_pConfigure->MaxAtt],
                    (v + 2) * sizeof(char *));
                _pConfigure->AttValName[_pConfigure->MaxAtt][0] = (char *) v;
                if(v > _pConfigure->MaxDiscrVal) _pConfigure->MaxDiscrVal = v;
            }
            else {
                if(!strcmp(Buffer, "ignore")) _pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 1;
                else {
                    if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                    _pszErrorMessage = (char *) calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")
                        + strlen(_pConfigure->AttName[_pConfigure->MaxAtt]) + 3, sizeof(char));
                    strcat(_pszErrorMessage, "error: illegal number of discrete values (< 2) for attribute ");
                    strcat(_pszErrorMessage, _pConfigure->AttName[_pConfigure->MaxAtt]);
                    strcat(_pszErrorMessage, ". ");
                    if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
                    return 5;
                }
            }
            _pConfigure->MaxAttVal[_pConfigure->MaxAtt] = 0;
        }
        else if(_pConfigure->MaxAttVal[_pConfigure->MaxAtt] > _pConfigure->MaxDiscrVal)
            _pConfigure->MaxDiscrVal = _pConfigure->MaxAttVal[_pConfigure->MaxAtt];
    }
    fclose(pNFile);
    // test prints
    /*printf("dentro initialize attributes da file\n");
    printf("MaxClass = %d\n", _pConfigure->MaxClass);
    for(int i = 0; i <= _pConfigure->MaxClass; i ++) printf("ClassName[%d] = %s\n", i, _pConfigure->ClassName[i]);
    printf("MaxDiscrVal = %d\n", _pConfigure->MaxDiscrVal);
    printf("MaxAtt = %d\n", _pConfigure->MaxAtt);
    for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
        printf("AttName[%d] = %s\n", i, _pConfigure->AttName[i]);
        printf("MaxAttVal[%d] = %d\n", i, _pConfigure->MaxAttVal[i]);
        printf("SpecialStatus[%d] = %d\n", i, _pConfigure->SpecialStatus[i]);
        if(_pConfigure->SpecialStatus[i] != 2) {
            for(int l = 0; l <= _pConfigure->MaxAttVal[i]; l ++)
                printf("AttValName[%d] = %s\n", l, _pConfigure->AttValName[i][l]);
        }
    }*/
    return 0;
}

int DataGenerator::initializeAttributes(const char * pszFileName, C45DecisionTree * decisionTree)
{
	if(pszFileName == NULL) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the passed file name is a NULL pointer. Unable to "
            "initialize the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: the passed file name is a NULL pointer. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	if(decisionTree == NULL) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the passed tree is a NULL pointer. Unable to initialize "
            "the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: the passed tree is a NULL pointer. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	FILE * pNFile;
	if(!(pNFile = fopen(pszFileName, "r"))) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: cannot open the file ''. Unable to initialize the "
            "data generator. \0") + strlen(pszFileName), sizeof(char));
		strcat(_pszErrorMessage, "error: cannot open the file '");
		strcat(_pszErrorMessage, pszFileName);
		strcat(_pszErrorMessage, "'. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	char Buffer[400];
	char delimiter;
    short v;
    int AttCeiling = 2;       // predict 2 different attributes
    int ClassCeiling = 2;     // the default number of classes is 2
    int ValCeiling;           // predict number of value for each attribute
    if(_pConfigure != NULL) {
		for(int i = 0; i <= _pConfigure->MaxClass; i ++) free(_pConfigure->ClassName[i]);
		for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
			for(int j = 0; j <= _pConfigure->MaxAttVal[i]; j ++) free(_pConfigure->AttValName[i][j]);
			free(_pConfigure->AttName[i]);
			free(_pConfigure->AttValName[i]);
		}
		free(_pConfigure->AttName);
		free(_pConfigure->AttValName);
		free(_pConfigure->ClassName);
		free(_pConfigure->MaxAttVal);
		free(_pConfigure->SpecialStatus);
	}
	if(_pConfigure == NULL) _pConfigure = (Configure *) malloc(sizeof(Configure));
	_pConfigure->MaxDiscrVal = 2;
	_pConfigure->MaxAtt = -1;
	_pConfigure->MaxAttVal = (short *) calloc(AttCeiling, sizeof(short));
	_pConfigure->AttName = (char * *) calloc(AttCeiling, sizeof(char *));
	_pConfigure->SpecialStatus = (char *) calloc(AttCeiling, sizeof(char));
	_pConfigure->AttValName = (char * * *) calloc(AttCeiling, sizeof(char * *));
	_pConfigure->ClassName = (char * *) calloc(ClassCeiling, sizeof(char *));
	_pConfigure->MaxClass = -1;
	do {
		readFromFile(pNFile, Buffer, &delimiter);
		if(++(_pConfigure->MaxClass) >= ClassCeiling) {
	    	ClassCeiling += 1;
	    	_pConfigure->ClassName = (char * *) realloc(_pConfigure->ClassName, ClassCeiling*sizeof(char *));
		}
		_pConfigure->ClassName[_pConfigure->MaxClass] = (char *) calloc(strlen(Buffer) + 1, sizeof(char));
		strcpy(_pConfigure->ClassName[_pConfigure->MaxClass], Buffer);
    } while(delimiter == ',');
    while(readFromFile(pNFile, Buffer, &delimiter)) {
		if(delimiter != ':') {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error: colon expected after attribute name ")
            + strlen(Buffer) + 3, sizeof(char));
			strcat(_pszErrorMessage, "error: colon expected after attribute name ");
			strcat(_pszErrorMessage, Buffer);
			strcat(_pszErrorMessage, ". ");
            if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 3;
		}
		if(++(_pConfigure->MaxAtt) >= AttCeiling) {
	    	AttCeiling ++;
	    	_pConfigure->AttName = (char * *) realloc(_pConfigure->AttName, AttCeiling * sizeof(char *));
	    	_pConfigure->MaxAttVal = (short *) realloc(_pConfigure->MaxAttVal, AttCeiling * sizeof(short));
	    	_pConfigure->AttValName = (char * * *) realloc(_pConfigure->AttValName, AttCeiling * sizeof(char * *));
	    	_pConfigure->SpecialStatus = (char *) realloc(_pConfigure->SpecialStatus, AttCeiling * sizeof(char));
		}
		_pConfigure->AttName[_pConfigure->MaxAtt] = (char *) calloc(strlen(Buffer) + 1, sizeof(char));
		strcpy(_pConfigure->AttName[_pConfigure->MaxAtt], Buffer);
		_pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 0;
		_pConfigure->MaxAttVal[_pConfigure->MaxAtt] = 0;
		ValCeiling = 1;
		_pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) calloc(ValCeiling, sizeof(char *));
		do {
	    	if(!(readFromFile(pNFile, Buffer, &delimiter))) {
	    		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
				_pszErrorMessage = (char *) calloc(strlen("error: unable to read values of attribute ")
                + strlen(Buffer) + 3, sizeof(char));
				strcat(_pszErrorMessage, "error: unable to read values of attribute ");
				strcat(_pszErrorMessage, Buffer);
				strcat(_pszErrorMessage, ". ");
                if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
				return 4;
	    	}
	    	if(++(_pConfigure->MaxAttVal[_pConfigure->MaxAtt]) >= ValCeiling) {
				ValCeiling ++;
				_pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) realloc(_pConfigure->AttValName[_pConfigure->MaxAtt],
                    ValCeiling * sizeof(char *));
	    	}
	    	if(strcmp(Buffer, "continuous") && memcmp(Buffer, "discrete", 8) && strcmp(Buffer, "ignore")) {
	    		_pConfigure->AttValName[_pConfigure->MaxAtt][_pConfigure->MaxAttVal[_pConfigure->MaxAtt]] =
                    (char *) calloc(strlen(Buffer) + 1, sizeof(char));
	    		strcpy(_pConfigure->AttValName[_pConfigure->MaxAtt][_pConfigure->MaxAttVal[_pConfigure->MaxAtt]], Buffer);
	    	}
		} while(delimiter == ',');
		if(_pConfigure->MaxAttVal[_pConfigure->MaxAtt] == 1) {
	    	if(!strcmp(Buffer, "continuous")) {}  // no special status in this case
	    	else if(!memcmp(Buffer, "discrete", 8)) {
				_pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 2;
				v = atoi(&Buffer[8]);
				if(v < 2) {
		    		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
					_pszErrorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")
                        + strlen(_pConfigure->AttName[_pConfigure->MaxAtt]) + 3, sizeof(char));
					strcat(_pszErrorMessage, "error: illegal number of discrete values (< 2) for attribute ");
					strcat(_pszErrorMessage, _pConfigure->AttName[_pConfigure->MaxAtt]);
					strcat(_pszErrorMessage, ". ");
                    if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
					return 5;
				}
				_pConfigure->AttValName[_pConfigure->MaxAtt] = (char * *) realloc(_pConfigure->AttValName[_pConfigure->MaxAtt],
                    (v + 2) * sizeof(char *));
				_pConfigure->AttValName[_pConfigure->MaxAtt][0] = (char *) v;
				if(v > _pConfigure->MaxDiscrVal) _pConfigure->MaxDiscrVal = v;
	    	}
	    	else {
	    		if(!strcmp(Buffer, "ignore")) _pConfigure->SpecialStatus[_pConfigure->MaxAtt] = 1;
	    		else {
	   				if(_pszErrorMessage != NULL) free(_pszErrorMessage);
					_pszErrorMessage = (char *) calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")
                        + strlen(_pConfigure->AttName[_pConfigure->MaxAtt]) + 3, sizeof(char));
					strcat(_pszErrorMessage, "error: illegal number of discrete values (< 2) for attribute ");
					strcat(_pszErrorMessage, _pConfigure->AttName[_pConfigure->MaxAtt]);
					strcat(_pszErrorMessage, ". ");
                    if(pLogger) pLogger->logMsg("DataGenerator::initializeAttributes", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
					return 5;
				}
	    	}
	    	_pConfigure->MaxAttVal[_pConfigure->MaxAtt] = 0;
		}
		else if(_pConfigure->MaxAttVal[_pConfigure->MaxAtt] > _pConfigure->MaxDiscrVal)
            _pConfigure->MaxDiscrVal = _pConfigure->MaxAttVal[_pConfigure->MaxAtt];
    }
    fclose(pNFile);
    // Copy configure into the tree
    if(decisionTree->_pTreeConfigure != NULL) {
		if(decisionTree->_pConsultedTree != NULL) {
			free(decisionTree->_pConsultedTree->className);
			if(decisionTree->_pConsultedTree->codeErrors != NULL) {
				free(decisionTree->_pConsultedTree->codeErrors->errorMessage);
				free(decisionTree->_pConsultedTree->codeErrors);
			}
			free(decisionTree->_pConsultedTree);
			decisionTree->_pConsultedTree = NULL;
		}
		if(decisionTree->_pResultedTree != NULL) {
			if(decisionTree->_pResultedTree->codeErrors != NULL) {
				free(decisionTree->_pResultedTree->codeErrors->errorMessage);
				free(decisionTree->_pResultedTree->codeErrors);
			}
			if(decisionTree->_pResultedTree->nTrees > 0) {
				for(int i = 0; i < decisionTree->_pResultedTree->nTrees; i ++) {
					if(decisionTree->_pResultedTree->trees[i]->testResults != NULL) {
						free(decisionTree->_pResultedTree->trees[i]->testResults->confusionMatrix);
						free(decisionTree->_pResultedTree->trees[i]->testResults);
					}
					decisionTree->freeTree(decisionTree->_pResultedTree->trees[i]->tree);
					free(decisionTree->_pResultedTree->trees[i]);
				}
				free(decisionTree->_pResultedTree->trees);
			}
			free(decisionTree->_pResultedTree);
			decisionTree->_pResultedTree = NULL;
		}
		if(decisionTree->_pItem != NULL) {
			if(decisionTree->_MaxItemIncrement > 0) {
				for(int i = 0; i <= decisionTree->_MaxItemIncrement; i ++) free(decisionTree->_pItem[i]);
			}
			else {
				for(int i = 0; i <= decisionTree->_MaxItem; i ++) free(decisionTree->_pItem[i]);
			}
			free(decisionTree->_pItem);
			decisionTree->_pItem = NULL;
		}
		if(decisionTree->_pItemTest != NULL) {
			for(int i = 0; i <= decisionTree->_MaxItemTest; i ++) free(decisionTree->_pItemTest[i]);
			free(decisionTree->_pItemTest);
			decisionTree->_pItemTest = NULL;
		}
		if(decisionTree->_pItemTree != NULL) {
			free(decisionTree->_pItemTree);
			decisionTree->_pItemTree = NULL;
		}
		decisionTree->_dataFlag = false;
		decisionTree->_iterate = 0;
		decisionTree->_treeCounter = 0;
		decisionTree->_errorCode = -1;
		for(int i = 0; i <= decisionTree->_pTreeConfigure->MaxClass; i ++) free(decisionTree->_pTreeConfigure->ClassName[i]);
		for(int i = 0; i <= decisionTree->_pTreeConfigure->MaxAtt; i ++) {
			for(int j = 0; j <= decisionTree->_pTreeConfigure->MaxAttVal[i]; j ++)
                free(decisionTree->_pTreeConfigure->AttValName[i][j]);
			free(decisionTree->_pTreeConfigure->AttName[i]);
			free(decisionTree->_pTreeConfigure->AttValName[i]);
		}
		free(decisionTree->_pTreeConfigure->AttName);
		free(decisionTree->_pTreeConfigure->AttValName);
		free(decisionTree->_pTreeConfigure->ClassName);
		free(decisionTree->_pTreeConfigure->MaxAttVal);
		free(decisionTree->_pTreeConfigure->SpecialStatus);
	}
	if(decisionTree->_pTreeConfigure == NULL) decisionTree->_pTreeConfigure = (Configure *) malloc(sizeof(Configure));
	decisionTree->_pTreeConfigure->MaxDiscrVal = _pConfigure->MaxDiscrVal;
	decisionTree->_pTreeConfigure->MaxClass = _pConfigure->MaxClass;
	decisionTree->_pTreeConfigure->ClassName = (char * *) calloc(decisionTree->_pTreeConfigure->MaxClass + 1, sizeof(char *));
	for(int i = 0; i <=decisionTree-> _pTreeConfigure->MaxClass; i ++) {
		decisionTree->_pTreeConfigure->ClassName[i] = (char *) calloc(strlen(_pConfigure->ClassName[i]) + 1, sizeof(char));
		strcat(decisionTree->_pTreeConfigure->ClassName[i], _pConfigure->ClassName[i]);
	}
	decisionTree->_pTreeConfigure->MaxAtt = _pConfigure->MaxAtt;
	decisionTree->_pTreeConfigure->AttName = (char * *) calloc(decisionTree->_pTreeConfigure->MaxAtt + 1, sizeof(char *));
	decisionTree->_pTreeConfigure->MaxAttVal = (short *) calloc(decisionTree->_pTreeConfigure->MaxAtt + 1, sizeof(short));
	decisionTree->_pTreeConfigure->SpecialStatus = (char *) malloc((decisionTree->_pTreeConfigure->MaxAtt + 1) * sizeof(char));
	decisionTree->_pTreeConfigure->AttValName = (char * * *) calloc(decisionTree->_pTreeConfigure->MaxAtt + 1, sizeof(char * *));
	for(int i = 0; i <= decisionTree->_pTreeConfigure->MaxAtt; i ++) {
		decisionTree->_pTreeConfigure->AttName[i] = (char *) calloc(strlen(_pConfigure->AttName[i]) + 1, sizeof(char));
		strcat(decisionTree->_pTreeConfigure->AttName[i], _pConfigure->AttName[i]);
		decisionTree->_pTreeConfigure->SpecialStatus[i] = _pConfigure->SpecialStatus[i];
		decisionTree->_pTreeConfigure->MaxAttVal[i] = _pConfigure->MaxAttVal[i];
		decisionTree->_pTreeConfigure->AttValName[i] = (char * *) calloc(decisionTree->_pTreeConfigure->MaxAttVal[i] + 1,
            sizeof(char *));
		for(int j = 0; j <= decisionTree->_pTreeConfigure->MaxAttVal[i]; j ++) {
			if(j == 0) decisionTree->_pTreeConfigure->AttValName[i][j] = NULL;
			else {
				decisionTree->_pTreeConfigure->AttValName[i][j] = (char *) calloc(strlen(_pConfigure->AttValName[i][j]) + 1,
                    sizeof(char));
				strcat(decisionTree->_pTreeConfigure->AttValName[i][j], _pConfigure->AttValName[i][j]);
			}
		}
	}
    // test prints
    /*printf("dentro initialize attributes da file\n");
	printf("MaxClass = %d\n", _pConfigure->MaxClass);
	for(int i = 0; i <= _pConfigure->MaxClass; i ++) printf("ClassName[%d] = %s\n", i, _pConfigure->ClassName[i]);
	printf("MaxDiscrVal = %d\n", _pConfigure->MaxDiscrVal);
	printf("MaxAtt = %d\n", _pConfigure->MaxAtt);
	for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
		printf("AttName[%d] = %s\n", i, _pConfigure->AttName[i]);
		printf("MaxAttVal[%d] = %d\n", i, _pConfigure->MaxAttVal[i]);
		printf("SpecialStatus[%d] = %d\n", i, _pConfigure->SpecialStatus[i]);
		if(_pConfigure->SpecialStatus[i] != 2) {
			for(int l = 0; l <= _pConfigure->MaxAttVal[i]; l ++)
				printf("AttValName[%d] = %s\n", l, _pConfigure->AttValName[i][l]);
		}
	}*/
	return 0;
}

int DataGenerator::configureGenerator(C45AVList * * rules, int noRules, C45AVList * ranges, float noiseLevel)
{
	if((noiseLevel > 1)||(noiseLevel < 0)) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("the parameter noiseLevel has an illegal value. Unable to "
            "configure the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "the parameter noiseLevel has an illegal value. Unable to configure the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	if(rules == NULL) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the passed parameter C45AVList * * rules is NULL. "
            "Unable to configure the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: the passed parameter C45AVList * * rules is NULL. Unable to configure "
            "the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	if(noRules < 1) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: there must be at least one rule. Unable to configure "
            "the data generator. \0"), sizeof(char));;
		strcat(_pszErrorMessage, "error: there must be at least one rule. Unable to configure the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	for(int i = 0; i < noRules; i ++) {
		if(rules[i]->getLength() < 1) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error: each rule must have at least one condition. Unable "
                "to configure the data generator. \0"), sizeof(char));
			strcat(_pszErrorMessage, "error: each rule must have at least one condition. Unable to configure the "
                "data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 1;
		}
	}
	if(ranges == NULL) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the passed parameter C45AVList * ranges is NULL. Unable "
            "to configure the data generator. \0"), sizeof(char));;
		strcat(_pszErrorMessage, "error: the passed parameter C45AVList * ranges is NULL. Unable to configure the "
            "data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	_countContinuous = 0;
	for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
		if(_pConfigure->MaxAttVal[i] == 0) _countContinuous ++;
	}
	if(ranges->getLength() < _countContinuous) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: there must be one range for each attribute declared as "
            "CONTINUOUS. Unable to configure the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: there must be one range for each attribute declared as CONTINUOUS. Unable "
            "to configure the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return 1;
	}
	if(_pAttrRange != NULL) {
		free(_pAttrRange);
		_pAttrRange = NULL;
	}
	_pAttrRange = (_attrRange *) calloc(_countContinuous, sizeof(_attrRange));
	for(int i = 0; i < _countContinuous; i ++) {
		for(int j = 0; j <= _pConfigure->MaxAtt; j ++) {
			if(!strcmp(ranges->getAttribute(i), _pConfigure->AttName[j])) {
				_pAttrRange[i]._attrNo = j;
				int ret = sscanf(ranges->getValueByIndex(i), "%f%*[,]%f", &(_pAttrRange[i]._lowerBound),
                    &(_pAttrRange[i]._upperBound));
				if(ret != 2) {
					if(_pszErrorMessage != NULL) free(_pszErrorMessage);
					_pszErrorMessage = (char *) calloc(strlen("error: one of the specified ranges is not valid. "
                        "Unable to configure the data generator. \0"), sizeof(char));
					strcat(_pszErrorMessage, "error: one of the specified ranges is not valid. Unable to configure "
                        "the data generator. \0");
                    if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
					return 1;
				}
				if(_pAttrRange[i]._lowerBound > _pAttrRange[i]._upperBound) {
					float temp = _pAttrRange[i]._lowerBound;
					_pAttrRange[i]._lowerBound = _pAttrRange[i]._upperBound;
					_pAttrRange[i]._upperBound = temp;
				}
			}
		}
	}
	if(noiseLevel > 1.0) noiseLevel = 1.0;
	if(noiseLevel < 0) noiseLevel = 0;
	_noiseLevel = noiseLevel;
	if(_pszDefaultClass != NULL) free(_pszDefaultClass);
	if(_pRules != NULL) {
		for(int i = 0; i <= _MaxRules; i ++) {
			if(_pRules[i]._MaxAttr > -1) {
				for(int j = 0; j < _pRules[i]._MaxAttr; j ++) {
					if(_pRules[i]._pConditionStatus[j] != NULL) free(_pRules[i]._pConditionStatus[j]);
					if(_pRules[i]._pConditionValue[j] != NULL) free(_pRules[i]._pConditionValue[j]);
					if(_pRules[i]._pConditionRange != NULL) {
						if(_pRules[i]._pConditionRange[j] != NULL) free(_pRules[i]._pConditionRange[j]);
					}
				}
			}
			if(_pRules[i]._pAttrNo != NULL) free(_pRules[i]._pAttrNo);
			if(_pRules[i]._pMaxCond != NULL) free(_pRules[i]._pMaxCond);
			free(_pRules[i]._pConditionStatus);
			free(_pRules[i]._pConditionValue);
			if(_pRules[i]._pConditionRange != NULL) free(_pRules[i]._pConditionRange);
		}
		free(_pRules);
		_pRules = NULL;
	}
	_MaxRules = noRules - 2;
	_pRules = (_ruleStruct *) calloc(_MaxRules + 1, sizeof(_ruleStruct));
	noRules = 0;
	for(int i = 0; i <= _MaxRules + 1; i ++) {      // for each rule
		if(!strcmp(rules[i]->getAttribute(0), rules[i]->_DEFAULT_CLASS)) {
			_pszDefaultClass = (char *) calloc(strlen(rules[i]->getValueByIndex (0)), sizeof(char));
			strcat(_pszDefaultClass, rules[i]->getValueByIndex (0));
		}
		else {
			_pRules[noRules]._MaxAttr = -1;
			_pRules[noRules]._pAttrNo = NULL;
			_pRules[noRules]._pMaxCond = NULL;
			for(int j = 0; j < rules[i]->getLength(); j ++) {    // for each pair in the rule
				int number;
				if(!strcmp(rules[i]->getAttribute(j), rules[i]->_CLASS)) {
					number = -1;
					for(int h = 0; h <= _pConfigure->MaxClass; h ++) {
						if(!strcmp(rules[i]->getValueByIndex(j), _pConfigure->ClassName[h])) {
							_pRules[noRules]._classNo = h;
							number = h;
						}
					}
					if(number == -1) {
						if(_pszErrorMessage != NULL) free(_pszErrorMessage);
						_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal CLASS value. Unable "
                            "to configure the data generator. \0")+strlen(rules[i]->getValueByIndex(j)), sizeof(char));
						strcat(_pszErrorMessage, "error: ");
						strcat(_pszErrorMessage, rules[i]->getValueByIndex(j));
						strcat(_pszErrorMessage, " is an illegal CLASS value. Unable to configure the data generator. \0");
                        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                            "%s\n", _pszErrorMessage);
						return 8;
					}
					continue;
				}
				number = -1;
				for(int h = 0; h <= _pConfigure->MaxAtt; h ++) {
					if(!strcmp(rules[i]->getAttribute(j), _pConfigure->AttName[h])) number = h;
				}
				if(number == -1) {
					if(_pszErrorMessage != NULL) free(_pszErrorMessage);
					_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal attribute name. Unable "
                        "to configure the data generator. \0")+strlen(rules[i]->getAttribute(j)), sizeof(char));
					strcat(_pszErrorMessage, "error: ");
					strcat(_pszErrorMessage, rules[i]->getAttribute(j));
					strcat(_pszErrorMessage, " is an illegal attribute name. Unable to configure the data generator. \0");
                    if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                        "%s\n", _pszErrorMessage);
					return 3;
				}
				if(_pConfigure->SpecialStatus[number] == 1) continue;  // "IGNORE" attributes
				if(_pRules[noRules]._pAttrNo == NULL) {
					_pRules[noRules]._MaxAttr = 0;
					_pRules[noRules]._pAttrNo = (int *) calloc(1, sizeof(int));
					_pRules[noRules]._pAttrNo[0] = number;
					_pRules[noRules]._pMaxCond = (int *) calloc(1, sizeof(int));
					_pRules[noRules]._pMaxCond[0] = -1;
					_pRules[noRules]._pConditionStatus = (short * *) calloc(1, sizeof(short *));
					_pRules[noRules]._pConditionValue = (Description *) calloc(1, sizeof(Description));
					_pRules[noRules]._pConditionRange = NULL;
					int ret = analizeCondition(rules, 0, number, i, j, noRules);
					if(ret != 0) return ret;
				}
				else {
					int number2 = -1;
					for(int h = 0; h <= _pRules[noRules]._MaxAttr; h ++) {
						if(_pRules[noRules]._pAttrNo[h] == number) {
							number2 = h;
							break;
						}
					}
					if(number2 == -1) {
						_pRules[noRules]._MaxAttr ++;
						_pRules[noRules]._pAttrNo = (int *) realloc(_pRules[noRules]._pAttrNo,
                            (_pRules[noRules]._MaxAttr + 1) * sizeof(int));
						_pRules[noRules]._pAttrNo[_pRules[noRules]._MaxAttr] = number;
						_pRules[noRules]._pMaxCond = (int *) realloc(_pRules[noRules]._pMaxCond,
                            (_pRules[noRules]._MaxAttr + 1) * sizeof(int));
						_pRules[noRules]._pMaxCond[_pRules[noRules]._MaxAttr] = -1;
						_pRules[noRules]._pConditionStatus = (short * *) realloc(_pRules[noRules]._pConditionStatus,
                            (_pRules[noRules]._MaxAttr + 1) * sizeof(short *));
						_pRules[noRules]._pConditionValue = (Description *) realloc(_pRules[noRules]._pConditionValue,
                            (_pRules[noRules]._MaxAttr + 1) * sizeof(Description));
						int ret = analizeCondition(rules, _pRules[noRules]._MaxAttr, number, i, j, noRules);
						if(ret != 0) return ret;
					}
					else {
						int ret = analizeCondition(rules, number2, number, i, j, noRules);
						if(ret != 0) return ret;
					}
				}
			}
			noRules ++;
		}
	}
	// test prints
	/*printf("at the end of configureGenerator, rules:\n");
	printf("_pszDefaultClass = %s\n", _pszDefaultClass);
	printf("_MaxRules = %d\n", _MaxRules);
	for(int i = 0; i <= _MaxRules; i ++) {
		printf("_pRules[%d]._classNo = %d\n", i, _pRules[i]._classNo);
		printf("_pRules[%d]._MaxAttr = %d\n", i, _pRules[i]._MaxAttr);
		for(int j = 0; j <= _pRules[i]._MaxAttr; j ++) {
			printf("_pRules[%d]._pAttrNo[%d] = %d\n", i, j, _pRules[i]._pAttrNo[j]);
			printf("_pRules[%d]._pMaxCond[%d] = %d\n", i, j, _pRules[i]._pMaxCond[j]);
			for(int k = 0; k <= _pRules[i]._pMaxCond[j]; k ++) {
				printf("_pRules[%d]._pConditionStatus[%d][%d] = %d\n", i, j, k, _pRules[i]._pConditionStatus[j][k]);
				if(i != 0) printf("_pRules[%d]._pConditionValue[%d][%d] = %d\n", i, j, k, _pRules[i]._pConditionValue[j][k]._discr_val);
				else printf("_pRules[%d]._pConditionValue[%d][%d] = %f\n", i, j, k, _pRules[i]._pConditionValue[j][k]._cont_val);
				if(_pRules[i]._pConditionStatus[j][k] == 3)
					printf("_pRules[%d]._pConditionRange[%d][%d] = %f\n", i, j, k, _pRules[i]._pConditionRange[j][k]);
			}
		}
	}*/
	_fileMode = false;
	return 0;
}

int DataGenerator::configureGenerator(const char * fileName)
{
	if(fileName == NULL) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the passed file name is a NULL pointer. Unable to "
            "configure the data generator. \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: the passed file name is a NULL pointer. Unable to configure the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return -1; 
	}
	FILE * pDFile;
	if(!(pDFile = fopen(fileName, "r"))) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: cannot open the file ''. Unable to configure the "
            "data generator. \0") + strlen(fileName), sizeof(char));
		strcat(_pszErrorMessage, "error: cannot open the file '");
		strcat(_pszErrorMessage, fileName);
		strcat(_pszErrorMessage, "'. Unable to configure the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return -1;
	}
	if(_pItem != NULL) {
		for(int i = 0; i <= _MaxItem; i ++) free(_pItem[i]);
		free(_pItem);
		_actualItem = -1;
		_MaxItem = -1;
	}
	int actual = 0;
	int itemSpace = 0;
	char buffer[400];
	char * endname;
	int discrVal;
	float contVal;
	short attr;
	Description descr;
	do {
		_MaxItem = actual;
		if(actual >= itemSpace)	{
	    	if(itemSpace) {
				itemSpace += 50;
				_pItem = (Description *) realloc(_pItem, itemSpace*sizeof(Description));
	    	}
	    	else _pItem = (Description *) calloc((itemSpace = 50), sizeof(Description));
		}
		if(readFromFile(pDFile, buffer, NULL)) {
			descr = (Description) calloc(_pConfigure->MaxAtt + 2, sizeof(AttValue));
    		for(attr = 0; attr <= _pConfigure->MaxAtt; attr ++) {
				if(_pConfigure->SpecialStatus[attr] == 1) descr[attr]._discr_val = 0;
	   			else if(_pConfigure->MaxAttVal[attr] || _pConfigure->SpecialStatus[attr] == 2) {
	       			if(!(strcmp(buffer, "?"))) discrVal = 0;
	        		else {
	        			short n = 1;
	        			while(n <= _pConfigure->MaxAttVal[attr] && strcmp(buffer, _pConfigure->AttValName[attr][n])) n ++;
	        			discrVal = (n <= _pConfigure->MaxAttVal[attr] ? n : 0);
		    			if(!discrVal) {
							if(_pConfigure->SpecialStatus[attr] == 2) {
								discrVal = ++(_pConfigure->MaxAttVal[attr]);
								#if defined (LINUX64) || defined (OSX64)
								if(discrVal > (long long int) _pConfigure->AttValName[attr][0]) {
								#else
								if(discrVal > (int) _pConfigure->AttValName[attr][0]) {
								#endif
									if(_pszErrorMessage != NULL) free(_pszErrorMessage);
									_pszErrorMessage = (char *) calloc(strlen("error: too many values for attribute  "
                                        "(max number of value is )") + strlen(_pConfigure->AttName[attr])
                                        + strlen(_pConfigure->AttValName[attr][0]) + 3, sizeof(char));
									strcat(_pszErrorMessage, "error: too many values for attribute ");
									strcat(_pszErrorMessage, _pConfigure->AttName[attr]);
									strcat(_pszErrorMessage, " (max number of value is ");
									strcat(_pszErrorMessage, _pConfigure->AttValName[attr][0]);
									strcat(_pszErrorMessage, " ). ");
									return -6;
								}
			    				_pConfigure->AttValName[attr][discrVal] = (char *) calloc(strlen(buffer) + 1, sizeof(char));
			    				strcpy(_pConfigure->AttValName[attr][discrVal], buffer);
							}
							else {
								if(_pszErrorMessage != NULL) free(_pszErrorMessage);
								_pszErrorMessage = (char *) calloc(strlen("error: value  for attribute  is illegal")
                                    + strlen(_pConfigure->AttName[attr]) + strlen(buffer) + 3, sizeof(char));
								strcat(_pszErrorMessage, "error: value ");
								strcat(_pszErrorMessage, buffer);
								strcat(_pszErrorMessage, " for attribute ");
								strcat(_pszErrorMessage, _pConfigure->AttName[attr]);
								strcat(_pszErrorMessage, " is illegal. ");
                                if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                                    "%s\n", _pszErrorMessage);
								return -7;
							}
		   				}
					}
					descr[attr]._discr_val = discrVal;
				}
				else {
		  			if(!(strcmp(buffer, "?"))) contVal = -999;
				    else {
		    			contVal = (float) strtod (buffer, &endname);
		    			if(endname == buffer || (*endname) != '\0') {
		    				if(_pszErrorMessage != NULL) free(_pszErrorMessage);
							_pszErrorMessage = (char *) calloc(strlen("error: value  for attribute  is illegal")
                                + strlen(_pConfigure->AttName[attr]) + strlen(buffer) + 3, sizeof(char));
							strcat(_pszErrorMessage, "error: value ");
							strcat(_pszErrorMessage, buffer);
							strcat(_pszErrorMessage, " for attribute ");
							strcat(_pszErrorMessage, _pConfigure->AttName[attr]);
							strcat(_pszErrorMessage, " is illegal. ");
                            if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                                "%s\n", _pszErrorMessage);
							return -7;
		    			}
					}
					descr[attr]._cont_val = contVal;
				}
				readFromFile(pDFile, buffer, NULL);
			}
			short n = 0;
			while(n <= _pConfigure->MaxClass && strcmp(buffer, _pConfigure->ClassName[n])) n ++;
			discrVal = (n <= _pConfigure->MaxClass ? n : -1);
			if(discrVal < 0) {
				if(_pszErrorMessage != NULL) free(_pszErrorMessage);
				_pszErrorMessage = (char *) calloc(strlen("error: class name  is illegal")
                    + strlen(buffer) + 3, sizeof(char));
				strcat(_pszErrorMessage, "error: class name ");
				strcat(_pszErrorMessage, buffer);
				strcat(_pszErrorMessage, " is illegal. ");
                if(pLogger) pLogger->logMsg("DataGenerator::configureGenerator", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
				return -8;
				discrVal = 0;
			}
			descr[_pConfigure->MaxAtt + 1]._discr_val = discrVal;
		}
		else descr = 0;
		_pItem[actual] = descr;
    } while(_pItem[actual] != 0 && ++actual);
    fclose(pDFile);
    _MaxItem = actual - 1;
    _actualItem = -1;
	_fileMode = true;
	// test prints
	//printf("dentro configureGenerator da file\n");
	//for(int i = 0; i <= _MaxItem; i ++) {
	//	for(int l = 0; l < _pConfigure->MaxAtt + 2; l ++) {
	//		if((l == 1)||(l == 2)) printf("Item[%d][%d] = %f\n", i, l, _pItem[i][l]._cont_val);
	//		else printf("Item[%d][%d] = %d\n", i, l, _pItem[i][l]._discr_val);
	//	}
	//}
	return _MaxItem + 1;
}

C45AVList * DataGenerator::generateDataset(int datasetLength)
{
	if(datasetLength < 1) {
		if(_pszErrorMessage != NULL) free(_pszErrorMessage);
		_pszErrorMessage = (char *) calloc(strlen("error: the parameter datasetLength must be at least equal to one. "
            "Unable to generate data for the tree, \0"), sizeof(char));
		strcat(_pszErrorMessage, "error: the parameter datasetLength must be at least equal to one. Unable to "
            "generate data for the tree, \0");
        if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
		return NULL;
	}
	// generate datasetLength examples
	C45AVList * example = new C45AVList(_pConfigure->MaxAtt + 2);
	C45AVList * dataset = new C45AVList((_pConfigure->MaxAtt + 2) * datasetLength);
	if(_fileMode) {
		for(int k = 0; k < datasetLength; k ++) {
			_actualItem ++;
			example->addPair(example->_CLASS, _pConfigure->ClassName[_pItem[_actualItem][_pConfigure->MaxAtt + 1]._discr_val]);
			for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
				if((_pConfigure->SpecialStatus[i] == 2) || ((_pConfigure->SpecialStatus[i] == 0) &&
                    (_pConfigure->MaxAttVal[i] != 0))) {
					if(_pItem[_actualItem][i]._discr_val == 0) example->addPair(_pConfigure->AttName[i], example->_UNKNOWN);
					else example->addPair(_pConfigure->AttName[i], _pConfigure->AttValName[i][_pItem[_actualItem][i]._discr_val]);
				}
				else example->addPair(_pConfigure->AttName[i], _pItem[_actualItem][i]._cont_val);
			}
			if(_actualItem == _MaxItem) {
				_actualItem = -1;
				break;
			}
			// test prints
			//for(int i = 0; i < example->getLength(); i ++)
            //  printf(" attribute = %s,\t\t value = %s\n", example->getAttribute(i), example->getValueByIndex(i));
			//printf("\n");
			dataset->concatLists(dataset, example);
			example->emptyList();
		}
		delete example;
		return dataset;
	}
	float randomNumber;
	for(int k = 0; k < datasetLength; k ++) {
		for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
			if(_pConfigure->SpecialStatus[i] == 1) {    // "IGNORE" attributes
				example->addPair(_pConfigure->AttName[i], "A");
				continue;
			}
			if(_pConfigure->SpecialStatus[i] == 2) {    // "DISCRETE" attributes with unknown values
				#if defined (LINUX64) || defined (OSX64)
				randomNumber = (float) (rand() % (long long int) _pConfigure->AttValName[i][0] + 1);
				#else
				randomNumber = (float) (rand() % (int) _pConfigure->AttValName[i][0] + 1);
				#endif
				if(_pConfigure->MaxAttVal[i] == 0) example->addPair(_pConfigure->AttName[i], (int) randomNumber);
				else {
					if(randomNumber <= _pConfigure->MaxAttVal[i])
                        example->addPair(_pConfigure->AttName[i], _pConfigure->AttValName[i][(int) randomNumber]);
					else example->addPair(_pConfigure->AttName[i], (int) randomNumber);
				}
				continue;
			}
			if(_pConfigure->MaxAttVal[i] == 0) {       // "CONTINUOUS" attributes
				for(int j = 0; j < _countContinuous; j ++) {
					if(_pAttrRange[j]._attrNo == i) {
						randomNumber = (rand() / (static_cast<float>(RAND_MAX) + 1.0)) *
                            (_pAttrRange[j]._upperBound - _pAttrRange[j]._lowerBound) + _pAttrRange[j]._lowerBound;
						example->addPair(_pConfigure->AttName[i], randomNumber);
						break;
					}
				}
				continue;
			}
			else {   								 // "DISCRETE" attributes with known values
				randomNumber = (float) (rand() % _pConfigure->MaxAttVal[i] + 1);
				example->addPair(_pConfigure->AttName[i], _pConfigure->AttValName[i][(int) randomNumber]);
			}
		}
		randomNumber = (float)((rand() / (static_cast<float>(RAND_MAX) + 1.0)) * 1.0);
		if(randomNumber <= _noiseLevel) {
			randomNumber = (float) (rand() % (_pConfigure->MaxClass + 1));
			example->addPair(example->_CLASS, _pConfigure->ClassName[(int) randomNumber]);
		}
		else {
			bool satisfied = false;
			for(int i = 0; i <= _MaxRules; i ++) {		// for each rule
				for(int j = 0; j <= _pRules[i]._MaxAttr; j ++) {  // for each attribute
					satisfied = false;
					for(int h = 0; h <= _pRules[i]._pMaxCond[j]; h ++) {   // for each condition
						switch(_pRules[i]._pConditionStatus[j][h]) {
							case 0 : {	// discrete  or continuous "equal"
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if((_pConfigure->SpecialStatus[_pRules[i]._pAttrNo[j]] == 2) ||
                                            ((_pConfigure->SpecialStatus[_pRules[i]._pAttrNo[j]] == 0) &&
                                                (_pConfigure->MaxAttVal[_pRules[i]._pAttrNo[j]] != 0))) {// discrete
											if(!strcmp(example->getValueByIndex(l),
                                                _pConfigure->AttValName[_pRules[i]._pAttrNo[j]]
                                                [_pRules[i]._pConditionValue[j][h]._discr_val])) {
												satisfied = true;
												break;
											}
										}
										else { // continuous
											if(atof(example->getValueByIndex(l)) == _pRules[i]._pConditionValue[j][h]._cont_val) {
												satisfied = true;
												break;
											}
										}
									}
								}
								break;
							}
							case 1 : {	// continuous ">"
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if(atof(example->getValueByIndex(l)) > _pRules[i]._pConditionValue[j][h]._cont_val) {
											satisfied = true;
											break;
										}
									}
								}
								break;
							}
							case 2 : {	// continuous "<"
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if(atof(example->getValueByIndex(l)) < _pRules[i]._pConditionValue[j][h]._cont_val) {
											satisfied = true;
											break;
										}
									}
								}
								break;
							}
							case 3 : {	// continuous "range"
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if((atof(example->getValueByIndex(l)) >= _pRules[i]._pConditionValue[j][h]._cont_val)&&(atof(example->getValueByIndex(l)) <= _pRules[i]._pConditionRange[j][h])) {
											satisfied = true;
											break;
										}
									}
								}
								break;
							}
							case 4 : {  // discrete or continuous "!="
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if(_pConfigure->AttValName[_pRules[i]._pAttrNo[j]][0] == NULL) { // continuous
											if(atof(example->getValueByIndex(l)) != _pRules[i]._pConditionValue[j][h]._cont_val) {
												satisfied = true;
												break;
											}
										}
										else { // discrete with unknown values
											if(strcmp(example->getValueByIndex(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
												satisfied = true;
												break;
											}
										}
									}
								}
								break;
							}
							case 5 : {  // continuous ">="
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if(atof(example->getValueByIndex(l)) >= _pRules[i]._pConditionValue[j][h]._cont_val) {
											satisfied = true;
											break;
										}
									}
								}
								break;
							}
							case 6 : {  // continuous "<="
								for(int l = 0; l < example->getLength(); l ++) {
									if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
										if(atof(example->getValueByIndex(l)) <= _pRules[i]._pConditionValue[j][h]._cont_val) {
											satisfied = true;
											break;
										}
									}
								}
								break;
							}
						}
					}
					if(!satisfied) {   // OR
						break;
					}
				}
				if(satisfied) {       // AND
					example->addPair(example->_CLASS, _pConfigure->ClassName[_pRules[i]._classNo]);
					break;
				}
			}
			if(!satisfied) {          //set default class
				example->addPair(example->_CLASS, _pszDefaultClass);
			}
		}
		// test prints
		//for(int i = 0; i < example->getLength(); i ++)
        //  printf(" attribute = %s,\t\t value = %s\n", example->getAttribute(i), example->getValueByIndex(i));
		//printf("\n");
		dataset->concatLists(dataset, example);
		example->emptyList();
	}
	delete example;
	return dataset;
}

int DataGenerator::generateDataset(int datasetLength, const char * pszFileName)
{
    if(datasetLength < 1) {
        if(_pszErrorMessage != NULL) free(_pszErrorMessage);
        _pszErrorMessage = (char *) calloc(strlen("error: the parameter datasetLength must be at least equal to one. "
            "Unable to generate data for the tree, \0"), sizeof(char));
        strcat(_pszErrorMessage, "error: the parameter datasetLength must be at least equal to one. Unable to "
            "generate data for the tree, \0");
        if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        return -1;
    }
    if(pszFileName == NULL) {
        if(_pszErrorMessage != NULL) free(_pszErrorMessage);
        _pszErrorMessage = (char *) calloc(strlen("error: the passed file name is a NULL pointer. Unable to "
            "initialize the data generator. \0"), sizeof(char));
        strcat(_pszErrorMessage, "error: the passed file name is a NULL pointer. Unable to initialize the data generator. \0");
        if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
            "%s\n", _pszErrorMessage);
        return -1;
    }
    FILE * pNFile;
    // generate datasetLength examples
    if(_fileMode) {
        for(int k = 0; k < datasetLength; k ++) {
            _actualItem ++;
            NOMADSUtil::String fileName = pszFileName;
            char number[10] = "\0";
            sprintf(number, "%d", _actualItem);
            fileName += number;
            fileName += ".xml";
            if(!(pNFile = fopen(fileName, "w"))) {
                if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                _pszErrorMessage = (char *) calloc(strlen("error: cannot open the file ''. Unable to initialize the "
                    "data generator. \0") + strlen(pszFileName), sizeof(char));
                strcat(_pszErrorMessage, "error: cannot open the file '");
                strcat(_pszErrorMessage, pszFileName);
                strcat(_pszErrorMessage, "'. Unable to initialize the data generator. \0");
                if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                return -1;
            }
            NOMADSUtil::String xmlDataItem = DataGenerator::_XML_HEADER;
            xmlDataItem += "\n<";
            xmlDataItem += DataGenerator::_XML_DATA_ITEM;
            xmlDataItem += ">\n\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
            xmlDataItem += ">\n\t\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
            xmlDataItem += ">";
            xmlDataItem += DataGenerator::_XML_CLASS_NAME;
            xmlDataItem += "</";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
            xmlDataItem += ">\n\t\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
            xmlDataItem += ">";
            xmlDataItem += _pConfigure->ClassName[_pItem[_actualItem][_pConfigure->MaxAtt + 1]._discr_val];
            xmlDataItem += "</";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
            xmlDataItem += ">\n\t</";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
            xmlDataItem += ">";
            for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
                xmlDataItem += "\n\t<";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">\n\t\t<";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
                xmlDataItem += ">";
                xmlDataItem += _pConfigure->AttName[i];
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
                xmlDataItem += ">\n\t\t<";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">";
                if((_pConfigure->SpecialStatus[i] == 2) || ((_pConfigure->SpecialStatus[i] == 0) &&
                    (_pConfigure->MaxAttVal[i] != 0))) {
                    if(_pItem[_actualItem][i]._discr_val != 0)
                        xmlDataItem += _pConfigure->AttValName[i][_pItem[_actualItem][i]._discr_val];
                }
                else {
                    char contNumber[30];
                    sprintf(contNumber, "%f", _pItem[_actualItem][i]._cont_val);
                    xmlDataItem += contNumber;
                }
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">\n\t</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">";
            }
            if(_actualItem == _MaxItem) {
                _actualItem = -1;
                break;
            }
            xmlDataItem += "\n</";
            xmlDataItem += DataGenerator::_XML_DATA_ITEM;
            xmlDataItem += ">";
            int retValue = fputs((const char *) xmlDataItem, pNFile);
            if(retValue < 0) {
                if(_pszErrorMessage != NULL) free(_pszErrorMessage);
                _pszErrorMessage = (char *) calloc(strlen("error: cannot write on file ''. Unable to create the "
                    "dataset. \0") + strlen(pszFileName), sizeof(char));
                strcat(_pszErrorMessage, "error: cannot write on file '");
                strcat(_pszErrorMessage, pszFileName);
                strcat(_pszErrorMessage, "'. Unable to create the dataset. \0");
                if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
                fclose(pNFile);
                return -1;
            }
            // test print
            //printf("xmlDataItem = \n%s\n\n", (const char *) xmlDataItem);
            fclose(pNFile);
        }
        return 0;
    }
    C45AVList * example = new C45AVList(_pConfigure->MaxAtt + 2);
    float randomNumber;
    for(int k = 0; k < datasetLength; k ++) {
        NOMADSUtil::String fileName = pszFileName;
        char number[10] = "\0";
        sprintf(number, "%d", k);
        fileName += number;
        fileName += ".xml";
        if(!(pNFile = fopen(fileName, "w"))) {
            if(_pszErrorMessage != NULL) free(_pszErrorMessage);
            _pszErrorMessage = (char *) calloc(strlen("error: cannot open the file ''. Unable to initialize the "
                "data generator. \0") + strlen(pszFileName), sizeof(char));
            strcat(_pszErrorMessage, "error: cannot open the file '");
            strcat(_pszErrorMessage, pszFileName);
            strcat(_pszErrorMessage, "'. Unable to initialize the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
            return -1;
        }
        NOMADSUtil::String xmlDataItem = DataGenerator::_XML_HEADER;
        xmlDataItem += "\n<";
        xmlDataItem += DataGenerator::_XML_DATA_ITEM;
        xmlDataItem += ">";
        for(int i = 0; i <= _pConfigure->MaxAtt; i ++) {
            xmlDataItem += "\n\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
            xmlDataItem += ">\n\t\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
            xmlDataItem += ">";
            xmlDataItem += _pConfigure->AttName[i];
            xmlDataItem += "</";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
            xmlDataItem += ">\n\t\t<";
            xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
            xmlDataItem += ">";
            if(_pConfigure->SpecialStatus[i] == 1) {    // "IGNORE" attributes
                xmlDataItem += "A";
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">\n\t</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">";
                example->addPair(_pConfigure->AttName[i], "A");
                continue;
            }
            if(_pConfigure->SpecialStatus[i] == 2) {    // "DISCRETE" attributes with unknown values
                #if defined (LINUX64) || defined (OSX64)
                randomNumber = (float) (rand() % (long long int) _pConfigure->AttValName[i][0] + 1);
                #else
                randomNumber = (float) (rand() % (int) _pConfigure->AttValName[i][0] + 1);
                #endif
                char contNumber[10] = "\0";
                if(_pConfigure->MaxAttVal[i] == 0) {
                    sprintf(contNumber, "%d", (int) randomNumber);
                    xmlDataItem += contNumber;
                    example->addPair(_pConfigure->AttName[i], (int) randomNumber);
                }
                else {
                    if(randomNumber <= _pConfigure->MaxAttVal[i]) {
                        xmlDataItem += _pConfigure->AttValName[i][(int) randomNumber];
                        example->addPair(_pConfigure->AttName[i], _pConfigure->AttValName[i][(int) randomNumber]);
                    }
                    else {
                        sprintf(contNumber, "%d", (int) randomNumber);
                        xmlDataItem += contNumber;
                        example->addPair(_pConfigure->AttName[i], (int) randomNumber);
                    }
                }
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">\n\t</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">";
                continue;
            }
            if(_pConfigure->MaxAttVal[i] == 0) {       // "CONTINUOUS" attributes
                for(int j = 0; j < _countContinuous; j ++) {
                    if(_pAttrRange[j]._attrNo == i) {
                        randomNumber = (rand() / (static_cast<float>(RAND_MAX) + 1.0)) *
                            (_pAttrRange[j]._upperBound - _pAttrRange[j]._lowerBound) + _pAttrRange[j]._lowerBound;
                        char contNumber[30] = "\0";
                        sprintf(contNumber, "%f", randomNumber);
                        xmlDataItem += contNumber;
                        example->addPair(_pConfigure->AttName[i], randomNumber);
                        break;
                    }
                }
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">\n\t</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">";
                continue;
            }
            else {                                   // "DISCRETE" attributes with known values
                randomNumber = (float) (rand() % _pConfigure->MaxAttVal[i] + 1);
                xmlDataItem += _pConfigure->AttValName[i][(int) randomNumber];
                xmlDataItem += "</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
                xmlDataItem += ">\n\t</";
                xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
                xmlDataItem += ">";
                example->addPair(_pConfigure->AttName[i], _pConfigure->AttValName[i][(int) randomNumber]);
            }
        }
        randomNumber = (float)((rand() / (static_cast<float>(RAND_MAX) + 1.0)) * 1.0);
        xmlDataItem += "\n\t<";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
        xmlDataItem += ">\n\t\t<";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
        xmlDataItem += ">";
        xmlDataItem += DataGenerator::_XML_CLASS_NAME;
        xmlDataItem += "</";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE_NAME;
        xmlDataItem += ">\n\t\t<";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
        xmlDataItem += ">";
        if(randomNumber <= _noiseLevel) {
            randomNumber = (float) (rand() % (_pConfigure->MaxClass + 1));
            xmlDataItem += _pConfigure->ClassName[(int) randomNumber];
        }
        else {
            bool satisfied = false;
            for(int i = 0; i <= _MaxRules; i ++) {      // for each rule
                for(int j = 0; j <= _pRules[i]._MaxAttr; j ++) {  // for each attribute
                    satisfied = false;
                    for(int h = 0; h <= _pRules[i]._pMaxCond[j]; h ++) {   // for each condition
                        switch(_pRules[i]._pConditionStatus[j][h]) {
                            case 0 : {  // discrete  or continuous "equal"
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if((_pConfigure->SpecialStatus[_pRules[i]._pAttrNo[j]] == 2) ||
                                            ((_pConfigure->SpecialStatus[_pRules[i]._pAttrNo[j]] == 0) &&
                                                (_pConfigure->MaxAttVal[_pRules[i]._pAttrNo[j]] != 0))) {// discrete
                                            if(!strcmp(example->getValueByIndex(l),
                                                _pConfigure->AttValName[_pRules[i]._pAttrNo[j]]
                                                [_pRules[i]._pConditionValue[j][h]._discr_val])) {
                                                satisfied = true;
                                                break;
                                            }
                                        }
                                        else { // continuous
                                            if(atof(example->getValueByIndex(l)) == _pRules[i]._pConditionValue[j][h]._cont_val) {
                                                satisfied = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            case 1 : {  // continuous ">"
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if(atof(example->getValueByIndex(l)) > _pRules[i]._pConditionValue[j][h]._cont_val) {
                                            satisfied = true;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            case 2 : {  // continuous "<"
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if(atof(example->getValueByIndex(l)) < _pRules[i]._pConditionValue[j][h]._cont_val) {
                                            satisfied = true;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            case 3 : {  // continuous "range"
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if((atof(example->getValueByIndex(l)) >= _pRules[i]._pConditionValue[j][h]._cont_val)&&(atof(example->getValueByIndex(l)) <= _pRules[i]._pConditionRange[j][h])) {
                                            satisfied = true;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            case 4 : {  // discrete or continuous "!="
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if(_pConfigure->AttValName[_pRules[i]._pAttrNo[j]][0] == NULL) { // continuous
                                            if(atof(example->getValueByIndex(l)) != _pRules[i]._pConditionValue[j][h]._cont_val) {
                                                satisfied = true;
                                                break;
                                            }
                                        }
                                        else { // discrete with unknown values
                                            if(strcmp(example->getValueByIndex(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                                satisfied = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                            case 5 : {  // continuous ">="
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if(atof(example->getValueByIndex(l)) >= _pRules[i]._pConditionValue[j][h]._cont_val) {
                                            satisfied = true;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                            case 6 : {  // continuous "<="
                                for(int l = 0; l < example->getLength(); l ++) {
                                    if(!strcmp(example->getAttribute(l), _pConfigure->AttName[_pRules[i]._pAttrNo[j]])) {
                                        if(atof(example->getValueByIndex(l)) <= _pRules[i]._pConditionValue[j][h]._cont_val) {
                                            satisfied = true;
                                            break;
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                    if(!satisfied) {   // OR
                        break;
                    }
                }
                if(satisfied) {       // AND
                    xmlDataItem += _pConfigure->ClassName[_pRules[i]._classNo];
                    break;
                }
            }
            if(!satisfied) {          //set default class
                xmlDataItem += _pszDefaultClass;
            }
        }
        xmlDataItem += "</";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE_VALUE;
        xmlDataItem += ">\n\t</";
        xmlDataItem += DataGenerator::_XML_ATTRIBUTE;
        xmlDataItem += ">";
        xmlDataItem += "\n</";
        xmlDataItem += DataGenerator::_XML_DATA_ITEM;
        xmlDataItem += ">";
        int retValue = fputs((const char *) xmlDataItem, pNFile);
        if(retValue < 0) {
            if(_pszErrorMessage != NULL) free(_pszErrorMessage);
            _pszErrorMessage = (char *) calloc(strlen("error: cannot write on file ''. Unable to create the "
                "dataset. \0") + strlen(pszFileName), sizeof(char));
            strcat(_pszErrorMessage, "error: cannot write on file '");
            strcat(_pszErrorMessage, pszFileName);
            strcat(_pszErrorMessage, "'. Unable to create the dataset. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::generateDataset", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
            fclose(pNFile);
            return -1;
        }
        fclose(pNFile);
        example->emptyList();
        // test print
        //printf("xmlDataItem = \n%s\n\n", (const char *) xmlDataItem);
    }
    delete example;
    return 0;
}

int DataGenerator::analizeCondition(C45AVList * * rules, int index, int number, int indexRule, int indexValue, int noRules)
{
	if(_pConfigure->SpecialStatus[number] == 2) {  // case DISCRETE with UNKNOWN VALUES
		char * val1 = (char *) calloc(strlen(rules[indexRule]->getValueByIndex(indexValue))+1, sizeof(char));
		char * val2 = (char *) calloc(10, sizeof(char));
		int ret = sscanf(rules[indexRule]->getValueByIndex(indexValue), "%s%*[,]%s", val1, val2);
		if(!((ret == 1)||(ret == 3))) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error: Unable to read the value . Unable to configure "
                "the data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: Unable to read the value ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 4;
		}
		if((ret == 3)&&(strcmp(val2, "!="))) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal condition for attribute . Unable to "
                "configure the data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)) +
                strlen(rules[indexRule]->getAttribute(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, " is an illegal condition for attribute ");
			strcat(_pszErrorMessage, rules[indexRule]->getAttribute(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 7;
		}
		int number2 = -1;
		if(_pConfigure->MaxAttVal[number] > 0) {
			for(int h = 1; h <=_pConfigure->MaxAttVal[number]; h ++) {
				if(!strcmp(_pConfigure->AttValName[number][h], val1)) number2 = h;
			}
		}
		if((number2 == -1)||(_pConfigure->MaxAttVal[number] == 0)) {
		     #if defined (LINUX64) || defined (OSX64)
			if(_pConfigure->MaxAttVal[number] + 1 > (long long int) _pConfigure->AttValName[number][0]) {
			#else
			if(_pConfigure->MaxAttVal[number] + 1 > (int) _pConfigure->AttValName[number][0]) {
			#endif
				if(_pszErrorMessage != NULL) free(_pszErrorMessage);
				_pszErrorMessage = (char *) calloc(strlen("error: Illegal number of values for attribute . "
                    "Unable to configure the data generator. \0") + strlen(_pConfigure->AttName[number]), sizeof(char));
				strcat(_pszErrorMessage, "error: Illegal number of values for the attribute ");
				strcat(_pszErrorMessage, _pConfigure->AttName[number]);
				strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
                if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
				return 6;
			}
			_pConfigure->MaxAttVal[number] ++;
			_pConfigure->AttValName[number][_pConfigure->MaxAttVal[number]] = (char *) calloc(strlen(val1)+1, sizeof(char));
			strcat(_pConfigure->AttValName[number][_pConfigure->MaxAttVal[number]], val1);
			number2 = _pConfigure->MaxAttVal[number];
		}
		_pRules[noRules]._pMaxCond[index] ++;
		_pRules[noRules]._pConditionStatus[index] = (short *) realloc(_pRules[noRules]._pConditionStatus[index],
            (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(short));
		if(ret == 3) _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 4;     // "!="
		else _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 0;             // equal
		_pRules[noRules]._pConditionValue[index] = (Description) realloc(_pRules[noRules]._pConditionValue[index],
            (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(AttValue));
		_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._discr_val = number2;// value
		free(val1);
		free(val2);
		return 0;
	}
	if(_pConfigure->MaxAttVal[number] == 0) {  // case CONTINUOUS
		float val1;
		char * val2 = (char *) calloc(10, sizeof(char));
		float val3;;
		int ret = sscanf(rules[indexRule]->getValueByIndex(indexValue), "%f %s %f", &val1, val2, &val3);
		if((ret < 1)||(ret > 3)) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error: Unable to read the value . Unable to configure the "
                "data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: Unable to read the value ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 4;
		}
		if(ret == 1) {  // equal
			_pRules[noRules]._pMaxCond[index] ++;
			_pRules[noRules]._pConditionStatus[index] = (short *) realloc(_pRules[noRules]._pConditionStatus[index],
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(short));
			_pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 0;               // equal
			_pRules[noRules]._pConditionValue[index] = (Description) realloc(_pRules[noRules]._pConditionValue[index],
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(AttValue));
			_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._cont_val = val1;	// value
		}
		if(ret == 2) {  // "<", ">"
			if((!strcmp(val2, ">"))||(!strcmp(val2, "<"))||(!strcmp(val2, ">="))||(!strcmp(val2, "<="))) {
				_pRules[noRules]._pMaxCond[index] ++;
				_pRules[noRules]._pConditionStatus[index] = (short *) realloc(_pRules[noRules]._pConditionStatus[index],
                    (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(short));
				if(!strcmp(val2, ">")) _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 1;
				if(!strcmp(val2, "<")) _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 2;
				if(!strcmp(val2, ">=")) _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 5;
				else _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 6;
				_pRules[noRules]._pConditionValue[index] = (Description) realloc(_pRules[noRules]._pConditionValue[index],
                    (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(AttValue));
				_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._cont_val = val1;	// value
			}
			else {
				if(_pszErrorMessage != NULL) free(_pszErrorMessage);
				_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal value, '' is illegal, it must be "
                    "one of these: '<', '>', '<=', '>='. Unable to configure the data generator. \0") +
                    strlen(rules[indexRule]->getValueByIndex(indexValue)) + strlen(val2), sizeof(char));
				strcat(_pszErrorMessage, "error: ");
				strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
				strcat(_pszErrorMessage, " is an illegal value, '");
				strcat(_pszErrorMessage, val2);
				strcat(_pszErrorMessage, "' is illegal, it must be one of these: '<', '>', '<=', '>='. Unable to "
                    "configure the data generator. \0");
                if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                    "%s\n", _pszErrorMessage);
				return 7;
			}
		}
		if(ret == 3) {  // range
			if(val1 > val3) {
				float temp = val1;
				val1 = val3;
				val3 = temp;
			}
			_pRules[noRules]._pMaxCond[index] ++;
			_pRules[noRules]._pConditionStatus[index] = (short *) realloc(_pRules[noRules]._pConditionStatus[index],
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(short));
			_pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 3;              // range
			_pRules[noRules]._pConditionValue[index] = (Description) realloc(_pRules[noRules]._pConditionValue[index],
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(AttValue));
			_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._cont_val = val1;  // lower bound
			_pRules[noRules]._pConditionRange = (float * *) realloc(_pRules[noRules]._pConditionRange,
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(float *));
			_pRules[noRules]._pConditionRange[index] = (float *) realloc(_pRules[noRules]._pConditionRange[index],
                (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(float));
			_pRules[noRules]._pConditionRange[index][_pRules[noRules]._pMaxCond[index]] = val3;            // upper bound
		}
		//printf("case continuous\n");
		//printf("_pConditionStatus[%d][%d] = %d\n", index, _pRules[noRules]._pMaxCond[index],
		//	_pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]]);
		//printf("_pConditionValue[%d][%d] = %f\n", index, _pRules[noRules]._pMaxCond[index],
		//	_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._cont_val);
		//printf("_pMaxCond[%d] = %d\n", index, _pRules[noRules]._pMaxCond[index]);
		//if(_pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] == 3)
		//	printf("_pConditionRange[%d][%d] = %f\n", index, _pRules[noRules]._pMaxCond[index],
		//	_pRules[noRules]._pConditionRange[index][_pRules[noRules]._pMaxCond[index]]);
		//else printf("\n");
		free(val2);
		return 0;
	}
	else {    // case DISCRETE with KNOWN VALUES
		char * val1 = (char *) calloc(strlen(rules[indexRule]->getValueByIndex(indexValue))+1, sizeof(char));
		char * val2 = (char *) calloc(10, sizeof(char));
		int ret = sscanf(rules[indexRule]->getValueByIndex(indexValue), "%s%*[,]%s", val1, val2);
		if(!((ret == 1)||(ret == 3))) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error: Unable to read the value . Unable to configure "
                "the data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: Unable to read the value ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 4;
		}
		if((ret == 3)&&(strcmp(val2, "!="))) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal condition for attribute . Unable "
                "to configure the data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)) +
                strlen(rules[indexRule]->getAttribute(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, " is an illegal condition for attribute ");
			strcat(_pszErrorMessage, rules[indexRule]->getAttribute(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 7;
		}
		int number2 = -1;
		for(int h = 1; h <= _pConfigure->MaxAttVal[number]; h ++) {
			if(!strcmp(_pConfigure->AttValName[number][h], val1)) number2 = h;
		}
		if(number2 == -1) {
			if(_pszErrorMessage != NULL) free(_pszErrorMessage);
			_pszErrorMessage = (char *) calloc(strlen("error:  is an illegal value for the attribute . Unable "
                "to configure the data generator. \0") + strlen(rules[indexRule]->getValueByIndex(indexValue)) +
                strlen(rules[indexRule]->getAttribute(indexValue)), sizeof(char));
			strcat(_pszErrorMessage, "error: ");
			strcat(_pszErrorMessage, rules[indexRule]->getValueByIndex(indexValue));
			strcat(_pszErrorMessage, " is an illegal value for the attribute ");
			strcat(_pszErrorMessage, rules[indexRule]->getAttribute(indexValue));
			strcat(_pszErrorMessage, ". Unable to configure the data generator. \0");
            if(pLogger) pLogger->logMsg("DataGenerator::analizeCondition", Logger::L_MildError,
                "%s\n", _pszErrorMessage);
			return 7;
		}
		_pRules[noRules]._pMaxCond[index] ++;
		_pRules[noRules]._pConditionStatus[index] = (short *) realloc(_pRules[noRules]._pConditionStatus[index],
            (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(short));
		if(ret == 3) _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 4;      // "!="
		else _pRules[noRules]._pConditionStatus[index][_pRules[noRules]._pMaxCond[index]] = 0;              // equal
		_pRules[noRules]._pConditionValue[index] = (Description) realloc(_pRules[noRules]._pConditionValue[index],
            (_pRules[noRules]._pMaxCond[index] + 1) * sizeof(AttValue));
		_pRules[noRules]._pConditionValue[index][_pRules[noRules]._pMaxCond[index]]._discr_val = number2;	// value
		free(val1);
		free(val2);
	}
	return 0;
}

bool DataGenerator::readFromFile(FILE * file, char * buffer, char * del)
{
	register char * Sp = buffer;
    register int c;
    while((c = getc(file)) == '|' || (c == ' ' || c == '\n' || c == '\t')) {
		if(c == '|') {
			while((c = getc(file)) != '\n');
		}
    }
    if(c == EOF) {
		if(del != NULL) (*del) = EOF;
		return false;
    }
    while(c != ':' && c != ',' && c != '\n' && c != '|' && c != EOF) {
		if(c == '.') {
	    	if((c = getc(file)) == '|' || (c == ' ' || c == '\n' || c == '\t')) break;
	    	*Sp++ = '.';
		}
		if(c == '\\') c = getc(file);
		*Sp++ = c;
		if(c == ' ') {
	    	while((c = getc(file)) == ' ');
		}
		else c = getc(file);
    }
    if(c == '|') {
    	while((c = getc(file)) != '\n');
    }
	if(del != NULL) (*del) = c;
    while((*(Sp-1)) == ' ' || (*(Sp-1)) == '\n' || (*(Sp-1)) == '\t') Sp--;
    *Sp++ = '\0';
    return true;
}
