/*
 * PtrQueue.h
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

#ifndef INCL_PTR_QUEUE_H
#define INCL_PTR_QUEUE_H

#include <stddef.h>
#include<stdio.h>
namespace NOMADSUtil
{
    template <class T> class PtrQueue
    {
        public:
            PtrQueue (void);
            ~PtrQueue (void);

            // Add to the tail of the queue
            int enqueue (T *px);

            // Remove from the head of the queue
            T * dequeue (void);

            // Return the element at the head of the queue (but do not dequeue)
            T * peek (void) const;

            // Return the element at the tail of the queue (but do not dequeue)
            T * peekTail (void) const;

            // Dequeue the specified element (from anywhere in the queue)
            T * remove (const T *px);

            // Remove all the elements from the queue (if removeData == true, the method also deletes referenced data)
            void removeAll (bool removeData = false);

            // Return number of elements in queue
            int size (void) const;

            // Find the element (but leave it in the queue)
            T * find (const T *px) const;

            // Reset the queue iterator
            void resetGet (void);

            // Get the first element in the queue (and reset the iterator)
            // Returns NULL if the list is empty
            T * getFirst (void);

            // Get the next element in the queue, and advances the iterator to the following element
            // Returns NULL if the list is empty
            T * getNext (void);

            // Check if queue is empty
            bool isEmpty (void) const;

        protected:
            struct Node {
                T *pData;
                Node *pNext;
            };
            Node *pHead;
            Node *pTail;
            Node *pNextGetNode;
    };

    template <class T> inline PtrQueue<T>::PtrQueue (void)
    {
        pHead = NULL;
        pTail = NULL;
    }

    template <class T> inline PtrQueue<T>::~PtrQueue (void)
    {
        Node *pTemp;
        while (pHead != NULL) {
            pTemp = pHead;
            pHead = pHead->pNext;
            delete pTemp;
        }
        pHead = pTail = NULL;
    }

    template <class T> int PtrQueue<T>::enqueue (T *px)
    {
        Node *pNewNode;
        if (NULL == (pNewNode = new Node)) {
            return -1;
        }
        pNewNode->pData = px;
        pNewNode->pNext = NULL;
        if (pHead == NULL) {
            pHead = pNewNode;
            pTail = pNewNode;
        }
        else {
            pTail->pNext = pNewNode;
            pTail = pNewNode;
        }
        return 0;
    }

    template <class T> T * PtrQueue<T>::dequeue (void)
    {
        T *px = NULL;
        if (pHead == NULL) {
            return NULL;
        }
        else if (pHead == pTail) {
            px = pHead->pData;
            delete pHead;
            pHead = pTail = NULL;
        }
        else {
            px = pHead->pData;
            Node *pTemp = pHead;
            pHead = pHead->pNext;
            delete pTemp;
        }
        return px;
    }

    template <class T> inline T * PtrQueue<T>::peek (void) const
    {
        if (pHead == NULL) {
            return NULL;
        }
        else {
            return pHead->pData;
        }
    }

    template <class T> inline T * PtrQueue<T>::peekTail (void) const
    {
        if (pTail == NULL) {
            return NULL;
        }
        else {
            return pTail->pData;
        }
    }

    template <class T> T * PtrQueue<T>::remove (const T *px)
    {
        if (pHead == NULL) {
            return NULL;
        }
        else if (pHead == pTail) {
            if (*(pHead->pData) == *px) {
                T *pData = pHead->pData;
                delete pHead;
                pHead = pTail = NULL;
                return pData;
            }
            else {
                return NULL;
            }
        }
        else if (*(pHead->pData) == *px) {
            Node *pTemp = pHead;
            pHead = pHead->pNext;
            T *pData = pTemp->pData;
            delete pTemp;
            return pData;
        }
        else {
            Node *pPrevious = pHead;
            Node *pCurrent = pHead->pNext;
            while (pCurrent != NULL) {
                if (*(pCurrent->pData) == *px) {
                    pPrevious->pNext = pCurrent->pNext;
                    if (pTail == pCurrent) {
                        pTail = pPrevious;
                    }
                    T *pData = pCurrent->pData;
                    delete pCurrent;
                    return pData;
                }
                pPrevious = pCurrent;
                pCurrent = pCurrent->pNext;
            }
            return NULL;
        }
    }

    template <class T> void PtrQueue<T>::removeAll (bool removeData)
    {
        Node *pTemp;
        if (removeData) {
            while (pHead != NULL) {
                pTemp = pHead;
                pHead = pHead->pNext;
                delete pTemp->pData;
                delete pTemp;
            }
        }
        else {
            while (pHead != NULL) {
                pTemp = pHead;
                pHead = pHead->pNext;
                delete pTemp;
            }
        }
        pHead = pTail = NULL;
    }

    template <class T> inline int PtrQueue<T>::size (void) const
    {
        Node *pTemp = pHead;
        int i = 0;
        while (pTemp != NULL) {
            i++;
            pTemp = pTemp->pNext;
        }
        return i;
    }

    template <class T> inline T * PtrQueue<T>::find (const T *px) const
    {
        Node *pTemp = pHead;
        while (pTemp != NULL) {
            if (*(pTemp->pData) == *px) {
                return pTemp->pData;
            }
            pTemp = pTemp->pNext;
        }
        return NULL;
    }

    template <class T> inline void PtrQueue<T>::resetGet (void)
    {
        pNextGetNode = pHead;
    }

    template <class T> inline T * PtrQueue<T>::getFirst (void)
    {
        pNextGetNode = pHead;
        if (pHead != NULL) {
            pNextGetNode = pHead->pNext;
            return pHead->pData;
        }
        return NULL;
    }

    template <class T> inline T * PtrQueue<T>::getNext (void)
    {
        if (pNextGetNode != NULL) {
            T *pData = pNextGetNode->pData;
            pNextGetNode = pNextGetNode->pNext;
            return pData;
        }
        return NULL;
    }

    template <class T> inline bool PtrQueue<T>::isEmpty (void) const
    {
        return pHead == NULL;
    }

}

#endif   // #ifndef INCL_PTR_QUEUE_H

