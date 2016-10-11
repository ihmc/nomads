/*************************************************************************/
/*									 */
/*	Routines to manage tree growth, pruning and evaluation		 */
/*	------------------------------------------------------		 */
/*									 */
/*************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include "defns.h"
#include "types.h"
#include "extern.h"

ItemNo * TargetClassFreq;
Tree * Raw;
extern Tree	* Pruned;

// functions
void OneTree(void);
int BestTree(processTreeResults * treeSet);
void FormTarget(ItemNo Size);
void FormInitialWindow(void);
void Shuffle(void);
void MoveItems(ItemNo Window);
Tree Iterate(ItemNo * Window, ItemNo IncExceptions, ItemNo * noTrees);
void Evaluate(Boolean CMInfo, short Saved);

/*************************************************************************/

//	Grow and prune a single tree from all data

/*************************************************************************/

void OneTree(void)
{
	Boolean Prune(Tree T);
	Tree FormTree(ItemNo Fp, ItemNo Lp);
	Tree CopyTree(Tree T);
    void FreeClassFreq();
    void InitialiseTreeData(void);
    void InitialiseWeights(void);
    void PrintTree(Tree T);
    void FreeVariables(void);
    InitialiseTreeData();
    InitialiseWeights();
    Raw = (Tree *) calloc(1, sizeof(Tree));
    Pruned = (Tree *) calloc(1, sizeof(Tree));
    AllKnown = true;
    Raw[0] = FormTree(0, MaxItem);
    Verbosity(1) {
    	printf("\n");
    	PrintTree(Raw[0]);
    }
    Pruned[0] = CopyTree(Raw[0]);
    if(Prune(Pruned[0])) {
		Verbosity(1) {
			printf("\nSimplified ");
			PrintTree(Pruned[0]);
		}
    }
    FreeVariables();
}

/*************************************************************************/
/*									 */
/*	Grow and prune TRIALS trees and select the best of them		 */
/*									 */
/*************************************************************************/

int BestTree(processTreeResults * treeSet)  // new version with C45AVList!!!
{
    void FreeClassFreq();
    void InitialiseTreeData(void);
    void FormTarget(ItemNo Size);
    void FormInitialWindow(void);
    void FreeVariables(void);
    void PrintTree(Tree T);
    Tree CopyTree(Tree T);
    Boolean Prune(Tree T);
    int noTrees = 0;
    InitialiseTreeData();
    TargetClassFreq = (ItemNo *) calloc(MaxClass+1, sizeof(ItemNo));
    //  If necessary, set initial size of window to 20% (or twice the sqrt, if this is larger) of the number of data items,
	// and the maximum number of items that can be added to the window at each iteration to 20% of the initial window size
    if(!WINDOW) WINDOW = Max(2 * sqrt(MaxItem+1.0), (MaxItem+1) / 5);
    if(!INCREMENT) INCREMENT = Max(WINDOW / 5, 1);
    FormTarget(WINDOW);
    //  Form set of trees by iteration and prune
    FormInitialWindow();
    treeSet->trees[0] = (treeResults) malloc(sizeof(TRes));
	treeSet->trees[0]->tree = Iterate(&WINDOW, INCREMENT, &noTrees);
	Verbosity(1) {
		printf("print unpruned tree\n");
		PrintTree(treeSet->trees[0]->tree);
	}
	treeSet->trees[1] = (treeResults) malloc(sizeof(TRes));
	treeSet->trees[1]->tree = CopyTree(treeSet->trees[0]->tree);
	Prune(treeSet->trees[1]->tree);
	treeSet->trees[0]->isPruned = 0;
	treeSet->trees[1]->isPruned = 1;
	treeSet->trees[0]->testResults = NULL;
	treeSet->trees[1]->testResults = NULL;
	treeSet->nTrees = 2;
	Verbosity(1) {
		printf("\nprint pruned tree\n");
		PrintTree(treeSet->trees[1]->tree);
	}
    FreeVariables();
    free(TargetClassFreq);
    return noTrees;
}

/*************************************************************************/
/*									 */
/*  The windowing approach seems to work best when the class		 */
/*  distribution of the initial window is as close to uniform as	 */
/*  possible.  FormTarget generates this initial target distribution,	 */
/*  setting up a TargetClassFreq value for each class.			 */
/*									 */
/*************************************************************************/

void FormTarget(ItemNo Size)
/*  -----------  */
{
    ItemNo i, * ClassFreq;
    ClassNo c, Smallest, ClassesLeft=0;
    ClassFreq = (ItemNo *) calloc(MaxClass+1, sizeof(ItemNo));
    //  Generate the class frequency distribution
    ForEach(i, 0, MaxItem) {
		ClassFreq[ Class(Item[i]) ]++;
    }
    //  Calculate the no. of classes of which there are items
    ForEach(c, 0, MaxClass) {
		if(ClassFreq[c]) {
	    	ClassesLeft++;
		}
		else {
	   		TargetClassFreq[c] = 0;
		}
    }
    while(ClassesLeft) {
	//  Find least common class of which there are some items
		Smallest = -1;
		ForEach(c, 0, MaxClass) {
	    	if(ClassFreq[c] && (Smallest < 0 || ClassFreq[c] < ClassFreq[Smallest])) {
				Smallest = c;
	   		}
		}
		//  Allocate the no. of items of this class to use in the window
		TargetClassFreq[Smallest] = Min(ClassFreq[Smallest], Round(Size/ClassesLeft));
		ClassFreq[Smallest] = 0;
		Size -= TargetClassFreq[Smallest];
		ClassesLeft--;
    }
    free(ClassFreq);
}


/*************************************************************************/
/*									 */
/*  Form initial window, attempting to obtain the target class profile	 */
/*  in TargetClassFreq.  This is done by placing the targeted number     */
/*  of items of each class at the beginning of the set of data items.	 */
/*									 */
/*************************************************************************/

void FormInitialWindow(void)
/*  -------------------  */
{
	void Swap(ItemNo a, ItemNo b);
    ItemNo i, Start = 0, More;
    ClassNo c;
    Shuffle();
    ForEach(c, 0, MaxClass) {
		More = TargetClassFreq[c];
		for(i = Start ; More ; i++) {
	    	if(Class(Item[i]) == c) {
				Swap(Start, i);
				Start++;
				More--;
	    	}
		}
    }
}

/*************************************************************************/
/*									 */
/*		Shuffle the data items randomly				 */
/*									 */
/*************************************************************************/

void Shuffle(void)
/*  -------  */
{
    ItemNo This, Alt, Left;
    Description Hold;
    This = 0;
    for(Left = MaxItem + 1 ; Left ; ) {
        Alt = This + (Left --) * Random;
        Hold = Item[This];
        Item[This ++] = Item[Alt];
        Item[Alt] = Hold;
    }
}

// Swap "Window" items from the end of the window to the beginning.
void MoveItems(ItemNo Window)
{
	ItemNo j, i;
	Description hold;
	for(i = MaxItem, j = 0; i > (MaxItem - Window); i --, j ++) {
		hold = Item[i];
		Item[i] = Item[j];
		Item[j] = hold;
	}
}

/*************************************************************************/
/*									 */
/*  Grow a tree iteratively with initial window size Window and		 */
/*  initial window increment IncExceptions.				 */
/*									 */
/*  Construct a classifier tree using the data items in the		 */
/*  window, then test for the successful classification of other	 */
/*  data items by this tree.  If there are misclassified items,		 */
/*  put them immediately after the items in the window, increase	 */
/*  the size of the window and build another classifier tree, and	 */
/*  so on until we have a tree which successfully classifies all	 */
/*  of the test items or no improvement is apparent.			 */
/*									 */
/*  On completion, return the tree which produced the least errors.	 */
/*									 */
/*************************************************************************/


Tree Iterate(ItemNo * Window, ItemNo IncExceptions, ItemNo * noTrees)
{
	Tree FormTree(ItemNo Fp, ItemNo Lp);
	ClassNo Category(Description CaseDesc, Tree DecisionTree);
	void Swap(ItemNo a, ItemNo b);
	void InitialiseWeights(void);
	void ReleaseTree(Tree Node);
	void PrintTree(Tree T);
	int TreeSize(Tree T);
    Tree Classifier, BestClassifier = Nil;
    ItemNo i, Errors, TotalErrors, BestTotalErrors = MaxItem + 1, Exceptions, Additions;
    ClassNo Assigned;
    short Cycle = 0;
    Verbosity(1) {
    	printf("Cycle   Tree    -----Cases----    -----------------Errors-----------------\n");
    	printf("        size    window   other    window  rate   other  rate   total  rate\n");
    	printf("-----   ----    ------  ------    ------  ----  ------  ----  ------  ----\n");
    }
    do {
		//  Build a classifier tree with the first Window items
		InitialiseWeights();
		AllKnown = true;
		Classifier = FormTree(0, *(Window)-1);
		Verbosity(1) PrintTree(Classifier);
		// Error analysis
		Errors = Round(Classifier->Errors);
		// Move all items that are incorrectly classified by the classifier tree to immediately
	    // after the items in the current window.
		Exceptions = *(Window);
		ForEach(i, *(Window), MaxItem) {
	    	Assigned = Category(Item[i], Classifier);
	    	if(Assigned != Class(Item[i])) {
				Swap(Exceptions, i);
				Exceptions++;
	    	}
		}
        Exceptions -= *(Window);
		TotalErrors = Errors + Exceptions;
		++Cycle;
		Verbosity(1) {  // Print error analysis
			printf("%3d  %7d  %8d  %6d  %8d%5.1f%%  %6d%5.1f%%  %6d%5.1f%%\n", Cycle, TreeSize(Classifier),
			*(Window), MaxItem-*(Window)+1, Errors, 100*(float)Errors / *(Window), Exceptions,
			100*Exceptions/(MaxItem-*(Window)+1.001), TotalErrors, 100*TotalErrors/(MaxItem+1.0));
		}
		//  Keep track of the most successful classifier tree so far
		if(! BestClassifier || TotalErrors < BestTotalErrors) {
	    	if(BestClassifier) ReleaseTree(BestClassifier);
	    	BestClassifier = Classifier;
	   		BestTotalErrors = TotalErrors;
        }
		else ReleaseTree(Classifier);
		//  Increment window size
		Additions = Min(Exceptions, IncExceptions);
		*(Window) = Min(*(Window) + Max(Additions, Exceptions / 2), MaxItem + 1);
    }
    while(Exceptions);
    (*noTrees) = (int) Cycle;
    return BestClassifier;
}

/*************************************************************************/
/*									 */
/*	Print report of errors for each of the trials			 */
/*									 */
/*************************************************************************/

void Evaluate(Boolean CMInfo, short Saved)
/*  --------  */
{
	int TreeSize(Tree t);
	void PrintConfusionMatrix(ItemNo * ConfusionMat);
	ClassNo Category(Description CaseDesc, Tree DecisionTree);
    ClassNo RealClass, PrunedClass;
    short t;
    ItemNo * ConfusionMat, i, RawErrors, PrunedErrors;
    if(CMInfo) {
		ConfusionMat = (ItemNo *) calloc((MaxClass+1)*(MaxClass+1), sizeof(ItemNo));
    }
    Verbosity(1) {
    	printf("\n");
    	if(TRIALS > 1) {
			printf("Trial\t Before Pruning           After Pruning\n");
			printf("-----\t----------------   ---------------------------\n");
    	}
    	else {
			printf("\t Before Pruning           After Pruning\n");
			printf("\t----------------   ---------------------------\n");
    	}
    	printf("\tSize      Errors   Size      Errors   Estimate\n\n");
    }
    ForEach(t, 0, TRIALS-1) {
		RawErrors = PrunedErrors = 0;
		ForEach(i, 0, MaxItem) {
	    	RealClass = Class(Item[i]);
	    	if(Category(Item[i], Raw[t]) != RealClass) RawErrors++;
	    	PrunedClass = Category(Item[i], Pruned[t]);
	    	if(PrunedClass != RealClass) PrunedErrors++;
	    	if(CMInfo && t == Saved) {
				ConfusionMat[RealClass*(MaxClass+1)+PrunedClass]++;
	    	}
		}
    	Verbosity(1) {
			if(TRIALS > 1) {
	    		printf("%4d", t);
			}
			printf("\t%4d  %3d(%4.1f%%)   %4d  %3d(%4.1f%%)    (%4.1f%%)%s\n", TreeSize(Raw[t]), RawErrors, 100.0*RawErrors / (MaxItem+1.0),
	      		 TreeSize(Pruned[t]), PrunedErrors, 100.0*PrunedErrors / (MaxItem+1.0), 100 * Pruned[t]->Errors / Pruned[t]->Items,
	       		 ( t == Saved ? "   <<" : "" ));
		}
    }
    if(CMInfo) {
		Verbosity(1) PrintConfusionMatrix(ConfusionMat);
    }
}
