#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONSULTR_H_
#define CONSULTR_H_

#include "types.h"

// functions declaration
consultRulesResults * consultRules(ruleOptions * options, Configure * ruleConf, RuleSet set, char * * list);
									// version used with AVList

// setting the options
void setConsultRulesVerbosity(short verbosity);// Set the verbosity level [0-1] (default 0) This
									 // option generates more voluminous output that may help to
									 // explain what the program is doing

void setConsultRulesVisualiseRules(Boolean rules);// Display the rules at the start of the
									 // consulting session

#endif /*CONSULTR_H_*/

#ifdef __cplusplus
}
#endif
