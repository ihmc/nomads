/*************************************************************************/
/*									 */
/*	Evaluatation of rulesets					 */
/*	------------------------					 */
/*									 */
/*************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "defns.h"
#include "types.h"
#include "extern.h"
#include "rulex.h"

// functions
void EvaluateRulesets(Boolean DeleteRules, processRulesResults * ruleResults);
ItemNo Interpret(ItemNo Fp, ItemNo Lp, Boolean DeleteRules, RuleSet * set);
RuleNo BestRuleIndex(Description CaseDesc, RuleNo Start);

/*************************************************************************/
/*									 */
/*	Evaluate all rulesets						 */
/*									 */
/*************************************************************************/

void EvaluateRulesets(Boolean DeleteRules, processRulesResults * ruleResults)
{
	ItemNo Interpret(ItemNo Fp, ItemNo Lp, Boolean DeleteRules, RuleSet * set);
    short t;
    ItemNo * Errors;
    if(ruleResults->nSet == 1) {
    	NRules = ruleResults->set[0].SNRules;
    	Rule = ruleResults->set[0].SRule;
    	RuleIndex = ruleResults->set[0].SRuleIndex;
    	DefaultClass = ruleResults->set[0].SDefaultClass;
		if(ruleResults->set[0].testResults == NULL) {
			ruleResults->set[0].testResults = (testRes2 *) malloc(sizeof(testRes2));
			ruleResults->set[0].testResults->confusionMatrix = NULL;
		}
		else {
			if(ruleResults->set[0].testResults->confusionMatrix != NULL) free(ruleResults->set[0].testResults->confusionMatrix);
			ruleResults->set[0].testResults->confusionMatrix = NULL;
		}
		Interpret(0, MaxItem, DeleteRules, &(ruleResults->set[0]));
		// INIZIO CODICE NUOVO!!!
		if(DeleteRules) {
			RuleNo * newIndex = (RuleNo *) calloc(NRules + 1, sizeof(RuleNo));
			memcpy(newIndex, RuleIndex, (NRules + 1)*sizeof(RuleNo));
			free(RuleIndex);
			ruleResults->set[0].SRuleIndex = newIndex;
			ruleResults->set[0].SNRules = NRules;
			ruleResults->set[0].SRule = Rule;
			if(ruleResults->set[0].SDefaultClass != DefaultClass) {
				ruleResults->set[0].SDefaultClass = DefaultClass;
				free(ruleResults->set[0].DefaultClassName);
				ruleResults->set[0].DefaultClassName = (char *) calloc(strlen(ClassName[DefaultClass]) + 1, sizeof(char));
				strcat(ruleResults->set[0].DefaultClassName, ClassName[DefaultClass]);
			}
		}
		// FINE CODICE NUOVO!!!
		return;
    }
    Errors = (ItemNo *) malloc((ruleResults->nSet+1) * sizeof(ItemNo));
    ForEach(t, 0, ruleResults->nSet) {
		NRules = ruleResults->set[t].SNRules;
		Rule = ruleResults->set[t].SRule;
		RuleIndex = ruleResults->set[t].SRuleIndex;
		DefaultClass = ruleResults->set[t].SDefaultClass;
		Verbosity(1) {
			if(t < ruleResults->nSet) printf("\nRuleset %d:\n", t);
			else  printf("\nComposite ruleset:\n");
		}
		if(ruleResults->set[t].testResults == NULL) {
			ruleResults->set[t].testResults = (testRes2 *) malloc(sizeof(testRes2));
			ruleResults->set[t].testResults->confusionMatrix = NULL;
		}
		else {
			if(ruleResults->set[t].testResults->confusionMatrix != NULL) free(ruleResults->set[t].testResults->confusionMatrix);
			ruleResults->set[t].testResults->confusionMatrix = NULL;
		}
		Errors[t] = Interpret(0, MaxItem, DeleteRules, &(ruleResults->set[t]));
		//if(DeleteRules) ruleResults->set[t].SNRules = NRules;		// VECCHIO!!!
		// INIZIO CODICE NUOVO!!!
		if(DeleteRules) {
			RuleNo * newIndex = (RuleNo *) calloc(NRules + 1, sizeof(RuleNo));
			memcpy(newIndex, RuleIndex, (NRules + 1)*sizeof(RuleNo));
			free(RuleIndex);
			ruleResults->set[0].SRuleIndex = newIndex;
			ruleResults->set[0].SNRules = NRules;
			ruleResults->set[0].SRule = Rule;
			if(ruleResults->set[0].SDefaultClass != DefaultClass) {
				ruleResults->set[0].SDefaultClass = DefaultClass;
				free(ruleResults->set[0].DefaultClassName);
				ruleResults->set[0].DefaultClassName = (char *) calloc(strlen(ClassName[DefaultClass]) + 1, sizeof(char));
				strcat(ruleResults->set[0].DefaultClassName, ClassName[DefaultClass]);
			}
		}
		// FINE CODICE NUOVO!!!
    }
    //  Print report
    Verbosity(1) {
    	printf("\n");
    	printf("Trial   Size      Errors\n");
    	printf("-----   ----      ------\n");
    }
    ForEach(t, 0, ruleResults->nSet) {
    	Verbosity(1) {
			if(t < ruleResults->nSet) printf("%4d", t);
			else printf("  **");
			printf("    %4d  %3d(%4.1f%%)\n", ruleResults->set[t].SNRules, Errors[t], 100 * Errors[t] / (MaxItem+1.0));
    	}
    	ruleResults->set[t].testResults->noErrors = Errors[t];
    	ruleResults->set[t].testResults->noItems = MaxItem + 1;
    	ruleResults->set[t].testResults->percErrors = (float)(100 * Errors[t] / (MaxItem + 1.0));
    }
}

/*************************************************************************/
/*									 */
/*	Evaluate current ruleset					 */
/*									 */
/*************************************************************************/

float	Confidence;		// certainty factor of fired rule (set by BestRuleIndex)

ItemNo Interpret(ItemNo Fp, ItemNo Lp, Boolean DeleteRules, RuleSet * set)
{
	void PrintConfusionMatrix(ItemNo * ConfusionMat);
    ItemNo i, Tested = 0, Errors = 0, * Better, * Worse;
    ItemNo * ConfusionMat;
    Boolean FoundRule;
    ClassNo AssignedClass, AltClass;
    Attribute Att;
    RuleNo p, Bestr, ri, ri2, riDrop = 0;
    float ErrorRate, BestRuleConfidence;
	ConfusionMat = (ItemNo *) calloc((MaxClass+1)*(MaxClass+1), sizeof(ItemNo));
    ForEach(ri, 1, NRules) {
		p = RuleIndex[ri];
		Rule[p].Used = Rule[p].Incorrect = 0;
    }
    Better = (ItemNo *) calloc(NRules+1, sizeof(ItemNo));
    Worse  = (ItemNo *) calloc(NRules+1, sizeof(ItemNo));
    ForEach(i, Fp, Lp) {
		//  Find first choice for rule for this item
		ri = BestRuleIndex(Item[i], 1);
		Bestr = (ri ? RuleIndex[ri] : 0);
		FoundRule = Bestr > 0;
		if(FoundRule) {
	    	Rule[Bestr].Used++;
	    	AssignedClass =  Rule[Bestr].Rhs;
	    	BestRuleConfidence = Confidence;
	    	//  Now find second choice
	    	ri2 = BestRuleIndex(Item[i], ri+1);
	    	AltClass = (ri2 ? Rule[RuleIndex[ri2]].Rhs : DefaultClass);
	    	if(AltClass != AssignedClass) {
				if(AssignedClass == Class(Item[i])) Better[ri]++;
				else if(AltClass == Class(Item[i])) Worse[ri]++;
	    	}
		}
		else AssignedClass = DefaultClass;
		ConfusionMat[Class(Item[i])*(MaxClass+1)+AssignedClass]++;
		Tested++;
		if(AssignedClass != Class(Item[i])) {
	    	Errors++;
	    	if(FoundRule) Rule[Bestr].Incorrect++;
	    	Verbosity(1) {
	    		printf("\n");
	    		ForEach(Att, 0, MaxAtt) {
	    	    	printf("\t%s: ", AttName[Att]);
	    	    	if(MaxAttVal[Att]) {
	    				if(DVal(Item[i],Att)) printf("%s\n", AttValName[Att][DVal(Item[i],Att)]);
	    				else printf("?\n");
	    	    	}
	    	    	else {
	    				if(CVal(Item[i],Att) != Unknown) printf("%g\n", CVal(Item[i],Att));
	    				else printf("?\n");
	    	    	}
	    		}
	    		printf("\t%4d:\tGiven class %s,", i, ClassName[Class(Item[i])]);
	    		if(FoundRule) printf(" rule %d [%.1f%%] gives class ", Bestr, 100 * BestRuleConfidence);
	    		else printf(" default class ");
	    		printf("%s\n", ClassName[AssignedClass]);
	    	}
		}
    }
    Verbosity(1) {
    	printf("\nRule  Size  Error  Used  Wrong\t          Advantage\n");
    	printf(  "----  ----  -----  ----  -----\t          ---------\n");
    }
    ForEach(ri, 1, NRules) {
		p = RuleIndex[ri];
		if(Rule[p].Used > 0) {
	    	ErrorRate = Rule[p].Incorrect / (float) Rule[p].Used;
	    	Verbosity(1) printf("%4d%6d%6.1f%%%6d%7d (%.1f%%)\t%6d (%d|%d) \t%s\n", p, Rule[p].Size, 100 * Rule[p].Error,
	    		Rule[p].Used, Rule[p].Incorrect, 100 * ErrorRate, Better[ri]-Worse[ri], Better[ri], Worse[ri], ClassName[Rule[p].Rhs]);
		    Rule[p].errorRate = 100 * ErrorRate;
		    Rule[p].better = Better[ri];
		    Rule[p].worse = Worse[ri];
	    	//  See whether this rule should be dropped.  Note: can only drop
			//	one rule at a time, because Better and Worse are affected
	    	if(DeleteRules && ! riDrop && Worse[ri] > Better[ri]) riDrop = ri;
		}
    }
    free(Better);
    free(Worse);
    if(riDrop) {
		int x, y;
		Verbosity(1) printf("\nDrop rule %d\n", RuleIndex[riDrop]);
		// INIZIO CODICE NUOVO!!!
    	ForEach(x, 1, Rule[RuleIndex[riDrop]].Size) {
    		if(Rule[RuleIndex[riDrop]].Lhs[x]->CondTest->NodeType == BrSubset) {
    			ForEach(y, 1, Rule[RuleIndex[riDrop]].Lhs[x]->CondTest->Forks) free(Rule[RuleIndex[riDrop]].Lhs[x]->CondTest->Subset[y]);
    			free(Rule[RuleIndex[riDrop]].Lhs[x]->CondTest->Subset);
    		}
    		free(Rule[RuleIndex[riDrop]].Lhs[x]->CondTest);
    		free(Rule[RuleIndex[riDrop]].Lhs[x]);
    	}
    	free(Rule[RuleIndex[riDrop]].Lhs);
		// FINE CODICE NUOVO!!!
		ForEach(ri, riDrop+1, NRules) RuleIndex[ri-1] = RuleIndex[ri];
		NRules--;
		set->testResults->confusionMatrix = NULL;
		free(ConfusionMat);
		return Interpret(Fp, Lp, DeleteRules, set);
    }
    else {
		Verbosity(1) printf("\nTested %d, errors %d (%.1f%%)\n", Tested, Errors, 100 * Errors / (float) Tested);
    	set->testResults->noItems = Tested;
    	set->testResults->noErrors = Errors;
    	set->testResults->percErrors = 100 * Errors / (float) Tested;
    }
    set->testResults->confusionMatrix = ConfusionMat;
    set->testResults->noClasses = MaxClass;
    printf("dentro Interpret, set->testResults->noItems = %d, set->testResults->noErrors = %d, set->testResults->percErrors = %f, set->testResults->noClasses = %d\n",
    set->testResults->noItems, set->testResults->noErrors, set->testResults->percErrors, set->testResults->noClasses);
	Verbosity(1) PrintConfusionMatrix(set->testResults->confusionMatrix);
    return Errors;
}

/*************************************************************************/
/*									 */
/*	Find the best rule for the given case, leaving probability       */
/*      in Confidence							 */
/*									 */
/*************************************************************************/

RuleNo BestRuleIndex(Description CaseDesc, RuleNo Start)
/* ------------  */
{
	float Strength(PR ThisRule, Description Case);
    RuleNo r, ri;
    ForEach(ri, Start, NRules) {
		r = RuleIndex[ri];
		Confidence = Strength(Rule[r], CaseDesc);
		if(Confidence > 0.1) return ri;
    }
    Confidence = 0.0;
    return 0;
}
