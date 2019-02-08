/*
 * C45DecisionTree.h
 *
 * This file is part of the IHMC C4.5 Decision Tree Library.
 * Copyright (c) 2008-2016 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 *
 * Author: Giacomo Benincasa            (gbenincasa@ihmc.us)
 * Created on November 23, 2011, 12:00 PM
 */

#ifndef INCL_C45_DECISION_TREE_H
#define INCL_C45_DECISION_TREE_H

#include "FTypes.h"
#include "Mutex.h"

#include "Classifier.h"
#include "C45TreeTestInfo.h"
#include "C45TreePrediction.h"

#include "types.h"

namespace NOMADSUtil
{
    class Mutex;
    class Reader;
    class Writer;
}

namespace IHMC_C45
{
    class C45AVList;
    class C45TreeInfo;

    class C45DecisionTree : public Classifier
    {
        public:
            friend class C45Rules;
            friend class DataGenerator;

            C45DecisionTree();
            virtual ~C45DecisionTree();
            // Initialize, construct and use the tree
            int configureClassifier(C45AVList * attributes);
                                                        // Set the classes, attributes and values names.
                                                        // Returns 0 if no errors occurred or a code
                                                        // between 1 and 10 if there is an error.

            C45TreeInfo * createNewTree(C45AVList * dataset);
                                                        // Create a couple of trees, one pruned and one
                                                        // unpruned. returns NULL if an error occurred.

            C45TreeInfo * createCompositeTree(C45DecisionTree * * trees, int noTrees);
                                                        // Create a new couple of trees from the passed
                                                        // trees. This function is useful when you have
                                                        // already constructed some trees with the same
                                                        // configuration but with different training
                                                        // datasets.

            int setupCycleMode(int initialWindow, int maxWindow, int increment, float percCycleErr, float percCycleWin);
                                                        // Costruct a couple of trees like createNewTree
                                                        // but the trees are constructed when the
                                                        // dataset contains at least "initialWindow"
                                                        // elements; then a new couple of trees is
                                                        // constructed again every time that the
                                                        // dataset is increased with "increment" number
                                                        // of elements. If the dataset reach "maxWindow"
                                                        // it automatically delete the first data
                                                        // inserted. Return a code for the errors.
                                                        // InitialWindow, maxWindow and increment must
                                                        // be in the range [1, 1000000] or they aren't
                                                        // accepted. Specify a % value in the range [0 - 1]
                                                        // for percCycleErr. This parameter indicates the
                                                        // max number of items that can be added to the
                                                        // window at each cycle in % of "increment" size.
                                                        // When percCycleErr is set to 0.0, the default value
                                                        // 0.20 (20% of increment size) will be used.
                                                        // The last parameter percCycleWin is a % value in the
                                                        // range [0 - 1] and represent the initial window size
                                                        // at the beginning of each cycle in % of the actual
                                                        // window in use. When percCycleWin is set to 0.0, the
                                                        // default value 1.0 (100% of actual window size) will
                                                        // be used.

            int setupWindowMode(int initialWindow, int maxWindow, int increment);
                                                        // new mode with window

            int addNewData(C45AVList * dataset);        // Add new data to the internal dataset. It
                                                        // works with "iterateTree". Returns a code
                                                        // for the errors.

            C45TreePrediction * consultClassifier(C45AVList * record);
                                                        // Consult the pruned tree for the given
                                                        // record. Returns a code for the errors.

            C45TreePrediction * consultUnprunedTree(C45AVList * record);
                                                        // Consult the unpruned tree for the given
                                                        // record. Returns a code for the errors.

            C45TreeTestInfo * testClassifierOnData(C45AVList * dataset);
                                                        // Test the pruned tree on the given data.
                                                        // Returns a code for the errors.

            C45TreeTestInfo * testPrunedTree(void);     // Test the pruned tree. If you have already
                                                        // done other tests, this function test the
                                                        // tree on the last dataset used in the tests,
                                                        // else the tree will be tested on the dataset
                                                        // used to construct the tree. In these cases
                                                        // is better to use this function instead of
                                                        // "testPrunedTreeOnData()" because it avoids
                                                        // to pre-process the dataset again. Returns
                                                        // a code for the errors.

            C45TreeTestInfo * testUnprunedTreeOnData(C45AVList * dataset);
                                                        // Test the unpruned tree on the given data.
                                                        // Returns a code for the errors.

            C45TreeTestInfo * testUnprunedTree(void);   // It works as "testPrunedTree()" but for the
                                                        // unpruned tree.

            C45TreeTestInfo * * testBothTreesOnData(C45AVList * dataset);
                                                        // Test the pruned and the unpruned tree on the
                                                        // given data. Returns a code for the errors.

            C45TreeTestInfo * * testBothTrees(void);    // Use this function in order to test the pruned
                                                        // and the unpruned tree on the dataset used to
                                                        // construct the tree.

            // Save space
            void deletePrunedTree(void);                // Delete the pruned tree.

            void deleteUnprunedTree(void);              // Delete the unpruned tree.

            void deleteTestData(void);                  // Delete the dataset used in the last test to save
                                                        // space in memory

            // construction options
            void restoreDefaultOptions(void);           // restore the default values for every option

            bool setInitialWindow(int initialWindow);   // Set the initial window parameter for the
                                                        // iterateTree() method. The passed value must
                                                        // be in the range [1, 1000000] or is not
                                                        // accepted (in this case "false" is returned).

            bool setMaximumWindow(int maxWindow);       // Set the maximum size for the window for
                                                        // iterateTree() method. The passed value must
                                                        // be in the range [1, 1000000] and > of the
                                                        // initial window or is not accepted (in this
                                                        // case "false" is returned).

            bool setIncrementSize(int increment);       // Set the increment size for iterateTree()
                                                        // method. The passed value must be in the range
                                                        // [1, 1000000] and respect the condition
                                                        // initialWindow + increment <= maxWindow
                                                        // or is not accepted (in this case "false" is
                                                        // returned).

            void setPercentMaximumErrors(float percCycleErr);
                                                        // Set the "percCycleErr" parameter used in Cycle Mode

            void setPercentInizialWindow(float percCycleWin);
                                                        // Set the "percCycleWin" parameter used in Cycle Mode

            void setProbabilisticThresholds(bool threshold);
                                                        // Probabilistic thresholds used for continuous
                                                        // attributes. For default is not used.

            void setGainCriterion(bool gain);           // If gain = true use the gain criterion
                                                        // to construct trees, if gain = false use the
                                                        // gain ratio criterion. The default uses the
                                                        // gain ratio criterion.

            bool setConfidence(float confidence);       // Set the pruning confidence level. The
                                                        // default is 0.25. The range must be between
                                                        // 0,001 and 100, if not "false" is returned and
                                                        // the option is not accepted.

            void setVerbosity(bool verb);               // If the verbosity is set, a standard output
                                                        // is generated. That may help to explain what
                                                        // the program is doing.

            void setSubsetting(bool subset);            // Force `subsetting' of all tests based on
                                                        // discrete attributes with more than two
                                                        // values. C4.5 will construct a test with
                                                           // a subset of values associated with each
                                                           // branch.

            bool setMinObjects(int minimum);            // In all tests, at least two branches must
                                                        // contain a minimum number of objects (default
                                                        // 2). This option allows the minimum number to
                                                        // be altered. The number must be between 1 and
                                                        // 1000000, if not "false" is returned and the
                                                        // option is not accepted.

            // Retrive the results
            C45TreeInfo * getTreeInfo(void);            // Returns some information about the constructed
                                                        // trees (pruned and unpruned). This method calculate
                                                        // the number of nodes in the pruned and unpruned
                                                        // tree with a recursive function. It can be slow
                                                        // in case of big trees. Returns NULL in case of
                                                        // errors.

            int getNoTreesConstructed(void);            // Returns the number of trees constructed. The same
                                                        // information is returned also by getTreeInfo().

            uint16 getVersion(void);                    // Returns the version of the actual tree. The version
                                                        // now is given by the number of trees constructed.

            int getMode(void);                          // Returns the actual setted mode for the tree.
                                                        // 0 = normal mode
                                                        // 1 = iterative mode with cycles
                                                        // 2 = iterative mode with window

            const char * getErrorMessage(void);         // Returns a string message that explains
                                                        // the error occurred.

            int getErrorCode(void);                     // Returns a code between 1 and 10 that indicate the
                                                        // type of error occurred.

            // Read and write the actual pruned tree
            int64 read(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize);
                                                        // Reads a pruned tree with the given Reader.
                                                        // Returns a number > 0 if there are no errors. The
                                                        // return value specify the number of bytes written.
                                                        // Returns -1 if the Reader made an error.
                                                        // A read tree will overwrite the previous tree and
                                                        // delete all the previous data. The mode "read tree"
                                                        // will be activated and that tree could be used just
                                                        // make tests or to make prediction. Is not possible
                                                        // to add new data (setup a different mode to do this).

            int64 skip (NOMADSUtil::Reader *pReader, uint32 ui32MaxSize);
            											// Reads the tree but don't update the local tree

            int64 write(NOMADSUtil::Writer * pWriter, uint32 ui32MaxSize);
                                                        // Writes the actual pruned tree with the given Writer.
                                                        // Returns a number > 0 if there are no errors. The
                                                        // return value specify the number of bytes written.
                                                        // Returns -1 if the Writer made an error. In this case
                                                        // the tree could be partially written. Returns -2
                                                        // if "ui32MaxSize" is reached and the last part of the
                                                        // tree is not written. Returns -3 if there isn't any
                                                        // pruned tree.

            int64 getWriteLength(void);                 // Returns the number of bytes that will be write when
                                                        // write() is called. Returns -1 if there isn't any pruned tree.

        private:
            const char * _pszErrorMessage;
            int _errorCode;
            treeOptions * _pOptions;
            Configure * _pTreeConfigure;
            processTreeResults * _pResultedTree;
            consultTreeResults * _pConsultedTree;
            CError * _pErrOcc;
            int _treeCounter;                           // keep the total number of trees constructed

            NOMADSUtil::Mutex _m;

            int _MaxItem;                               // max size of _Item
            int _MaxDelete;                             // items in [0 - _MaxDelete] don't exist in _ItemTree
            int _MaxItemIncrement;                      // items in [_MaxItem - _MaxItemIncrement] are new and not
                                                        // yet used in the tree
            int _MaxItemPos;                            // max allocated index (for Description)
            int _MaxItemIndex;                          // max allocated index (for *Description)
            Description * _pItem;                       // keep the items ordered in time

            int _MaxItemTest;                           // items from the last test
            Description * _pItemTest;                   // items from the last test

            int _MaxItemTree;                           // actual window
            int _MaxItemTreeIncrement;                  // actual increment window = _MaxItemTreeIncrement - _MaxItemTree
            Description * _pItemTree;                   // items used to construct the trees, not ordered

            bool _memoryFlag;                           // Used to know when alloc new memory
            bool _dataFlag;                             // false = training dataset, true = test dataset
            int _iterate;                               // 0 = normal mode, 1 = iterative mode with cycles,
                                                        // 2 = iterative mode with window, 3 = read tree

            bool compareConfiguration(C45DecisionTree * first, C45DecisionTree * second);
                                                        // Returns true if the two trees have the same configuration

            void freeTree(Tree tree);                   // Release memory allocated for the tree

            int64 read(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize, int64 totLength, Tree tree);
                                                        // Read a tree node

            int64 skip(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize, int64 totLength);

            int64 write(NOMADSUtil::Writer * pWriter, uint32 ui32MaxSize, int64 totLength, Tree tree);
                                                        // Write a tree node

            int64 getWriteLength(int64 totLength, Tree tree);
    };

    inline void C45DecisionTree::restoreDefaultOptions(void)
    {
        _pOptions->gainCriterion = true;
        _pOptions->increment = 0;
        _pOptions->initialWindow = 0;
        _pOptions->maxWindow = 0;
        _pOptions->cycleErrors = 0.20f;
        _pOptions->cycleWindow = 1.0f;
        _pOptions->minObjects = 2;
        _pOptions->probThresh = false;
        _pOptions->pruneConfidence = 0.25f;
        _pOptions->subsetting = false;
        _pOptions->verbosity = false;
    }

    inline void C45DecisionTree::setPercentMaximumErrors(float percCycleErr)
    {
        if(percCycleErr > 1.0) percCycleErr = 1.0f;
        if(percCycleErr == 0.0) percCycleErr = 0.20f;
        _pOptions->cycleErrors = percCycleErr;
    }

    inline void C45DecisionTree::setPercentInizialWindow(float percCycleWin)
    {
        if((percCycleWin > 1.0)||(percCycleWin == 0.0)) percCycleWin = 1.0;
        _pOptions->cycleWindow = percCycleWin;
    }

    inline void C45DecisionTree::setProbabilisticThresholds(bool threshold)
    {
        _pOptions->probThresh = (Boolean)threshold;
    }

    inline void C45DecisionTree::setGainCriterion(bool gain)
    {
        _pOptions->gainCriterion = (Boolean)!gain;
    }

    inline bool C45DecisionTree::setConfidence(float confidence)
    {
        if((confidence >= 0.001)&&(confidence <= 100)) {
            _pOptions->pruneConfidence = confidence;
            return true;
        }
        return false;
    }

    inline void C45DecisionTree::setVerbosity(bool verb)
    {
        _pOptions->verbosity = (Boolean)verb;
    }

    inline void C45DecisionTree::setSubsetting(bool subset)
    {
        _pOptions->subsetting = (Boolean)subset;
    }

    inline bool C45DecisionTree::setMinObjects(int minimum)
    {
        if((minimum >= 1)&&(minimum <= 1000000)) {
            _pOptions->minObjects = minimum;
            return true;
        }
        return false;
    }

    inline int C45DecisionTree::getNoTreesConstructed(void)
    {
        return _treeCounter;
    }

    inline uint16 C45DecisionTree::getVersion(void)
    {
        return _treeCounter;
    }

    inline int C45DecisionTree::getMode(void)
    {
        return _iterate;
    }

    inline const char * C45DecisionTree::getErrorMessage(void)
    {
        return _pszErrorMessage;
    }

    inline int C45DecisionTree::getErrorCode(void)
    {
        return _errorCode;
    }
}

#endif // INCL_C45_DECISION_TREE_H

