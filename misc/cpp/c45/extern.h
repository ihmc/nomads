#ifndef EXTERN_H_
#define EXTERN_H_

#include "types.h"
/*************************************************************************/
/*									 */
/*		Global data for C4.5					 */
/*		--------------------					 */
/*									 */
/*************************************************************************/

extern short MaxAtt;			// max att number
extern short MaxClass;			// max class number
extern short MaxDiscrVal;		// max discrete values for any att
extern ItemNo MaxItem;			// max data item number
extern Description * Item;		// data items
extern DiscrValue * MaxAttVal;	// number of values for each att
extern char	* SpecialStatus;	// special att treatment
extern String * ClassName;		// class names
extern String * AttName;		// att names
extern String * * AttValName;	// att value names
extern Boolean AllKnown;		// true if there have been no splits on atts with missing values above
					   			// the current position in the tree

/*************************************************************************/
/*									 */
/*		Global parameters for C4.5				 */
/*		--------------------------				 */
/*									 */
/*************************************************************************/

extern short VERBOSITY;			// verbosity level (0 = none)
extern short TRIALS;			// number of trees to be grown
extern Boolean GAINRATIO;		// true=gain ratio, false=gain
extern Boolean SUBSET;			// true if subset tests allowed
extern Boolean BATCH;			// true if windowing turned off
extern Boolean UNSEENS;			// true if to evaluate on test data
extern Boolean PROBTHRESH;		// true if to use soft thresholds
extern ItemNo MINOBJS;			// minimum items each side of a cut
extern ItemNo WINDOW;			// initial window size
extern ItemNo INCREMENT;		// max window increment each iteration
extern float CF;				// confidence limit for tree pruning

#endif /*EXTERN_H_*/
