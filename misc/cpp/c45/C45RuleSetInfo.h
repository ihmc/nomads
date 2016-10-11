/*
 * C45RuleSetInfo.h
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

#ifndef C45RULESETINFO_H_
#define C45RULESETINFO_H_

#include "FTypes.h"

namespace IHMC_C45
{
    class C45RuleSetInfo
    {
        public:
            C45RuleSetInfo();
            C45RuleSetInfo(int noRules, const char * defaultClass, bool isComposite, uint16 ui16Version);
            virtual ~C45RuleSetInfo();
            int getNoRules(void);
            const char * getDefaultClass(void);
            bool isCompositeRuleSet(void);
            uint16 getVersion(void);
            void copyInfo(C45RuleSetInfo * info);

        private:
            int	_noRules;               // number of rules present in the rule set
            char * _pszDefaultClass;	// default class for this rule set
            bool _isComposite;		// if true means that this is a composite rule set
            uint16 _ui16Version;        // actual version
    };
}

#endif /*C45RULESETINFO_H_*/
