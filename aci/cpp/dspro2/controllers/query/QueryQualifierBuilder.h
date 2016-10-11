/*
 * QueryQualifierBuilder.h
 *
 * This file is part of the IHMC DSPro Library/Component
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
 * Author: Giacomo Benincasa   (gbenincasa@ihmc.us)
 * Created on March 12, 2013, 2:35 PM
 */

#ifndef INCL_QUERY_QUALIFIER_BUILDER_H
#define	INCL_QUERY_QUALIFIER_BUILDER_H

#include "StrClass.h"

namespace IHMC_ACI
{
    class QueryQualifier
    {  
    };

    class QueryQualifierBuilder : public QueryQualifier
    {
        public:
            QueryQualifierBuilder (void);
            virtual ~QueryQualifierBuilder (void);

            static QueryQualifierBuilder * parse (const char *pszQueryQualifiers);

            const char * getGroupBy (void);
            const char * getLimit (void);
            const char * getOrder (void);

        private:
            int setArgument (QueryQualifierBuilder &qualifierBuilder, const char *pszPropertyAndValue);

        private:
            static const NOMADSUtil::String GROUP_BY;
            static const NOMADSUtil::String LIMIT;
            static const NOMADSUtil::String ORDER;

            NOMADSUtil::String _groupBy;
            NOMADSUtil::String _limit;
            NOMADSUtil::String _order;
    };
}

#endif	/* INCL_QUERY_QUALIFIER_BUILDER_H */

