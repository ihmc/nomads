/*
 * C45RulesTest.h
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

#ifndef INCL_C45_RULES_TEST_H
#define INCL_C45_RULES_TEST_H

#include "types.h"

#include <stddef.h>

namespace IHMC_C45
{
    class C45RulesTest
    {
        public:
            C45RulesTest();
            C45RulesTest(processRulesResults * rules);
            virtual ~C45RulesTest();
            int getRuleSize(int noRule);
            float getEstimatedErrorRate(int noRule);
            float getRealErrorRate(int noRule);
            int getTimesRuleUsed(int noRule);
            int getTimesRuleIncorrect(int noRule);
            int getAdvantage(int noRule);
            void copyTest(C45RulesTest * rulesTest);

        private:
            struct _testRules {
                int _ruleSize;    // number of condition in this rule
                float _estimate;  // estimated error rate
                float _errorRate; // real error rate
                int _used;        // times rule used
                int _incorrect;   // times rule incorrect
                int _advantage;   // if < 0 means that this rule is disavantageous for
                                  // the tested dataset and could be dropped by do the
                                  // test with the "drop" parameter set.
            } * _pTest;
            int _noRules;
    };

    inline C45RulesTest::C45RulesTest()
    {
        _noRules = 0;
        _pTest = NULL;
    }

    inline int C45RulesTest::getRuleSize(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._ruleSize;
        }
        return 0;
    }

    inline float C45RulesTest::getEstimatedErrorRate(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._estimate;
        }
        return 0;
    }

    inline float C45RulesTest::getRealErrorRate(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._errorRate;
        }
        return 0;
    }

    inline int C45RulesTest::getTimesRuleUsed(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._used;
        }
        return 0;
    }

    inline int C45RulesTest::getTimesRuleIncorrect(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._incorrect;
        }
        return 0;
    }

    inline int C45RulesTest::getAdvantage(int noRule)
    {
        if((noRule >= 0)&&(noRule < _noRules)&&(_pTest != NULL)) {
            return _pTest[noRule]._advantage;
        }
        return 0;
    }
}

#endif // INCL_C45_RULES_TEST_H
