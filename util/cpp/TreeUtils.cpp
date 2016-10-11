/*
 * TreeUtils.cpp
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
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 */

#include "TreeUtils.h"

using namespace NOMADSUtil;
using namespace TreeUtil;

Node::Node (void)
    : pLeft (NULL),
      pRight (NULL)
{
}

Node::~Node (void)
{
}

//--------------------------------------------------------------

TreeIterator::TreeIterator (Node *pRoot)
    : _pCurr (pRoot)
{
}

TreeIterator::~TreeIterator (void)
{
}

//--------------------------------------------------------------

LevelOrderTreeIterator::LevelOrderTreeIterator (Node *pRoot)
    : TreeIterator (pRoot)
{
    _q.enqueue (_pCurr);
}

LevelOrderTreeIterator::~LevelOrderTreeIterator (void)
{
}

Node * LevelOrderTreeIterator::getNext (void)
{
    Node *pCurr = _q.dequeue();
    if (pCurr != NULL) {
        if (pCurr->pLeft != NULL) {
            _q.enqueue (pCurr->pLeft);
        }
        if (pCurr->pRight != NULL) {
            _q.enqueue (pCurr->pRight);
        }
    }
    return pCurr;
}

//--------------------------------------------------------------

InOrderTreeIterator::InOrderTreeIterator (Node *pRoot)
    : TreeIterator (pRoot),
      _stack (&_llist)
{
}

InOrderTreeIterator::~InOrderTreeIterator (void)
{
}

Node * InOrderTreeIterator::getNext (void)
{
    if ((_stack.isEmpty()) && (_pCurr == NULL)) {
        return NULL;
    }
    Node *pNodeToVisit = NULL;
    while (pNodeToVisit == NULL) {
        if (_pCurr != NULL) {
            _stack.push (_pCurr);
            _pCurr = _pCurr->pLeft;
        }
        else {
            _pCurr = _stack.pop();
            pNodeToVisit = _pCurr;
            if (_pCurr != NULL) {
                _pCurr = _pCurr->pRight;
            }
        }
    }
    return pNodeToVisit;
}

//--------------------------------------------------------------

PostOrderTreeIterator::PostOrderTreeIterator (Node *pRoot)
    : TreeIterator (pRoot),
      _pLastVisited (NULL),
      _stack (&_llist)
{
}

PostOrderTreeIterator::~PostOrderTreeIterator (void)
{
}

Node * PostOrderTreeIterator::getNext (void)
{
    while ((!_stack.isEmpty()) || (_pCurr != NULL)) {
        if (_pCurr != NULL) {
            _stack.push (_pCurr);
            _pCurr = _pCurr->pLeft;
        }
        else {
            Node *pNode = _stack.peek();
            if ((pNode->pRight != NULL) && (_pLastVisited != pNode->pRight)) {
                _pCurr = pNode->pRight;
            }
            else {
                _pLastVisited = _stack.pop();
                return pNode;
            }
        }
    }
    return NULL;
}

