/* 
 * NodeIdSet.cpp
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on August 22, 2014, 6:22 AM
 */

#include "NodeIdSet.h"

using namespace IHMC_ACI;
using namespace NOMADSUtil;

const char NodeIdSet::SEPARATOR = ',';

NodeIdSet::NodeIdSet (void)
    : _bUpdateString (false),
      _nodeIds (true, // bCaseSensitiveKeys
                true, // bCloneKeys
                true) // bDeleteKeys
{
}

NodeIdSet::NodeIdSet (const char *pszFirstNode)
    : _nodeIds (true, // bCaseSensitiveKeys
                true, // bCloneKeys
                true) // bDeleteKeys
{
    _bUpdateString = (pszFirstNode == NULL ? false : _nodeIds.put (pszFirstNode));
}

NodeIdSet::NodeIdSet (NodeIdSet &nodeIdSet)
    : _bUpdateString (false),
      _nodeIds (true, // bCaseSensitiveKeys
                true, // bCloneKeys
                true) // bDeleteKeys
{
    NodeIdIterator iter = nodeIdSet.getIterator();
    for (; !iter.end(); iter.nextElement()) {
        if (_nodeIds.put (iter.getKey())) {
            _bUpdateString = true;
        }
    }
}

NodeIdSet::~NodeIdSet (void)
{
}

NodeIdIterator NodeIdSet::getIterator (void)
{
    return _nodeIds.getAllElements();
}

void NodeIdSet::add (const char *pszNodeId)
{
    if (_nodeIds.put (pszNodeId)) {
        _bUpdateString = true;
    }
}

bool NodeIdSet::contains (const char *pszNodeId) const
{
    return _nodeIds.containsKey (pszNodeId);
}

bool NodeIdSet::isEmpty (void) const
{
    return (_nodeIds.getCount() == 0);
}

void NodeIdSet::remove (const char *pszNodeId)
{
    _nodeIds.remove (pszNodeId);
}

NodeIdSet::operator const char * (void)
{
    if (_bUpdateString) {
        _sNodeIds = "";
        StringHashset::Iterator iter = _nodeIds.getAllElements();
        for (; !iter.end(); iter.nextElement()) {
            if (_sNodeIds.length() > 0) {
                _sNodeIds += SEPARATOR;
            }
            _sNodeIds += iter.getKey();
        }
    }
    return _sNodeIds.c_str();
}

bool NodeIdSet::operator == (NodeIdSet &rhsNodeIdSet)
{
    if (_nodeIds.getCount() != rhsNodeIdSet._nodeIds.getCount()) {
        return false;
    }
    StringHashset::Iterator iter = rhsNodeIdSet._nodeIds.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        if (!_nodeIds.containsKey (iter.getKey())) {
            return false;
        }
    }
    return true;
}

NodeIdSet & NodeIdSet::operator = (NodeIdSet &rhsNodeIdSet)
{
    _nodeIds.removeAll();
    (*this) += rhsNodeIdSet;
    return *this;
}

NodeIdSet & NodeIdSet::operator += (NodeIdSet &rhsNodeIdSet)
{
    StringHashset::Iterator iter = rhsNodeIdSet._nodeIds.getAllElements();
    for (; !iter.end(); iter.nextElement()) {
        if (_nodeIds.put (iter.getKey())) {
            _bUpdateString = true;
        }
    }
    return *this;
}

