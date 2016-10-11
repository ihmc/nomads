/*************************************************************************/
/*	Main routine, c4.5						 */
/*	------------------						 */
/*************************************************************************/

#include "c4.5.h"

#include "defns.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// External data, described in extern.h
short MaxAtt, MaxClass, MaxDiscrVal = 2, VERBOSITY, TRIALS = 10;
ItemNo MaxItem, MINOBJS = 2, INCREMENT = 0;
ItemNo WINDOW = 0; 		// initial window
Description	* Item;
DiscrValue * MaxAttVal;
char * SpecialStatus;
String * ClassName, * AttName, * * AttValName;
Tree * Pruned;
Boolean	AllKnown = true, GAINRATIO = true, SUBSET = false, BATCH = true, PROBTHRESH = false;
float CF;

extern Tree * Raw;  	// contains the unpruned tree
extern char Delimiter;	// used by ReadValue()

/*************************************************************************/
/*   options setting                                                     */
/*************************************************************************/

void setTreeSubsetting(Boolean subset)
{
    SUBSET = subset;
}

void setTreeProbabilisticThresholds(Boolean thresholds)
{
    PROBTHRESH = thresholds;
}

void setTreeBatchMode(void)
{
    BATCH = true;
}

Boolean setTreeIterativeMode(int trials)
{
    if(Check(trials, 1, 10000)){
        BATCH = false;
        TRIALS = trials;
        return true;
    }
    else return false;
}

void setTreeVerbosity(int level)
{
    if(level == 0) VERBOSITY = 0;
    else VERBOSITY = 1;
}

Boolean setTreeInitialWindow(int window)
{
    if(Check(window, 1, 1000000)){
        BATCH = false;
        WINDOW = window;
        return true;
    }
    else return false;
}

Boolean setTreeMaxIncrement(int increment)
{
    if(Check(increment, 1, 1000000)){
        BATCH = false;
        INCREMENT = increment;
        return true;
    }
    else return false;
}

void setTreeGainRatioCriterion(Boolean ratio)
{
    GAINRATIO = ratio;
}

Boolean setTreeMinObjects(int minimum)
{
    if(Check(minimum, 1, 1000000)){
        MINOBJS = minimum;
        return true;
    }
    else return false;
}

Boolean setTreeConfidence(float confidence)
{
    if(Check(confidence, Epsilon, 100)){
        CF = confidence / 100;
        return true;
    }
    else return false;
}

/*****************************************************************************/
/*  this function constructs trees for the cycle iterative mode              */
/*****************************************************************************/

int iterativeMode(treeOptions * options, Configure * treeConf, int win, int maxI, Description * items, processTreeResults * treeSet, int * noTrees)
{
    int BestTree(processTreeResults * treeSet);
    //  Initialise
    WINDOW = Max(win * options->cycleWindow, 1);
    INCREMENT = Max((maxI - win) * options->cycleErrors, 1);
    PROBTHRESH = options->probThresh;
    GAINRATIO = options->gainCriterion;
    CF = options->pruneConfidence;
    VERBOSITY = options->verbosity;
    SUBSET = options->subsetting;
    MINOBJS = options->minObjects;
    MaxClass = treeConf->MaxClass;
    ClassName = treeConf->ClassName;
    MaxAtt = treeConf->MaxAtt;
    AttName = treeConf->AttName;
    MaxAttVal = treeConf->MaxAttVal;
    AttValName = treeConf->AttValName;
    SpecialStatus = treeConf->SpecialStatus;
    MaxDiscrVal = treeConf->MaxDiscrVal;
    MaxItem = maxI;
    Item = items;
    (*noTrees) = BestTree(treeSet);
    return WINDOW-1;
}

/****************************************************************************/
/*  this function is used to fill the Configure structure for trees         */
/****************************************************************************/

CError * getNames(Configure * treeConf, const char * attribute, const char * value)
{
    char * CopyString(char * x);
    void resetI(void);
    Boolean ReadValue(const char * phrase, String value, Attribute position);
    char Buffer[400];
    Boolean returnValue;     // check the return value of ReadValue
    CError * errOcc;         // error message
    DiscrValue v;
    int AttCeiling=10;       // predict 10 different attributes
    int ClassCeiling = 2;    // the default number of classes is 2
    int ValCeiling;          // predict number of value for each attribute
    // check if the passed attribute is a class
    if(strcmp(attribute, "CLASS") == 0) {
    	//  Get class names from string value
    	treeConf->ClassName = (String *) calloc(ClassCeiling, sizeof(String));
    	treeConf->MaxClass = -1;
    	resetI();    // initialize the counter for ReadValue()
    	Verbosity(1) printf("\nreading classes from string names\n");
    	do {
            if(++(treeConf->MaxClass) >= ClassCeiling) {
                ClassCeiling ++;  // add space for another class
                treeConf->ClassName = (String *) realloc(treeConf->ClassName, ClassCeiling*sizeof(String));
            }
            returnValue = ReadValue(value, Buffer, 0);
            if(!returnValue) {
                errOcc = (CError *) malloc(sizeof(CError));
                errOcc->errorCode = 2;
                errOcc->errorMessage ="error: unable to read the name of one of the classes. Unable to configure the tree. \0";
                return errOcc;
            }
            treeConf->ClassName[treeConf->MaxClass] = CopyString(Buffer);
            Verbosity(1) printf("class %d: :%s:\n", treeConf->MaxClass, treeConf->ClassName[treeConf->MaxClass]);
    	} while(Delimiter == ',');
    	return NULL;
    }
    else {
    	Verbosity(1) printf("\nreading attributes and values names from string attribute and value\n");
    	if(++MaxAtt >= AttCeiling) {   // correct sintax: "attributeName: value1, value2."
            AttCeiling += 5;           // add space for other 5 attributes
            AttName = (String *) realloc(AttName, AttCeiling*sizeof(String));
            MaxAttVal = (DiscrValue *) realloc(MaxAttVal, AttCeiling*sizeof(DiscrValue));
            AttValName = (String **) realloc(AttValName, AttCeiling*sizeof(String *));
            SpecialStatus = (char *) realloc(SpecialStatus, AttCeiling);
        }
    	treeConf->MaxAtt ++;
    	treeConf->AttName[treeConf->MaxAtt] = CopyString((char *)attribute);
        Verbosity(1) printf("attribute %d: :%s:\n", treeConf->MaxAtt, treeConf->AttName[treeConf->MaxAtt]);
        treeConf->SpecialStatus[treeConf->MaxAtt] = Nil;
        treeConf->MaxAttVal[treeConf->MaxAtt] = 0;  // initial value
        ValCeiling = 1;         // allocate space for 1 value for this attribute (AttName[MaxAtt])
        treeConf->AttValName[treeConf->MaxAtt] = (String *) calloc(ValCeiling, sizeof(String));
    	resetI();    			// initialize the counter for ReadValue()
    	do { 					// reading the values for this attribute
            if(!(ReadValue(value, Buffer, 0))) {
                errOcc = (CError *) malloc(sizeof(CError));
                errOcc->errorCode = 4;
                errOcc->errorMessage = (char *)calloc(strlen("error: unable to read values of attribute ")+strlen(Buffer)+3, sizeof(char));
                strcat(errOcc->errorMessage, "error: unable to read values of attribute ");
                strcat(errOcc->errorMessage, Buffer);
                strcat(errOcc->errorMessage, ". ");
                return errOcc;
            }
            if(++(treeConf->MaxAttVal[treeConf->MaxAtt]) >= ValCeiling) {
                ValCeiling += 5;  // add space for other 5 values
                treeConf->AttValName[treeConf->MaxAtt] = (String *) realloc(treeConf->AttValName[treeConf->MaxAtt], ValCeiling*sizeof(String));
            }
            treeConf->AttValName[treeConf->MaxAtt][treeConf->MaxAttVal[treeConf->MaxAtt]] = CopyString(Buffer);
            Verbosity(1) printf("\tvalue %d: :%s:\n", treeConf->MaxAttVal[treeConf->MaxAtt], treeConf->AttValName[treeConf->MaxAtt][treeConf->MaxAttVal[treeConf->MaxAtt]]);
	} while(Delimiter == ',');
    	Verbosity(1) printf("\n");
        if(treeConf->MaxAttVal[treeConf->MaxAtt] == 1) {
            //  Check for special treatment
            if(! strcmp(Buffer, "CONTINUOUS")) {}  // no special status in this case
            else
            if(! memcmp(Buffer, "DISCRETE", 8)) {
                treeConf->SpecialStatus[treeConf->MaxAtt] = DISCRETE;
                //  Read max values, reserve space and check MaxDiscrVal
                v = atoi(&Buffer[8]);  // take the number after "discrete"
                if(v < 2) {
                    errOcc = (CError *) malloc(sizeof(CError));
                    errOcc->errorCode = 5;
                    errOcc->errorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")+strlen(Buffer)+3, sizeof(char));
                    strcat(errOcc->errorMessage, "error: illegal number of discrete values (< 2) for attribute ");
                    strcat(errOcc->errorMessage, treeConf->AttName[treeConf->MaxAtt]);
                    strcat(errOcc->errorMessage, ". ");
                    return errOcc;
                }
                treeConf->AttValName[treeConf->MaxAtt] = (String *) realloc(treeConf->AttValName[treeConf->MaxAtt], (v+2)*sizeof(String));
                treeConf->AttValName[treeConf->MaxAtt][0] = (char *) v;
                if(v > treeConf->MaxDiscrVal) treeConf->MaxDiscrVal = v;
            }
            else if(! strcmp(Buffer, "IGNORE")) {
                treeConf->SpecialStatus[treeConf->MaxAtt] = IGNORE;
            }
            else {
                errOcc = (CError *) malloc(sizeof(CError));
                errOcc->errorCode = 5;
                errOcc->errorMessage = (char *)calloc(strlen("error: illegal number of discrete values (< 2) for attribute ")+strlen(Buffer)+3, sizeof(char));
                strcat(errOcc->errorMessage, "error: illegal number of discrete values (< 2) for attribute ");
                strcat(errOcc->errorMessage, treeConf->AttName[treeConf->MaxAtt]);
                strcat(errOcc->errorMessage, ". ");
                return errOcc;
            }
            treeConf->MaxAttVal[treeConf->MaxAtt] = 0;
        }
        else if(treeConf->MaxAttVal[treeConf->MaxAtt] > treeConf->MaxDiscrVal) treeConf->MaxDiscrVal = treeConf->MaxAttVal[treeConf->MaxAtt];
    }
    return NULL;
}

/******************************************************************************/
/* this function retrieves data from the given dataset and puts               */
/* the data in the Item table                                                 */
/******************************************************************************/

CError * getDataset(Configure * treeConf, const char * attribute, const char * value, int _MaxItem, Description * _Item)
{
	char * CopyString(char * x);
	int Which(String Val, String List[], short First, short Last);
	CError * errOcc;
	int i;
	if(!strcmp(attribute, "CLASS")) {
            if((i = Which((char *) value, treeConf->ClassName, 0, treeConf->MaxClass)) < 0) {
                errOcc = (CError *) malloc(sizeof(CError));
                errOcc->errorCode = 2;
                errOcc->errorMessage = (char *) calloc(strlen("error:  is not a valid class name. Unable to process the data. ")+strlen(value)+1, sizeof(char));
                strcat(errOcc->errorMessage, "error: ");
                strcat(errOcc->errorMessage, value);
                strcat(errOcc->errorMessage, " is not a valid class name. Unable to process the data. ");
                return errOcc;
            }
            _Item[_MaxItem][treeConf->MaxAtt+1]._discr_val = i;
            return NULL;
	}
	else {
		ForEach(i, 0, treeConf->MaxAtt + 1) {
                    if(!strcmp(attribute, treeConf->AttName[i])) break;
		}
		if(i > treeConf->MaxAtt) {
                    errOcc = (CError *) malloc(sizeof(CError));
                    errOcc->errorCode = 3;
                    errOcc->errorMessage = (char *) calloc(strlen("error:  is not a valid attribute name. Unable to process the data. ")+strlen(attribute)+1, sizeof(char));
                    strcat(errOcc->errorMessage, "error: ");
                    strcat(errOcc->errorMessage, attribute);
                    strcat(errOcc->errorMessage, " is not a valid attribute name. Unable to process the data. ");
                    return errOcc;
		}
		if(treeConf->SpecialStatus[i] == IGNORE) {
                    _Item[_MaxItem][i]._discr_val = 0;
		}
		else if(treeConf->MaxAttVal[i] || treeConf->SpecialStatus[i] == DISCRETE) {
                    short dval;
                    if(!(strcmp(value, "UNKNOWN"))) {
                        //_Item[_MaxItem][i]._discr_val = 0;
                        dval = 0;
                    }
                    else {
                        dval = Which((char *) value, treeConf->AttValName[i], 1, treeConf->MaxAttVal[i]);
                        if(!dval) {
                            if(treeConf->SpecialStatus[i] == DISCRETE) {
                                dval = ++(treeConf->MaxAttVal[i]);
                                if(dval > (int)treeConf->AttValName[i][0]) {
                                    errOcc = (CError *) malloc(sizeof(CError));
                                    errOcc->errorCode = 6;
                                    errOcc->errorMessage = (char *)calloc(strlen("error: too many values for attribute  (max number of value is )")+
                                    strlen(treeConf->AttName[i])+strlen(treeConf->AttValName[i][0])+3, sizeof(char));
                                    strcat(errOcc->errorMessage, "error: too many values for attribute ");
                                    strcat(errOcc->errorMessage, treeConf->AttName[i]);
                                    strcat(errOcc->errorMessage, " (max number of value is ");
                                    strcat(errOcc->errorMessage, treeConf->AttValName[i][0]);
                                    strcat(errOcc->errorMessage, " ). ");
                                    return errOcc;
                                }
                                treeConf->AttValName[i][dval] = CopyString((char *)value);
                                _Item[_MaxItem][i]._discr_val = dval;
                            }
                            else {
                                errOcc = (CError *) malloc(sizeof(CError));
                                errOcc->errorCode = 7;
                                errOcc->errorMessage = (char *)calloc(strlen("error: value  for attribute  is illegal")+
                                strlen(treeConf->AttName[i])+strlen(value)+3, sizeof(char));
                                strcat(errOcc->errorMessage, "error: value ");
                                strcat(errOcc->errorMessage, value);
                                strcat(errOcc->errorMessage, " for attribute ");
                                strcat(errOcc->errorMessage, treeConf->AttName[i]);
                                strcat(errOcc->errorMessage, " is illegal. ");
                                return errOcc;
                            }
                        }
                    }
                    _Item[_MaxItem][i]._discr_val = dval;
		}
	    else {
	    	float cval;
	    	char * endp;
	        if(!(strcmp(value, "UNKNOWN"))) {
	        	_Item[_MaxItem][i]._cont_val = Unknown;
	        }
	        else {
                    cval = (float) strtod(value, &endp);
                    if(endp == value || *endp != '\0' ){
                        errOcc = (CError *) malloc(sizeof(CError));
                        errOcc->errorCode = 7;
                        errOcc->errorMessage = (char *)calloc(strlen("error: value  for attribute  is illegal")+
                                strlen(treeConf->AttName[i])+strlen(value)+3, sizeof(char));
                        strcat(errOcc->errorMessage, "error: value ");
                        strcat(errOcc->errorMessage, value);
                        strcat(errOcc->errorMessage, " for attribute ");
                        strcat(errOcc->errorMessage, treeConf->AttName[i]);
                        strcat(errOcc->errorMessage, " is illegal. ");
                        return errOcc;
                    }
                    _Item[_MaxItem][i]._cont_val = cval;
		}
	    }
	}
	return NULL;
}

/******************************************************************************/
/* constructs a simple tree on the given dataset (Items)                      */
/******************************************************************************/

processTreeResults * constructTree(treeOptions * options, Configure * treeConf, int maxI, Description * items)
{
    Tree CopyTree(Tree T);
    void OneTree(void);
    void SoftenThresh(Tree T);
    void PrintTree(Tree T);
    void deleteTree(Tree t);
    short Best;
    processTreeResults * resultedTree;
    int xz;
    //  Initialise
    WINDOW = options->initialWindow;
    INCREMENT = options->increment;
    PROBTHRESH = options->probThresh;
    GAINRATIO = options->gainCriterion;
    CF = options->pruneConfidence;
    VERBOSITY = options->verbosity;
    SUBSET = options->subsetting;
    MINOBJS = options->minObjects;
    MaxClass = treeConf->MaxClass;
    ClassName = treeConf->ClassName;
    MaxAtt = treeConf->MaxAtt;
    AttName = treeConf->AttName;
    MaxAttVal = treeConf->MaxAttVal;
    AttValName = treeConf->AttValName;
    SpecialStatus = treeConf->SpecialStatus;
    MaxDiscrVal = treeConf->MaxDiscrVal;
    MaxItem = maxI;
    Item = items;
    resultedTree = (processTreeResults *) malloc(sizeof(processTreeResults));
    Verbosity(1) printf("\nRead %d cases (%d attributes) from data\n", MaxItem+1, MaxAtt+1);
    BATCH = true;
    TRIALS = 1;
    OneTree();
    Best = 0;
    //  Soften thresholds in best tree
    if(PROBTHRESH) {
        Verbosity(1) {
                printf("Softening thresholds");
                if(! BATCH) printf(" for best tree from trial %d", Best);
                printf("\n");
        }
        SoftenThresh(Pruned[Best]);
        Verbosity(1) {
                printf("\n");
                PrintTree(Pruned[Best]);
        }
    }
    resultedTree->trees = (treeResults *)calloc(TRIALS+1, sizeof(treeResults)); // array of treeResults
    ForEach(xz, 0, TRIALS-1) {
    	resultedTree->trees[xz] = (treeResults) malloc(sizeof(TRes));
    	resultedTree->trees[xz]->tree = Raw[xz];
    	resultedTree->trees[xz]->isPruned = 0;
    	resultedTree->trees[xz]->testResults = NULL;
    }
    resultedTree->nTrees = TRIALS+1;
    resultedTree->trees[TRIALS] = (treeResults) malloc(sizeof(TRes));
    resultedTree->trees[TRIALS]->tree = Pruned[Best];
    resultedTree->trees[TRIALS]->isPruned = 1;
    resultedTree->trees[TRIALS]->testResults = NULL;
    resultedTree->codeErrors = NULL;
   	//printf("\nprint of unpruned tree\n");
    //PrintTree(resultedTree->trees[0]->tree);
    //printf("\nprint of pruned tree\n");
    //PrintTree(resultedTree->trees[1]->tree);
    free(Pruned);
    free(Raw);
    return resultedTree;
}

/******************************************************************************/
/*  test the given tree on the given dataset                                  */
/******************************************************************************/

void testTree(treeOptions * options, Configure * treeConf, int maxI, Description * items, processTreeResults * treeSet, short testFlag)
{
    ClassNo Category(Description CaseDesc, Tree DecisionTree);
    int TreeSize(Tree t);
    void PrintConfusionMatrix(ItemNo * ConfusionMat);
    ClassNo RealClass, PrunedClass;
    short t, num;
    ItemNo i, PrunedErrors;
    if(treeSet->nTrees < 1){
    	treeSet->codeErrors = (CError *) malloc(sizeof(CError));
    	treeSet->codeErrors->errorCode = 9;
    	treeSet->codeErrors->errorMessage = (char *) calloc(strlen("error: there isn't any tree to test. Unable to test any tree. ")+1, sizeof(char));
    	treeSet->codeErrors->errorMessage = "error: there isn't any tree to test. Unable to test any tree. ";
    	return;
    }
    //  Initialize
    VERBOSITY = options->verbosity;
    SUBSET = options->subsetting;
    MINOBJS = options->minObjects;
    MaxClass = treeConf->MaxClass;
    ClassName = treeConf->ClassName;
    MaxAtt = treeConf->MaxAtt;
    AttName = treeConf->AttName;
    MaxAttVal = treeConf->MaxAttVal;
    AttValName = treeConf->AttValName;
    SpecialStatus = treeConf->SpecialStatus;
    MaxDiscrVal = treeConf->MaxDiscrVal;
    MaxItem = maxI;
    Item = items;
    Verbosity(1) printf("\nEvaluation on test data (%d items):\n", MaxItem+1);
    Verbosity(1) printf("\n");
    if(testFlag == 0) {
        if(treeSet->trees[0]->isPruned != 0) {
            treeSet->codeErrors = (CError *) malloc(sizeof(CError));
            treeSet->codeErrors->errorCode = 9;
            treeSet->codeErrors->errorMessage = (char *) calloc(strlen("error: there isn't any unpruned tree to test. Unable to test the tree. ")+1, sizeof(char));
            treeSet->codeErrors->errorMessage = "error: there isn't any unpruned tree to test. Unable to test the tree. ";
            return;
        }
        num = 0;
    }
    if(testFlag == 1) {
        if(treeSet->trees[0]->isPruned != 1) {
            if(treeSet->nTrees < 2) {
                treeSet->codeErrors = (CError *) malloc(sizeof(CError));
                treeSet->codeErrors->errorCode = 9;
                treeSet->codeErrors->errorMessage = (char *) calloc(strlen("error: there isn't any pruned tree to test. Unable to test the tree. ")+1, sizeof(char));
                treeSet->codeErrors->errorMessage = "error: there isn't any pruned tree to test. Unable to test the tree. ";
                return;
            }
            num = 1;
        }
    }
    if((testFlag == 2)&&(treeSet->nTrees < 2)) {
            treeSet->codeErrors = (CError *) malloc(sizeof(CError));
    treeSet->codeErrors->errorCode = 9;
    treeSet->codeErrors->errorMessage = (char *) calloc(strlen("error: there aren't all the trees to test. Unable to test the trees. ")+1, sizeof(char));
    treeSet->codeErrors->errorMessage = "error: there aren't all the trees to test. Unable to test the trees. ";
    return;
    }
    if((testFlag == 0)||(testFlag == 1)) {
        Verbosity(1) {
        if(testFlag == 0) printf("Unpruned Tree\t results from test\n");
        else printf("Pruned Tree\t results from test\n");
            printf("-----\t-----------------------\n");
            if(testFlag == 0) printf("\tSize      Errors\n\n");
            else printf("\tSize      Errors   Estimate\n\n");
        }
        if(treeSet->trees[num]->testResults == NULL) {
            treeSet->trees[num]->testResults = (testRes *) malloc(sizeof(testRes));
            treeSet->trees[num]->testResults->confusionMatrix = (ItemNo *) calloc((MaxClass+1)*(MaxClass+1), sizeof(ItemNo));
        }
        PrunedErrors = 0;
        ForEach(i, 0, MaxItem) {
        RealClass = Class(Item[i]);
        PrunedClass = Category(Item[i], treeSet->trees[num]->tree);
        if(PrunedClass != RealClass) PrunedErrors++;
            treeSet->trees[num]->testResults->confusionMatrix[RealClass*(MaxClass+1)+PrunedClass]++;
        }
        treeSet->trees[num]->testResults->treeSize = TreeSize(treeSet->trees[num]->tree);
        treeSet->trees[num]->testResults->noErrors = PrunedErrors;
        treeSet->trees[num]->testResults->noItems = MaxItem + 1;
        treeSet->trees[num]->testResults->percErrors = (float)(100.0 * PrunedErrors / (MaxItem + 1.0));
        treeSet->trees[num]->testResults->noClasses = MaxClass + 1;
        if(testFlag == 1) treeSet->trees[num]->testResults->estimate = 100 * treeSet->trees[num]->tree->Errors / treeSet->trees[num]->tree->Items;
        else treeSet->trees[num]->testResults->estimate = 0;
        Verbosity(1) {
            if(testFlag == 0) {
                printf("\t%4d  %3d(%4.1f%%)\n", treeSet->trees[num]->testResults->treeSize,
                treeSet->trees[num]->testResults->noErrors, treeSet->trees[num]->testResults->percErrors);
            }
            else {
                printf("\t%4d  %3d(%4.1f%%)    (%4.1f%%)\n", treeSet->trees[num]->testResults->treeSize,
                treeSet->trees[num]->testResults->noErrors, treeSet->trees[num]->testResults->percErrors,
                treeSet->trees[num]->testResults->estimate);
            }
            PrintConfusionMatrix(treeSet->trees[num]->testResults->confusionMatrix);
            printf("\n\n");
        }
        return;
    }
    if(testFlag == 2) {
        ForEach(t, 0, 1) {
            Verbosity(1) {
                    if(treeSet->trees[t]->isPruned == 0) printf("Unpruned Tree\t results from test %d\n", t);
                    else printf("Pruned Tree\t results from test %d\n", t);
                            printf("-----\t-----------------------\n");
                            if(treeSet->trees[t]->isPruned == 0) printf("\tSize      Errors\n\n");
                            else printf("\tSize      Errors   Estimate\n\n");
            }
            if(treeSet->trees[t]->testResults == NULL) {
                treeSet->trees[t]->testResults = (testRes *) malloc(sizeof(testRes));
                treeSet->trees[t]->testResults->confusionMatrix = (ItemNo *) calloc((MaxClass+1)*(MaxClass+1), sizeof(ItemNo));
            }
            PrunedErrors = 0;
            ForEach(i, 0, MaxItem) {
                RealClass = Class(Item[i]);
                PrunedClass = Category(Item[i], treeSet->trees[t]->tree);
                if(PrunedClass != RealClass) PrunedErrors++;
                treeSet->trees[t]->testResults->confusionMatrix[RealClass*(MaxClass+1)+PrunedClass]++;
            }
            treeSet->trees[t]->testResults->treeSize = TreeSize(treeSet->trees[t]->tree);
            treeSet->trees[t]->testResults->noErrors = PrunedErrors;
            treeSet->trees[t]->testResults->noItems = MaxItem + 1;
            treeSet->trees[t]->testResults->percErrors = (float)(100.0 * PrunedErrors / (MaxItem + 1.0));
            treeSet->trees[t]->testResults->noClasses = MaxClass + 1;
            if(treeSet->trees[t]->isPruned == 1) treeSet->trees[t]->testResults->estimate = 100 * treeSet->trees[t]->tree->Errors / treeSet->trees[t]->tree->Items;
            Verbosity(1) {
                if(treeSet->trees[t]->isPruned == 0) {
                        printf("\t%4d  %3d(%4.1f%%)\n", treeSet->trees[t]->testResults->treeSize,
                        treeSet->trees[t]->testResults->noErrors, treeSet->trees[t]->testResults->percErrors);
                }
                else {
                        printf("\t%4d  %3d(%4.1f%%)    (%4.1f%%)\n", treeSet->trees[t]->testResults->treeSize,
                        treeSet->trees[t]->testResults->noErrors, treeSet->trees[t]->testResults->percErrors,
                        treeSet->trees[t]->testResults->estimate);
                }
                PrintConfusionMatrix(treeSet->trees[t]->testResults->confusionMatrix);
                printf("\n\n");
            }
        }
        treeSet->codeErrors = NULL;
        return;
    }
}

/***************************************************************************/
/*   returns the number of nodes in the tree                               */
/***************************************************************************/

int treeDim(Tree tree)
{
    int TreeSize(Tree t);
    return TreeSize(tree);
}

/***************************************************************************/
/* delete a tree                                                           */
/***************************************************************************/

void deleteTree(Tree t)
{
    int i, a;
    if(t->NodeType) {
        ForEach(i, 1, t->Forks - 1) deleteTree(t->Branch[i]);
        free(t->Branch);
        if(t->NodeType == BrSubset) {
            ForEach(a, 0, MaxDiscrVal) {
                    free(t->Subset[a]);
            }
            free(t->Subset);
        }
    }
    free(t->ClassDist);
    free(t);
}
