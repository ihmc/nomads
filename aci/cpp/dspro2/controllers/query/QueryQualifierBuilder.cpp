/*
 * QueryQualifierBuilder.cpp
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
 */

#include "QueryQualifierBuilder.h"

#include "BufferReader.h"
#include "LineOrientedReader.h"
#include "StringTokenizer.h"

#if defined (UNIX)
    #define stricmp strcasecmp
#elif defined (WIN32)
    #define stricmp _stricmp
#endif

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const String QueryQualifierBuilder::GROUP_BY = "GROUP BY";
const String QueryQualifierBuilder::LIMIT = "LIMIT";
const String QueryQualifierBuilder::ORDER = "ORDER BY";

QueryQualifierBuilder::QueryQualifierBuilder()
{
}

QueryQualifierBuilder::~QueryQualifierBuilder()
{
}

QueryQualifierBuilder * QueryQualifierBuilder::parse (const char *pszQueryQualifiers)
{
    if (pszQueryQualifiers == NULL) {
        return NULL;
    }

    unsigned int uiLen = strlen (pszQueryQualifiers);
    if (uiLen == 0) {
        return NULL;
    }

    BufferReader br (pszQueryQualifiers, uiLen, false);
    LineOrientedReader lr (&br, false);
    char *pszLine = (char *) calloc (uiLen+1, sizeof (char));
    if (pszLine == NULL) {
        return NULL;
    }
    QueryQualifierBuilder *pQualifierBuilder = new QueryQualifierBuilder();
    if (pQualifierBuilder == NULL) {
        free (pszLine);
        return NULL;
    }

    for (int rc; (rc = lr.readLine (pszLine, uiLen)) >= 0;) {
        if (rc == 0) {
            continue;   // empty line
        }
        static StringTokenizer tokenizer;
        tokenizer.init (pszQueryQualifiers, ';', ';');
        for (const char *pszToken; (pszToken = tokenizer.getNextToken()) != NULL;) {
            if (pQualifierBuilder->setArgument (*pQualifierBuilder, pszToken) < 0) {
                free (pszLine);
                free (pQualifierBuilder);
                return NULL;
            }
        }
    }

    free (pszLine);
    
  /*  if (pszQueryQualifiers->_groupBy.) {
        
    }
            NOMADSUtil::String _limit;
            NOMADSUtil::String _order;*/

    return pQualifierBuilder;
}

const char * QueryQualifierBuilder::getGroupBy (void)
{
    return _groupBy.c_str();
}

const char * QueryQualifierBuilder::getLimit (void)
{
    return _limit.c_str();
}

const char * QueryQualifierBuilder::getOrder (void)
{
    return _order.c_str();
}

int QueryQualifierBuilder::setArgument (QueryQualifierBuilder &qualifierBuilder, const char *pszPropertyAndValue)
{
    if (pszPropertyAndValue == NULL) {
        return -1;
    }
    static StringTokenizer tokenizer;
    tokenizer.init (pszPropertyAndValue, '=', '=');
    
    const char *pszProperty = tokenizer.getNextToken();
    if (pszProperty == NULL) {
        return -2;
    }

    String value = tokenizer.getNextToken();
    if (value.length() <= 0) {
        return -3;
    }

    value.trim();

    if (stricmp (GROUP_BY.c_str(), pszProperty) == 0) {
        qualifierBuilder._groupBy = value.c_str();
    }
    else if (stricmp (LIMIT.c_str(), pszProperty) == 0) {
        qualifierBuilder._limit = value.c_str();
    }
    else if (stricmp (ORDER.c_str(), pszProperty) == 0) {
        qualifierBuilder._order = value.c_str();
    }
    else {
        return -4;
    }

    return 0;
}
