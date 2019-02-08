/*
 * C45Rules.h
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

#ifndef INCL_C45_RULES_H
#define INCL_C45_RULES_H

#include "Mutex.h"
#include "C45RulesPrediction.h"
#include "Classifier.h"

#include "types.h"

#include "FTypes.h"

namespace NOMADSUtil
{
    class Reader;
    class Writer;
    class Mutex;
}

namespace IHMC_C45
{
    class C45AVList;
    class C45DecisionTree;
    class C45RuleSetInfo;
    class C45RuleSetTestInfo;
    class C45RulesTest;

    class C45Rules : public Classifier
    {
        public:
            C45Rules();
            virtual ~C45Rules();
            // Construct and use the rules
            C45RuleSetInfo * createRuleSet (C45DecisionTree * tree);
                                                    // Create a new rule set for the passed decision
                                                    // tree. The rule set is constructed using the
                                                    // unpruned tree contained in C45DecisionTree. If
                                                    // the unpruned tree don't exist, the function
                                                    // returns an error.
                                                    // Returns a code for identify errors occurred.

            C45RulesPrediction * consultClassifier(C45AVList * record);
                                                    // Consult the rule set for the given
                                                    // record. Returns a code for the errors.

            C45RuleSetTestInfo * testClassifierOnData(C45AVList * dataset, bool drop);
                                                    // Test the rule set on the given data.
                                                    // If "drop" is true, after the test all the rules
                                                    // disadvantageous are deleted from the rule set.
                                                    // returns a code for the errors.

            // construction options
            void restoreDefaultOptions(void);       // Restore the default values for every option.

            bool setConfidence(float confidence);   // Set the pruning confidence level. the
                                                    // default is 0.25 The confidence must be in the
                                                    // range [0, 100] or "false" is returned ad the
                                                    // option is not accepted.

            bool setRedundancy(float redundancy);   // If many irrelevant or redundant attributes are
                                                    // included, estimate the ratio of attributes to
                                                    // "sensible" attributes. The default is 1. The
                                                    // rendundancy must be in the range [0, 10000] or
                                                    // false is return and the option is not accepted.

            void setSignificanceTest(bool test);    // Invoke Fisher's significance test when pruning
                                                    // rules. For default the test is not called.

            bool setSignificanceTestThreshold(float thresh);
                                                    // Set the threshold used in Fisher's test.
                                                    // If a rule contains a condition whose
                                                    // probability of being irrelevant is greater
                                                    // than the stated level, the rule is pruned
                                                    // further (default: no significance testing).
                                                    // The thresh must be in the range [0, 100] or
                                                    // false is returned and the option is not accepted.

            void setAnnealing(bool set);            // Simulated annealing for selecting rules. For
                                                    // default is not used.

            void setVerbosity(bool verb);           // If the verbosity is set, a standard output
                                                    // is generated. That may help to explain what
                                                    // the program is doing.

            // get methods
            C45RuleSetInfo * getRuleSetInfo(void);  // Give some information about the rule
                                                    // set, returns NULL if that rule set doesn't exist.

            C45RulesTest * getTestResultsForRules(void);

            uint16 getVersion(void);                // Returns a version number that identifies the current
                                                    // rule set.

            const char * getErrorMessage(void);     // Returns a string message that explains the
                                                    // error occurred.

            int getErrorCode(void);

            // Save space
            void deleteTestData(void);              // Delete the data used in the last test to free some space.

            // Read and write the actual rule set
            int configureRuleSet(C45DecisionTree * tree);
                                                    // This method must be called before read() if the C45Rules
                                                    // instance is currently empty. A rule set is always derived
                                                    // from a tree. To read a rule set correctly, the rule set needs
                                                    // to know the configuration of the tree who has generated the
                                                    // written rule set.

            int64 read(NOMADSUtil::Reader * pReader, uint32 ui32MaxSize);
                                                    // Reads a rule set with the given Reader. Returns a number > 0
                                                    // if there are no errors. The return value specify the number of
                                                    // bytes written. Returns -1 if the Reader made an error. A read
                                                    // rule set will overwrite the previous rule set and delete all
                                                    // the previous data.

            int64 write(NOMADSUtil::Writer * pWriter, uint32 ui32MaxSize);
                                                    // Writes the actual rule set with the given Writer. Returns a
                                                    // number > 0 if there are no errors. The return value specify
                                                    // the number of bytes written. Returns -1 if the Writer made
                                                    // an error. In this case the rule set could be partially written.
                                                    // Returns -2 if "ui32MaxSize" is reached and the last part of the
                                                    // rule set is not written. Returns -3 if there isn't any rule set.

            int64 getWriteLength(void);             // Returns the number of bytes that will be write when write()
                                                    // is called. Returns -1 if there isn't any rule set.

        private:
            ruleOptions *_pOptions;
            Configure *_pRuleConfigure;
            const char *_pszErrorMessage;
            int _errorCode;
            int _MaxItemTest;
            Description *_pItemTest;
            processRulesResults *_pResultedRules;
            consultRulesResults *_pConsultedRules;
            uint16 _noRuleSetConstructed;
            CError *_pErrOcc;

            NOMADSUtil::Mutex _m;
    };

    inline void C45Rules::restoreDefaultOptions(void)
    {
        _pOptions->annealing = false;
        _pOptions->fisherThresh = 0.05f;
        _pOptions->isFisherInUse = false;
        _pOptions->pruneConfidence = 0.25f;
        _pOptions->redundancy = 1.0f;
        _pOptions->verbosity = false;
    }

    inline bool C45Rules::setConfidence(float confidence)
    {
        if((confidence >= 0.0f)&&(confidence <= 100.0f)) {
            _pOptions->pruneConfidence = confidence;
            return true;
        }
        return false;
    }

    inline bool C45Rules::setRedundancy(float redundancy)
    {
        if((redundancy >= 0.0f)&&(redundancy <= 10000.0f)) {
            _pOptions->redundancy = redundancy;
            return true;
        }
        return false;
    }

    inline void C45Rules::setSignificanceTest(bool test)
    {
        _pOptions->isFisherInUse = (Boolean)test;
    }

    inline bool C45Rules::setSignificanceTestThreshold(float thresh)
    {
        if((thresh >= 0.0f)&&(thresh <= 100.0f)) {
            _pOptions->fisherThresh = thresh;
            return true;
        }
        return false;
    }

    inline void C45Rules::setAnnealing(bool set)
    {
        _pOptions->annealing = (Boolean)set;
    }

    inline void C45Rules::setVerbosity(bool verb)
    {
        _pOptions->verbosity = (Boolean)verb;
    }

    inline const char * C45Rules::getErrorMessage(void)
    {
        return _pszErrorMessage;
    }

    inline int C45Rules::getErrorCode(void)
    {
        return _errorCode;
    }
}

#endif // INCL_C45_RULES_H

