#ifndef RULEX_H_
#define RULEX_H_

/*************************************************************************/
/*									 */
/*	Global data for constructing and applying rules			 */
/*      -----------------------------------------------			 */
/*									 */
/*************************************************************************/

extern PR * Rule;				// production rules
extern RuleNo NRules;			// number of production rules
extern RuleNo * RuleIndex;		// index to production rules
extern short RuleSpace;			// space currently allocated for rules
extern RuleSet * PRSet;			// set of rulesets
extern ClassNo DefaultClass;	// default class associated with ruleset
extern Boolean SIGTEST;			// use Fisher's test in rule pruning
extern Boolean SIMANNEAL;		// use simulated annealing
extern float SIGTHRESH;			// sig level used in rule pruning
extern float REDUNDANCY;		// factor governing encoding tradeoff between rules and exceptions
extern float AttTestBits;		// average bits to encode tested attribute
extern float * BranchBits;		// ditto attribute value
extern float * LogItemNo;		// LogItemNo[i] = log2(i)
extern double * LogFact;	    // LogFact[i] = log2(i!)

#endif /*RULEX_H_*/
