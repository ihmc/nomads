#ifdef __cplusplus
extern "C" {
#endif

#ifndef C4_5RULES_H_
#define C4_5RULES_H_

#include "types.h"

// functions declaration
processRulesResults * createRules(ruleOptions * options, Configure * ruleConf, int maxI, Description * items, processTreeResults * unpruned);
										 // version used with AVList

void testRuleset(ruleOptions * options, Configure * ruleConf, int maxI, Description * items, processRulesResults * ruleResult, Boolean drop);
										 // version used with AVList

// setting the options
void setRulesVerbosity(int level);  	 // Set the verbosity level [0-1] (default 0) This option
									     // generates more voluminous output that may help to
									     // explain what the program is doing
									
Boolean setRulesConfidence(float confidence);// Set the pruning confidence level (default 25%)

Boolean setRulesRedundancy(float redundancy);// If many irrelevant or redundant attributes are included,
									      // estimate the ratio of attributes to ``sensible''attributes
									      // (default 1)
									 
Boolean setRulesSignificanceTest(float thresh);// Invoke Fisher's significance test when pruning rules.
									        // If a rule contains a condition whose probability of being
									        // irrelevant is greater than the stated level, the rule is
									        // pruned further (default: no significance testing)
									   
void setRulesAnnealing(Boolean set);        // Simulated annealing for selecting rules

#endif /*C4_5RULES_H_*/

#ifdef __cplusplus
}
#endif
