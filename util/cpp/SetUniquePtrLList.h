/*
 * SetUniquePtrLList.h
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
 * Linked List that stores pointers to objects and that does not allow
 * duplicates.
 * Two objects A and B are considered duplicates if:
 * 1) A not greater than B
 *           AND
 * 2)  A not less than B
 *           AND
 * 3)      A == B
 *
 * NOTE: the methods of the parent class - PtrLList - are not virtual, thus the
 * use of polymorphism should be avoided.
 *
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on February 20, 2010, 7:21 PM
 */

#ifndef INCL_SET_UNIQUE_PTR_LIST_H
#define INCL_SET_UNIQUE_PTR_LIST_H

#include "PtrLList.h"

#pragma warning (disable:4786)

namespace NOMADSUtil
{
    template <class T>
    class SetUniquePtrLList : public PtrLList<T>
    {
        public:
            SetUniquePtrLList (void);
            SetUniquePtrLList (bool descendingOrder);
            // need to implement a version sorting the sourceList and checking
            // the uniqueness of the elements
            SetUniquePtrLList (const SetUniquePtrLList<T> &SourceList);
            virtual ~SetUniquePtrLList (void);
            // NOTE: append does not check if the order not the uniqueness of the
            // elements in the list is maintained.
            // NOTE: if pel is a duplicate, it will be deleted. In order to
            // avoid deletion, search() should be called first. If the search
            // returns no element, pel can be safely appended.
            void append (T *pel);
            // NOTE: prepend does not check if the order nor the uniqueness of the
            // elements in the list are maintained. However, if pel is equal to
            // the root, it will not be prepended and it will be deleted. In
            // order to avoid deletion, getFirst() should be called first. If
            // the returned element is NULL or it is different from pel, then
            // pel can be safely prepended.
            void prepend (T *pel);
            // NOTE: if pel is a duplicate, it will be deleted. In order to
            // avoid deletion, search() should be called first. If the search
            // returns no element, pel can be safely inserted. Alternatively
            // (and more efficiently), insertUnique() can be used to safely insert
            // an element.  Look at insertUnique() comments for more information.
            void insert (T *pel);
            // Insert the element at the proper position.  If the element is
            // already in the list, the element being inserted  is not inserted
            // and it is returned.
            T * insertUnique (T *pel);
            T * remove (T *pel);
            void removeAll (void);
            // NOTE: replace does not check if the order nor the uniqueness of
            // the elements in the list are maintained.
            T * replace (T *pelOld, T *pelNew);
            T * getFirst (void);
            T * getNext (void);
            T * search (T *pel);
            void resetGet (void);
            int getCount (void);
            bool isEmpty (void);
            virtual SetUniquePtrLList<T> & operator = (SetUniquePtrLList<T> &SourceList);

        private:
            typedef typename PtrLList<T>::Node Node;
    };

    template <class T> SetUniquePtrLList<T>::SetUniquePtrLList (void)
        : PtrLList<T>()
    {
    }

    template <class T> SetUniquePtrLList<T>::SetUniquePtrLList (bool descendingOrder)
        : PtrLList<T>(descendingOrder)
    {
    }

    template <class T> SetUniquePtrLList<T>::SetUniquePtrLList (const SetUniquePtrLList<T> &SourceList)
        : PtrLList<T>(SourceList)
    {
    }

    template <class T> SetUniquePtrLList<T>::~SetUniquePtrLList ()
    {
    }

    template <class T> void SetUniquePtrLList<T>::append (T *pel)
    {
        PtrLList<T>::append (pel);
    }

    template <class T> void SetUniquePtrLList<T>::prepend (T *pel)
    {
        PtrLList<T>::prepend (pel);
    }

    template <class T> void SetUniquePtrLList<T>::insert (T *pel)
    {
        T *pReturnedEl = insertUnique (pel);
        if (pReturnedEl != NULL) {
            delete pReturnedEl;
        }
    }

    template <class T> T * SetUniquePtrLList<T>::insertUnique (T *pel)
    {
        Node *pNewNode, *pCurrNode, *pPrevNode;

        pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

        if (this->pRoot == NULL) {
            this->pRoot = this->pTail = pNewNode;
            this->pNextGetNode = this->pRoot;
            return NULL;
        }

        if (*pel == *(this->pRoot->pel)) {
            delete pNewNode;
            return pel;
        }

        // Check to see if the element needs to go in front of the root node
        if (this->_descendingOrder) {
            if (*pel > *(this->pRoot->pel)) {
                prepend (pel);
                return NULL;
            }
        }
        else {
            if (*pel < *(this->pRoot->pel)) {
                prepend (pel);
                return NULL;
            }
        }

        pCurrNode = this->pRoot->pNextNode;
        pPrevNode = this->pRoot;

        while (pCurrNode) {
            if (this->_descendingOrder) {
                if (*pel > *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return NULL;
                }
                else if (*pel == *(pCurrNode->pel)) {
                    // pel is a duplicate. Return it
                    delete pNewNode;
                    return pel;
                }
                else {
                    pPrevNode = pCurrNode;
                    pCurrNode = pCurrNode->pNextNode;
                }
            }
            else  {
                // Ascending order
                if (*pel < *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return NULL;
                }
                else if (*pel == *(pCurrNode->pel)) {
                    // pel is a duplicate. Return it
                    delete pNewNode;
                    return pel;
                }
                else {
                    pPrevNode = pCurrNode;
                    pCurrNode = pCurrNode->pNextNode;
                }
            }
        }

        // Reached the end of the list
        pPrevNode->pNextNode = pNewNode;
        this->pTail = pNewNode;
        return NULL;
    }

    template <class T> T * SetUniquePtrLList<T>::remove (T *pel)
    {
        return PtrLList<T>::remove (pel);
    }

    template <class T> void SetUniquePtrLList<T>::removeAll (void)
    {
        PtrLList<T>::removeAll();
    }

    template <class T> T * SetUniquePtrLList<T>::replace (T *pelOld, T *pelNew)
    {
        return PtrLList<T>::replace(pelOld, pelNew);
    }

    template <class T> T * SetUniquePtrLList<T>::getFirst (void)
    {
        return PtrLList<T>::getFirst();
    }

    template <class T> T * SetUniquePtrLList<T>::getNext (void)
    {
        return PtrLList<T>::getNext();
    }

    template <class T> T * SetUniquePtrLList<T>::search (T *pel)
    {
        return PtrLList<T>::search (pel);
    }

    template <class T> void SetUniquePtrLList<T>::resetGet (void)
    {
        PtrLList<T>::resetGet();
    }

    template <class T> int SetUniquePtrLList<T>::getCount (void)
    {
        return PtrLList<T>::getCount();
    }

    template <class T> bool SetUniquePtrLList<T>::isEmpty (void)
    {
        return PtrLList<T>::isEmpty();
    }

    template <class T> SetUniquePtrLList<T> & SetUniquePtrLList<T>::operator = (SetUniquePtrLList<T> & SourceList)
    {
        Node *pTempNode;
        while (this->pRoot) {
            pTempNode = this->pRoot;
            this->pRoot = this->pRoot->pNextNode;
            delete pTempNode;
        }
        this->pRoot = this->pTail = NULL;
        this->pNextGetNode = NULL;

        Node *pNewNode;
        pTempNode = SourceList.pRoot;
        if (pTempNode) {
            pNewNode = new Node;
            pNewNode->pel = pTempNode->pel;
            pNewNode->pNextNode = NULL;
            this->pRoot = this->pTail = pNewNode;
            this->pNextGetNode = this->pRoot;
            pTempNode = pTempNode->pNextNode;
        }
        while (pTempNode) {
            pNewNode = new Node;
            pNewNode->pel = pTempNode->pel;
            pNewNode->pNextNode = NULL;

            this->pTail->pNextNode = pNewNode;
            this->pTail = pNewNode;
            pTempNode = pTempNode->pNextNode;
        }

        return *this;
    }
}

#endif    // INCL_SET_UNIQUE_PTR_LIST_H
