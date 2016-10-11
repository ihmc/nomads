/*
 * TreeUtils.h
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

#ifndef INCL_TREE_UTIL_H
#define INCL_TREE_UTIL_H

#include "PtrQueue.h"
#include "PtrStack.h"

namespace TreeUtil
{
    enum TraversalOrder
    {
        POST_ORDER,
        LEVEL_ORDER,
        IN_ORDER
    };

    struct Node
    {
        Node (void);
        ~Node (void);

        Node *pLeft;
        Node *pRight;
    };

    class TreeIterator
    {
        public:
            explicit TreeIterator (Node *pRoot);
            virtual ~TreeIterator (void);

            // It returns the next node if any, NULL otherwise
            virtual Node * getNext (void) = 0;

        protected:
            Node *_pCurr;
    };

    class LevelOrderTreeIterator : public TreeIterator
    {
        public:
            explicit LevelOrderTreeIterator (Node *pRoot);
            ~LevelOrderTreeIterator (void);

           Node * getNext (void);

        private:
            NOMADSUtil::PtrQueue<Node> _q;
    };

    class InOrderTreeIterator : public TreeIterator
    {
        public:
            explicit InOrderTreeIterator (Node *pRoot);
            ~InOrderTreeIterator (void);

            Node * getNext (void);

        private:
            NOMADSUtil::PtrLList<Node> _llist;
            NOMADSUtil::PtrStack<Node> _stack;
    };

    class PostOrderTreeIterator : public TreeIterator
    {
        public:
            explicit PostOrderTreeIterator (Node *pRoot);
            ~PostOrderTreeIterator (void);

            Node * getNext (void);

        private:
            Node *_pLastVisited;
            NOMADSUtil::PtrLList<Node> _llist;
            NOMADSUtil::PtrStack<Node> _stack;
    };
}

#endif    // INCL_TREE_UTIL_H

