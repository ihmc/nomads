/*************************************************************************/
/*								  	 */
/*	Miscellaneous routines for rule handling		  	 */
/*	----------------------------------------		  	 */
/*								  	 */
/*************************************************************************/

#include<stdlib.h>
#include<string.h>
#include "defns.h"
#include "types.h"
#include "extern.h"
#include "rulex.h"

Test * TestVec;
short NTests = 0;
short TestSpace = 1;

// functions
Test FindTest(Test Newtest);
Boolean SameTest(Test t1, Test t2);
void InitialiseRules(void);
Boolean NewRule(Condition Cond[], short NConds, ClassNo TargetClass, float Err);
Boolean SameRule(RuleNo r, Condition Cond[], short NConds, ClassNo TargetClass);
void PrintRule(RuleNo r);
void PrintCondition(Condition c);
void deleteVari(void);

/*************************************************************************/
/*								  	 */
/*  Find a test in the test vector; if it's not there already, add it	 */
/*								  	 */
/*************************************************************************/

Test FindTest(Test Newtest)
/*   ---------  */
{
    //static short TestSpace = 1;   		// PER ORA E' IN PROVA!!!
    short i;
    ForEach(i, 1, NTests) {
		if(SameTest(Newtest, TestVec[i])) {
	   	 	//free(Newtest);
	    	//return TestVec[i];
	    	return Newtest;
		}
    }
    NTests++;
    if(NTests >= TestSpace) {
		TestSpace += 1;
		if(TestSpace > 2) TestVec = (Test *) realloc(TestVec, TestSpace * sizeof(Test));
		else TestVec = (Test *) malloc(TestSpace * sizeof(Test));

    }
    //TestVec[NTests] = Newtest;
    //return TestVec[NTests];
    TestVec[NTests] = (Test) malloc(sizeof(struct TestRec)); // parte nuova!!!!!!!!
    TestVec[NTests]->NodeType = Newtest->NodeType;
	TestVec[NTests]->Tested = Newtest->Tested;
	TestVec[NTests]->Forks = Newtest->Forks;
	TestVec[NTests]->Cut = Newtest->Cut;
	if(Newtest->NodeType == BrSubset) {
	   	TestVec[NTests]->Subset = (Set *) calloc(Newtest->Forks + 1, sizeof(Set));
	    ForEach(i, 1, Newtest->Forks) {
	    	TestVec[NTests]->Subset[i] = (char *) calloc(strlen(Newtest->Subset[i]) + 1, sizeof(char));
	    	strcat(TestVec[NTests]->Subset[i], Newtest->Subset[i]);
		}
	}
    return Newtest;
}

void deleteVari(void)
{
	int i;
	// da qualche parte subset viene copiato!!!!!
	ForEach(i, 1, NTests) free(TestVec[i]);
	free(TestVec);
	TestSpace = 1;
	NTests = 0;
}

/*************************************************************************/
/*								  	 */
/*	See if test t1 is the same test as test t2		  	 */
/*								  	 */
/*************************************************************************/

Boolean SameTest(Test t1, Test t2)
/*      ---------  */
{
    short i;
    if(t1->NodeType != t2->NodeType || t1->Tested != t2->Tested) return false;
    switch(t1->NodeType) {
		case BrDiscr:
			return true;
		case ThreshContin:
			return  t1->Cut == t2->Cut;
		case BrSubset:
			ForEach(i, 1, t1->Forks) {
		 		//if(t1->Subset[i] != t2->Subset[i]) return false;
		 		if(strcmp(t1->Subset[i], t2->Subset[i])) return false;
			}
    }
    return true;
}

/*************************************************************************/
/*								  	 */
/*		Clear for new set of rules			  	 */
/*								  	 */
/*************************************************************************/

void InitialiseRules(void)
/*  ----------------  */
{
    NRules = 0;
    Rule = 0;
    RuleSpace = 1;
}

/*************************************************************************/
/*								  	 */
/*  Add a new rule to the current ruleset, by updating Rule[],	  	 */
/*  NRules and, if necessary, RuleSpace				  	 */
/*								  	 */
/*************************************************************************/

Boolean NewRule(Condition Cond[], short NConds, ClassNo TargetClass, float Err)
/*       -------  */
{
    short d, r, x;
    // See if rule already exists
    ForEach(r, 1, NRules) {
		if(SameRule(r, Cond, NConds, TargetClass)) {
	    	Verbosity(1) printf("\tduplicates rule %d\n", r);
	    	// Keep the most pessimistic error estimate
	    	if(Err > Rule[r].Error) Rule[r].Error = Err;
	    	return false;
		}
    }
    // Make sure there is enough room for the new rule
    NRules++;
    if(NRules >= RuleSpace) {
    	//printf("alloca RULE NO %d\n", RuleSpace + 1);
		RuleSpace += 1;
		if(RuleSpace > 1) Rule = (PR *) realloc(Rule, RuleSpace * sizeof(PR));
		else Rule = (PR *) malloc(RuleSpace * sizeof(PR));
    }
    // Form the new rule
    //printf("accede a RULE NO %d\n", NRules);
    Rule[NRules].Size = NConds;
    Rule[NRules].Lhs = (Condition *) calloc(NConds+1, sizeof(Condition));
    //printf("dentro NewRule, alloca Lhs = %p for Rule[%d]\n", Rule[NRules].Lhs, NRules);
    ForEach(d, 1, NConds) {
		Rule[NRules].Lhs[d] = (Condition) malloc(sizeof(struct CondRec));
		Rule[NRules].Lhs[d]->CondTest = (Test) malloc(sizeof(struct TestRec));
		Rule[NRules].Lhs[d]->CondTest->NodeType = Cond[d]->CondTest->NodeType;
		Rule[NRules].Lhs[d]->CondTest->Cut = Cond[d]->CondTest->Cut;
		Rule[NRules].Lhs[d]->CondTest->Forks = Cond[d]->CondTest->Forks;
		Rule[NRules].Lhs[d]->CondTest->Tested = Cond[d]->CondTest->Tested;
		if(Rule[NRules].Lhs[d]->CondTest->NodeType == BrSubset) {
			Rule[NRules].Lhs[d]->CondTest->Subset = (char * *) calloc(Rule[NRules].Lhs[d]->CondTest->Forks+1, sizeof(char *));
			ForEach(x, 1, Rule[NRules].Lhs[d]->CondTest->Forks+1) {
				Rule[NRules].Lhs[d]->CondTest->Subset[x] = (char *) calloc(strlen(Cond[d]->CondTest->Subset[x])+1, sizeof(char));
				strcat(Rule[NRules].Lhs[d]->CondTest->Subset[x], Cond[d]->CondTest->Subset[x]);
			}
		}
		Rule[NRules].Lhs[d]->TestValue = Cond[d]->TestValue;
    }
    Rule[NRules].Rhs = TargetClass;
    Rule[NRules].Error = Err;
    Verbosity(1) PrintRule(NRules);
    return true;
}

/*************************************************************************/
/*								  	 */
/*  Decide whether the given rule duplicates rule r		  	 */
/*								  	 */
/*************************************************************************/

Boolean SameRule(RuleNo r, Condition Cond[], short NConds, ClassNo TargetClass)
/*      --------  */
{
    short d, i;
    Test SubTest1, SubTest2;
    if(Rule[r].Size != NConds || Rule[r].Rhs != TargetClass) {
		return false;
    }
    ForEach(d, 1, NConds) {
		if(Rule[r].Lhs[d]->CondTest->NodeType != Cond[d]->CondTest->NodeType || Rule[r].Lhs[d]->CondTest->Tested != Cond[d]->CondTest->Tested) {
	    	return false;
		}
		switch(Cond[d]->CondTest->NodeType) {
	    	case BrDiscr:
				if(Rule[r].Lhs[d]->TestValue != Cond[d]->TestValue) {
		    		return false;
				}
				break;
	    	case ThreshContin:
				if(Rule[r].Lhs[d]->CondTest->Cut != Cond[d]->CondTest->Cut) {
		    		return false;
				}
				break;
	    	case BrSubset:
				SubTest1 = Rule[r].Lhs[d]->CondTest;
				SubTest2 = Cond[d]->CondTest;
				ForEach(i, 1, SubTest1->Forks) {
		     		if(SubTest1->Subset[i] != SubTest2->Subset[i]) {
						return false;
		    		}
				}
		}
    }
    return true;
}

/*************************************************************************/
/*								  	 */
/*		Print the current indexed ruleset		  	 */
/*								  	 */
/*************************************************************************/

void PrintIndexedRules(void)
/*  -----------------  */
{
    short ri;
    ForEach(ri, 1, NRules) PrintRule(RuleIndex[ri]);
    printf("\nDefault class: %s\n", ClassName[DefaultClass]);
}

/*************************************************************************/
/*								  	 */
/*		Print the rule r				  	 */
/*								  	 */
/*************************************************************************/

void PrintRule(RuleNo r)
/*  ---------  */
{
    short d;
    printf("\nRule %d:\n", r);
    ForEach(d, 1, Rule[r].Size) {
        printf("    ");
        PrintCondition(Rule[r].Lhs[d]);
    }
    printf("\t->  class %s  [%.1f%%]\n", ClassName[Rule[r].Rhs], 100 * (1 - Rule[r].Error));
}

/*************************************************************************/
/*								  	 */
/*	Print a condition c of a production rule		  	 */
/*								  	 */
/*************************************************************************/

void PrintCondition(Condition c)
/*  --------------  */
{
    Test tp;
    DiscrValue v, pv, Last, Values=0;
    Boolean First = true;
    Attribute Att;
    tp = c->CondTest;
    v = c->TestValue;
    Att = tp->Tested;
    printf("\t%s", AttName[Att]);
    if(v < 0) {
		printf(" is unknown\n");
		return;
    }
    switch(tp->NodeType) {
		case BrDiscr:
	    	printf(" = %s\n", AttValName[Att][v]);
	    	break;
		case ThreshContin:
	   		printf(" %s %g\n", ( v == 1 ? "<=" : ">" ), tp->Cut);
	    	break;
		case BrSubset:	// Count values at this branch
	    	for(pv=1 ; Values <= 1 && pv <= MaxAttVal[Att] ; pv++) {
				if(In(pv, tp->Subset[v])) {
		    		Last = pv;
		   	 		Values++;
				}
	    	}
	    	if(Values == 1) {
				printf(" = %s\n", AttValName[Att][Last]);
				break;
	    	}
	    	printf(" in ");
	    	ForEach(pv, 1, MaxAttVal[Att]) {
				if(In(pv, tp->Subset[v])) {
		  	 		if(First) {
						printf("{");
						First = false;
		   			}
		    		else printf(", ");
		   			printf("%s", AttValName[Att][pv]);
				}
	    	}
	    	printf("}\n");
    }
}
