/*************************************************************************/
/*									 */
/*  Statistical routines for C4.5					 */
/*  -----------------------------					 */
/*									 */
/*************************************************************************/

#include "defns.h"
#include "types.h"
#include "extern.h"

// functions
float AddErrs(ItemCount N, ItemCount e);

/*************************************************************************/
/*									 */
/*  Compute the additional errors if the error rate increases to the	 */
/*  upper limit of the confidence level.  The coefficient is the	 */
/*  square of the number of standard deviations corresponding to the	 */
/*  selected confidence level.  (Taken from Documenta Geigy Scientific	 */
/*  Tables (Sixth Edition), p185 (with modifications).)			 */
/*									 */
/*************************************************************************/

float Val[] = {  0.0f,  0.001f, 0.005f, 0.01f, 0.05f, 0.10f, 0.20f, 0.40f, 1.00f};
float Dev[] = {4.0f,  3.09f,  2.58f,  2.33f, 1.65f, 1.28f, 0.84f, 0.25f, 0.00f};


float AddErrs(ItemCount N, ItemCount e)
/*    -------  */
{
    static float Coeff = 0;
    float Val0, Pr;
    if(! Coeff) {
        // Compute and retain the coefficient value, interpolating from the values in Val and Dev
        int i;
        i = 0;
        while(CF > Val[i]) i++;
        Coeff = Dev[i-1] + (Dev[i] - Dev[i-1]) * (CF - Val[i-1]) /(Val[i] - Val[i-1]);
        Coeff = Coeff * Coeff;
    }
    if(e < 1E-6) {
        return (float)(N * (1 - exp(log(CF) / N)));
    }
    else if(e < 0.9999) {
        Val0 = (float)(N * (1 - exp(log(CF) / N)));
        return Val0 + e * (AddErrs(N, 1.0) - Val0);
    }
    else if(e + 0.5 >= N) {
        return (float) (0.67 * (N - e));
    }
    else {
        Pr = (float)((e + 0.5 + Coeff/2 + sqrt(Coeff * ((e + 0.5) * (1 - (e + 0.5)/N) + Coeff/4))) / (N + Coeff));
        return (N * Pr - e);
    }
}
