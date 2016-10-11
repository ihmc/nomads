/*
 * LList.h
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
 * Base linked list class
 */

#ifndef INCL_LLIST_H
#define INCL_LLIST_H

#include <stdlib.h>

#pragma warning (disable:4786)

namespace NOMADSUtil
{

    template <class T>
    class LList
    {
        public:
             LList (void);
             LList (const LList<T> &SourceList);
             virtual ~LList (void);
             void add (T el);

             // Returns 1 if the element was found and removed, 0 otherwise
             int remove (T el);

             void removeAll (void);

             int replace (T elOld, T elNew);
             int getFirst (T &El);
             int getNext (T &El);

             // Returns 1 if the element was found and 0 otherwise
             int search (T &El);

             void resetGet (void);
             int getCount (void);
             virtual LList<T> & operator = (LList<T> &SourceList);
             int length;

        protected:
             struct Node {
                  T el;
                  Node * pNextNode;
             };
             Node *pRoot;
             Node *pGetNextNode;
    };

    template <class T> LList<T>::LList (void)
    {
        pRoot = NULL;
        pGetNextNode = NULL;
        length = 0;
    }

    template <class T> LList<T>::LList (const LList<T> &SourceList)
    {
        Node *pSourceNode, *pNewNode, *pTempNode;

        pRoot = NULL;
        pGetNextNode = NULL;

        pSourceNode = SourceList . pRoot;
        if (pSourceNode)
        {
             pRoot = new Node;
             pRoot -> el = pSourceNode -> el;
             pRoot -> pNextNode = NULL;
             pTempNode = pRoot;
             pSourceNode = pSourceNode -> pNextNode;
             while (pSourceNode)
             {
                  pNewNode = new Node;
                  pNewNode -> el = pSourceNode -> el;
                  pNewNode -> pNextNode = NULL;
                  pTempNode -> pNextNode = pNewNode;
                  pTempNode = pNewNode;
                  pSourceNode = pSourceNode -> pNextNode;
             }
        }
    }

    template <class T> LList<T>::~LList ()
    {
        removeAll();
    }

    template <class T> void LList<T>::add (T el)
    {
        Node *pNewNode, *pTempNode;
        length++;
        pNewNode = new Node;
        pNewNode -> el = el;
        pNewNode -> pNextNode = NULL;

        if (pRoot == NULL)
        {
             pRoot = pNewNode;
             pGetNextNode = pRoot;
        }
        else
        {
             pTempNode = pRoot;
             while (pTempNode -> pNextNode != NULL)
             {
                  pTempNode = pTempNode -> pNextNode;
             }
             pTempNode -> pNextNode = pNewNode;
             if (pGetNextNode == NULL)
             {
                  pGetNextNode = pNewNode;
             }
        }
    }

    template <class T> int LList<T>::remove (T el)
    {
        int iResult = 0;
        Node *pPrevNode, *pCurrNode, *pTempNode;

        if (pRoot)
        {
             if (pRoot -> el == el)
             {
                  pTempNode = pRoot;
                  pRoot = pRoot -> pNextNode;
                  delete pTempNode;
                  length--;
                  iResult = 1;
             }
             else
             {
                  pPrevNode = pRoot;
                  pCurrNode = pRoot -> pNextNode;
                  while ((pCurrNode) && !(iResult))
                  {
                       if (pCurrNode -> el == el)
                       {
                            pPrevNode -> pNextNode = pCurrNode -> pNextNode;
                            delete pCurrNode;
                            length--;
                            iResult = 1;
                       }
                       else
                       {
                            pPrevNode = pCurrNode;
                            pCurrNode = pCurrNode -> pNextNode;
                       }
                  }
             }
        }
        return iResult;
    }

    template <class T> void LList<T>::removeAll (void)
    {
        Node *pTempNode;

        while (pRoot)
        {
             pTempNode = pRoot;
             pRoot = pRoot -> pNextNode;
             delete pTempNode;
        }
    }
 
    template <class T> int LList<T>::replace (T elOld, T elNew)
    {
        int iResult = 0;
        Node *pTempNode;

        if (pRoot)
        {
             if (pRoot -> el == elOld)
             {
                  pRoot -> el = elNew;
                  iResult = 1;
             }
             else
             {
                  pTempNode = pRoot -> pNextNode;
                  while ((pTempNode) && !(iResult))
                  {
                       if (pTempNode -> el == elOld)
                       {
                            pTempNode -> el = elNew;
                            iResult = 1;
                       }
                       else
                       {
                            pTempNode = pTempNode -> pNextNode;
                       }
                  }
             }

        }
        return iResult;
    }

    template <class T> int LList<T>::getFirst (T &el)
    {
        if (pRoot != NULL)
        {
             el = pRoot -> el;
             pGetNextNode = pRoot -> pNextNode;
             return 1;
        }
        else
        {
             return 0;
        }
    }

    template <class T> int LList<T>::getNext (T &el)
    {

        if (pGetNextNode != NULL)
        {
             el = pGetNextNode -> el;
             pGetNextNode = pGetNextNode -> pNextNode;
             return 1;
        }
        else
        {
             return 0;
        }
    }

    template <class T> int LList<T>::search (T &el)
    {
        Node *pTempNode;
        int iFound;

        pTempNode = pRoot;
        iFound = 0;
        while ((pTempNode) && (!iFound)) {
            if (pTempNode -> el == el) {
                el = pTempNode -> el;
                iFound = 1;
            }
            else {
                pTempNode = pTempNode -> pNextNode;
            }
        }
        return iFound;
    }

    template <class T> void LList<T>::resetGet (void)
    {
        pGetNextNode = pRoot;
    }

    template <class T> int LList<T>::getCount (void)
    {
        Node *pTempNode;
        int iCount = 0;

        pTempNode = pRoot;
        while (pTempNode)
        {
             iCount ++;
             pTempNode = pTempNode -> pNextNode;
        }
        return iCount;
    }

    template <class T> LList<T> & LList<T>::operator = (LList<T> & SourceList)
    {
        Node *pTempNode;

        pTempNode = pRoot;
        while (pRoot)
        {
             pTempNode = pRoot;
             pRoot = pRoot -> pNextNode;
             delete pTempNode;
        }
        pRoot = NULL;
        pTempNode = SourceList.pRoot;

        while (pTempNode)
        {
             add (pTempNode->el);
             pTempNode = pTempNode->pNextNode;
        }
        return *this;
    }

}

#endif // INCL_LIST_H
