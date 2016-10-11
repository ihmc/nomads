#ifndef BUILDEX_H_
#define BUILDEX_H_

/*************************************************************************/
/*									 */
/*	  Global data for C4.5 used for building decision trees		 */
/*	  -----------------------------------------------------		 */
/*									 */
/*************************************************************************/

#include "defns.h"
#include "types.h"
#include "extern.h"

extern ItemCount * Weight;		// Weight[i]  = current fraction of item i
extern ItemCount * * Freq;		// Freq[x][c] = no. items of class c with outcome x
extern ItemCount * ValFreq;		// ValFreq[x] = no. items with att value v
extern float * Gain;			// Gain[a] = info gain by split on att a
extern float * Info;			// Info[a] = potential info from split on att a
extern float * Bar;				// Bar[a]  = best threshold for contin att a
extern float * UnknownRate;		// UnknownRate[a] = current unknown rate for att a
extern char * Tested;			// Tested[a] = true if att a already tested

#endif /*BUILDEX_H_*/
