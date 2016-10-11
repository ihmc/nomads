/*
 * Targets.cpp
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

#include "Targets.h"

#include "Defs.h"
#include "Logger.h"
#include "StringHashset.h"
#include "Topology.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

Targets::Targets (Targets &targets)
    : adaptorId (targets.adaptorId),
      aTargetNodeIds ((char*) NULL, targets.aTargetNodeIds.size()),
      aInterfaces ((char*) NULL, targets.aInterfaces.size())
{
    for (unsigned int i = 0; i < targets.aTargetNodeIds.size(); i++) {
        aTargetNodeIds[i] = strDup (targets.aTargetNodeIds.get (i));
    }
    for (unsigned int i = 0; i < targets.aInterfaces.size(); i++) {
        aInterfaces[i] = strDup (targets.aInterfaces.get (i));
    }
}

Targets::~Targets (void)
{
    if (aTargetNodeIds.getSize() > 0) {
        for (unsigned int ui = 0; ui < aTargetNodeIds.getSize(); ui++) {
            if (aTargetNodeIds[ui]) {
                free (aTargetNodeIds[ui]);
                aTargetNodeIds[ui] = NULL;
            }
        }
        aTargetNodeIds.trimSize (0);
    }
    if (aInterfaces.getSize() > 0) {
        for (unsigned int ui = 0; ui < aInterfaces.getSize(); ui++) {
            if (aInterfaces[ui]) {
                free (aInterfaces[ui]);
                aInterfaces[ui] = NULL;
            }
        }
        aInterfaces.trimSize (0);
    }
}

bool Targets::subsume (const Targets &target)
{
    if (aInterfaces.size() != target.aInterfaces.size()) {
        return false;
    }
    StringHashset hs (true, false, false); // bCaseSensitiveKeys, bCloneKeys, bDeleteKeys
    for (unsigned int i = 0; i < aInterfaces.size(); i++) {
        hs.put (aInterfaces[i]);
    }
    for (unsigned int i = 0; i < target.aInterfaces.size(); i++) {
        if (!hs.containsKey (target.aInterfaces.get (i))) {
            return false;
        }
    }
    hs.removeAll();
    for (unsigned int i = 0; i < aTargetNodeIds.size(); i++) {
        hs.put (aTargetNodeIds.get (i));
    }
    for (unsigned int i = 0; i < target.aInterfaces.size(); i++) {
        if (!hs.containsKey (target.aTargetNodeIds.get (i))) {
            const unsigned int uiIndex = firstFreeTargetNodeId();
            aTargetNodeIds[uiIndex] = strDup (target.aTargetNodeIds.get (i));
        }
    }
    return true;
}

void Targets::log (void)
{
    for (unsigned int i = 0; i < aTargetNodeIds.size(); i++) {
        if (aTargetNodeIds[i] != NULL && aInterfaces[i] != NULL) {
            logTopology ("Targets::display", Logger::L_Info,
                         "target node %u: <%s>\tinterface: <%s>\n",
                         i, aTargetNodeIds[i], aInterfaces[i]);
        }
        else {
            if (aTargetNodeIds[i] == NULL) {
                logTopology ("Targets::display", Logger::L_Info,
                             "aTargetNodeIds[%u] not used\n", i);
            }
            if (aInterfaces[i] == NULL) {
                logTopology ("Targets::display", Logger::L_Info,
                             "aInterfaces[%u] not used\n", i);
            }
        }
    }
}

void Targets::deallocateTarget (Targets *pTargets)
{
    delete pTargets;   
}

void Targets::deallocateTargets (Targets **ppTargets)
{
    if (ppTargets == NULL) {
        return;
    }
    for (unsigned int i = 0; ppTargets[i] != NULL; i++) {
        deallocateTarget (ppTargets[i]);
    }
    free (ppTargets);    
}

int Targets::firstFreeInternal (NOMADSUtil::DArray<char*> &array)
{
    int j = (int) array.size();
    for (int i = 0; i < j; i++) {
        if (array[i] == NULL) {
            return i;
        }
    }
    return j;
}

