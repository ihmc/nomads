#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONSULT_H_
#define CONSULT_H_

#include "types.h"

// functions declaration
consultTreeResults * consultTree(treeOptions * options, Configure * treeConf, Tree tree, char * * list);
								// New version used with the c++ interface

// setting the options
void setConsultTreevisualiseTree(Boolean tree); // Display the decision tree at the start of
											    // the consulting session

void setConsultTreeVerbosity(short verbosity);// Set the verbosity level [0-1] (default 0)
												// This option generates more voluminous output
												// that may help to explain what the program is doing

#endif /*CONSULT_H_*/

#ifdef __cplusplus
}
#endif
