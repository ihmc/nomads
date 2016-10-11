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
            void prepend (T *pel);
            void insert (T *pel);
            T * remove (T *pel);
            T * removeFirst (void);
            void removeAll (void);
            T * replace (T *pelOld, T *pelNew);
            T * getFirst (void);
            T * getTail (void);
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

    template <class T> void PtrLList<T>::prepend (T *pel)
    {
        Node *pNewNode;

        pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

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
        Node *pNewNode, *pCurrNode, *pPrevNode;

        pNewNode = new Node;
        pNewNode->pel = pel;
        pNewNode->pNextNode = NULL;

        if (pRoot == NULL) {
            pRoot = pTail = pNewNode;
            pNextGetNode = pRoot;
            return;
        }

        // Check to see if the element needs to go in front of the root node
        if (_descendingOrder) {
            if (*pel > *(pRoot->pel)) {
                prepend (pel);
                return;
            }
        }
        else {
            if (*pel < *(pRoot->pel)) {
                prepend (pel);
                return;
            }
        }

        pCurrNode = pRoot->pNextNode;
        pPrevNode = pRoot;

        if (_descendingOrder) {
            while (pCurrNode) {
                if (*pel > *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return;
                } 
                else {
                    pPrevNode = pCurrNode;
                    pCurrNode = pCurrNode->pNextNode;
                }
            }
        }
        else {
            while (pCurrNode) {
                if (*pel < *(pCurrNode->pel)) {
                    pPrevNode->pNextNode = pNewNode;
                    pNewNode->pNextNode = pCurrNode;
                    return;
                }
                else {
                    pPrevNode = pCurrNode;
                    pCurrNode = pCurrNode->pNextNode;
                }
            }
        }

        // Reached the end of the list
        pPrevNode->pNextNode = pNewNode;
        pTail = pNewNode;
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

    template <class T> void PtrLList<T>::removeAll (void)
    {
        Node *pTempNode;
        while (pRoot) {
            pTempNode = pRoot;
            pRoot = pRoot->pNextNode;
            delete pTempNode;
        }

        pRoot = pTail = NULL;
        pNextGetNode = NULL;
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

    template <class T> inline T * PtrLList<T>::getFirst (void)
    {
        if (pRoot != NULL) {
            pNextGetNode = pRoot->pNextNode;
            return pRoot->pel;
        }
        return NULL;
    }

     template <class T> inline T * PtrLList<T>::getTail (void)
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

}

#endif // INCL_PTR_LLIST_H
