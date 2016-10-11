/*************************************************************************/
/*                                                                       */
/*	Generate all rulesets from the decision trees                    */
/*	---------------------------------------------                    */
/*                                                                       */
/*************************************************************************/

#include<stdlib.h>
#include<string.h>
#include "defns.h"
#include "types.h"
#include "extern.h"
#include "rulex.h"

// functions declaration
processRulesResults * GenerateRulesFromTree(treeResults * unpruned, int no);
void FindTestCodes(void);
void SwapUnweighted(ItemNo a, ItemNo b);
void CompositeRuleset(void);

/*************************************************************************/
/*                                                                       */
/*  For each tree, form a set of rules and process them, then form a	 */
/*  composite set of rules from all of these sets.                       */
/*  If there is only one tree, then no composite set is formed.	  	 */
/*                                                                       */
/*  Rulesets are stored in  PRSet[0]  to  PRSet[TRIALS], where           */
/*  PRSet[TRIALS] contains the composite ruleset.                        */
/*                                                                       */
/*  On completion, the current ruleset is the composite ruleset (if one	 */
/*  has been made), otherwise the ruleset from the single tree.          */
/*                                                                       */
/*************************************************************************/

processRulesResults * GenerateRulesFromTree(treeResults * unpruned, int no)
{
    void FormRules(Tree t);
    void ConstructRuleset(void);
    void PrintIndexedRules(void);
    void deleteVar(void);
    void deleteWrong(void);
    Tree DecisionTree;
    short t = -1, RuleSetSpace = 0;
    int xa;
    int xc, xb;   // counters
    processRulesResults * resultedRules;
    RuleNo * newIndex;
    resultedRules = (processRulesResults *) malloc(sizeof(processRulesResults));
    if((unpruned == NULL) || (no == 0)) {
    	resultedRules->codeErrors = (CError *) malloc(sizeof(CError));
    	resultedRules->codeErrors->errorCode = 9;
    	resultedRules->codeErrors->errorMessage = (char *) calloc(strlen("error: there isn't any input decision tree. Unable to generate the rule set. ")+1, sizeof(char));
    	resultedRules->codeErrors->errorMessage = "error: there isn't any input decision tree. Unable to generate the rule set. ";
		return resultedRules;
    }
    //  Find bits to encode attributes and branches
    FindTestCodes();
    // Now process each decision tree
    xb = 0;
    ForEach(xc, 0, no-1) {
    	if(unpruned[xc]->isPruned == 0) {
            DecisionTree = unpruned[xc]->tree;
            xb ++;
    	}
    	else continue;  // avoid pruned decision trees
    	Verbosity(1) {
            printf("\n------------------\n");
            printf("Processing tree %d\n", xc);
    	}
        //  Form a set of rules from the next tree
        printf("dentro GenerateRulesFromTree, prima di FormRules\n");
        FormRules(DecisionTree);
        //  Process the set of rules for this trial
        printf("dentro GenerateRulesFromTree, prima di ConstructRuleset\n");
        ConstructRuleset();
        printf("dentro GenerateRulesFromTree, dopo ConstructRulest\n");
        //Verbosity(1) {
            printf("\nFinal rules from tree %d:\n", xc);
            PrintIndexedRules();
        //}
        // Make sure there is enough room for the new ruleset
        ++ t;
        if(t >= RuleSetSpace) {
        RuleSetSpace += 1;
        if(RuleSetSpace > 1) PRSet = (RuleSet *) realloc(PRSet, RuleSetSpace * sizeof(RuleSet));
        else PRSet = (RuleSet *) malloc(RuleSetSpace * sizeof(RuleSet));
        }
        newIndex = (RuleNo *) calloc(NRules + 1, sizeof(RuleNo));
        memcpy(newIndex, RuleIndex, (NRules + 1)*sizeof(RuleNo));
        free(RuleIndex);
        PRSet[t].SRuleIndex = newIndex;
        PRSet[t].SNRules = NRules;
        PRSet[t].SRule = Rule;
        PRSet[t].SDefaultClass = DefaultClass;
        PRSet[t].DefaultClassName = (char *) calloc(strlen(ClassName[DefaultClass]) + 1, sizeof(char));
        strcat(PRSet[t].DefaultClassName, ClassName[DefaultClass]);
    }
    TRIALS = xb;
    if(TRIALS < 1) {
        resultedRules->codeErrors = (CError *) malloc(sizeof(CError));
        resultedRules->codeErrors->errorCode = 9;
        resultedRules->codeErrors->errorMessage = (char *) calloc(strlen("error: there isn't any unpruned decision tree. Check the type of the passed trees. Unable to generate the rule set. ")+1, sizeof(char));
        resultedRules->codeErrors->errorMessage = "error: there isn't any unpruned decision tree. Check the type of the passed trees. Unable to generate the rule set. ";
        free(LogItemNo);
        free(LogFact);
        return resultedRules;
    }
    // If there is more than one tree in the trees file, make a composite ruleset of the rules from all trees
    if(TRIALS > 1) CompositeRuleset();
    if(TRIALS == 1) {
    	resultedRules->set = PRSet;
    	resultedRules->set[0].testResults = NULL;
    	resultedRules->set[0].isComposite = 0;
    	resultedRules->nSet = 1;
    }
    else {
    	resultedRules->set = PRSet;
    	ForEach(xa, 0, TRIALS) {
            resultedRules->set[xa].testResults = NULL;
            if(xa == TRIALS) resultedRules->set[xa].isComposite = 1;
            else resultedRules->set[xa].isComposite = 0;
    	}
    	resultedRules->nSet = TRIALS+1;
    }
    resultedRules->codeErrors = NULL;
    free(LogItemNo);
    free(LogFact);
    deleteVar();
    deleteWrong();
    return resultedRules;
}

/*************************************************************************/
/*								  	 */
/*	Determine code lengths for attributes and branches		 */
/*								  	 */
/*************************************************************************/

void FindTestCodes(void)
{
    void Quicksort(ItemNo Fp, ItemNo Lp, Attribute Att, void (*Exchange)());
    Attribute Att;
    DiscrValue v, V;
    ItemNo i, * ValFreq;
    int PossibleCuts;
    float Sum, SumBranches = 0, p;
    BranchBits  = (float *) malloc((MaxAtt+1) * sizeof(float));
    ForEach(Att, 0, MaxAtt) {
        if(V = MaxAttVal[Att]) {
            ValFreq = (ItemNo *) calloc(V+1, sizeof(ItemNo));
            ForEach(i, 0, MaxItem) ValFreq[DVal(Item[i],Att)]++;
            Sum = 0;
            ForEach(v, 1, V) {
                if(ValFreq[v]) Sum += (ValFreq[v] / (MaxItem + 1.0)) * (LogItemNo[MaxItem+1] - LogItemNo[ValFreq[v]]);
            }
            free(ValFreq);
            BranchBits[Att] = Sum;
        }
        else {
            printf("dentro FindTestCodes, prima di Quicksort, MaxItem = %d\n", MaxItem);
            Quicksort(0, MaxItem, Att, SwapUnweighted);
            PossibleCuts = 1;
            ForEach(i, 1, MaxItem) {
                if(CVal(Item[i],Att) > CVal(Item[i-1],Att)) PossibleCuts++;
            }
            BranchBits[Att] = PossibleCuts > 1 ? 1 + LogItemNo[PossibleCuts] / 2 : 0 ;
        }
        SumBranches += BranchBits[Att];
    }
    AttTestBits = 0;
    ForEach(Att, 0, MaxAtt) {
        if((p = BranchBits[Att] / SumBranches) > 0) {
            AttTestBits -= (float) (p * log(p) / log(2.0));
        }
    }
}

/*************************************************************************/
/*                                                                	 */
/*  Exchange items at a and b.  Note:  unlike the similar routine in	 */
/*  buildtree, this does not assume that items have a Weight to be	 */
/*  swapped as well!							 */
/*                                                                	 */
/*************************************************************************/

void SwapUnweighted(ItemNo a, ItemNo b)
{
    Description Hold;
    Hold = Item[a];
    Item[a] = Item[b];
    Item[b] = Hold;
}

/*************************************************************************/
/*								  	 */
/*	Form composite ruleset for all trials			  	 */
/*								  	 */
/*************************************************************************/

void CompositeRuleset(void)
{
    void InitialiseRules(void);
    void ConstructRuleset(void);
    void PrintIndexedRules(void);
    Boolean NewRule(Condition Cond[], short NConds, ClassNo TargetClass, float Err);
    RuleNo r;
    short t, ri;
    InitialiseRules();
    //  Lump together all the rules from each ruleset
    ForEach(t, 0, TRIALS-1) {
        ForEach(ri, 1, PRSet[t].SNRules) {
            r = PRSet[t].SRuleIndex[ri];
            NewRule(PRSet[t].SRule[r].Lhs, PRSet[t].SRule[r].Size, PRSet[t].SRule[r].Rhs, PRSet[t].SRule[r].Error);
        }
    }
    // ... and select a subset in the usual way
    ConstructRuleset();
    Verbosity(1) {
    	printf("\nComposite ruleset:\n");
    	PrintIndexedRules();
    }
    PRSet[TRIALS].SNRules = NRules;
    PRSet[TRIALS].SRule = Rule;
    PRSet[TRIALS].SRuleIndex = RuleIndex;
    PRSet[TRIALS].SDefaultClass = DefaultClass;
}
