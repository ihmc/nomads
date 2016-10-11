#ifdef __cplusplus
extern "C" {
#endif

#ifndef C4_5_H_
#define C4_5_H_

#include "types.h"

CError * getNames(Configure * treeConf, const char * attribute, const char * value);
                                        // Retrieve the classes, attributes and value names from
                                        // AVList, puts the results in treeConfigure struct.

CError * getDataset(Configure * treeConf, const char * attribute, const char * value, int MaxItem, Description * Item);
                                        // Retrive a single record from AVList and puts the results in
                                        // MaxItem and Item.

processTreeResults * constructTree(treeOptions * options, Configure * treeConf, int maxI, Description * items);
                                        // Costruct a tree with the given options, classes, attributes
                                        // and names (treeConf), maximum number of items (maxI), and
                                        // the items vector (items), for AVList record.

int iterativeMode(treeOptions * options, Configure * treeConf, int win, int maxI, Description * items,
                  processTreeResults * treeSet, int * noTrees);
                                        // version for AVList

void testTree(treeOptions * options, Configure * treeConf, int maxI, Description * items,
              processTreeResults * treeSet, short testFlag);
                                        // testFlag = 0 -> test the unpruned tree, = 1 the pruned one,
                                        // = 2 all the 2 trees.

int treeDim(Tree tree);                    // Give the size of the passed tree. Doesn't check the input.

// setting the options
void setTreeSubsetting(Boolean subset);// Force `subsetting' of all tests based on discrete attributes
                                       // with more than two values. C4.5 will construct a test with
                                       // a subset of values associated with each branch.

void setTreeProbabilisticThresholds(Boolean thresholds);// Probabilistic thresholds used for
                                                        // continuous attributes.

void setTreeBatchMode(void);            // use the batch mode (default mode).

Boolean setTreeIterativeMode(int trials);// Set iterative mode with specified number of trials.

void setTreeVerbosity(int level);          // Set the verbosity level [0-1] (default 0) This option
                                        // generates more voluminous output that may help to
                                        // explain what the program is doing.

Boolean setTreeInitialWindow(int window);// Set the size of the initial window (default is the maximum
                                        // of 20 percent and twice the square root of the number of
                                        // data objects).

Boolean setTreeMaxIncrement(int increment);// Set the maximum number of objects that can be added to the
                                        // window at each iteration (default is 20 percent of the
                                        // initial window size).

void setTreeGainRatioCriterion(Boolean ratio);// if ratio = true use the gain ratio criterion to select
                                              // tests, if ratio = false use the gain criterion.
                                              // The default uses the gain ratio criterion.

Boolean setTreeMinObjects(int minimum); // In all tests, at least two branches must contain a minimum
                                        // number of objects (default 2)
                                        // This option allows the minimum number to be altered.

Boolean setTreeConfidence(float confidence);// Set the pruning confidence level (default 25%).

#endif /*C4_5_H_*/

#ifdef __cplusplus
}
#endif
