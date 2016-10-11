/*
 * SortedLList.h
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

#ifndef INCL_SORTED_LLIST_H
#define INCL_SORTED_LLIST_H

#include "LList.h"

namespace NOMADSUtil
{

    template <class T>
    class SortedLList : public LList<T>
    {
        public:
             void insert (T el);
    };

    template <class T> void SortedLList<T>::insert (T el)
    {
        struct Node *pNewNode, *pTempNode;
        this->length++;
        pNewNode = new Node;
        pNewNode->el = el;
        pNewNode->pNextNode = NULL;

        if (this->pRoot == NULL) {
            this->pRoot = pNewNode;
            this->pGetNextNode = this->pRoot;
        }
        else if (this->pRoot->el > el) {
            pNewNode->pNextNode = this->pRoot;
            this->pRoot = pNewNode;
        }
        else {
             pTempNode = this->pRoot;
             while (pTempNode->pNextNode != NULL) {
                 if (pTempNode->pNextNode->el > el) {
                     pNewNode->pNextNode = pTempNode->pNextNode;
                     pTempNode->pNextNode = pNewNode;
                     break;
                 }
                 else
                 {
                     pTempNode = pTempNode->pNextNode;
                 }
             }
             if (pTempNode->pNextNode == NULL) {
                 pTempNode->pNextNode = pNewNode;
             }
        }
    }

}

#endif   // #ifndef INCL_SORTED_LLIST_H
