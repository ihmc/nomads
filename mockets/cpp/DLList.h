/*
 * DLList.h
 * 
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
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
 * Doubly Linked List
 */

#if !defined (ANDROID) //No std support on ANDROID
	#include <stdexcept>
	#include <iostream>
#endif
#include "FTypes.h"
#include "NLFLib.h"

#ifndef INCL_DLLIST_H
#define INCL_DLLIST_H


template <class T> class DLList
{
    public:
        DLList (uint32 ui32MaxNumNodes = 0);
        ~DLList (void);

        uint32 getNumNodes (void);
        uint32 getMaxNumNodes (void);
        void pushTail (T el);
        void pushHead (T el);
        void popTail (void);
        void popHead (void);
        int getFirst( T &el );
        int getLast( T &el );
        int getNext( T &el );
        int getPrev( T &el );
        int getCurrent( T &el );
        void resetToHead(void);
        void resetToTail(void);
        void printNodes (void);

    protected:
        void next(void);
        void prev(void);

        struct Node
        {
            Node (void);

            T el;
            Node *pNext;
            Node *pPrev;

            void printAddress(void);
        };

        uint32 _ui32MaxNumNodes; //if = 0 then no limit
        Node *_pHead;
        Node *_pTail;
        Node *_pCurrent;
        uint32 _ui32NumNodes;
};

template <class T> DLList<T>::DLList (uint32 ui32MaxNumNodes)
{
    _ui32MaxNumNodes = ui32MaxNumNodes;
    _pCurrent =_pHead = _pTail = NULL;
    _ui32NumNodes = 0;
}

template <class T> DLList<T>::~DLList (void)
{
    while (_pHead != NULL) {
        Node *pTemp = _pHead;
        _pHead = _pHead->pNext;
        delete pTemp;
    }
    _pCurrent = NULL;
    _pTail = NULL;
    _ui32NumNodes = 0;
}

template <class T> uint32 DLList<T>::getNumNodes (void)
{
    return _ui32NumNodes;
}

template <class T> uint32 DLList<T>::getMaxNumNodes (void)
{
    return _ui32MaxNumNodes;
}

template <class T> void DLList<T>::pushTail (T el)
{
    Node *pNewNode;
    pNewNode = new Node;
    pNewNode->el = el;

    // If the list is empty...
    if (_pTail == NULL) {
        _pCurrent = _pHead = _pTail = pNewNode;
        _ui32NumNodes++;
        return;
    }

    // If the list reached its max size...
    if( (_ui32MaxNumNodes > 0) && (_ui32NumNodes >= _ui32MaxNumNodes) ) {
        // ... remove the first node
        popHead();
    }

    // Add the new node after the tail
    pNewNode->pPrev = _pTail;
    pNewNode->pNext = NULL;
    _pTail->pNext = pNewNode;
    _pTail = pNewNode;
    _ui32NumNodes++;
    return;
}

template <class T> void DLList<T>::pushHead (T el)
{
    Node *pNewNode;
    pNewNode = new Node;
    pNewNode->el = el;

    // If the list is empty...
    if ( _pHead == NULL ) {
        _pCurrent = _pHead = _pTail = pNewNode;
        _ui32NumNodes++;
        return;
    }

    // If the list reached its max size...
    if( (_ui32MaxNumNodes > 0) && (_ui32NumNodes >= _ui32MaxNumNodes) ) {
        // ... remove the last node
        popTail();
    }

    // Add the new node before the head
    pNewNode->pNext = _pHead;
    pNewNode->pPrev = NULL;
    _pHead->pPrev = pNewNode;
    _pCurrent = _pHead = pNewNode;
    _ui32NumNodes++;
    return;
}

template <class T> void DLList<T>::popTail(void)
{
    // If the list is empty...
    if( _pTail == NULL ) {
        // ... then do nothing
        return;
    }

    Node *pTempNode = _pTail;

    // If there is just one node...
    if( _ui32NumNodes == 1 ) {
        _pCurrent = _pHead = _pTail = NULL;
    }
    // otherwise...
    else {
        if( _pCurrent == _pTail )
            _pCurrent = _pTail->pPrev;
        _pTail = pTempNode->pPrev;
        _pTail->pNext = NULL;
        pTempNode->pPrev = NULL;
    }

    _ui32NumNodes--;
    delete pTempNode;
    return;
}

template <class T> void DLList<T>::popHead(void)
{
    // If the list is empty...
    if( _pHead == NULL ) {
        // ... then do nothing
        return;
    }

    Node *pTempNode = _pHead;

    // If there is just one node...
    if( _ui32NumNodes == 1 ) {
        _pCurrent = _pHead = _pTail = NULL;
    }
    // otherwise...
    else {
        if( _pCurrent == _pHead )
            _pCurrent = _pHead->pNext;
        _pHead = pTempNode->pNext;
        _pHead->pPrev = NULL;
        pTempNode->pNext = NULL;
    }

    _ui32NumNodes--;
    delete pTempNode;
    return;
}

template <class T> void DLList<T>::resetToHead(void)
{
    _pCurrent = _pHead;
    return;
}

template <class T> void DLList<T>::resetToTail(void)
{
    _pCurrent = _pTail;
    return;
}

template <class T> void DLList<T>::next( void )
{
    _pCurrent = _pCurrent->pNext;
    return;
}

template <class T> void DLList<T>::prev( void )
{
    _pCurrent = _pCurrent->pPrev;
    return;
}

template <class T> int DLList<T>::getCurrent( T &el )
{
    if( _pCurrent ) {
        el = _pCurrent->el;
        return 1;
    }
    else {
        return 0;
    }
}

template <class T> int DLList<T>::getNext( T &el )
{
    if( _pCurrent ) {
        el = _pCurrent->el;
        _pCurrent = _pCurrent->pNext;
        return 1;
    }
    else {
        return 0;
    }
}

template <class T> int DLList<T>::getPrev( T &el )
{
    if( _pCurrent ) {
        el = _pCurrent->el;
        _pCurrent = _pCurrent->pPrev;
        return 1;
    }
    else {
        return 0;
    }
}

template <class T> int DLList<T>::getFirst( T &el )
{
    if( _pHead ) {
        el = _pHead->el;
        _pCurrent = _pHead->pNext;
        return 1;
    }
    else {
        return 0;
    }
}

template <class T> int DLList<T>::getLast( T &el )
{
    if( _pTail ) {
        el = _pTail->el;
        _pCurrent = _pTail->pPrev;
        return 1;
    }
    else  {
        return 0;
    }
}

template <class T> DLList<T>::Node::Node (void) {
    pPrev = NULL;
    pNext = NULL;
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  printNodes
 *  Description:  Prints list size and the addresses of the nodes
 * =====================================================================================
 */
template <class T> void DLList<T>::printNodes (void)
{
    Node *pTempNode = _pHead;
	
	#if !defined (ANDROID) //No std support on ANDROID
    	std::cout << "Nodes: " << _ui32NumNodes << " (Size: " << sizeof( *_pHead )
        << " bytes)" << std::endl;
	#endif

    // If the list is empty...
    if( pTempNode == NULL )  {
        return;
    }
    else {
        do {
            pTempNode->printAddress();
            pTempNode = pTempNode->pNext;
        }
        while ( pTempNode != NULL );
    }
}

#if !defined (ANDROID) //No std support on ANDROID
template <class T> void DLList<T>::Node::printAddress (void)
{
    if( pNext == NULL && pPrev == NULL ) {
        std::cout << this << " (PREV: NULL, NEXT: NULL)" << std::endl;
    }
    else if ( pNext == NULL ) {
        std::cout << this << " (PREV: " << pPrev << ", NEXT: NULL)" << std::endl;
    }
    else if ( pPrev == NULL ) {
        std::cout << this << " (PREV: NULL, NEXT: " << pNext << ")" << std::endl;
    }
    else {
        std::cout << this << " (PREV: " << pPrev << ", NEXT: " << pNext << ")" <<
            std::endl;
    }
}
#endif

#endif   //INCL_DLLIST_H
