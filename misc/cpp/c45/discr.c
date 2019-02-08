/*************************************************************************/
/*	Evaluation of a test on a discrete valued attribute		 */
/*      ---------------------------------------------------		 */
/*************************************************************************/

#include "buildex.h"

// functions
void EvalDiscreteAtt(Attribute Att, ItemNo Fp, ItemNo Lp, ItemCount Items);
void ComputeFrequencies(Attribute Att, ItemNo Fp, ItemNo Lp);
float DiscrKnownBaseInfo(ItemCount KnownItems, DiscrValue MaxVal);
void DiscreteTest(Tree Node, Attribute Att);

/*************************************************************************/
/*  Set Info[] and Gain[] for discrete partition of items Fp to Lp	 */
/*************************************************************************/

void EvalDiscreteAtt(Attribute Att, ItemNo Fp, ItemNo Lp, ItemCount Items)
/*  ---------------  */
{
    void ComputeFrequencies(Attribute Att, ItemNo Fp, ItemNo Lp);
    void PrintDistribution(Attribute Att, DiscrValue MaxVal, Boolean ShowNames);
    float DiscrKnownBaseInfo(ItemCount KnownItems, DiscrValue MaxVal);
    float ComputeGain(float BaseInfo, float UnknFrac, DiscrValue MaxVal, ItemCount TotalItems);
    float TotalInfo(ItemCount V[], DiscrValue MinVal, DiscrValue MaxVal);
    ItemCount KnownItems;
    ComputeFrequencies(Att, Fp, Lp);
    KnownItems = Items - ValFreq[0];
    // Special case when no known values of the attribute
    if(Items <= ValFreq[0]) {
        Verbosity(1) printf("\tAtt %s: no known values\n", AttName[Att]);
        Gain[Att] = (float) (-Epsilon);
        Info[Att] = 0.0f;
        return;
    }
    Gain[Att] = ComputeGain(DiscrKnownBaseInfo(KnownItems, MaxAttVal[Att]), UnknownRate[Att], MaxAttVal[Att], KnownItems);
    Info[Att] = TotalInfo(ValFreq, 0, MaxAttVal[Att]) / Items;
    Verbosity(1) {
    	printf("\tAtt %s", AttName[Att]);
    	PrintDistribution(Att, MaxAttVal[Att], true);
    	printf("\tinf %.3f, gain %.3f\n", Info[Att], Gain[Att]);
    }

}

/*************************************************************************/
/*  Compute frequency tables Freq[][] and ValFreq[] for attribute	 */
/*  Att from items Fp to Lp, and set the UnknownRate for Att		 */
/*************************************************************************/

void ComputeFrequencies(Attribute Att, ItemNo Fp, ItemNo Lp)
/*  ------------------  */
{
    void ResetFreq(DiscrValue MaxVal);
    ItemCount CountItems(ItemNo Fp, ItemNo Lp);
    Description Case;
    ClassNo c;
    DiscrValue v;
    ItemNo p;
    ResetFreq(MaxAttVal[Att]);
    //  Determine the frequency of each class amongst cases with each possible value for the given attribute
    ForEach(p, Fp, Lp) {
        Case = Item[p];
        Freq[ DVal(Case,Att) ][ Class(Case) ] += Weight[p];
    }
    // Determine the frequency of each possible value for the given attribute
    ForEach(v, 0, MaxAttVal[Att]) {
        ForEach(c, 0, MaxClass) {
            ValFreq[v] += Freq[v][c];
        }
    }
    //  Set the rate of unknown values of the attribute
    UnknownRate[Att] = ValFreq[0] / CountItems(Fp, Lp);
}

/*************************************************************************/
/*									 */
/*  Return the base info for items with known values of a discrete	 */
/*  attribute, using the frequency table Freq[][]			 */
/*	 								 */
/*************************************************************************/

float DiscrKnownBaseInfo(ItemCount KnownItems, DiscrValue MaxVal)
/*    ------------------  */
{
    ClassNo c;
    ItemCount ClassCount;
    double Sum = 0;
    DiscrValue v;
    float retx;
    ForEach(c, 0, MaxClass) {
        ClassCount = 0;
        ForEach(v, 1, MaxVal) {
            ClassCount += Freq[v][c];
        }
        Sum += ClassCount * Log(ClassCount);
    }
    retx = (float)((KnownItems * Log(KnownItems) - Sum) / KnownItems);
    return retx;
}

/*************************************************************************/
/*									 */
/*  Construct and return a node for a test on a discrete attribute	 */
/*									 */
/*************************************************************************/

void DiscreteTest(Tree Node, Attribute Att)
/*  ----------  */
{
    void Sprout(Tree Node, DiscrValue Branches);
    Sprout(Node, MaxAttVal[Att]);
    Node->NodeType = BrDiscr;
    Node->Tested = Att;
    Node->Errors = 0;
}
