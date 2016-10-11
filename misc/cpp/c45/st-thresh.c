/*************************************************************************/
/*									 */
/*	Soften thresholds for continuous attributes			 */
/*	-------------------------------------------			 */
/*									 */
/*************************************************************************/

#include<stdlib.h>
#include "defns.h"
#include "types.h"
#include "extern.h"

Boolean * LHSErr;       // Does a misclassification occur with this value of an att
Boolean	* RHSErr;       // if the below or above threshold branches are taken
ItemNo * ThreshErrs;    // ThreshErrs[i] is the no. of misclassifications if thresh is i
float * CVals;          // All values of a continuous attribute

#define	Below(v,t)	(v <= t + 1E-6)

// functions
void SoftenThresh(Tree T);
void ScanTree(Tree T, ItemNo Fp, ItemNo Lp);

/*************************************************************************/
/*									 */
/*  Soften all thresholds for continuous attributes in tree T		 */
/*									 */
/*************************************************************************/

void SoftenThresh(Tree T)
/*  ------------  */
{
    void InitialiseWeights(void);
    CVals = (float *) calloc(MaxItem+1, sizeof(float));
    LHSErr = (Boolean *) calloc(MaxItem+1, sizeof(Boolean));
    RHSErr = (Boolean *) calloc(MaxItem+1, sizeof(Boolean));
    ThreshErrs = (ItemNo *) calloc(MaxItem+1, sizeof(ItemNo));
    InitialiseWeights();
    ScanTree(T, 0, MaxItem);
    free(ThreshErrs);
    free(RHSErr);
    free(LHSErr);
    free(CVals);
}

/*************************************************************************/
/*								  	 */
/*  Calculate upper and lower bounds for each test on a continuous	 */
/*  attribute in tree T, using data items from Fp to Lp			 */
/*								  	 */
/*************************************************************************/

void ScanTree(Tree T, ItemNo Fp, ItemNo Lp)
/*  --------  */
{
    ItemNo Group(DiscrValue V, ItemNo Fp, ItemNo Lp, Tree TestNode);
    void Swap(ItemNo a, ItemNo b);
    void Quicksort(ItemNo Fp, ItemNo Lp, Attribute Att, void (*Exchange)());
    float GreatestValueBelow(Attribute Att, float t);
    ClassNo Category(Description CaseDesc, Tree DecisionTree);
    short v;
    float Val, Se, Limit, Lower, Upper;
    ItemNo i, Kp, Ep, LastI, Errors, BaseErrors;
    ClassNo CaseClass, Class1, Class2;
    Boolean LeftThresh = false;
    Description CaseDesc;
    Attribute Att;
    // Stop when get to a leaf
    if(! T->NodeType) return;
    // Group the unknowns together
    Kp = Group(0, Fp, Lp, T);
    // Soften a threshold for a continuous attribute
    Att = T->Tested;
    if(T->NodeType == ThreshContin) {
        Verbosity(1) printf("\nTest %s <> %g\n", AttName[Att], T->Cut);
        Quicksort(Kp+1, Lp, Att, Swap);
        ForEach(i, Kp+1, Lp) {
            // See how this item would be classified if its value were on each side of the threshold
            CaseDesc = Item[i];
            CaseClass = Class(CaseDesc);
            Val = CVal(CaseDesc, Att);
            Class1 = Category(CaseDesc, T->Branch[1]);
            Class2 = Category(CaseDesc, T->Branch[2]);
            CVals[i] = Val;
            LHSErr[i] = (Class1 != CaseClass ? 1 : 0);
            RHSErr[i] = (Class2 != CaseClass ? 1 : 0);
        }
        // Set Errors to total errors if take above thresh branch, and BaseErrors to errors if threshold has original value
        Errors = BaseErrors = 0;
        ForEach(i, Kp+1, Lp) {
            Errors += RHSErr[i];
            if(Below(CVals[i], T->Cut)) {
                BaseErrors += LHSErr[i];
            }
            else {
                BaseErrors += RHSErr[i];
            }
        }
        // Calculate standard deviation of the number of errors
        Se = (float)(sqrt( (BaseErrors+0.5) * (Lp-Kp-BaseErrors+0.5) / (Lp-Kp+1) ));
        Limit = BaseErrors + Se;
        Verbosity(1) {
            printf("\t\t\tBase errors %d, items %d, se=%.1f\n", BaseErrors, Lp-Kp, Se);
                    printf("\n\tVal <=   Errors\t\t+Errors\n");
            printf("\t         %6d\n", Errors);
        }
        // Set ThreshErrs[i] to the no. of errors if the threshold were i
        ForEach(i, Kp+1, Lp) {
            ThreshErrs[i] = Errors = Errors + LHSErr[i] - RHSErr[i];
            if(i == Lp || CVals[i] != CVals[i+1]) {
                Verbosity(1) printf("\t%6g   %6d\t\t%7d\n", CVals[i], Errors, Errors - BaseErrors);
            }
        }
        // Choose Lower and Upper so that if threshold were set to either, the number of items misclassified would be one
        // standard deviation above BaseErrors
        LastI = Kp+1;
        Lower = Min(T->Cut, CVals[LastI]);
        Upper = Max(T->Cut, CVals[Lp]);
        while(CVals[LastI+1] == CVals[LastI]) LastI++;
        while(LastI < Lp) {
            i = LastI + 1;
            while(i < Lp && CVals[i+1] == CVals[i]) i++;
            if(! LeftThresh && ThreshErrs[LastI] > Limit && ThreshErrs[i] <= Limit && Below(CVals[i], T->Cut)) {
                Lower = CVals[i] - (CVals[i] - CVals[LastI]) * (Limit - ThreshErrs[i]) / (ThreshErrs[LastI] - ThreshErrs[i]);
                LeftThresh = true;
            }
            else if(ThreshErrs[LastI] <= Limit && ThreshErrs[i] > Limit && ! Below(CVals[i], T->Cut)) {
                Upper = CVals[LastI] + (CVals[i] - CVals[LastI]) * (Limit - ThreshErrs[LastI])/(ThreshErrs[i] - ThreshErrs[LastI]);
                if(Upper < T->Cut) Upper = T->Cut;
            }
            LastI = i;
        }
        T->Lower = Lower;
        T->Upper = Upper;
        Verbosity(1) {
            printf("\n");
            printf("\tLower = %g, Upper = %g\n", T->Lower, T->Upper);
        }
    }
    // Recursively scan each branch
    ForEach(v, 1, T->Forks) {
        Ep = Group(v, Kp+1, Lp, T);
        if(Kp < Ep) {
            ScanTree(T->Branch[v], Kp+1, Ep);
            Kp = Ep;
        }
    }
}
