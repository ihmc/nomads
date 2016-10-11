/*************************************************************************/
/*								   	 */
/*	Classify items interactively using a decision tree	   	 */
/*	--------------------------------------------------   		 */
/*								   	 */
/*************************************************************************/

#include "consult.h"

#include "defns.h"

#include <stdlib.h>
#include <string.h>

// External data  -- see c4.5.c for meanings
short MaxAtt, MaxClass, MaxDiscrVal, VERBOSITY, TRACE;
ItemNo MaxItem;
Description	* Item;
DiscrValue * MaxAttVal;
String * ClassName, * AttName, * * AttValName;
char * SpecialStatus;
// The interview module uses a more complex description of an case called a "Range Description".
// The value of an attribute is given by - lower and upper bounds (continuous attribute) and
// - probability of each possible value (discrete attribute)
typedef	struct ValRange * RangeDescRec;

struct ValRange {
	Boolean	Known;			// is range known?
	Boolean Asked;		    // has it been asked?
	float LowerBound;		// lower bound given
	float UpperBound;	    // upper ditto
	float * Probability;	// prior prob of each discr value
};
RangeDescRec RangeDesc;
Tree DecisionTree;			// tree being used
float * LowClassSum;		// accumulated lower estimates
float * ClassSum;			// accumulated central estimates
Boolean firstTime1 = true;

#define Fuzz	0.01		// minimum weight

// functions
consultTreeResults * InterpretTree(char * * list);
CError * ClassifyCase(Tree Subtree, float Weight, char * * list);
float Interpolate(Tree t, float v);
float Area(Tree t, float v);
void Decision(ClassNo c, float p, float lb, float ub);

/***********************************************************************/
/*  Classify the extended case description in RangeDesc using the	 */
/*  given subtree, by adjusting the values ClassSum and LowClassSum	 */
/*  for each class, indicating the likelihood of the case being  	 */
/*  of that class.						   	 */
/***********************************************************************/

CError * ClassifyCase(Tree Subtree, float Weight, char * * list)
{
	CError * CheckValue(Attribute Att, Tree T, const char * phrase);
	DiscrValue v;
    float BranchWeight;
    Attribute a, xa;
    short s;
    ClassNo c;
    CError * errOcc;
    if(!Subtree->NodeType) {
		Verbosity(1) printf("\tClass %s weight %g cases %g\n", ClassName[Subtree->Leaf], Weight, Subtree->Items);
		if(Subtree->Items > 0) {	//  Adjust class sum of ALL classes, but adjust low class sum of leaf class only
	    	ForEach(c, 0, MaxClass) ClassSum[c] += Weight * Subtree->ClassDist[c] / Subtree->Items;
	    	LowClassSum[Subtree->Leaf] += Weight * (1 - Subtree->Errors / Subtree->Items);
		}
		else ClassSum[Subtree->Leaf] += Weight;
		return NULL;
    }
    a = Subtree->Tested;
    ForEach(xa, 0, MaxAtt + 1) {
    	if(!strncmp(list[xa], AttName[a], strlen(AttName[a]))) break;
    }  // now 'xa' contains the index for list to use in CheckValue()
    errOcc = CheckValue(a, Subtree, list[xa]);
    if(errOcc != NULL) return errOcc;
    //  Unknown value
    if(!RangeDesc[a].Known) {
		ForEach(v, 1, Subtree->Forks) {
	 	   errOcc = ClassifyCase(Subtree->Branch[v],(Weight * Subtree->Branch[v]->Items)/Subtree->Items, list);
	 	   if(errOcc != NULL) return errOcc;
		}
		return NULL;
    }
    //  Known value
    switch(Subtree->NodeType) {
		case BrDiscr:  // test for discrete attribute
	    	ForEach(v, 1, MaxAttVal[a]) {
				BranchWeight = RangeDesc[a].Probability[v];
				if(BranchWeight > 0) {
		    		Verbosity(1) printf("\tWeight %g: test att %s (val %s = %g)\n",Weight, AttName[a], AttValName[a][v], BranchWeight);
		    		errOcc = ClassifyCase(Subtree->Branch[v], Weight * BranchWeight, list);
		    		if(errOcc != NULL) return errOcc;
				}
	    	}
	    	break;
	    case ThreshContin:  // test for continuous attribute
	    	BranchWeight = RangeDesc[a].UpperBound <= Subtree->Lower ? 1.0 :
			RangeDesc[a].LowerBound > Subtree->Upper ? 0.0 : RangeDesc[a].LowerBound != RangeDesc[a].UpperBound ?
			(Area(Subtree, RangeDesc[a].LowerBound) -Area(Subtree, RangeDesc[a].UpperBound)) /
			(RangeDesc[a].UpperBound - RangeDesc[a].LowerBound) : Interpolate(Subtree, RangeDesc[a].LowerBound);
	    	Verbosity(1) printf("\tWeight %g: test att %s (branch weight=%g)\n", Weight, AttName[a], BranchWeight);
	    	if(BranchWeight > Fuzz) {
				errOcc = ClassifyCase(Subtree->Branch[1], Weight * BranchWeight, list);
				if(errOcc != NULL) return errOcc;
	    	}
	    	if(BranchWeight < 1-Fuzz) {
				errOcc = ClassifyCase(Subtree->Branch[2], Weight * (1 - BranchWeight), list);
				if(errOcc != NULL) return errOcc;
	    	}
	    	break;
	    case BrSubset:  // subset test on discrete attribute
	    	ForEach(s, 1, Subtree->Forks) {
				BranchWeight = 0.0;
				ForEach(v, 1, MaxAttVal[a]) {
		    		if(In(v, Subtree->Subset[s])) {
						BranchWeight += RangeDesc[a].Probability[v];
		    		}
				}
				if(BranchWeight > 0) {
		    		Verbosity(1)
		    		printf("\tWeight %g: test att %s (val %s = %g)\n",Weight, AttName[a], AttValName[a][v], BranchWeight);
		    		errOcc = ClassifyCase(Subtree->Branch[s], Weight * BranchWeight, list);
		    		if(errOcc != NULL) return errOcc;
				}
	    	}
	    	break;
    }
    return NULL;
}

/*************************************************************************/
/*								   	 */
/*  Interpolate a single value between Lower, Cut and Upper		 */
/*								   	 */
/*************************************************************************/

float Interpolate(Tree t, float v)
{
    if(v <= t->Lower) {
		return 1.0f;
    }
    if(v <= t->Cut) {
		return (float) (1 - 0.5 * (v - t->Lower) / (t->Cut - t->Lower + Epsilon));
    }
    if(v < t->Upper) {
		return (float) (0.5 - 0.5 * (v - t->Cut) / (t->Upper - t->Cut + Epsilon));
    }
    return 0.0;
}

/*************************************************************************/
/*								   	 */
/*  Compute the area under a soft threshold curve to the right of a	 */
/*  given value.							 */
/*								   	 */
/*************************************************************************/

float Area(Tree t, float v)
{
    float Sum = (float)Epsilon, F;
    if(v < t->Lower) {
		Sum += t->Lower - v;
		v = t->Lower;
    }
    if(v < t->Cut) {
		F = (float)((t->Cut - v ) / (t->Cut - t->Lower + Epsilon));
		Sum += (float)(0.5 * (t->Cut - v) + 0.25 * F * (t->Cut - v));
		v = t->Cut;
    }
    if(v < t->Upper) {
		F = (float)((t->Upper - v ) / (t->Upper - t->Cut + Epsilon));
		Sum += (float)(0.25 * (t->Upper - v) * F);
    }
    Verbosity(1) printf("lower=%g  cut=%g  upper=%g  area=%g\n", t->Lower, t->Cut, t->Upper, Sum);
    return Sum;
}

/*************************************************************************/
/*								  	 */
/*		Process a single case				  	 */
/*								  	 */
/*************************************************************************/

consultTreeResults * InterpretTree(char * * list)
{
	ClassNo c, BestClass;
    float Uncertainty = 1.0;
    Attribute a;
    int number = 0;  // number of classes in the result
	consultTreeResults * pointerToResults;
	CError * errOcc;
    ClassNo best;
    float guess;
    float low;
    float upper;
	ForEach(a, 0, MaxAtt) RangeDesc[a].Asked = false;
    ClassSum = (float *) malloc((MaxClass+1) * sizeof(float));
	LowClassSum = (float *) malloc((MaxClass+1) * sizeof(float));
	Verbosity(1) printf("\n");
	ForEach(c, 0, MaxClass) LowClassSum[c] = ClassSum[c] = 0;
	errOcc = ClassifyCase(DecisionTree, 1.0, list);
    if(errOcc != NULL) {
    	pointerToResults = (consultTreeResults *) malloc(sizeof(consultTreeResults));
    	pointerToResults->codeErrors = (CError *) malloc(sizeof(CError));
    	pointerToResults->codeErrors->errorCode = errOcc->errorCode;
    	pointerToResults->codeErrors->errorMessage = (char *) calloc(strlen(errOcc->errorMessage)+strlen("Unable to consult the tree. ")+1, sizeof(char));
    	strcat(pointerToResults->codeErrors->errorMessage, errOcc->errorMessage);
    	strcat(pointerToResults->codeErrors->errorMessage, "Unable to consult the tree. ");
    	free(errOcc);
    	return pointerToResults;
    }
    BestClass = 0;
    ForEach(c, 0, MaxClass) {
		Verbosity(1) printf("class %d weight %.2f\n", c, ClassSum[c]);
		Uncertainty -= LowClassSum[c];
		if ( ClassSum[c] > ClassSum[BestClass] ) BestClass = c;
    }
    Verbosity(1) printf("\nDecision:\n");
    number = 1;   // save the first result
    best = BestClass;
    guess = ClassSum[BestClass];
    low = LowClassSum[BestClass];
    upper = Uncertainty + LowClassSum[BestClass];
    if(MaxClass > 1)  {// find out how many classes there are
    	while(true) {
	  		ClassSum[BestClass] = 0;  // this overraids the first result
	    	BestClass = 0;
	    	ForEach(c, 0, MaxClass) {
				if(ClassSum[c] > ClassSum[BestClass]) BestClass = c;
	    	}
	    	if(ClassSum[BestClass] < Fuzz) break;
	    	number ++;
		}
    }
    pointerToResults = (consultTreeResults *) calloc(number, sizeof(consultTreeResults));
    pointerToResults[0].className = ClassName[best];  // alloc memory and save the results
    pointerToResults[0].guessProb = guess;
    pointerToResults[0].lowProb = low;
    pointerToResults[0].upperProb = upper;
    pointerToResults[0].nClasses = number;
    Verbosity(1) Decision(best, guess, low, upper);  // display the results
    if(MaxClass > 1) {
    	number = 0;   // use number as an index for the results
		while(true) {
	  		ClassSum[BestClass] = 0;
	    	BestClass = 0;
	    	ForEach(c, 0, MaxClass) {
				if(ClassSum[c] > ClassSum[BestClass]) BestClass = c;
	    	}
	    	if(ClassSum[BestClass] < Fuzz) break;
	    	number ++;
	    	pointerToResults[number].className = ClassName[BestClass];  // save other results
	    	pointerToResults[number].guessProb = ClassSum[BestClass];
	    	pointerToResults[number].lowProb = LowClassSum[BestClass];
	    	pointerToResults[number].upperProb = Uncertainty + LowClassSum[BestClass];
	    	Verbosity(1) Decision(BestClass, ClassSum[BestClass], LowClassSum[BestClass], Uncertainty + LowClassSum[BestClass]);
		}
    }
    pointerToResults[0].codeErrors = NULL;
    return pointerToResults;
}

/*************************************************************************/
/*								  	 */
/*  Print the chosen class with certainty factor and range	  	 */
/*								  	 */
/*************************************************************************/

void Decision(ClassNo c, float p, float lb, float ub)
{
    printf("\t%s", ClassName[c]);
    if(p < 1-Fuzz || lb < ub - Fuzz) {
		printf("  CF = %.2f", p);
		if(lb < ub - Fuzz) {
	  		printf("  [ %.2f - %.2f ]", lb, ub);
		}
    }
    printf("\n");
}

/*************************************************************************/

/*   Setting the options   */

/*************************************************************************/

void setConsultTreevisualiseTree(Boolean tree)
{
	if(tree)
	TRACE = 1;
	else
	TRACE = 0;
}

void setConsultTreeVerbosity(short verbosity)
{
	if(verbosity == 0) VERBOSITY = 0;
	else VERBOSITY = 1;
}

/***************************************************************************/

/*   make predictions with a given tree   */

/***************************************************************************/

consultTreeResults * consultTree(treeOptions * options, Configure * treeConf, Tree tree, char * * list)
{
	void PrintTree(Tree T);
	void Clear(void);
	Attribute a;
	// initialise
	VERBOSITY = options->verbosity;
	MaxClass = treeConf->MaxClass;
	ClassName = treeConf->ClassName;
	MaxAtt = treeConf->MaxAtt;
	AttName = treeConf->AttName;
	MaxAttVal = treeConf->MaxAttVal;
	AttValName = treeConf->AttValName;
	SpecialStatus = treeConf->SpecialStatus;
	MaxDiscrVal = treeConf->MaxDiscrVal;
	DecisionTree = tree;
	ClassSum = Nil;
	LowClassSum = Nil;
	Verbosity(1) PrintTree(DecisionTree);
	RangeDesc = (struct ValRange *) calloc(MaxAtt+1, sizeof(struct ValRange));
    ForEach(a, 0, MaxAtt) {
		if(MaxAttVal[a]) {	// if this attribute is discrete, set the probability
		    RangeDesc[a].Probability =
			(float *) calloc(MaxAttVal[a]+1, sizeof(float));
		}
    }
    Clear();
	return InterpretTree(list);
}
