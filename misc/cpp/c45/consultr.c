/*************************************************************************/
/*                                                                       */
/*	Classify items interactively using a set of rules		             */
/*	-------------------------------------------------		             */
/*								   	                                     */
/*************************************************************************/

#include "consultr.h"

#include "defns.h"

#include <stdlib.h>
#include <string.h>

// External data
short MaxAtt, MaxClass, MaxDiscrVal, VERBOSITY, TRACE, RuleSpace;
ItemNo MaxItem;
Description	* Item;
DiscrValue * MaxAttVal;
String * ClassName, * AttName, * * AttValName;
char * SpecialStatus;
Boolean	FirstTime = true;
PR * Rule;
RuleNo NRules, * RuleIndex;
ClassNo DefaultClass;

// The interview module uses a more complex description of an case called a
// "Range Description" with lower and upper bounds.
// For further information, see consult.c

typedef	struct ValRange * RangeDescRec;
struct ValRange {
	Boolean Known;
	Boolean Asked;
	float LowerBound;
	float UpperBound;
	float * Probability;
};

RangeDescRec RangeDesc;
float Confidence;

#define	MINCF	0.50		// minimum cf for useable rule

// functions declaration
RuleNo BestRule(char * * list, CError * * errOcc);
float RuleStrength(PR Rule, char * * list, CError * * errOcc);
float ProbSatisfied(Condition c, char * * list, CError * * errOcc);
consultRulesResults * InterpretRuleset(char * * list);

/*************************************************************************/
/*								  	 */
/*  Find the best rule for the current case.			  	 */
/*  Note:  leave probability in Confidence				 */
/*								  	 */
/*************************************************************************/

RuleNo BestRule(char * * list, CError * * errOcc)
{
	RuleNo r;
    float cf, RuleStrength(PR Rule, char * * list, CError * * errOcc);
    Confidence = 0.0;
    ForEach(r, 1, NRules) {
		cf = RuleStrength(Rule[r], list, errOcc);
		if((*errOcc) != NULL) return 0;
		if(cf > 0.3) {
	  		Confidence = cf;
	    	return r;
		}
    }
    return 0;
}

/*************************************************************************/
/*								    	 */
/*  Given a rule, determine the strength with which we can conclude	 */
/*  that the current case belongs to the class specified by the RHS	 */
/*  of the rule								 */
/*								    	 */
/*************************************************************************/

float RuleStrength(PR Rule, char * * list, CError * * errOcc)
{
	short d;
    float RuleProb=1.0, ProbSatisfied();
    ForEach(d, 1, Rule.Size) {
		RuleProb *= ProbSatisfied(Rule.Lhs[d], list, errOcc);
		if((*errOcc) != NULL) return 0.0;
		if(RuleProb < MINCF) {
	    	return 0.0;
		}
    }
    return ((1 - Rule.Error) * RuleProb);
}

/*************************************************************************/
/*							   		 */
/*  Determine the probability of the current case description		 */
/*  satisfying the given condition					 */
/*							   		 */
/*************************************************************************/

float ProbSatisfied(Condition c, char * * list, CError * * errOcc)
{
	CError * CheckValue(Attribute Att, Tree T, const char * phrase);
    Attribute a, xa;
    short v;
    float AddProb = 0.0;
    Test t;
    DiscrValue i;
    t = c->CondTest;
    a = t->Tested;
    v = c->TestValue;
    ForEach(xa, 0, MaxAtt + 1) {
    	if(!strncmp(list[xa], AttName[a], strlen(AttName[a]))) break;
    }  // now 'xa' contains the index for list to use in CheckValue()
    (*errOcc) = CheckValue(a, Nil, list[xa]);
    if((*errOcc) != NULL) return 0.0;
	if(!RangeDesc[a].Known) {
		return 0.0;
    }
    switch(t->NodeType) {
		case BrDiscr:  // test of discrete attribute
	    	return RangeDesc[a].Probability[v];
		case ThreshContin:  // test of continuous attribute
	    	if(RangeDesc[a].UpperBound <= t->Cut) {
				return ( v == 1 ? 1.0f : 0.0f );
	    	}
	    	else if(RangeDesc[a].LowerBound > t->Cut) {
				return ( v == 2 ? 1.0f : 0.0f );
	    	}
	   		else if(v == 1) {
				return (t->Cut - RangeDesc[a].LowerBound) / (RangeDesc[a].UpperBound - RangeDesc[a].LowerBound);
	    	}
	    	else {
				return (RangeDesc[a].UpperBound - t->Cut) / (RangeDesc[a].UpperBound - RangeDesc[a].LowerBound);
	    	}
		case BrSubset:  // subset test on discrete attribute
	    	ForEach(i, 1, MaxAttVal[a]) {
				if(In(i, t->Subset[v])) {
		   			AddProb += RangeDesc[a].Probability[i];
				}
	   	 	}
	    	return AddProb;
    } 
    return 0.0;
}

/*************************************************************************/
/*								  	                                     */
/*		Process a single case				  	                         */
/*								  	                                     */
/*************************************************************************/

consultRulesResults * InterpretRuleset(char * * list)
{
    Attribute a;
    RuleNo r;
    CError * errOcc;
    consultRulesResults * pointerToResults;
    ForEach(a, 0, MaxAtt) {
		RangeDesc[a].Asked = false;
    }
    if(FirstTime) {
		FirstTime = false;
		Verbosity(1) printf("\n");
    }
    else {
		Verbosity(1) printf("\n-------------------------------------------\n\n");
    }
    //  Find the first rule that fires on the item
    pointerToResults = (consultRulesResults *)malloc(sizeof(consultRulesResults));
    r = BestRule(list, &errOcc);
    if(errOcc != NULL) {
    	pointerToResults->codeErrors = (CError *) malloc(sizeof(CError));
    	pointerToResults->codeErrors->errorCode = errOcc->errorCode;
    	pointerToResults->codeErrors->errorMessage = (char *) calloc(strlen(errOcc->errorMessage)+strlen("Unable to consult the rule set. ")+1, sizeof(char));
    	strcat(pointerToResults->codeErrors->errorMessage, errOcc->errorMessage);
    	strcat(pointerToResults->codeErrors->errorMessage, "Unable to consult the rule set. ");
    	free(errOcc);
    	return pointerToResults;
    }
    if(r) {
    	pointerToResults->className = ClassName[Rule[r].Rhs];  // save the results
    	pointerToResults->probability = Confidence;
    	pointerToResults->isDefault = false;
		Verbosity(1) {
			printf("\nDecision:\n");
			printf("\t%s", ClassName[Rule[r].Rhs]);
			if(Confidence < 1.0) {
	    		printf("  CF = %.2f", Confidence);
			}
			printf("\n");
		}
    }
    else {
    	pointerToResults->className = ClassName[DefaultClass];
    	pointerToResults->probability = 0;      // default value when use the default class
    	pointerToResults->isDefault = true;
		Verbosity(1) {
			printf("\nDecision:\n");
			printf("\t%s (default class)\n", ClassName[DefaultClass]);
		}
    }
    pointerToResults->codeErrors = NULL;
    return pointerToResults;
}

/****************************************************************************/

/*   setting the options  */

/****************************************************************************/

void setConsultRulesVerbosity(short verbosity)
{
	if(verbosity == 0) VERBOSITY = 0;
	else VERBOSITY = 1;
}

void setConsultRulesVisualiseRules(Boolean rules)
{
	if(rules)
	TRACE = 1;
	else
	TRACE = 0;
}

/*****************************************************************************/

/*   call this function to consult the rule set */

/*****************************************************************************/

consultRulesResults * consultRules(ruleOptions * options, Configure * ruleConf, RuleSet set, char * * list)
{
	void PrintRule(RuleNo r);
	void Clear(void);
	Attribute a;
    RuleNo r;
    // initialise
	VERBOSITY = options->verbosity;
	MaxClass = ruleConf->MaxClass;
	ClassName = ruleConf->ClassName;
	MaxAtt = ruleConf->MaxAtt;
	AttName = ruleConf->AttName;
	MaxAttVal = ruleConf->MaxAttVal;
	AttValName = ruleConf->AttValName;
	SpecialStatus = ruleConf->SpecialStatus;
	MaxDiscrVal = ruleConf->MaxDiscrVal;
	DefaultClass = set.SDefaultClass;
    NRules = set.SNRules;
    RuleIndex = set.SRuleIndex;
    Rule = set.SRule;
    Verbosity(1) {
		ForEach(r, 1, NRules) {
		    PrintRule(r);
		}
		printf("\nDefault class: %s\n", ClassName[DefaultClass]);
    }
    //  Allocate value ranges
    RangeDesc = (struct ValRange *) calloc(MaxAtt+1, sizeof(struct ValRange));
    ForEach(a, 0, MaxAtt) {
		if(MaxAttVal[a]) {
	 	   RangeDesc[a].Probability =
			(float *) calloc(MaxAttVal[a]+1, sizeof(float));
		}
    }
    //  Consult
    Clear();
	return InterpretRuleset(list);
}
