/*
 * IntervalTree.h
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

#ifndef INCL_INTERVAL_TREE_H
#define INCL_INTERVAL_TREE_H

#include "TreeUtils.h"

#include  "NLFLib.h"

#include <stdio.h>

namespace NOMADSUtil
{
    /**
     * IntervalTree implements a search binary treap for intervals
     * sorted by the beginning index. It can also optionally do merging
     * of overlapping ranges in order to limit the number of elements in
     * the treap. However the merging does not guarantee that all the
     * intervals in the trap are not overlapping. However, a facility
     * method to get the list of the merged non-overlapping interval is
     * also provided.
     *
     * This data structure was designed to efficiently store the indexes
     * of packet fragments. Because packet fragments tend to be received,
     * and therefore addedd to the data structure, in order, the
     * probabilistic re-balancing approach is beneficial for the task.
     */
    template <class T>
    class IntervalTree
    {
        public:
            /**
             * If bMergeUponInsertion is set on true, overlapping and partially
             * intervals _might_ be merged in order to reduce the size of the tree.
             * However, this does _not_ ensure that the tree contains a collection
             * of non-overlapping intervals. See getCompactList() for this case.
             */
            IntervalTree (bool bMergeUponInsertion = false, bool bSelfBalancing = true);
            ~IntervalTree (void);

            struct Interval
            {
                Interval (T begin = 0U, T end = 0U);
                ~Interval (void);

                T begin;
                T end;
            };

            struct Node : TreeUtil::Node
            {
                Node (T begin, T end, Node *pParent, double dPriority);
                ~Node (void);

                private:
                    friend class IntervalTree;
                    T max;
                    double dPriority;
                    Node *pParent;

                public:
                    Interval interval;
            };

            class Iterator
            {
                public:
                    ~Iterator (void);

                    // Return true if a new element was returned, false
                    // if the iterator ended
                    bool getNext (T &begin, T &end);

                private:
                    friend class IntervalTree;
                    explicit Iterator (TreeUtil::TreeIterator *pIter);

                    TreeUtil::TreeIterator *_pIter;
            };

            Iterator getAllElements (TreeUtil::TraversalOrder order);
            unsigned int getCount (void);

            /**
             It returs a list of non-overlapping intervals that cover
             the set of potentially overlapping intervals contained in
             the tree
             */
            void getCompactList (PtrLList<Node> &llist);
            int insert (T begin, T end);
            int insert (T begin, T end, double dPriority);
            void print (FILE *pFile);

        private:
            int balance (Node *pNewNode, Node *pParent);
            int adjustGrandParent (Node *pNewNode, Node *pGrandParent);
            int leftRotation (Node *pNewNode, Node *pParent);
            int rightRotation (Node *pNewNode, Node *pParent);
            int insert (TreeUtil::Node **pRoot, Node *pParent, T begin, T end, double dPriority);
            bool isLeftChild (Node *pNode);

            const bool _bMergeUponInsertion;
            const bool _bSelfBalancing;
            unsigned int _uiCount;
            TreeUtil::Node *_pRoot;

    };

    template <class T>
    IntervalTree<T>::IntervalTree (bool bMergeUponInsertion, bool bSelfBalancing)
        : _bMergeUponInsertion (bMergeUponInsertion),
          _bSelfBalancing (bSelfBalancing),
          _uiCount (0),
          _pRoot (NULL)
    {
    }

    template <class T>
    IntervalTree<T>::~IntervalTree (void)
    {
    }

    template <class T>
    typename IntervalTree<T>::Iterator IntervalTree<T>::getAllElements (TreeUtil::TraversalOrder order)
    {
        switch (order)
        {
            case TreeUtil::LEVEL_ORDER:
                return Iterator (new TreeUtil::LevelOrderTreeIterator (_pRoot));
            case TreeUtil::IN_ORDER:
            default:
                return Iterator (new TreeUtil::InOrderTreeIterator (_pRoot));
        }
    }

    template <class T>
    unsigned int IntervalTree<T>::getCount (void)
    {
        return _uiCount;
    }

    template <class T>
    void IntervalTree<T>::getCompactList (PtrLList<Node> &llist)
    {
        if (_pRoot == NULL) {
            return;
        }

        PtrStack<Node> s (&llist);
        TreeUtil::InOrderTreeIterator iter (_pRoot);  // Iter though intervals in
                                                      // increasing order of "begin"
        TreeUtil::Node *ptmp = iter.getNext();
        if (ptmp == NULL) {
            return;
        }
        Node *pCurr = static_cast<Node *>(ptmp);
        s.push (pCurr);

        while ((ptmp = iter.getNext()) != NULL) {
            Node *pNext = static_cast<Node *>(ptmp);
            // get interval from stack top
            pCurr = s.peek();
            if (pCurr->interval.end < pNext->interval.begin) {
                // if current interval is not overlapping with stack top,
                // push it to the stack
                s.push (pNext);
            }
            else if (pCurr->interval.end < pNext->interval.end) {
                // Otherwise update the ending time of top if ending of current
                // interval is more
                pCurr->interval.end = pNext->interval.end;
                s.pop();
                s.push (pCurr);
            }
        }
    }

    template <class T>
    int IntervalTree<T>::insert (T begin, T end)
    {
        return insert (&_pRoot, NULL, begin, end, rand());
    }

    template <class T>
    int IntervalTree<T>::insert (T begin, T end, double dPriority)
    {
        return insert (&_pRoot, NULL, begin, end, dPriority);
    }

    template <class T>
    void IntervalTree<T>::print (FILE *pFile)
    {
        if (pFile == NULL) {
            return;
        }
        Iterator iter = getAllElements (TreeUtil::LEVEL_ORDER);
        for (T beginning, end; iter.getNext (beginning, end);) {
            fprintf (pFile, "[%d, %d] ", beginning, end);
        }
    }

    template <class T>
    int IntervalTree<T>::balance (Node *pNewNode, Node *pParent)
    {
        if ((pParent == NULL) || (pNewNode->dPriority >= pParent->dPriority)) {
            return 0;
        }
        int rc = (pNewNode->interval.begin < pParent->interval.begin) ?
                  rightRotation (pNewNode, pParent) :
                  leftRotation (pNewNode, pParent);
        if (rc < 0) {
            return -1;
        }

        return balance (pNewNode, pNewNode->pParent);
    }

    template <class T>
    int IntervalTree<T>::adjustGrandParent (Node *pNewNode, Node *pParent)
    {
        if (pParent->pParent == NULL) {
            _pRoot = pNewNode;
        }
        else if (isLeftChild (pParent)) {
            pParent->pParent->pLeft = pNewNode;
        }
        else {
            pParent->pParent->pRight = pNewNode;
        }
        pNewNode->pParent = pParent->pParent;
        pParent->pParent = pNewNode;

        pParent->max = maximum (pParent->pLeft ? static_cast<Node *>(pParent->pLeft)->max : 0,
                                pParent->pRight ? static_cast<Node *>(pParent->pRight)->max : 0);
        pNewNode->max = maximum (pNewNode->pLeft ? static_cast<Node *>(pNewNode->pLeft)->max : 0,
                                 pNewNode->pRight ? static_cast<Node *>(pNewNode->pRight)->max : 0);
        return 0;
    }

    template <class T>
    int IntervalTree<T>::leftRotation (Node *pRightChild, Node *pParent)
    {
        pParent->pRight = pRightChild->pLeft;
        if (pRightChild->pLeft != NULL) {
            static_cast<Node *>(pRightChild->pLeft)->pParent = pParent;
        }
        pRightChild->pLeft = pParent;
        return adjustGrandParent (pRightChild, pParent);
    }

    template <class T>
    int IntervalTree<T>::rightRotation (Node *pLeftChild, Node *pParent)
    {
        pParent->pLeft = pLeftChild->pRight;
        if (pLeftChild->pRight != NULL) {
            static_cast<Node *>(pLeftChild->pRight)->pParent = pParent;
        }
        pLeftChild->pRight = pParent;
        return adjustGrandParent (pLeftChild, pParent);
    }

    template <class T>
    int IntervalTree<T>::insert (TreeUtil::Node **ppRoot, Node *pParent, T begin, T end, double dPriority)
    {
        if (ppRoot == NULL) {
            return -1;
        }
        if ((*ppRoot) == NULL) {
            Node *ptmp = new Node (begin, end, pParent, dPriority);
            if ((ptmp) == NULL) {
                // TODO: rollback changes to max
                return -1;
            }
            (*ppRoot) = ptmp;
            _uiCount++;
            return (_bSelfBalancing ? balance (ptmp, pParent) : 0);
        }

        // Otherwise, recurse
        Node **ppRootTmp = reinterpret_cast<Node **>(ppRoot);
        if ((*ppRootTmp)->max < end) {
            // Change the max _before_ recursing and, in case,
            // rollback changes in order to allow for tail recursion
            (*ppRootTmp)->max = end;
        }

        const T currNodeBegin = (*ppRootTmp)->interval.begin;
        if (begin < currNodeBegin) {
            return insert (&((*ppRoot)->pLeft), *ppRootTmp, begin, end, dPriority);
        }

        const T currNodeEnd = (*ppRootTmp)->interval.end;
        if (_bMergeUponInsertion  && (begin <= currNodeEnd)) {
            if (currNodeEnd < end) {
                (*ppRootTmp)->interval.end = end;
            }
            return 0;
        }

        return insert (&((*ppRoot)->pRight), *ppRootTmp, begin, end, dPriority);
    }

    template <class T>
    bool IntervalTree<T>::isLeftChild (Node *pNode)
    {
        if (pNode->pParent == NULL) {
            return false;
        }
        return (pNode == pNode->pParent->pLeft);
    }

    //------------------------------------------------------------------

    template <class T>
    IntervalTree<T>::Node::Node (T begin, T end, Node *parent, double priority)
        : max (end),
          dPriority (priority),
          pParent (parent),
          interval (begin, end)
    {
    }

    template <class T>
    IntervalTree<T>::Node::~Node (void)
    {
    }

    //------------------------------------------------------------------

    template <class T>
    IntervalTree<T>::Interval::Interval (T intervalBegin, T intervalEnd)
        : begin (intervalBegin),
          end (intervalEnd)
    {
    }

    template <class T>
    IntervalTree<T>::Interval::~Interval (void)
    {
    }

    //------------------------------------------------------------------

    template <class T>
    IntervalTree<T>::Iterator::Iterator (TreeUtil::TreeIterator *pIter)
        : _pIter (pIter)
    {
    }

    template <class T>
    IntervalTree<T>::Iterator::~Iterator (void)
    {
        if (_pIter != NULL) {
            delete _pIter;
        }
    }

    template <class T>
    bool IntervalTree<T>::Iterator::getNext (T &begin, T &end)
    {
        if (_pIter != NULL) {
            TreeUtil::Node *ptmp = _pIter->getNext();
            if (ptmp != NULL) {
                Node *pNode = static_cast<Node *>(ptmp);
                begin = pNode->interval.begin;
                end = pNode->interval.end;
                return true;
            }
        }
        return false;
    }
}

#endif    // INCL_INTERVAL_TREE_H

