/*
 * NodeContext.cpp
 *
 * This file is part of the IHMC Voi Library/Component
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

#include "NodeContext.h"

using namespace IHMC_VOI;
using namespace NOMADSUtil;

const NodeContext::PositionApproximationMode NodeContext::DEFAULT_PATH_ADJUSTING_MODE = GO_TO_NEXT_WAY_POINT;
const unsigned int NodeContext::WAYPOINT_UNSET = -1;

const char * NodeContext::PATH_UNSET_DESCRIPTOR = "PATH_UNSET";
const char * NodeContext::ON_WAY_POINT_DESCRIPTOR = "ON_WAY_POINT";
const char * NodeContext::PATH_DETOURED_DESCRIPTOR = "PATH_DETOURED";
const char * NodeContext::PATH_IN_TRANSIT_DESCRIPTOR = "PATH_IN_TRANSIT";
const char * NodeContext::TOO_FAR_FROM_PATH_DESCRIPTOR = "TOO_FAR_FROM_PATH";

const double NodeContext::TOO_FAR_FROM_PATH_COEFF = 10.0;
const double NodeContext::APPROXIMABLE_TO_POINT_COEFF = 10.0;

NodeContext::NodeContext (void)
{
}

NodeContext::~NodeContext (void)
{
}

const char * NodeContext::getStatusAsString (NodeStatus status)
{
    switch (status) {
        case PATH_UNSET:
            return PATH_UNSET_DESCRIPTOR;

        case ON_WAY_POINT:
            return ON_WAY_POINT_DESCRIPTOR;

        case PATH_DETOURED:
            return PATH_DETOURED_DESCRIPTOR;

        case PATH_IN_TRANSIT:
            return PATH_IN_TRANSIT_DESCRIPTOR;

        case TOO_FAR_FROM_PATH:
            return TOO_FAR_FROM_PATH_DESCRIPTOR;

        default:
            return NULL;
    }
}

