/*************************************************************************/
/*									 */
/*  Main routine for constructing sets of production rules from trees	 */
/*  -----------------------------------------------------------------	 */
/*									 */
/*************************************************************************/

#include "c4.5rules.h"

#include "defns.h"

#include <stdlib.h>
#include <string.h>

// External data.  Note: uncommented variables have the same meaning as for decision trees
short MaxAtt, MaxClass, MaxDiscrVal, VERBOSITY, TRIALS;
ItemNo MaxItem;
Description	* Item;
DiscrValue * MaxAttVal;
char * SpecialStatus;
String * ClassName, * AttName, * * AttValName;
Boolean	UNSEENS;
Boolean SIGTEST = false;		// use significance test in rule pruning
Boolean SIMANNEAL = false;		// use simulated annealing
float SIGTHRESH = 0.05f, CF;
float REDUNDANCY = 1.0f;		// factor that guesstimates the amount of redundancy and irrelevance in the attributes
float AttTestBits;			// bits to encode tested att
float * BranchBits;			// ditto attribute value
PR * Rule;				// current rules
RuleNo NRules = 0;			// number of current rules
RuleNo * RuleIndex;			// rule index
short RuleSpace = 0;			// space allocated for rules
ClassNo	DefaultClass;			// current default class
RuleSet	* PRSet;			// sets of rulesets

/********************************************************************/

/*  options setting   */

/********************************************************************/

void setRulesVerbosity(int level)
{
    if(level == 0) VERBOSITY = 0;
    else VERBOSITY = 1;
}

Boolean setRulesConfidence(float confidence)
{
    if(Check(confidence, 0, 100)){
        CF = confidence / 100;
        return true;
    }
    else return false;
}

Boolean setRulesRedundancy(float redundancy)
{
    if(Check(redundancy, 0, 10000)){
        REDUNDANCY = redundancy;
        return true;
    }
    else return false;
}

Boolean setRulesSignificanceTest(float thresh)
{
    if(Check(thresh, 0, 100)){
        SIGTHRESH = thresh / 100;
        SIGTEST = true;
        return true;
    }
    else return false;
}

void setRulesAnnealing(Boolean set)
{
    SIMANNEAL = true;
}

/********************************************************************/
/*  this function tests a given rule set on a given dataset         */
/********************************************************************/

void testRuleset(ruleOptions * options, Configure * ruleConf, int maxI, Description * items, processRulesResults * ruleResult, Boolean drop)
{
    void EvaluateRulesets(Boolean DeleteRules, processRulesResults * ruleResults);
    // initialize
    CF = options->pruneConfidence;
    REDUNDANCY = options->redundancy;
    SIGTHRESH = options->fisherThresh;
    SIGTEST = options->isFisherInUse;
    SIMANNEAL = options->annealing;
    VERBOSITY = options->verbosity;
    MaxClass = ruleConf->MaxClass;
    ClassName = ruleConf->ClassName;
    MaxAtt = ruleConf->MaxAtt;
    AttName = ruleConf->AttName;
    MaxAttVal = ruleConf->MaxAttVal;
    AttValName = ruleConf->AttValName;
    SpecialStatus = ruleConf->SpecialStatus;
    MaxDiscrVal = ruleConf->MaxDiscrVal;
    MaxItem = maxI;
    Item = items;
    Verbosity(1) printf("\nRead %d cases (%d attributes) from the test dataset\n", MaxItem+1, MaxAtt+1);
    EvaluateRulesets(drop, ruleResult);
    // C'E' UN ERRORE NELL'ESECUZIONE DEL TEST: I RISULTATI IN C45RULESETTESTINFO SONO SBAGLIATI!!!!
}

/*********************************************************************/
/*  create a new rule set given an unpruned tree and a dataset       */
/*********************************************************************/

processRulesResults * createRules(ruleOptions * options, Configure * ruleConf, int maxI, Description * items, processTreeResults * unpruned)
{
    processRulesResults * GenerateRulesFromTree(treeResults * unpruned, int no);
    void GenerateLogs(void);
    void EvaluateRulesets(Boolean DeleteRules, processRulesResults * ruleResults);
    processRulesResults * resultedRules;
    // initialize
    CF = options->pruneConfidence;
    REDUNDANCY = options->redundancy;
    SIGTHRESH = options->fisherThresh;
    SIGTEST = options->isFisherInUse;
    SIMANNEAL = options->annealing;
    VERBOSITY = options->verbosity;
    MaxClass = ruleConf->MaxClass;
    ClassName = ruleConf->ClassName;
    MaxAtt = ruleConf->MaxAtt;
    AttName = ruleConf->AttName;
    MaxAttVal = ruleConf->MaxAttVal;
    AttValName = ruleConf->AttValName;
    SpecialStatus = ruleConf->SpecialStatus;
    MaxDiscrVal = ruleConf->MaxDiscrVal;
    MaxItem = maxI;
    Item = items;
    Verbosity(1) printf("\nRead %d cases (%d attributes) from the tree\n", MaxItem+1, MaxAtt+1);
    GenerateLogs();
    printf("dentro createRules, prima di GenerateRulesFromTree\n");
    // ogni tanto ci sono rules con lo stesso Lhs, capire perche'!!!!!!!!!
    resultedRules = GenerateRulesFromTree(unpruned->trees, 1);
    printf("dentro createRules, dopo GenerateRulesFromTree\n");
    if(resultedRules->codeErrors != NULL) {
        return resultedRules;
    }
    Verbosity(1) printf("\n\nEvaluation on training data (%d items):\n", MaxItem+1);
    EvaluateRulesets(true, resultedRules);
    //int x;
    //int y = RuleSpace-1;
    //ForEach(x, 0, y) {
    //	printf("rule[%d].better = %d\n", x, resultedRules->set[0].SRule[x].better);
    //	printf("rule[%d].Bits = %f\n", x, resultedRules->set[0].SRule[x].Bits);
    //	printf("rule[%d].Error = %f\n", x, resultedRules->set[0].SRule[x].Error);
    //	printf("rule[%d].errorRate = %f\n", x, resultedRules->set[0].SRule[x].errorRate);
    //	printf("rule[%d].Incorrect = %d\n", x, resultedRules->set[0].SRule[x].Incorrect);
    //	printf("rule[%d].Lhs = %p\n", x, resultedRules->set[0].SRule[x].Lhs);
    //	printf("rule[%d].Rhs = %d\n", x, resultedRules->set[0].SRule[x].Rhs);
    //	printf("rule[%d].Size = %d\n", x, resultedRules->set[0].SRule[x].Size);
    //	printf("rule[%d].Used = %d\n", x, resultedRules->set[0].SRule[x].Used);
   	//	printf("rule[%d].worse = %d\n", x, resultedRules->set[0].SRule[x].worse);
    //}
    free(BranchBits);
    return resultedRules;
}
