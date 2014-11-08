/*
 * Queue.h
 *
 * This file is part of the IHMC Util Library
 * Copyright (c) 1993-2014 IHMC.
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

#ifndef INCL_QUEUE_H
#define INCL_QUEUE_H

#include <stddef.h>

namespace NOMADSUtil
{

    template <class T> class Queue
    {
        public:
            Queue (void);
            ~Queue (void);
            // Add to the tail of the queue
            int enqueue (const T &x);
            // Remove from the head of the queue
            int dequeue (T &x);
            // Return the element at the head of the queue (but do not dequeue)
            int peek (T &x);
            // Dequeue the specified element (from anywhere in the queue)
            int remove (T &x);
            // Return number of elements in queue
            int size ();
            // Find the element (but leave it in the queue)
            bool find (T &x);
            // Replace the element in the queue with the new one
            bool findAndReplace (T &x);
            // Check if queue is empty
            bool isEmpty (void);

        protected:        
            struct Node
            {
                T data;
                Node *pNext;
            };
            Node *pHead;
            Node *pTail;
    };

    template <class T> Queue<T>::Queue (void)
    {
        pHead = NULL;
        pTail = NULL;
    }

    template <class T> Queue<T>::~Queue (void)
    {
        Node *pTemp;
        while (pHead != NULL) {
            pTemp = pHead;
            pHead = pHead->pNext;
            delete pTemp;
        }
        pHead = pTail = NULL;
    }

    template <class T> int Queue<T>::enqueue (const T &x)
    {
        Node *pNewNode;
        if (NULL == (pNewNode = new Node)) {
            return -1;
        }
        pNewNode->data = x;
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

    template <class T> int Queue<T>::dequeue (T &x)
    {
        if (pHead == NULL) {
            return -1;
        }
        else if (pHead == pTail) {
            x = pHead->data;
            delete pHead;
            pHead = pTail = NULL;
        }
        else {
            x = pHead->data;
            Node *pTemp = pHead;
            pHead = pHead->pNext;
            delete pTemp;
        }
        return 0;
    }

    template <class T> int Queue<T>::peek (T &x)
    {
        if (pHead == NULL) {
            return -1;
        }
        else {
            x = pHead->data;
        }
        return 0;
    }

    template <class T> int Queue<T>::remove (T &x)
    {
        if (pHead == NULL) {
            return -1;
        }
        else if (pHead == pTail) {
            if (pHead->data == x) {
                x = pHead->data;
                delete pHead;
                pHead = pTail = NULL;
                return 0;
            }
            else {
                return -1;
            }
        }
        else if (pHead->data == x) {
            x = pHead->data;
            Node *pTemp = pHead;
            pHead = pTemp->pNext;
            delete pTemp;
            return 0;
        }
        else {
            Node *pPrevious = pHead;
            Node *pCurrent = pHead->pNext;
            while (pCurrent != NULL) {
                if (pCurrent->data == x) {
                    pPrevious->pNext = pCurrent->pNext;
                    if (pTail == pCurrent) {
                        pTail = pPrevious;
                    }
                    x = pCurrent->data;
                    delete pCurrent;
                    return 0;
                }
                pPrevious = pCurrent;
                pCurrent = pCurrent->pNext;
            }
            return -1;
        }
    }

    template <class T> int Queue<T>::size()
    {
        Node *pTemp = pHead;
        int i = 0;
        while (pTemp != NULL) {
            i++;
            pTemp = pTemp->pNext;
        }
        return i;
    }

    template <class T> bool Queue<T>::find (T &x)
    {
        Node *pTemp = pHead;
        while (pTemp != NULL) {
            if (pTemp->data == x) {
                x = pTemp->data;
                return true;
            }
            pTemp = pTemp->pNext;
        }
        return false;
    }

    template <class T> bool Queue<T>::findAndReplace (T &x)
    {
        Node *pTemp = pHead;
        while (pTemp != NULL) {
            if (pTemp->data == x) {
                pTemp->data = x;
                return true;
            }
            pTemp = pTemp->pNext;
        }
        return false;
    }

    template <class T> bool Queue<T>::isEmpty (void)
    {
        return pHead == NULL;
    }

}

#endif   // #ifndef INCL_QUEUE_H
