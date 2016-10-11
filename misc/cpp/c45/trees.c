/*************************************************************************/
/*									 */
/*	Routines for displaying, building, saving and restoring trees	 */
/*	-------------------------------------------------------------	 */
/*									 */
/*************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#if defined (UNIX)
    #include<strings.h>
#endif
#include "defns.h"
#include "types.h"
#include "extern.h"

#define	Tab		"|   "
#define	TabSize		4
#define	Width		80	// approx max width of printed trees

// If lines look like getting too long while a tree is being printed, subtrees are broken off and printed
// separately after the main tree is finished

short Subtree;		// highest subtree to be printed
Tree Subdef[100];	// pointers to subtrees

// functions
void PrintTree(Tree T);
void Show(Tree T, short Sh);
void ShowBranch(short Sh, Tree T, DiscrValue v);
short MaxLine(Tree St);
void Indent(short Sh, char * Mark);
void ReleaseTree(Tree Node);
Tree Leaf(ItemCount * ClassFreq, ClassNo NodeClass, ItemCount Cases, ItemCount Errors);
void Sprout(Tree Node, DiscrValue Branches);
int TreeSize(Tree Node);
Tree CopyTree(Tree T);

/*************************************************************************/
/*	Display entire decision tree T					*/
/*************************************************************************/

void PrintTree(Tree T)
/*  ----------  */
{
    short s;
    Subtree = 0;
    printf("Decision Tree:\n");
    Show(T, 0);
    printf("\n");
    ForEach(s, 1, Subtree) {
        printf("\n\nSubtree [S%d]\n", s);
        Show(Subdef[s], 0);
        printf("\n");
    }
    printf("\n");
}

/*************************************************************************/
/*	Display the tree T with offset Sh				 */
/*************************************************************************/

void Show(Tree T, short Sh)
/*  ---- */
{
    DiscrValue v, MaxV;
    short MaxLine();
    if(T->NodeType) {
        // See whether separate subtree needed
        if(T != Nil && Sh && Sh * TabSize + MaxLine(T) > Width) {
            if(Subtree < 99) {
                Subdef[++Subtree] = T;
                printf("[S%d]", Subtree);
            }
            else {
                printf("[S??]");
            }
        }
        else {
            MaxV = T->Forks;
            // Print simple cases first
            ForEach(v, 1, MaxV) {
                if(! T->Branch[v]->NodeType) {
                    ShowBranch(Sh, T, v);
                }
            }
            // Print subtrees
            ForEach(v, 1, MaxV) {
                if(T->Branch[v]->NodeType) {
                    ShowBranch(Sh, T, v);
                }
            }
        }
    }
    else {
        printf(" %s (%.1f", ClassName[T->Leaf], T->Items);
        if(T->Errors > 0) printf("/%.1f", T->Errors);
        printf(")");
    }
}

/*************************************************************************/
/*									 */
/*	Print a node T with offset Sh, branch value v, and continue	 */
/*									 */
/*************************************************************************/

void ShowBranch(short Sh, Tree T, DiscrValue v)
/*  -----------  */
{
    DiscrValue Pv, Last;
    Attribute Att;
    Boolean FirstValue;
    short TextWidth, Skip, Values = 0, i;
    Att = T->Tested;
    switch(T->NodeType) {
        case BrDiscr:
            Indent(Sh, Tab);
            printf("%s = %s:", AttName[Att], AttValName[Att][v]);
            break;
        case ThreshContin:
            Indent(Sh, Tab);
            printf("%s %s %g ", AttName[Att], ( v == 1 ? "<=" : ">" ), T->Cut);
            if(T->Lower != T->Upper) {
                printf("[%g,%g]", T->Lower, T->Upper);
            }
            printf(":");
            break;
        case BrSubset:
            // Count values at this branch
            ForEach(Pv, 1, MaxAttVal[Att]) {
                if(In(Pv, T->Subset[v])) {
                    Last = Pv;
                    Values++;
                }
            }
            if(! Values) return;
            Indent(Sh, Tab);
            if(Values == 1) {
                printf("%s = %s:", AttName[Att], AttValName[Att][Last]);
                break;
            }
            printf("%s in {", AttName[Att]);
            FirstValue = true;
            Skip = TextWidth = strlen(AttName[Att]) + 5;
            ForEach(Pv, 1, MaxAttVal[Att]) {
                if(In(Pv, T->Subset[v])) {
                    if(! FirstValue && TextWidth + strlen(AttValName[Att][Pv]) + 11 > Width) {
                        Indent(Sh, Tab);
                        ForEach(i, 1, Skip) putchar(' ');
                        TextWidth = Skip;
                        FirstValue = true;
                    }
                    printf("%s%c", AttValName[Att][Pv], Pv == Last ? '}' : ',');
                    TextWidth += strlen(AttValName[Att][Pv]) + 1;
                    FirstValue = false;
                }
            }
            putchar(':');
    }
    Show(T->Branch[v], Sh+1);
}

/*************************************************************************/
/*									 */
/*	Find the maximum single line size for non-leaf subtree St.	 */
/*	The line format is						 */
/*			<attribute> <> X.xx:[ <class (<Items>)], or	 */
/*			<attribute> = <DVal>:[ <class> (<Items>)]	 */
/*									 */
/*************************************************************************/

short MaxLine(Tree St)
/*    --------  */
{
    Attribute a;
    DiscrValue v, MaxV, Next;
    short Ll, MaxLl = 0;
    a = St->Tested;
    MaxV = St->Forks;
    ForEach(v, 1, MaxV) {
        Ll = ( St->NodeType == 2 ? 4 : strlen(AttValName[a][v]) ) + 1;
        // Find the appropriate branch
        Next = v;
        if(! St->Branch[Next]->NodeType) {
            Ll += strlen(ClassName[St->Branch[Next]->Leaf]) + 6;
        }
        MaxLl = Max(MaxLl, Ll);
    }
    return strlen(AttName[a]) + 4 + MaxLl;
}

/*************************************************************************/
/*								   	 */
/*	Indent Sh columns					  	 */
/*								  	 */
/*************************************************************************/

void Indent(short Sh, char * Mark)
/*  ------  */
{
    printf("\n");
    while(Sh--) printf("%s", Mark);
}

/*************************************************************************/
/*									 */
/*	Free up space taken up by tree Node				 */
/*									 */
/*************************************************************************/

void ReleaseTree(Tree Node)
/*  -------  */
{
    DiscrValue v, x;
    if(Node->NodeType) {
        ForEach(v, 1, Node->Forks) {
            ReleaseTree(Node->Branch[v]);
        }
        free(Node->Branch);
        if(Node->NodeType == BrSubset) {
            ForEach(x, 1, Node->Forks)
                free(Node->Subset[x]);
            free(Node->Subset);
        }
    }
    free(Node->ClassDist);
    free(Node);
}

/*************************************************************************/
/*									 */
/*	Construct a leaf in a given node				 */
/*									 */
/*************************************************************************/

Tree Leaf(ItemCount * ClassFreq, ClassNo NodeClass, ItemCount Cases, ItemCount Errors)
/*   ----  */
{
    Tree Node;
    Node = (Tree) calloc(1, sizeof(TreeRec));
    Node->ClassDist = (ItemCount *) calloc(MaxClass+1, sizeof(ItemCount));
    memcpy(Node->ClassDist, ClassFreq, (MaxClass+1) * sizeof(ItemCount));
    Node->NodeType = 0; 
    Node->Leaf = NodeClass;
    Node->Items	= Cases;
    Node->Errors = Errors;
    return Node;
}

/*************************************************************************/
/*									 */
/*	Insert branches in a node 	                 		 */
/*									 */
/*************************************************************************/

void Sprout(Tree Node, DiscrValue Branches)
/*  ------  */
{
    Node->Forks = Branches;
    Node->Branch = (Tree *) calloc(Branches+1, sizeof(Tree));
}

/*************************************************************************/
/*									 */
/*	Count the nodes in a tree					 */
/*									 */
/*************************************************************************/
	
int TreeSize(Tree Node)
{
    int Sum = 0;
    DiscrValue v;
    if(Node->NodeType) {
        ForEach(v, 1, Node->Forks) {
            Sum += TreeSize(Node->Branch[v]);
        }
    }
    return Sum + 1;
}

/*************************************************************************/
/*									 */
/*	Return a copy of tree T						 */
/*									 */
/*************************************************************************/

Tree CopyTree(Tree T)
{
    DiscrValue v;
    Tree New;
    New = (Tree) malloc(sizeof(TreeRec));
    memcpy(New, T, sizeof(TreeRec));
    New->ClassDist = (ItemCount *) calloc(MaxClass+1, sizeof(ItemCount));
    memcpy(New->ClassDist, T->ClassDist, (MaxClass + 1) * sizeof(ItemCount));
    if(T->NodeType) {
        New->Branch = (Tree *) calloc(T->Forks + 1, sizeof(Tree));
        ForEach(v, 1, T->Forks) {
            New->Branch[v] = CopyTree(T->Branch[v]);
        }
    }
    return New;
}
