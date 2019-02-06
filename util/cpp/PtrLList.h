/*
 * PtrLList.h
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
 * Linked List that stores pointers to objects.
 * If the node pointed by pNextGetNode is removed
 * as a consequence of a call to the remove method,
 * pNextGetNode will point to the node which used
 * to follow the removed one in the list.
 * Note that removing a node does not delete the
 * element pointed by the node. removeAll() provides
 * this functionality passing bDeleteValues as true.
 */

#ifndef INCL_PTR_LLIST_H
#define INCL_PTR_LLIST_H

#include <stdlib.h>

#pragma warning (disable:4786)

namespace NOMADSUtil
{
    template <class T>
    class PtrLList
    {
        public:
            PtrLList (bool descendingOrder = true);
            PtrLList (T * pfirstel, bool descendingOrder = true);
            //need to implement a version sorting the sourcelist
            PtrLList (const PtrLList<T> &SourceList);
            virtual ~PtrLList (void);

            void append (T *pel);
            void appendCopy (T *pel);
            void prepend (T *pel);
            void prependCopy (T *pel);
            void insert (T *pel);
            void insertCopy (T *pel);
            void insertAll (PtrLList<T> *plist);
            void insertAllCopies (PtrLList<T> *plist);

            T * replace (T *pelOld, T *pelNew);
            T * remove (T *pel);
            T * removeFirst (void);
            void removeAll (bool bDeleteValues = false);

            T * getFirst (void);
            T * getTail (void) const;
            T * getNext (void);
            T * search (const T * const pel) const;
            void resetGet (void);
            int getCount (void) const;
            bool isEmpty (void) const;

            virtual PtrLList<T> & operator = (PtrLList<T> &SourceList);

        protected:
            struct Node {
                T *pel;
                Node *pNextNode;
            };
            Node *pRoot;
            Node *pTail;
            Node *pNextGetNode;
            bool _descendingOrder; // If true insert nodes in descending
                                   //    order from the root.
                                   // Otherwise insert nodes in ascending
                                   //    order from the root.

        private:
            void insertInternal (Node *pNewNode, Node *pCurrRoot);
            void prependInternal (Node *pNewNode);
    };

    template <class T> PtrLList<T>::PtrLList (bool descendingOrder)
    {
        pRoot = pTail = NULL;
        pNextGetNode = NULL;
        _descendingOrder = descendingOrder;
    }

    template <class T> PtrLList<T>::PtrLList (T * pfirstel, bool descendingOrder)
    {
        pRoot = new Node;
        pRoot->pel = pfirstel;
        pRoot->pNextNode = NULL;
        pTail = pRoot;
        pNextGetNode = pRoot;
        _descendingOrder = descendingOrder;
    }

    template <class T> PtrLList<T>::PtrLList (const PtrLList<T> &SourceList)
    {
        Node *pSourceNode, *pNewNode, *pTempNode;

        pRoot = pTail = NULL;
        pNextGetNode = NULL;

        pSourceNode = SourceList.pRoot;
        if (pSourceNode) {
            pRoot = new Node;
            pRoot->pel = pSourceNode->pel;
            pRoot->pNextNode = NULL;
            pTail = pRoot;
            pTempNode = pRoot;
            pSourceNode = pSourceNode->pNextNode;
            while (pSourceNode) {
                pNewNode = new Node;
                pNewNode->pel = pSourceNode->pel;
                pNewNode->pNextNode = NULL;
                pTempNode->pNextNode = pNewNode;
                pTempNode = pNewNode;
                pTail = pNewNode;
                pSourceNode = pSourceNode->pNextNode;
            }
        }
    }

    template <class T> PtrLList<T>::~PtrLList ()
    {
        Node *pTempNode;

        while (pRoot) {
            pTempNode = pRoot;
            pRoot = pRoot->pNextNode;
            delete pTempNode;
        }
        pRoot = pTail = NULL;
        pNextGetNode = NULL;
        _descendingOrder = true;
    }

    template <class T> void PtrLList<T>::append (T *pel)
    {
        Node *pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

        if (pRoot == NULL) {
            pRoot = pTail = pNewNode;
            pNextGetNode = pRoot;
        }
        else {
            pTail->pNextNode = pNewNode;
            pTail = pNewNode;
            if (pNextGetNode == NULL) {
                pNextGetNode = pNewNode;
            }
        }
    }

    template <class T> void PtrLList<T>::appendCopy (T *pel)
    {
        T * pelCopy = new T(*pel);
        append (pelCopy);
    }

    template <class T> void PtrLList<T>::prepend (T *pel)
    {
        Node *pNewNode;

        pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

        prependInternal (pNewNode);
    }

    template <class T> void PtrLList<T>::prependCopy (T *pel)
    {
        T * pelCopy = new T(*pel);
        prepend (pelCopy);
    }

    template <class T> void PtrLList<T>::prependInternal (Node *pNewNode)
    {
        if (pRoot == NULL) {
            pRoot = pTail = pNewNode;
            pNextGetNode = pRoot;
        }
        else {
            pNewNode->pNextNode = pRoot;
            pRoot = pNewNode;
            pNextGetNode = pNewNode;
        }
    }

    template <class T> void PtrLList<T>::insert (T *pel)
    {
        Node *pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

        if (pRoot == NULL) {
            pRoot = pTail = pNewNode;
            pNextGetNode = pRoot;
            return;
        }

        insertInternal (pNewNode, pRoot);
    }

    template <class T> void PtrLList<T>::insertCopy (T *pel)
    {
        T * pelCopy = new T(*pel);
        insert (pelCopy);
    }

    template <class T> void PtrLList<T>::insertInternal (Node *pNewNode, Node *pCurrRoot)
    {
        if ((pNewNode == NULL) || (pCurrRoot == NULL)) {
            return;
        }

        // Check to see if the element needs to go in front of the root node
        if (_descendingOrder) {
            if (*pNewNode->pel > *(pCurrRoot->pel)) {
                prependInternal (pNewNode);
                return;
            }
        }
        else if (*pNewNode->pel < *(pCurrRoot->pel)) {
            prependInternal (pNewNode);
            return;
        }

        Node *pCurrNode = pCurrRoot->pNextNode;
        Node *pPrevNode = pCurrRoot;

        if (_descendingOrder) {
            while (pCurrNode) {
                if (*pNewNode->pel > *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return;
                }
                pPrevNode = pCurrNode;
                pCurrNode = pCurrNode->pNextNode;
            }
        }
        else {
            while (pCurrNode) {
                if (*pNewNode->pel < *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return;
                }
                pPrevNode = pCurrNode;
                pCurrNode = pCurrNode->pNextNode;
            }
        }

        // Reached the end of the list
        pPrevNode->pNextNode = pNewNode;
        pTail = pNewNode;
    }

    template <class T> void PtrLList<T>::insertAll (PtrLList<T> *plist)
    {
        if (plist == NULL) {
            return;
        }
        Node *pPrevNode = NULL;
        for (T *pEl = plist->getFirst(); pEl != NULL; pEl = plist->getNext()) {
            Node *pNewNode = new Node;
            pNewNode->pel = pEl;
            pNewNode->pNextNode = NULL;

            if (pPrevNode == NULL) {
                insertInternal (pNewNode, pRoot);
                pPrevNode = pNewNode;
            }
            else {
                insertInternal (pNewNode, pPrevNode);
            }
        }
    }

    template <class T> void PtrLList<T>::insertAllCopies (PtrLList<T> *plist)
    {
        if (plist == NULL) {
            return;
        }
        Node *pPrevNode = NULL;
        for (T *pEl = plist->getFirst(); pEl != NULL; pEl = plist->getNext()) {
            pEl = new T(*pEl);
            Node *pNewNode = new Node;
            pNewNode->pel = pEl;
            pNewNode->pNextNode = NULL;

            if (pPrevNode == NULL) {
                insertInternal (pNewNode, pRoot);
                pPrevNode = pNewNode;
            }
            else {
                insertInternal (pNewNode, pPrevNode);
            }
        }
    }

    template <class T> T * PtrLList<T>::replace (T *pelOld, T *pelNew)
    {
        if (pRoot) {
            if (*(pRoot->pel) == *pelOld) {
                T *pelReplaced = pRoot->pel;
                pRoot->pel = pelNew;
                return pelReplaced;
            }
            else {
                Node *pTempNode = pRoot->pNextNode;
                while (pTempNode) {
                    if (*(pTempNode->pel) == *pelOld) {
                        T *pelReplaced = pTempNode->pel;
                        pTempNode->pel = pelNew;
                        return pelReplaced;
                    }
                    else {
                        pTempNode = pTempNode -> pNextNode;
                    }
                }
            }
        }
        return NULL;
    }

    template <class T> T * PtrLList<T>::remove (T *pel)
    {
        if (pRoot) {
            if (*(pRoot->pel) == *pel) {
                Node *pTempNode = pRoot;
                T *pelDeleted = pRoot->pel;
                pRoot = pRoot->pNextNode;
                if (!pRoot) {
                    pTail = NULL;
                }
                if (pNextGetNode == pTempNode) {
                    pNextGetNode = pRoot;
                }
                delete pTempNode;
                return pelDeleted;
            }
            else {
                Node *pPrevNode, *pCurrNode;
                pPrevNode = pRoot;
                pCurrNode = pRoot->pNextNode;
                while (pCurrNode) {
                    if (*(pCurrNode->pel) == *pel) {
                        T *pelDeleted = pCurrNode->pel;
                        pPrevNode->pNextNode = pCurrNode->pNextNode;
                        if (pTail == pCurrNode) {
                            pTail = pPrevNode;
                        }
                        if (pNextGetNode == pCurrNode) {
                            pNextGetNode = pCurrNode->pNextNode;
                        }
                        delete pCurrNode;
                        return pelDeleted;
                    }
                    else {
                        pPrevNode = pCurrNode;
                        pCurrNode = pCurrNode->pNextNode;
                    }
                }
            }
        }
        return NULL;
    }

    template <class T> T * PtrLList<T>::removeFirst (void)
    {
        if (pRoot) {
            Node *pTempNode = pRoot;
            T *pelDeleted = pRoot->pel;
            pRoot = pRoot->pNextNode;
            if (!pRoot) {
                pTail = NULL;
            }
            if (pNextGetNode == pTempNode) {
                pNextGetNode = pRoot;
            }
            delete pTempNode;
            return pelDeleted;
        }
        return NULL;
    }

    template <class T> void PtrLList<T>::removeAll (bool bDeleteValues)
    {
        Node *pTempNode;
        while (pRoot) {
            pTempNode = pRoot;
            pRoot = pRoot->pNextNode;
            if (bDeleteValues) {
                delete pTempNode->pel;
            }
            delete pTempNode;
        }

        pRoot = pTail = NULL;
        pNextGetNode = NULL;
    }

    template <class T> inline T * PtrLList<T>::getFirst (void)
    {
        if (pRoot != NULL) {
            pNextGetNode = pRoot->pNextNode;
            return pRoot->pel;
        }
        return NULL;
    }

     template <class T> inline T * PtrLList<T>::getTail (void) const
     {
        if (pTail != NULL) {
            return pTail->pel;
        }
        return NULL;
     }

    template <class T> inline T * PtrLList<T>::getNext (void)
    {
        if (pNextGetNode != NULL) {
            T *pel = pNextGetNode->pel;
            pNextGetNode = pNextGetNode->pNextNode;
            return pel;
        }
        return NULL;
    }

    template <class T> T * PtrLList<T>::search (const T * const pel) const
    {
        const Node *pTempNode = pRoot;
        while (pTempNode) {
            if (*(pTempNode->pel) == *pel) {
                return pTempNode->pel;
            }
            else {
                pTempNode = pTempNode->pNextNode;
            }
        }
        return NULL;
    }

    template <class T> inline void PtrLList<T>::resetGet (void)
    {
        pNextGetNode = pRoot;
    }

    template <class T> int PtrLList<T>::getCount (void) const
    {
        int iCount = 0;
        const Node *pTempNode = pRoot;
        while (pTempNode) {
            iCount++;
            pTempNode = pTempNode->pNextNode;
        }
        return iCount;
    }

    template <class T> inline bool PtrLList<T>::isEmpty (void) const
    {
        return (pRoot == NULL);
    }

    template <class T> PtrLList<T> & PtrLList<T>::operator = (PtrLList<T> & SourceList)
    {
        Node *pTempNode;
        while (pRoot) {
            pTempNode = pRoot;
            pRoot = pRoot->pNextNode;
            delete pTempNode;
        }
        pRoot = pTail = NULL;
        pNextGetNode = NULL;

        Node *pNewNode;
        pTempNode = SourceList.pRoot;
        if (pTempNode) {
            pNewNode = new Node;
            pNewNode->pel = pTempNode->pel;
            pNewNode->pNextNode = NULL;
            pRoot = pTail = pNewNode;
            pNextGetNode = pRoot;
            pTempNode = pTempNode->pNextNode;
        }
        while (pTempNode) {
            pNewNode = new Node;
            pNewNode->pel = pTempNode->pel;
            pNewNode->pNextNode = NULL;

            pTail->pNextNode = pNewNode;
            pTail = pNewNode;
            pTempNode = pTempNode->pNextNode;
        }

        return *this;
    }

    template <class T>
    void deallocateAllPtrLListElements (PtrLList<T> *pList)
    {
        if (pList == NULL) {
            return;
        }

        pList->removeAll (true);
    }
}

#endif // INCL_PTR_LLIST_H
