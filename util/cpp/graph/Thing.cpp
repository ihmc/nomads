/*
 * Thing.cpp
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2016 IHMC.
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

#include "Thing.h"

using namespace NOMADSUtil;

Thing::Thing (const char * pszId)
{
    _pGraph = NULL;
    _pszId = pszId;
}

Thing::Thing (Graph *pGraph, const char * pszId)
{
    _pGraph = pGraph;
    _pszId = pszId;
}

Thing::~Thing ()
{
}

void Thing::setGraph (Graph *pGraph)
{
    _pGraph = pGraph;
}

Graph * Thing::getGraph (void)
{
    return _pGraph;
}

const char * Thing::getId (void)
{
    return _pszId;
}
