/*************************************************************************/
/*								  	 */
/*	Form a set of rules from a decision tree			 */
/*	----------------------------------------			 */
/*								  	 */
/*************************************************************************/

#include<stdlib.h>
#include<string.h>
#include "defns.h"
#include "types.h"
#include "extern.h"
#include "rulex.h"

ItemNo * TargetClassFreq;	// [Boolean]
ItemNo * Errors;			// [Condition]
ItemNo * Total;				// [Condition]
float * Pessimistic;		// [Condition]
float * Actual;				// [Condition]
float * CondSigLevel;		// [Condition]
Boolean	* * CondSatisfiedBy;// [Condition][ItemNo]
Boolean * Deleted;			// [Condition]
DiscrValue * SingleValue;	// [Attribute]
Condition * Stack;
short MaxDisjuncts;
short MaxDepth;

void FormRules(Tree t);
void TreeParameters(Tree t, short d);
void Scan(Tree t, short d);
void deleteVar(void);

/*************************************************************************/
/*								  	 */
/*	Form a ruleset from decision tree t			  	 */
/*								  	 */
/*************************************************************************/

void FormRules(Tree t)
/*  ----------  */
{
	void InitialiseRules(void);
	void deleteVari(void);
    short i;
    //  Find essential parameters and allocate storage
    MaxDepth = 0;
    MaxDisjuncts = 0;
    TreeParameters(t, 0);
    Actual = (float *) calloc(MaxDepth+2, sizeof(float));
    Total = (ItemNo *) calloc(MaxDepth+2, sizeof(ItemNo));
    Errors = (ItemNo *) calloc(MaxDepth+2, sizeof(ItemNo));
    Pessimistic = (float *) calloc(MaxDepth+2, sizeof(float));
    CondSigLevel = (float *) calloc(MaxDepth+2, sizeof(float));
    TargetClassFreq = (ItemNo *) calloc(2, sizeof(ItemNo));
    Deleted = (Boolean *) calloc(MaxDepth+2, sizeof(Boolean));
    CondSatisfiedBy = (char **) calloc(MaxDepth+2, sizeof(char *));
    Stack = (Condition *) calloc(MaxDepth+2, sizeof(Condition));
    ForEach(i, 0, MaxDepth+1) {
		CondSatisfiedBy[i] = (char *) calloc(MaxItem+1, sizeof(char));
		Stack[i] = (Condition) malloc(sizeof(struct CondRec));
		Stack[i]->CondTest = NULL;
    }
    SingleValue = (DiscrValue *) calloc(MaxAtt+1, sizeof(DiscrValue));
    InitialiseRules();
    //  Extract and prune disjuncts
    printf("dentro FormRules, prima di Scan\n");
    Scan(t, 0);
    printf("dentro FormRules, dopo Scan\n");
    //  Deallocate storage
    ForEach(i, 0, MaxDepth + 1) {
		free(CondSatisfiedBy[i]);
		//if(Stack[i]->CondTest->NodeType == BrSubset) {
			//ForEach(x, 1, Stack[i]->CondTest->Forks) {
			//	free(Stack[i]->CondTest->Subset[x]);
			//}
			//free(Stack[i]->CondTest->Subset);
		//}
		if((i > 0)&&(i < MaxDepth + 1)) {printf("FREE DI STACK[%d]\n", i); free(Stack[i]->CondTest); }
		// alcuni puntatori puntano alla stessa area di memoria!!!!!
		free(Stack[i]);
    }
    free(Deleted);
    free(CondSatisfiedBy);
    free(Stack);
    free(Actual);
    free(Total);
    free(Errors);
    free(Pessimistic);
    free(CondSigLevel);
    free(TargetClassFreq);
    deleteVari();
}

void deleteVar(void)
{
	free(SingleValue);
}

/*************************************************************************/
/*                                                                	 */
/*  Find the maximum depth and the number of leaves in tree t	  	 */
/*  with initial depth d					  	 */
/*                                                                	 */
/*************************************************************************/

void TreeParameters(Tree t, short d)
/*  ---------------  */
{
    DiscrValue v;
    if(t->NodeType) {
		ForEach(v, 1, t->Forks) TreeParameters(t->Branch[v], d+1);
    }
    else {	//  This is a leaf
		if(d > MaxDepth) MaxDepth = d;
		MaxDisjuncts++;
    }
}

/*************************************************************************/
/*								  	 */
/*  Extract disjuncts from tree t at depth d, and process them	  	 */
/*								  	 */
/*************************************************************************/

void Scan(Tree t, short d)
/*  ----  */
{
	void PruneRule(Condition Cond[], short NCond, ClassNo TargetClass);
	Test FindTest(Test Newtest);
    DiscrValue v;
    short i;
    Condition * Term;
    Test x;
    if(t->NodeType) {
		d++;
		x = (Test) malloc(sizeof(struct TestRec));
		x->NodeType = t->NodeType;
		x->Tested = t->Tested;
		x->Forks = t->Forks;
		x->Cut = (t->NodeType == ThreshContin ? t->Cut : 0);
		if(t->NodeType == BrSubset) {
	   		x->Subset = (Set *) calloc(t->Forks + 1, sizeof(Set));
	    	ForEach(v, 1, t->Forks) {
	    		//x->Subset[v] = t->Subset[v];
	    		x->Subset[v] = (char *) calloc(strlen(t->Subset[v]) + 1, sizeof(char));
	    		strcat(x->Subset[v], t->Subset[v]);
	    	}
		}
		// il valore di D si ripete, di conseguenza Stack[d] viene sovrascritto!!! ora non piu'
		//printf("VALORE DI D = %d !!!!!!!!!\n", d);
		if(Stack[d]->CondTest != NULL) {  // PARTE IN PROVA!!!!
			if(Stack[d]->CondTest->NodeType == BrSubset) {
				ForEach(i, 1, Stack[d]->CondTest->Forks) free(Stack[d]->CondTest->Subset[i]);
				free(Stack[d]->CondTest->Subset);
			}
			free(Stack[d]->CondTest);
			//printf("CondTest non e' nullo!!!");
		}
		Stack[d]->CondTest = FindTest(x);
		ForEach(v, 1, t->Forks) {
	    	Stack[d]->TestValue = v;
	    	Scan(t->Branch[v], d);
		}
    }
    else if(t->Items >= 1) {
		//  Leaf of decision tree - construct the set of conditions associated with this leaf and prune
		Term = (Condition *) calloc(d+1, sizeof(Condition));
		ForEach(i, 1, d) {
	    	Term[i] = (Condition) malloc(sizeof(struct CondRec));
	    	Term[i]->CondTest = Stack[i]->CondTest;
	   		Term[i]->TestValue = Stack[i]->TestValue;
	   		//printf("dentro SCAN, Term[%d] = %p prima di PruneRule\n", i, Term[i]);
		}
		PruneRule(Term, d, t->Leaf);
		ForEach(i, 1, d) {
			//printf("dentro SCAN, Term[%d] = %p dopo PruneRule\n", i, Term[i]);
			//free(Term[i]->CondTest);
			free(Term[i]);
		}
		free(Term);
    }
}
